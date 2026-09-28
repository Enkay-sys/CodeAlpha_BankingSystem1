# CodeAlpha_BankingSystem

A console-based Banking Management System in C++, built for **Task 4 of the
CodeAlpha C++ Programming Internship**.

## Description

This project simulates the core operations of a bank: registering
customers, opening accounts, depositing and withdrawing money, transferring
funds between accounts, and reviewing transaction history — all through a
text menu, with data saved to disk so it survives between runs.

It is an **educational simulation**, not production banking software (see
[Security & Limitations](#security--limitations)).

## Objectives

- Demonstrate real object-oriented design (encapsulation, abstraction) in C++
- Model a small but complete real-world domain: customers, accounts, money
  movement, and an auditable transaction history
- Practice file I/O, input validation, and error handling in a domain where
  correctness genuinely matters

## Features

### Official CodeAlpha Task 4 requirements
- `Customer`, `Account`, and `Transaction` classes with meaningful attributes
- Create and manage customers and accounts
- Deposits, withdrawals, and fund transfers
- Transaction history, with a "recent transactions" view
- Account information display (balance + details)

### Additional enhancements (beyond the minimum requirements)
These are clearly optional and are labelled as such in the menu/code:
- Auto-generated Customer IDs (`CUST1001`), account numbers, and Transaction IDs
- Account types (Savings / Current)
- Account status (Active / Closed)
- Timestamps on every transaction
- Persistent storage across program restarts (`.txt` files)
- A formatted **Account Statement** view
- Defensive parsing of money input (rejects `abc`, negative numbers, `12.5.6`, etc.)

## Architecture

```
main.cpp  ->  thin menu loop only
Bank      ->  central coordinator: owns all data, enforces business rules
Customer  ->  personal/contact info + which accounts they own
Account   ->  balance + controlled deposit/withdraw/credit/debit
Transaction -> one immutable record of a money movement
FileManager -> the only class that touches std::ifstream/std::ofstream
Utils     ->  small free functions shared by the classes above
```

`main.cpp` never manipulates a `Customer`/`Account`/`Transaction` directly —
it only calls `Bank` methods and prints the result. All the banking *rules*
(a transfer needs two different, existing, active accounts; a withdrawal
can't exceed the balance; etc.) live in `Bank`/`Account`, in one place, so
they can't drift out of sync with each other.

## Classes

| Class | Responsibility |
|---|---|
| `Customer` | Holds a customer's identity/contact details and the list of account numbers they own. |
| `Account` | Holds one account's balance and status; the *only* code allowed to change the balance. |
| `Transaction` | An immutable record of one money movement (deposit, withdrawal, transfer-out, transfer-in). |
| `Bank` | Owns every `Customer`/`Account`/`Transaction`, generates unique IDs, and enforces every business rule (a transfer's validation-before-mutation logic, duplicate-ID prevention, etc.). |
| `FileManager` | Reads/writes the three `.txt` data files; the only class using `<fstream>`. |
| `Utils` | Free helper functions: string splitting, timestamp generation, money formatting/parsing. |

**Relationship:** one `Customer` can own many `Account`s (a `Customer`
stores a list of account numbers); each `Account` belongs to exactly one
`Customer` (stored as `customerId` on the `Account`). Each `Transaction`
belongs to exactly one `Account` (a transfer produces two linked
`Transaction` records — a `TransferOut` on the source account and a
`TransferIn` on the destination account — so each account's own history is
simply "every transaction whose account number matches mine").

## Technologies

- **C++17** (standard library only: `<string>`, `<vector>`, `<unordered_map>`,
  `<fstream>`, `<sstream>`, `<ctime>` — no external dependencies)
- Console I/O via `<iostream>`

## Project Structure

```
BankingSystem/
├── main.cpp
├── Customer.h / Customer.cpp
├── Account.h / Account.cpp
├── Transaction.h / Transaction.cpp
├── Bank.h / Bank.cpp
├── FileManager.h / FileManager.cpp
├── Utils.h / Utils.cpp
├── customers.txt        (created on first run)
├── accounts.txt          (created on first run)
├── transactions.txt      (created on first run)
├── README.md
├── TECHNICAL_REPORT.md
└── .gitignore
```

## Compilation

Requires a C++17-capable compiler (e.g. g++ 9+):

```bash
g++ -std=c++17 -Wall -Wextra -o banking main.cpp Customer.cpp Account.cpp Transaction.cpp Bank.cpp FileManager.cpp Utils.cpp
```

## Execution

```bash
./banking
```

On Windows, replace `./banking` with `banking.exe`.

## Usage

You'll see a numbered menu. Type the number of the operation you want and
press Enter:

```
========================================
              BANKING SYSTEM
========================================
 1. Create Customer
 2. Create Account
 3. View Customer
 4. View Account
 5. Deposit
 6. Withdraw
 7. Transfer Funds
 8. View Full Transaction History (one account)
 9. View Recent Transactions (last 5, one account)
10. Account Statement
11. Exit
========================================
Enter your choice:
```

A typical first run: create a customer (option 1) → note the `CUST####`
ID → create an account for that customer ID (option 2) → note the account
number → deposit/withdraw/transfer using that account number.

## Data Storage

Data is stored in three plain-text, pipe-delimited (`|`) files in the
working directory:

- `customers.txt` — `customerId|name|phone|email|address|acc1,acc2,...`
- `accounts.txt` — `accountNumber|customerId|accountType|balanceInSubunits|status|creationDate`
- `transactions.txt` — `transactionId|type|accountNumber|relatedAccount|amountInSubunits|timestamp|description`

**Money is stored as an integer number of pesewas/cents** (`long long`),
not as a `double`. See [`TECHNICAL_REPORT.md`](TECHNICAL_REPORT.md#money-representation)
for why: repeated floating-point deposits/withdrawals can silently drift
away from the "true" value by fractions of a cent, which is unacceptable
for money. All display formatting converts back to `123.45`-style output
only when printing.

If a data file is missing, the program treats that as "nothing saved yet"
and starts with empty data — it does not crash. If a line in a data file
is malformed, that single line is skipped (and reported at startup) rather
than aborting the whole load.

## Transaction Processing

Every successful deposit, withdrawal, or transfer appends one (or, for a
transfer, two) `Transaction` records with a unique ID, a timestamp, and a
description. A **failed** operation (insufficient funds, invalid amount,
account not found, etc.) never creates a transaction record and never
changes any balance — see `TECHNICAL_REPORT.md` for the atomicity argument.

## Testing

See [`TECHNICAL_REPORT.md`](TECHNICAL_REPORT.md#14-testing) for the full
test plan (29 cases covering customers, accounts, deposits, withdrawals,
transfers, transaction records, and persistence across restarts). All
cases were run manually against the compiled binary before this project
was submitted.

## Complexity

See [`TECHNICAL_REPORT.md`](TECHNICAL_REPORT.md#15-complexity-analysis).
Customer/account lookup is average **O(1)** (`std::unordered_map`);
transaction history for one account is **O(n)** in the total number of
transactions recorded so far.

## Security & Limitations

This is a **teaching project**, not a real banking system:

- There is no login/authentication — anyone running the program can act as
  any customer or account.
- Data files are plain, unencrypted text, readable by anyone with
  filesystem access.
- There is no concurrency control — it assumes a single user, single
  process at a time.
- No PIN/password system is implemented (optional and out of scope here);
  if one were added, PINs would need to be hashed, never stored in plain
  text.

See `TECHNICAL_REPORT.md` for the full discussion, plus what a
production-grade version would need instead (a real database with
transactions/ACID guarantees, hashed credentials, encrypted storage,
proper session-based authorization, etc.).

## Future Improvements

- PIN/password authentication per customer
- `SavingsAccount`/`CurrentAccount` subclasses if interest rates or
  overdraft rules ever need to differ by type
- A real database backend instead of flat files
- Interest calculation and scheduled statements
- Multi-currency support

## Author

Prepared as a CodeAlpha C++ Programming Internship submission (Task 4).
