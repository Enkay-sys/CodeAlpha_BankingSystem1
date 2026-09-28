#include "Transaction.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>

std::string transactionTypeToString(TransactionType type) {
    switch (type) {
        case TransactionType::Deposit:     return "DEPOSIT";
        case TransactionType::Withdrawal:  return "WITHDRAWAL";
        case TransactionType::TransferOut: return "TRANSFER-OUT";
        case TransactionType::TransferIn:  return "TRANSFER-IN";
    }
    return "UNKNOWN";
}

TransactionType transactionTypeFromString(const std::string& text) {
    if (text == "WITHDRAWAL") return TransactionType::Withdrawal;
    if (text == "TRANSFER-OUT") return TransactionType::TransferOut;
    if (text == "TRANSFER-IN") return TransactionType::TransferIn;
    return TransactionType::Deposit;
}

Transaction::Transaction()
    : transactionId(""), type(TransactionType::Deposit), accountNumber(""),
      relatedAccount(""), amountInSubunits(0), timestamp(""), description("") {}

Transaction::Transaction(const std::string& transactionId,
                          TransactionType type,
                          const std::string& accountNumber,
                          const std::string& relatedAccount,
                          long long amountInSubunits,
                          const std::string& timestamp,
                          const std::string& description)
    : transactionId(transactionId), type(type), accountNumber(accountNumber),
      relatedAccount(relatedAccount), amountInSubunits(amountInSubunits),
      timestamp(timestamp), description(description) {}

std::string Transaction::getTransactionId() const { return transactionId; }
TransactionType Transaction::getType() const { return type; }
std::string Transaction::getAccountNumber() const { return accountNumber; }
std::string Transaction::getRelatedAccount() const { return relatedAccount; }
long long Transaction::getAmountInSubunits() const { return amountInSubunits; }
std::string Transaction::getTimestamp() const { return timestamp; }
std::string Transaction::getDescription() const { return description; }

bool Transaction::isCredit() const {
    return type == TransactionType::Deposit || type == TransactionType::TransferIn;
}

void Transaction::display() const {
    std::cout << "----------------------------------------\n";
    std::cout << "Transaction ID : " << transactionId << "\n";
    std::cout << "Type           : " << transactionTypeToString(type) << "\n";
    std::cout << "Account        : " << accountNumber << "\n";
    if (!relatedAccount.empty()) {
        std::cout << "Related Account: " << relatedAccount << "\n";
    }
    std::cout << "Amount         : " << (isCredit() ? "+" : "-") << "GHS "
               << formatMoney(amountInSubunits) << "\n";
    std::cout << "Date           : " << timestamp << "\n";
    std::cout << "Description    : " << description << "\n";
    std::cout << "----------------------------------------\n";
}

void Transaction::displayShort() const {
    std::ostringstream oss;
    oss << std::left << std::setw(10) << transactionId << " | "
        << std::left << std::setw(12) << transactionTypeToString(type) << " | "
        << (isCredit() ? "+" : "-") << "GHS " << formatMoney(amountInSubunits)
        << "  (" << timestamp << ")";
    std::cout << oss.str() << "\n";
}

// File record layout:
// transactionId|type|accountNumber|relatedAccount|amountInSubunits|timestamp|description
std::string Transaction::serialize() const {
    std::ostringstream oss;
    oss << transactionId << '|' << transactionTypeToString(type) << '|' << accountNumber << '|'
        << relatedAccount << '|' << amountInSubunits << '|' << timestamp << '|' << description;
    return oss.str();
}

bool Transaction::deserialize(const std::string& line, Transaction& outTransaction) {
    std::vector<std::string> fields = splitString(line, '|');
    if (fields.size() < 6) return false;

    try {
        outTransaction.transactionId = fields[0];
        outTransaction.type = transactionTypeFromString(fields[1]);
        outTransaction.accountNumber = fields[2];
        outTransaction.relatedAccount = fields[3];
        outTransaction.amountInSubunits = std::stoll(fields[4]);
        outTransaction.timestamp = fields[5];
        outTransaction.description = fields.size() >= 7 ? fields[6] : "";
    } catch (...) {
        return false;
    }
    return true;
}
