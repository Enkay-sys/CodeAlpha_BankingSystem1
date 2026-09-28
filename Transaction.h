#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

// Why an enum for transaction type instead of a raw std::string?
// - It rules out typos ("Deposit" vs "deposit" vs "DEPOSIT") at compile time.
// - Comparisons (type == TransactionType::Deposit) are cheap integer
//   comparisons rather than string comparisons.
// - `enum class` (rather than plain `enum`) additionally requires every use
//   to be qualified as TransactionType::Deposit, which avoids name clashes
//   and stops it being accidentally compared to an unrelated int.
enum class TransactionType {
    Deposit,
    Withdrawal,
    TransferOut,  // this account was the *source* of a transfer
    TransferIn    // this account was the *destination* of a transfer
};

std::string transactionTypeToString(TransactionType type);
TransactionType transactionTypeFromString(const std::string& text);

// ============================================================
// Transaction
// ------------------------------------------------------------
// An immutable record of one movement of money. We model this as a
// structured object (rather than writing free-form strings straight to
// a log) so that every record is guaranteed to have all the fields a
// statement or audit needs, in a predictable format, and so the program
// can filter/sort/search transactions (e.g. "last 10 for account X")
// instead of having to re-parse loosely formatted text.
//
// A single transfer produces TWO Transaction records: one TransferOut
// row on the source account and one TransferIn row on the destination
// account. This keeps "recent transactions for account X" simple --
// each account's history is just "every Transaction whose accountNumber
// == X" -- at the small cost of storing the transfer twice.
// ============================================================
class Transaction {
private:
    std::string transactionId;     // e.g. "TXN10001"
    TransactionType type;
    std::string accountNumber;     // the account this record belongs to
    std::string relatedAccount;    // other side of a transfer; empty otherwise
    long long amountInSubunits;    // always stored positive; type says direction
    std::string timestamp;
    std::string description;

public:
    Transaction();
    Transaction(const std::string& transactionId,
                TransactionType type,
                const std::string& accountNumber,
                const std::string& relatedAccount,
                long long amountInSubunits,
                const std::string& timestamp,
                const std::string& description);

    std::string getTransactionId() const;
    TransactionType getType() const;
    std::string getAccountNumber() const;
    std::string getRelatedAccount() const;
    long long getAmountInSubunits() const;
    std::string getTimestamp() const;
    std::string getDescription() const;

    // Returns true for types that increase the account's balance
    // (Deposit, TransferIn) -- used to decide the +/- sign when printing.
    bool isCredit() const;

    void display() const;         // full detail view
    void displayShort() const;    // one-line view, used in "recent transactions"

    std::string serialize() const;
    static bool deserialize(const std::string& line, Transaction& outTransaction);
};

#endif // TRANSACTION_H
