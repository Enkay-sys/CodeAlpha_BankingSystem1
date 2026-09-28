#include "FileManager.h"
#include <fstream>
#include <iostream>

FileManager::FileManager(const std::string& customersFile,
                          const std::string& accountsFile,
                          const std::string& transactionsFile)
    : customersFile(customersFile), accountsFile(accountsFile),
      transactionsFile(transactionsFile) {}

int FileManager::loadCustomers(std::unordered_map<std::string, Customer>& customers) const {
    std::ifstream in(customersFile);
    if (!in.is_open()) {
        // File doesn't exist yet -- perfectly normal on the very first run.
        return 0;
    }
    int skipped = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        Customer c;
        if (Customer::deserialize(line, c)) {
            customers[c.getCustomerId()] = c;
        } else {
            skipped++;
        }
    }
    return skipped;
}

int FileManager::loadAccounts(std::unordered_map<std::string, Account>& accounts) const {
    std::ifstream in(accountsFile);
    if (!in.is_open()) return 0;
    int skipped = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        Account a;
        if (Account::deserialize(line, a)) {
            accounts[a.getAccountNumber()] = a;
        } else {
            skipped++;
        }
    }
    return skipped;
}

int FileManager::loadTransactions(std::vector<Transaction>& transactions) const {
    std::ifstream in(transactionsFile);
    if (!in.is_open()) return 0;
    int skipped = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        Transaction t;
        if (Transaction::deserialize(line, t)) {
            transactions.push_back(t);
        } else {
            skipped++;
        }
    }
    return skipped;
}

bool FileManager::saveCustomers(const std::unordered_map<std::string, Customer>& customers) const {
    std::ofstream out(customersFile, std::ios::trunc);
    if (!out.is_open()) return false;
    for (const auto& pair : customers) {
        out << pair.second.serialize() << "\n";
    }
    return true;
}

bool FileManager::saveAccounts(const std::unordered_map<std::string, Account>& accounts) const {
    std::ofstream out(accountsFile, std::ios::trunc);
    if (!out.is_open()) return false;
    for (const auto& pair : accounts) {
        out << pair.second.serialize() << "\n";
    }
    return true;
}

bool FileManager::saveTransactions(const std::vector<Transaction>& transactions) const {
    std::ofstream out(transactionsFile, std::ios::trunc);
    if (!out.is_open()) return false;
    for (const auto& t : transactions) {
        out << t.serialize() << "\n";
    }
    return true;
}
