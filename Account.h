#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>

// enum class -> a strongly-typed, scoped enumeration.
// "Strongly-typed" means an AccountType can't silently be used as a plain
// int, and "scoped" means the names (Savings, Current) live inside
// AccountType:: and won't collide with other identifiers in the program.
// This is safer than a plain C-style `enum`.
enum class AccountType {
    Savings,
    Current
};

enum class AccountStatus {
    Active,
    Closed
};

std::string accountTypeToString(AccountType type);
AccountType accountTypeFromString(const std::string& text);

std::string accountStatusToString(AccountStatus status);
AccountStatus accountStatusFromString(const std::string& text);

// ============================================================
// Account
// ------------------------------------------------------------
// Represents one bank account belonging to exactly one customer
// (see Bank.h for how the Customer <-> Account relationship is
// enforced). The account is where money actually lives.
//
// IMPORTANT DESIGN DECISION (see README / technical report, section
// "Money Representation"): the balance is stored internally as an
// integer number of the smallest currency subunit (pesewas/cents),
// NOT as a double. Financial values must never lose precision to
// floating-point rounding, so `long long balanceInSubunits` is used
// instead of `double balance`.
//
// The balance is private and can only change through deposit()/
// withdraw()/credit()/debit(), each of which validates its input.
// Nothing outside this class can do `account.balanceInSubunits = -1;`
// -- that line would not even compile, because the member is private.
// This is what "protecting the balance from uncontrolled modification"
// (encapsulation) means in practice.
// ============================================================
class Account {
private:
    std::string accountNumber;   // unique, e.g. "1000000001"
    std::string customerId;      // owner's Customer::customerId
    AccountType accountType;
    long long balanceInSubunits; // e.g. 150000 == GHS 1,500.00
    AccountStatus status;
    std::string creationDate;    // "YYYY-MM-DD HH:MM:SS"

public:
    Account(); // default constructor (required for use inside containers)
    Account(const std::string& accountNumber,
            const std::string& customerId,
            AccountType accountType,
            long long initialBalanceInSubunits,
            const std::string& creationDate);

    // Getters -------------------------------------------------------
    std::string getAccountNumber() const;
    std::string getCustomerId() const;
    AccountType getAccountType() const;
    long long getBalanceInSubunits() const;
    AccountStatus getStatus() const;
    std::string getCreationDate() const;
    bool isActive() const;

    // Controlled balance operations ---------------------------------
    // Each returns true on success. On failure, the balance is left
    // completely unchanged -- a failed operation must never have a
    // partial or corrupting side effect (see "Transaction Integrity"
    // in the technical report).
    //
    // deposit()/withdraw() are the *public-facing* operations invoked
    // from the menu. credit()/debit() are lower-level primitives that
    // Bank::transferFunds() uses internally once it has already
    // validated both accounts; they exist so a transfer does not have
    // to (mis)use the withdrawal/deposit validation messages meant for
    // direct user actions.
    bool deposit(long long amountInSubunits);
    bool withdraw(long long amountInSubunits);
    bool credit(long long amountInSubunits); // add funds (used by transfer-in)
    bool debit(long long amountInSubunits);  // remove funds (used by transfer-out)

    void closeAccount();

    // Display ----------------------------------------------------
    void displayInfo() const;

    // Serialization --------------------------------------------------
    std::string serialize() const;
    static bool deserialize(const std::string& line, Account& outAccount);
};

#endif // ACCOUNT_H
