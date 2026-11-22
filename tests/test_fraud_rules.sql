
-- TEST 1: High value transactions
SELECT transaction_id, amount
FROM transactions
WHERE amount >= 50000
ORDER BY amount DESC;

-- TEST 2: Repeated failed transactions
SELECT account_id, COUNT(*) AS failed_count
FROM transactions
WHERE status = 'Failed'
GROUP BY account_id
HAVING COUNT(*) >= 3;

-- TEST 3: Transactions with fraud alerts
SELECT
    t.transaction_id,
    t.account_id,
    t.amount,
    t.fraud_decision,
    fa.rule_name,
    fa.risk_score,
    fa.severity
FROM transactions t
JOIN fraud_alerts fa
    ON t.transaction_id = fa.transaction_id
ORDER BY fa.risk_score DESC;

-- TEST 4: Decision distribution
SELECT fraud_decision, COUNT(*) AS total
FROM transactions
GROUP BY fraud_decision
ORDER BY total DESC;

-- TEST 5: Account transaction volume
SELECT account_id, COUNT(*) AS total_transactions
FROM transactions
GROUP BY account_id
ORDER BY total_transactions DESC;
