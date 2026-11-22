CREATE TABLE users (
    user_id BIGINT PRIMARY KEY,
    full_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE accounts (
    account_id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
    account_number VARCHAR(20) UNIQUE NOT NULL,
    account_type VARCHAR(20) NOT NULL CHECK (account_type IN ('Savings', 'Current')),
    balance NUMERIC(15,2) NOT NULL CHECK (balance >= 0),
    status VARCHAR(20) NOT NULL DEFAULT 'Active' CHECK (status IN ('Active', 'Inactive')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE merchants (
    merchant_id BIGINT PRIMARY KEY,
    merchant_name VARCHAR(100) NOT NULL,
    category VARCHAR(50) NOT NULL,
    location VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE devices (
    device_id BIGINT PRIMARY KEY,
    device_fingerprint VARCHAR(255) UNIQUE NOT NULL,
    device_type VARCHAR(50) NOT NULL,
    registered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE transactions (
    transaction_id BIGINT PRIMARY KEY,
    account_id BIGINT NOT NULL REFERENCES accounts(account_id),
    merchant_id BIGINT REFERENCES merchants(merchant_id),
    device_id BIGINT REFERENCES devices(device_id),
    amount NUMERIC(15,2) NOT NULL CHECK (amount > 0),
    transaction_type VARCHAR(20) NOT NULL
        CHECK (transaction_type IN ('Payment', 'Transfer', 'Withdrawal')),
    transaction_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status VARCHAR(20) NOT NULL DEFAULT 'Success'
        CHECK (status IN ('Success', 'Failed', 'Pending'))
);

CREATE TABLE fraud_alerts (
    alert_id BIGINT PRIMARY KEY,
    transaction_id BIGINT NOT NULL REFERENCES transactions(transaction_id),
    rule_name VARCHAR(100) NOT NULL,
    risk_score INT NOT NULL CHECK (risk_score BETWEEN 0 AND 100),
    alert_status VARCHAR(20) NOT NULL DEFAULT 'Open'
        CHECK (alert_status IN ('Open', 'Investigating', 'Resolved')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);