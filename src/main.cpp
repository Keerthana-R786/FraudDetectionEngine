
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <libpq-fe.h>

using namespace std;

int main() {
    const char* conninfo =
        "host=localhost port=5433 dbname=fraud_detection "
        "user=postgres password=NewPassword123";

    PGconn* conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "Connection failed: " << PQerrorMessage(conn) << endl;
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
    getline(file, line); // Skip CSV header

    int processed = 0;
    int alerted = 0;

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string accountID, merchantID, deviceID;
        string amount, transactionType, status;

        getline(ss, accountID, ',');
        getline(ss, merchantID, ',');
        getline(ss, deviceID, ',');
        getline(ss, amount, ',');
        getline(ss, transactionType, ',');
        getline(ss, status, ',');

        // INSERT TRANSACTION
        const char* insertQuery =
    "INSERT INTO transactions "
    "(account_id, merchant_id, device_id, amount, "
    "transaction_type, status) "
    "VALUES ($1,$2,$3,$4,$5,$6) "
    "RETURNING transaction_id";

        const char* insertValues[] = {
            accountID.c_str(), merchantID.c_str(),
            deviceID.c_str(), amount.c_str(),
            transactionType.c_str(), status.c_str()
        };

        PGresult* res = PQexecParams(
            conn, insertQuery, 6, nullptr,
            insertValues, nullptr, nullptr, 0
        );

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            cerr << "Insert error: " << PQerrorMessage(conn) << endl;
            PQclear(res);
            continue;
        }

        string transactionID = PQgetvalue(res, 0, 0);
        PQclear(res);

        // FRAUD DETECTION
        vector<string> rules;
        int riskScore = 0;

        // Velocity: 8 or more within 60 seconds
        const char* velocityQuery =
            "SELECT COUNT(*) FROM transactions "
            "WHERE account_id=$1 "
            "AND transaction_time >= NOW()-INTERVAL '60 seconds'";

        const char* accountValue[] = {accountID.c_str()};

        res = PQexecParams(
            conn, velocityQuery, 1, nullptr,
            accountValue, nullptr, nullptr, 0
        );

        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            int count = stoi(PQgetvalue(res, 0, 0));
            if (count >= 8) {
                rules.push_back("Velocity Rule");
                riskScore += 40;
            }
        }
        PQclear(res);

        // High value: 50000 or more
        if (stod(amount) >= 50000) {
            rules.push_back("High Value Rule");
            riskScore += 50;
        }

        // Failed transactions: 3 or more within 5 minutes
        const char* failedQuery =
            "SELECT COUNT(*) FROM transactions "
            "WHERE account_id=$1 AND status='Failed' "
            "AND transaction_time >= NOW()-INTERVAL '5 minutes'";

        res = PQexecParams(
            conn, failedQuery, 1, nullptr,
            accountValue, nullptr, nullptr, 0
        );

        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            int count = stoi(PQgetvalue(res, 0, 0));
            if (count >= 3) {
                rules.push_back("Repeated Failure Rule");
                riskScore += 30;
            }
        }
        PQclear(res);

        if (riskScore > 100) riskScore = 100;
        string decision = "Approved";

if (riskScore >= 70)
    decision = "Blocked";
else if (riskScore >= 30)
    decision = "Flagged";
const char* decisionQuery =
    "UPDATE transactions "
    "SET fraud_decision = $1 "
    "WHERE transaction_id = $2";

const char* decisionValues[] = {
    decision.c_str(),
    transactionID.c_str()
};

res = PQexecParams(
    conn, decisionQuery, 2, nullptr,
    decisionValues, nullptr, nullptr, 0
);

if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    cerr << "Decision update failed: "
         << PQerrorMessage(conn) << endl;
}

PQclear(res);

cout << "Decision: " << decision << endl;

        cout << "\nTransaction " << transactionID
             << " | Account " << accountID
             << " | Amount " << amount
             << " | Risk " << riskScore << "/100\n";

        // GENERATE ALERTS
        for (const string& rule : rules) {
            string severity = "Low";

            if (riskScore >= 80) severity = "Critical";
            else if (riskScore >= 50) severity = "High";
            else if (riskScore >= 30) severity = "Medium";

            const char* alertQuery =
    "INSERT INTO fraud_alerts "
    "(transaction_id, rule_name, risk_score, alert_status, severity) "
    "VALUES ($1,$2,$3,$4,$5) "
    "RETURNING alert_id";

            string score = to_string(riskScore);

            const char* alertValues[] = {
                transactionID.c_str(), rule.c_str(),
                score.c_str(), "Open", severity.c_str()
            };

            res = PQexecParams(
                conn, alertQuery, 5, nullptr,
                alertValues, nullptr, nullptr, 0
            );

            if (PQresultStatus(res) == PGRES_TUPLES_OK) {
                cout << "  ALERT: " << rule
                     << " | Severity: " << severity << "\n";
                alerted++;
            } else {
                cerr << "Alert error: " << PQerrorMessage(conn) << endl;
            }

            PQclear(res);
        }

        if (rules.empty())
            cout << "  No rules triggered.\n";

        processed++;
    }

    file.close();
    PQfinish(conn);

    cout << "\n========== BULK PROCESSING SUMMARY ==========\n";
    cout << "Transactions processed: " << processed << endl;
    cout << "Alerts generated: " << alerted << endl;

    return 0;
}
