
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <libpq-fe.h>

using namespace std;

bool executeCommand(PGconn* conn, const char* query) {
    PGresult* res = PQexec(conn, query);

    bool success = res &&
        PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!success)
        cerr << PQerrorMessage(conn) << endl;

    if (res)
        PQclear(res);

    return success;
}

int main() {

    // DATABASE CONNECTION
    const char* dbPassword = getenv("PGPASSWORD");

    if (!dbPassword) {
        cerr << "PGPASSWORD environment variable not set.\n";
        return 1;
    }

    string connectionString =
        "host=localhost port=5433 dbname=fraud_detection "
        "user=postgres password=" + string(dbPassword);

    PGconn* conn = PQconnectdb(connectionString.c_str());

    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "Connection failed: " << PQerrorMessage(conn);
        PQfinish(conn);
        return 1;
    }

    cout << "Connected to PostgreSQL!\n";

    ifstream file("data/transactions.csv");

    if (!file.is_open()) {
        cerr << "Cannot open data/transactions.csv\n";
        PQfinish(conn);
        return 1;
    }

    string line;
    getline(file, line); // Skip header

    int processed = 0;
    int skipped = 0;
    int alerted = 0;

    while (getline(file, line)) {

        if (line.empty())
            continue;

        stringstream ss(line);

        string accountID, merchantID, deviceID;
        string amount, transactionType, status;

        getline(ss, accountID, ',');
        getline(ss, merchantID, ',');
        getline(ss, deviceID, ',');
        getline(ss, amount, ',');
        getline(ss, transactionType, ',');
        getline(ss, status, ',');

        string sourceKey = accountID + "|" + merchantID + "|" +
                           deviceID + "|" + amount + "|" +
                           transactionType + "|" + status;

        // BEGIN TRANSACTION
        if (!executeCommand(conn, "BEGIN")) {
            cerr << "Unable to begin transaction.\n";
            continue;
        }

        // INSERT TRANSACTION
        const char* insertQuery =
            "INSERT INTO transactions "
            "(account_id, merchant_id, device_id, amount, "
            "transaction_type, status, source_key) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7) "
            "ON CONFLICT (source_key) DO NOTHING "
            "RETURNING transaction_id";

        const char* insertValues[] = {
            accountID.c_str(),
            merchantID.c_str(),
            deviceID.c_str(),
            amount.c_str(),
            transactionType.c_str(),
            status.c_str(),
            sourceKey.c_str()
        };

        PGresult* res = PQexecParams(
            conn, insertQuery, 7, nullptr,
            insertValues, nullptr, nullptr, 0
        );

        if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
            cerr << "Insert error: " << PQerrorMessage(conn);
            if (res) PQclear(res);
            executeCommand(conn, "ROLLBACK");
            continue;
        }

        // DUPLICATE CHECK
        if (PQntuples(res) == 0) {
            PQclear(res);
            executeCommand(conn, "ROLLBACK");
            cout << "Duplicate skipped.\n";
            skipped++;
            continue;
        }

        string transactionID = PQgetvalue(res, 0, 0);
        PQclear(res);

        vector<string> rules;
        int riskScore = 0;
        bool processingFailed = false;

        const char* accountValue[] = {
            accountID.c_str()
        };

        // RULE 1: VELOCITY
        const char* velocityQuery =
            "SELECT COUNT(*) FROM transactions "
            "WHERE account_id=$1 "
            "AND transaction_time >= NOW()-INTERVAL '60 seconds'";

        res = PQexecParams(
            conn, velocityQuery, 1, nullptr,
            accountValue, nullptr, nullptr, 0
        );

        if (res && PQresultStatus(res) == PGRES_TUPLES_OK) {
            int count = stoi(PQgetvalue(res, 0, 0));

            if (count >= 8) {
                rules.push_back("Velocity Rule");
                riskScore += 40;
            }
        } else {
            cerr << "Velocity query failed.\n";
            processingFailed = true;
        }

        if (res) PQclear(res);

        // RULE 2: HIGH VALUE
        try {
            if (stod(amount) >= 50000) {
                rules.push_back("High Value Rule");
                riskScore += 50;
            }
        } catch (...) {
            cerr << "Invalid amount.\n";
            processingFailed = true;
        }

        // RULE 3: REPEATED FAILURE
        const char* failedQuery =
            "SELECT COUNT(*) FROM transactions "
            "WHERE account_id=$1 AND status='Failed' "
            "AND transaction_time >= NOW()-INTERVAL '5 minutes'";

        res = PQexecParams(
            conn, failedQuery, 1, nullptr,
            accountValue, nullptr, nullptr, 0
        );

        if (res && PQresultStatus(res) == PGRES_TUPLES_OK) {
            int count = stoi(PQgetvalue(res, 0, 0));

            if (count >= 3) {
                rules.push_back("Repeated Failure Rule");
                riskScore += 30;
            }
        } else {
            cerr << "Failure query failed.\n";
            processingFailed = true;
        }

        if (res) PQclear(res);

        // ABORT IF RULE PROCESSING FAILED
        if (processingFailed) {
            executeCommand(conn, "ROLLBACK");
            continue;
        }

        if (riskScore > 100)
            riskScore = 100;

        // DECISION
        string decision = "Approved";

        if (riskScore >= 70)
            decision = "Blocked";
        else if (riskScore >= 30)
            decision = "Flagged";

        // UPDATE DECISION
        const char* decisionQuery =
            "UPDATE transactions "
            "SET fraud_decision=$1 "
            "WHERE transaction_id=$2";

        const char* decisionValues[] = {
            decision.c_str(),
            transactionID.c_str()
        };

        res = PQexecParams(
            conn, decisionQuery, 2, nullptr,
            decisionValues, nullptr, nullptr, 0
        );

        if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
            cerr << "Decision update failed.\n";
            if (res) PQclear(res);
            executeCommand(conn, "ROLLBACK");
            continue;
        }

        PQclear(res);

        // INSERT ALERTS
        bool alertFailure = false;

        for (const string& rule : rules) {

            string severity = "Low";

            if (riskScore >= 80)
                severity = "Critical";
            else if (riskScore >= 50)
                severity = "High";
            else if (riskScore >= 30)
                severity = "Medium";

            const char* alertQuery =
                "INSERT INTO fraud_alerts "
                "(transaction_id, rule_name, risk_score, "
                "alert_status, severity) "
                "VALUES ($1,$2,$3,$4,$5) "
                "RETURNING alert_id";

            string score = to_string(riskScore);

            const char* alertValues[] = {
                transactionID.c_str(),
                rule.c_str(),
                score.c_str(),
                "Open",
                severity.c_str()
            };

            res = PQexecParams(
                conn, alertQuery, 5, nullptr,
                alertValues, nullptr, nullptr, 0
            );

            if (res && PQresultStatus(res) == PGRES_TUPLES_OK) {
                cout << "Alert: " << rule
                     << " | " << severity << endl;
            } else {
                cerr << "Alert insertion failed.\n";
                alertFailure = true;
            }

            if (res) PQclear(res);

            if (alertFailure)
                break;
        }

        // ROLLBACK IF ALERT INSERTION FAILED
        if (alertFailure) {
            executeCommand(conn, "ROLLBACK");
            continue;
        }

        // COMMIT TRANSACTION
        if (!executeCommand(conn, "COMMIT")) {
            executeCommand(conn, "ROLLBACK");
            cerr << "Commit failed.\n";
            continue;
        }

        // COUNT ONLY COMMITTED TRANSACTIONS
        processed++;
        alerted += static_cast<int>(rules.size());

        cout << "\nTransaction " << transactionID
             << " | Account " << accountID
             << " | Amount " << amount
             << " | Risk " << riskScore << "/100\n";

        cout << "Decision: " << decision << endl;

        if (rules.empty())
            cout << "No rules triggered.\n";
    }

    file.close();
    PQfinish(conn);

    cout << "\n========== BULK PROCESSING SUMMARY ==========\n";
    cout << "Committed transactions: " << processed << endl;
    cout << "Duplicates skipped: " << skipped << endl;
    cout << "Alerts generated: " << alerted << endl;

    return 0;
}
