# Fraud Detection & Transaction Monitoring Engine

A C++ and PostgreSQL-based transaction monitoring system that analyzes financial transactions, identifies suspicious patterns using rule-based detection, calculates risk scores, generates alerts and assigns transaction decisions.

## Features

- **Transaction Ingestion:** Bulk transaction processing from CSV files using C++.
- **Velocity Detection:** Identifies accounts with excessive transaction frequency within a 60-second window.
- **High-Value Detection:** Flags transactions of ₹50,000 or above.
- **Repeated Failure Detection:** Detects accounts with 3 or more failed transactions within 5 minutes.
- **Risk Scoring:** Combines triggered rule scores into a score capped at 100.
- **Fraud Alerts:** Generates persistent alerts for triggered detection rules.
- **Severity Classification:** Categorizes alerts as Low, Medium, High or Critical.
- **Transaction Decisioning:** Assigns Approved, Flagged or Blocked decisions based on risk scores.
- **Database Analytics:** SQL-based account analytics, alert investigation and transaction summaries.
- **Performance Optimization:** Indexes and query benchmarking using PostgreSQL EXPLAIN ANALYZE.

## Technology Stack

- C++17
- PostgreSQL
- libpq (PostgreSQL C client library)
- CMake
- SQL
- CSV

## Architecture

CSV Input → C++ Transaction Processor → PostgreSQL → Fraud Rule Engine → Risk Scoring → Decision Assignment → Alert Storage & Analytics

## Fraud Detection Rules

| Rule | Condition | Score |
|---|---|---:|
| Transaction Velocity | 8 or more transactions within 60 seconds | 40 |
| High Value | Transaction amount ≥ ₹50,000 | 50 |
| Repeated Failures | 3 or more failed transactions within 5 minutes | 30 |

The combined score is capped at 100.

## Decision Logic

| Risk Score | Decision |
|---|---|
| 0–29 | Approved |
| 30–69 | Flagged |
| 70–100 | Blocked |

## Severity Classification

| Risk Score | Severity |
|---|---|
| 0–29 | Low |
| 30–49 | Medium |
| 50–79 | High |
| 80–100 | Critical |

## Project Structure

```text
FraudDetectionEngine/
├── src/
│   └── main.cpp
├── sql/
│   └── schema.sql
├── data/
│   └── transactions.csv
├── tests/
│   └── test_fraud_rules.sql
├── CMakeLists.txt
├── README.md
└── .gitignore
```

## Setup

1. Install C++ compiler, CMake and PostgreSQL.
2. Create and configure the PostgreSQL database.
3. Configure the PostgreSQL connection string in the application.
4. Build the application using CMake.
5. Execute the application from the project root.

## Build

```bash
cmake --build build
```

## Run

```bash
./build/Debug/FraudDetectionEngine
```

On Windows PowerShell:

```powershell
.\build\Debug\FraudDetectionEngine.exe
```

## Database Analytics

The project uses SQL queries to:

- Investigate generated alerts.
- Analyze transaction volume by account.
- Identify accounts with repeated failures.
- Analyze suspicious transaction patterns.
- Summarize fraud alerts by severity.
- Benchmark queries using EXPLAIN ANALYZE.

## Performance

Database indexes are applied to frequently queried transaction and alert columns. Query execution plans and execution times are analyzed using PostgreSQL EXPLAIN ANALYZE.

## Limitations

- Transactions and fraud patterns are synthetic.
- Detection is rule-based rather than ML-based.
- Blocked transactions are simulated decisions and are not connected to a real payment network.
- The prototype is intended for learning, demonstration and portfolio purposes, not production financial deployment.

## Future Improvements

- Transaction deduplication and idempotent imports.
- Unit tests and automated integration testing.
- Improved concurrent transaction handling.
- Machine-learning-based anomaly detection.
- Dashboard for alert investigation.
- Transaction risk model calibration.

## Author

Developed as a portfolio project focused on C++, PostgreSQL, database design, transaction processing, fraud detection and analytical SQL.
