#ifndef BANK_H
#define BANK_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Customer.h"
#include "Account.h"
#include "Transaction.h"
#include "FileManager.h"

// A tiny, reusable "result" type: every Bank operation that can fail
// returns one of these instead of throwing exceptions or printing
// directly. This keeps Bank UI-agnostic (main.cpp decides how to show
// success/failure) and makes each operation easy to unit-test.
struct OperationResult {
    bool success;
    std::string message;   // human-readable reason, e.g. "Insufficient funds."
};

// ============================================================
// Bank
// ------------------------------------------------------------
// The central coordinating class of the whole application ("God
// object" risk noted -- see technical report, Limitations). Bank owns
// every Customer, every Account and the full Transaction history, and
// is the ONLY class allowed to link them together (e.g. it is Bank
// that checks "does this customerId exist?" before creating an
// Account for it, and Bank that generates unique IDs).
//
// WHY A CENTRAL CLASS INSTEAD OF PUTTING THIS LOGIC IN main()?
//   - main() becomes a thin loop: show menu, read choice, call a Bank
//     method, print the result. That loop is easy to read completely.
//   - Bank can be reused or tested without any console/menu code at
//     all (e.g. a future GUI or unit-test file could reuse Bank
//     directly).
//   - All the "business rules" (a transfer needs two distinct valid
//     accounts, a customer ID must exist before an account references
//     it, etc.) live in exactly one place instead of being duplicated
//     wherever the menu happens to need them.
//
// DATA STRUCTURE CHOICE: std::unordered_map<std::string, X>
//   Customers and Accounts are looked up by ID/account-number far more
//   often than they are iterated over from start to end, so we index
//   them in a hash map: average O(1) lookup/insert by key, versus O(n)
//   linear search through a std::vector. (Worst case is O(n) if many
//   keys collide, but std::string hashing in the standard library
//   makes that essentially never happen in practice for this dataset
//   size.) Transactions, by contrast, are naturally an append-only,
//   ordered *list* we scan from the end backwards for "recent
//   transactions", so a std::vector (contiguous, cheap to append, easy
//   to iterate in insertion order) is the right fit there instead.
// ============================================================
class Bank {
private:
    std::unordered_map<std::string, Customer> customers;   // key: customerId
    std::unordered_map<std::string, Account> accounts;      // key: accountNumber
    std::vector<Transaction> transactions;                  // insertion order

    FileManager fileManager;

    // Monotonically increasing counters used to generate the next unique
    // ID of each kind. Persisted implicitly: on load, each counter is
    // advanced past the highest ID already found in the loaded data, so
    // restarting the program never reuses an ID that's already on disk.
    long long nextCustomerNumber; // -> "CUST" + number
    long long nextAccountNumber;  // -> 10-digit account number
    long long nextTransactionNumber; // -> "TXN" + number

    std::string generateCustomerId();
    std::string generateAccountNumber();
    std::string generateTransactionId();

    // Appends one Transaction record for `accountNumber` and returns it.
    void recordTransaction(TransactionType type,
                            const std::string& accountNumber,
                            const std::string& relatedAccount,
                            long long amountInSubunits,
                            const std::string& description);

public:
    Bank(const std::string& customersFile,
         const std::string& accountsFile,
         const std::string& transactionsFile);

    // Startup / shutdown persistence ---------------------------------
    // Loads all three files into memory. Safe to call even if none of
    // the files exist yet (first run). Prints a short summary of how
    // many records were loaded / skipped.
    void loadData();
    // Saves all three files. Returns true only if all three succeeded.
    bool saveData();

    // Customer management ---------------------------------------------
    OperationResult createCustomer(const std::string& name,
                                    const std::string& phone,
                                    const std::string& email,
                                    const std::string& address,
                                    std::string& outCustomerId);
    const Customer* findCustomer(const std::string& customerId) const;
    bool viewCustomer(const std::string& customerId) const; // prints; false if not found

    // Account management -----------------------------------------------
    OperationResult createAccount(const std::string& customerId,
                                   AccountType type,
                                   long long initialDepositInSubunits,
                                   std::string& outAccountNumber);
    const Account* findAccount(const std::string& accountNumber) const;
    bool viewAccount(const std::string& accountNumber) const; // prints; false if not found

    // Transactions -------------------------------------------------------
    OperationResult deposit(const std::string& accountNumber, long long amountInSubunits);
    OperationResult withdraw(const std::string& accountNumber, long long amountInSubunits);
    OperationResult transferFunds(const std::string& fromAccount,
                                   const std::string& toAccount,
                                   long long amountInSubunits);

    // Returns transactions for one account, most-recent-first, capped at `limit`
    // (limit <= 0 means "no cap" -> full history).
    std::vector<Transaction> getTransactionsForAccount(const std::string& accountNumber,
                                                        int limit) const;

    // Convenience for the menu / tests
    bool customerExists(const std::string& customerId) const;
    bool accountExists(const std::string& accountNumber) const;
};

#endif // BANK_H
