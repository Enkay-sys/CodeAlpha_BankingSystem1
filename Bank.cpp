#include "Bank.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

Bank::Bank(const std::string& customersFile,
           const std::string& accountsFile,
           const std::string& transactionsFile)
    : fileManager(customersFile, accountsFile, transactionsFile),
      nextCustomerNumber(1001), nextAccountNumber(1000000001), nextTransactionNumber(10001) {}

// ---------------------------------------------------------------------
// ID generation
// ---------------------------------------------------------------------
// Customer IDs look like "CUST1001", account numbers are a plain 10-digit
// number (so they resemble a real account number), transaction IDs look
// like "TXN10001". Each generator simply reads the next counter value and
// increments it, so IDs are unique for the lifetime of the in-memory Bank;
// loadData() below advances the counters past whatever is already on disk
// so a restarted program never reuses an old ID.
std::string Bank::generateCustomerId() {
    std::ostringstream oss;
    oss << "CUST" << nextCustomerNumber++;
    return oss.str();
}

std::string Bank::generateAccountNumber() {
    std::ostringstream oss;
    oss << nextAccountNumber++;
    return oss.str();
}

std::string Bank::generateTransactionId() {
    std::ostringstream oss;
    oss << "TXN" << nextTransactionNumber++;
    return oss.str();
}

void Bank::recordTransaction(TransactionType type,
                              const std::string& accountNumber,
                              const std::string& relatedAccount,
                              long long amountInSubunits,
                              const std::string& description) {
    Transaction t(generateTransactionId(), type, accountNumber, relatedAccount,
                  amountInSubunits, currentTimestamp(), description);
    transactions.push_back(t);
}

// ---------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------
void Bank::loadData() {
    int badCustomers = fileManager.loadCustomers(customers);
    int badAccounts = fileManager.loadAccounts(accounts);
    int badTransactions = fileManager.loadTransactions(transactions);

    // Advance the ID counters past anything already loaded, so newly
    // generated IDs never collide with IDs already stored on disk.
    for (const auto& pair : customers) {
        const std::string& id = pair.first; // e.g. "CUST1042"
        if (id.rfind("CUST", 0) == 0) {
            try {
                long long n = std::stoll(id.substr(4));
                nextCustomerNumber = std::max(nextCustomerNumber, n + 1);
            } catch (...) { /* ignore malformed id, keep counter as-is */ }
        }
    }
    for (const auto& pair : accounts) {
        try {
            long long n = std::stoll(pair.first);
            nextAccountNumber = std::max(nextAccountNumber, n + 1);
        } catch (...) { }
    }
    for (const auto& t : transactions) {
        const std::string& id = t.getTransactionId(); // e.g. "TXN10077"
        if (id.rfind("TXN", 0) == 0) {
            try {
                long long n = std::stoll(id.substr(3));
                nextTransactionNumber = std::max(nextTransactionNumber, n + 1);
            } catch (...) { }
        }
    }

    std::cout << "Data loaded: " << customers.size() << " customer(s), "
              << accounts.size() << " account(s), " << transactions.size()
              << " transaction(s).\n";
    int totalSkipped = badCustomers + badAccounts + badTransactions;
    if (totalSkipped > 0) {
        std::cout << "Warning: " << totalSkipped
                  << " corrupted record(s) were found and skipped.\n";
    }
}

bool Bank::saveData() {
    bool ok1 = fileManager.saveCustomers(customers);
    bool ok2 = fileManager.saveAccounts(accounts);
    bool ok3 = fileManager.saveTransactions(transactions);
    if (!ok1) std::cout << "Error: could not save customers file.\n";
    if (!ok2) std::cout << "Error: could not save accounts file.\n";
    if (!ok3) std::cout << "Error: could not save transactions file.\n";
    return ok1 && ok2 && ok3;
}

// ---------------------------------------------------------------------
// Customer management
// ---------------------------------------------------------------------
OperationResult Bank::createCustomer(const std::string& name,
                                      const std::string& phone,
                                      const std::string& email,
                                      const std::string& address,
                                      std::string& outCustomerId) {
    if (name.empty() || phone.empty()) {
        return { false, "Name and phone number are required." };
    }
    std::string id = generateCustomerId();
    // generateCustomerId() always returns a fresh, never-before-used ID
    // (see comment above), so a duplicate-ID collision cannot happen here.
    customers[id] = Customer(id, name, phone, email, address);
    outCustomerId = id;
    return { true, "Customer created successfully." };
}

const Customer* Bank::findCustomer(const std::string& customerId) const {
    auto it = customers.find(customerId);
    if (it == customers.end()) return nullptr;
    return &it->second;
}

bool Bank::customerExists(const std::string& customerId) const {
    return customers.find(customerId) != customers.end();
}

bool Bank::viewCustomer(const std::string& customerId) const {
    const Customer* c = findCustomer(customerId);
    if (!c) return false;
    c->displayInfo();
    return true;
}

// ---------------------------------------------------------------------
// Account management
// ---------------------------------------------------------------------
OperationResult Bank::createAccount(const std::string& customerId,
                                     AccountType type,
                                     long long initialDepositInSubunits,
                                     std::string& outAccountNumber) {
    auto custIt = customers.find(customerId);
    if (custIt == customers.end()) {
        return { false, "Customer not found. Cannot create an account for a nonexistent customer." };
    }
    if (initialDepositInSubunits < 0) {
        return { false, "Initial deposit cannot be negative." };
    }

    std::string accNum = generateAccountNumber();
    Account acc(accNum, customerId, type, initialDepositInSubunits, currentTimestamp());
    accounts[accNum] = acc;
    custIt->second.addAccountNumber(accNum);

    if (initialDepositInSubunits > 0) {
        recordTransaction(TransactionType::Deposit, accNum, "", initialDepositInSubunits,
                          "Initial deposit on account opening");
    }

    outAccountNumber = accNum;
    return { true, "Account created successfully." };
}

const Account* Bank::findAccount(const std::string& accountNumber) const {
    auto it = accounts.find(accountNumber);
    if (it == accounts.end()) return nullptr;
    return &it->second;
}

bool Bank::accountExists(const std::string& accountNumber) const {
    return accounts.find(accountNumber) != accounts.end();
}

bool Bank::viewAccount(const std::string& accountNumber) const {
    const Account* a = findAccount(accountNumber);
    if (!a) return false;
    a->displayInfo();
    return true;
}

// ---------------------------------------------------------------------
// Deposit / Withdraw / Transfer
// ---------------------------------------------------------------------
OperationResult Bank::deposit(const std::string& accountNumber, long long amountInSubunits) {
    auto it = accounts.find(accountNumber);
    if (it == accounts.end()) return { false, "Account not found." };
    if (amountInSubunits <= 0) return { false, "Invalid amount. Deposit must be greater than zero." };
    if (!it->second.isActive()) return { false, "Account is closed." };

    if (!it->second.deposit(amountInSubunits)) {
        return { false, "Deposit failed." }; // defensive; should not happen given checks above
    }
    recordTransaction(TransactionType::Deposit, accountNumber, "", amountInSubunits, "Cash deposit");
    return { true, "Deposit successful." };
}

OperationResult Bank::withdraw(const std::string& accountNumber, long long amountInSubunits) {
    auto it = accounts.find(accountNumber);
    if (it == accounts.end()) return { false, "Account not found." };
    if (amountInSubunits <= 0) return { false, "Invalid amount. Withdrawal must be greater than zero." };
    if (!it->second.isActive()) return { false, "Account is closed." };
    if (amountInSubunits > it->second.getBalanceInSubunits()) {
        std::ostringstream oss;
        oss << "Insufficient funds. Available balance: GHS "
            << formatMoney(it->second.getBalanceInSubunits());
        return { false, oss.str() };
    }

    if (!it->second.withdraw(amountInSubunits)) {
        return { false, "Withdrawal failed." };
    }
    recordTransaction(TransactionType::Withdrawal, accountNumber, "", amountInSubunits, "Cash withdrawal");
    return { true, "Withdrawal successful." };
}

// See technical report section "Transaction Atomicity" for the full
// discussion. In short: EVERY validation check below runs before EITHER
// balance is touched. Only once we know the transfer is fully legal do we
// call debit() then credit() -- so a rejected transfer never leaves the
// accounts in a half-updated state, and (in this single-threaded, in-memory
// design) there is no window in which one side could succeed without the
// other.
OperationResult Bank::transferFunds(const std::string& fromAccount,
                                     const std::string& toAccount,
                                     long long amountInSubunits) {
    if (fromAccount == toAccount) {
        return { false, "Source and destination accounts must be different." };
    }

    auto fromIt = accounts.find(fromAccount);
    if (fromIt == accounts.end()) return { false, "Source account not found." };

    auto toIt = accounts.find(toAccount);
    if (toIt == accounts.end()) return { false, "Destination account not found." };

    if (amountInSubunits <= 0) return { false, "Invalid amount. Transfer must be greater than zero." };
    if (!fromIt->second.isActive()) return { false, "Source account is closed." };
    if (!toIt->second.isActive()) return { false, "Destination account is closed." };

    if (amountInSubunits > fromIt->second.getBalanceInSubunits()) {
        std::ostringstream oss;
        oss << "Insufficient funds. Available balance: GHS "
            << formatMoney(fromIt->second.getBalanceInSubunits());
        return { false, oss.str() };
    }

    // --- All validation passed above this line. Only now do we mutate. ---
    fromIt->second.debit(amountInSubunits);
    toIt->second.credit(amountInSubunits);

    recordTransaction(TransactionType::TransferOut, fromAccount, toAccount, amountInSubunits,
                      "Transfer to " + toAccount);
    recordTransaction(TransactionType::TransferIn, toAccount, fromAccount, amountInSubunits,
                      "Transfer from " + fromAccount);

    return { true, "Transfer successful." };
}

std::vector<Transaction> Bank::getTransactionsForAccount(const std::string& accountNumber,
                                                          int limit) const {
    std::vector<Transaction> result;
    // Walk backwards so the most recent transaction comes first, stopping
    // early once `limit` matches have been collected (limit <= 0 = no cap).
    for (auto it = transactions.rbegin(); it != transactions.rend(); ++it) {
        if (it->getAccountNumber() == accountNumber) {
            result.push_back(*it);
            if (limit > 0 && static_cast<int>(result.size()) >= limit) break;
        }
    }
    return result;
}
