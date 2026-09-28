#include "Account.h"
#include "Utils.h"
#include <iostream>
#include <sstream>

std::string accountTypeToString(AccountType type) {
    switch (type) {
        case AccountType::Savings: return "Savings";
        case AccountType::Current: return "Current";
    }
    return "Unknown";
}

AccountType accountTypeFromString(const std::string& text) {
    if (text == "Current") return AccountType::Current;
    return AccountType::Savings; // default/fallback
}

std::string accountStatusToString(AccountStatus status) {
    switch (status) {
        case AccountStatus::Active: return "Active";
        case AccountStatus::Closed: return "Closed";
    }
    return "Unknown";
}

AccountStatus accountStatusFromString(const std::string& text) {
    if (text == "Closed") return AccountStatus::Closed;
    return AccountStatus::Active;
}

Account::Account()
    : accountNumber(""), customerId(""), accountType(AccountType::Savings),
      balanceInSubunits(0), status(AccountStatus::Active), creationDate("") {}

Account::Account(const std::string& accountNumber,
                  const std::string& customerId,
                  AccountType accountType,
                  long long initialBalanceInSubunits,
                  const std::string& creationDate)
    : accountNumber(accountNumber), customerId(customerId), accountType(accountType),
      balanceInSubunits(initialBalanceInSubunits), status(AccountStatus::Active),
      creationDate(creationDate) {}

std::string Account::getAccountNumber() const { return accountNumber; }
std::string Account::getCustomerId() const { return customerId; }
AccountType Account::getAccountType() const { return accountType; }
long long Account::getBalanceInSubunits() const { return balanceInSubunits; }
AccountStatus Account::getStatus() const { return status; }
std::string Account::getCreationDate() const { return creationDate; }
bool Account::isActive() const { return status == AccountStatus::Active; }

bool Account::deposit(long long amountInSubunits) {
    if (amountInSubunits <= 0) return false; // rejects zero and negative amounts
    if (!isActive()) return false;
    balanceInSubunits += amountInSubunits;
    return true;
}

bool Account::withdraw(long long amountInSubunits) {
    if (amountInSubunits <= 0) return false;
    if (!isActive()) return false;
    if (amountInSubunits > balanceInSubunits) return false; // insufficient funds
    balanceInSubunits -= amountInSubunits;
    return true;
}

bool Account::credit(long long amountInSubunits) {
    if (amountInSubunits <= 0) return false;
    if (!isActive()) return false;
    balanceInSubunits += amountInSubunits;
    return true;
}

bool Account::debit(long long amountInSubunits) {
    if (amountInSubunits <= 0) return false;
    if (!isActive()) return false;
    if (amountInSubunits > balanceInSubunits) return false;
    balanceInSubunits -= amountInSubunits;
    return true;
}

void Account::closeAccount() {
    status = AccountStatus::Closed;
}

void Account::displayInfo() const {
    std::cout << "----------------------------------------\n";
    std::cout << "Account Number : " << accountNumber << "\n";
    std::cout << "Customer ID    : " << customerId << "\n";
    std::cout << "Account Type   : " << accountTypeToString(accountType) << "\n";
    std::cout << "Status         : " << accountStatusToString(status) << "\n";
    std::cout << "Balance        : GHS " << formatMoney(balanceInSubunits) << "\n";
    std::cout << "Created On     : " << creationDate << "\n";
    std::cout << "----------------------------------------\n";
}

// File record layout:
// accountNumber|customerId|accountType|balanceInSubunits|status|creationDate
std::string Account::serialize() const {
    std::ostringstream oss;
    oss << accountNumber << '|' << customerId << '|' << accountTypeToString(accountType) << '|'
        << balanceInSubunits << '|' << accountStatusToString(status) << '|' << creationDate;
    return oss.str();
}

bool Account::deserialize(const std::string& line, Account& outAccount) {
    std::vector<std::string> fields = splitString(line, '|');
    if (fields.size() < 6) return false;

    try {
        outAccount.accountNumber = fields[0];
        outAccount.customerId = fields[1];
        outAccount.accountType = accountTypeFromString(fields[2]);
        outAccount.balanceInSubunits = std::stoll(fields[3]);
        outAccount.status = accountStatusFromString(fields[4]);
        outAccount.creationDate = fields[5];
    } catch (...) {
        return false; // corrupted numeric field
    }
    return true;
}
