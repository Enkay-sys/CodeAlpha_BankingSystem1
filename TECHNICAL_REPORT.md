# Technical Report — CodeAlpha_BankingSystem (Task 4)

## 1. Introduction

This report documents a console-based Banking Management System written in
C++17, submitted for Task 4 of the CodeAlpha C++ Programming Internship.
The system models customers, their accounts, and the transactions that move
money between accounts, with all data persisted to disk between runs.

## 2. Problem Statement

CodeAlpha Task 4 asks for a banking system that models `Customer`,
`Account`, and `Transaction` as classes, supports creating and managing
customers/accounts, supports deposits/withdrawals/transfers, and keeps a
transaction history the user can review. The challenge is not the banking
domain itself (which is simple) but doing it with **real** OOP structure,
correct **input validation**, and **safe** money handling — the kind of
correctness a real financial system would need, at a scale appropriate for
a learning project.

## 3. Objectives

- Model the domain with clean, single-responsibility classes.
- Guarantee that a failed operation (bad input, insufficient funds, unknown
  account) never corrupts a balance or creates a phantom transaction.
- Persist data to plain text files, tolerating a missing or partially
  corrupted file without crashing.
- Keep every design decision explainable in plain language, so the author
  can defend the code to a mentor without reciting memorized text.

## 4. Functional Requirements

1. Create a customer with contact details and a unique, system-generated ID.
2. Create an account for an existing customer, with a unique account number.
3. View a customer's details and the accounts they own.
4. View an account's details and balance.
5. Deposit a positive amount into an account.
6. Withdraw a positive amount, rejected if it exceeds the balance.
7. Transfer a positive amount between two different, existing, active accounts.
8. Record every successful operation as a `Transaction`.
9. View the full transaction history for an account.
10. View the 5 most recent transactions for an account.
11. View a formatted account statement (enhancement).
12. Persist and reload all data across program restarts.
13. Reject invalid menu input without crashing.

## 5. Non-Functional Requirements

- **Correctness over cleverness**: no operation may leave the system in an
  inconsistent state (e.g. money debited from one account but never
  credited to another).
- **Robustness**: missing files, corrupted lines, and bad user input must
  be handled gracefully, never with a crash.
- **Readability**: every class has one clear job; `main.cpp` contains no
  business logic.
- **No external dependencies**: standard library only, so the project
  builds anywhere a C++17 compiler is available.

## 6. System Architecture

```
                +--------------------+
                |      main.cpp      |   <- menu loop only
                +---------+----------+
                          |
                          v
                    +-----------+
                    |   Bank    |   <- central coordinator / business rules
                    +-----+-----+
             owns   /     |     \   owns
                    v      |      v
          +-----------+    |    +--------------+
          | Customer  |    |    |   Account    |
          +-----------+    |    +--------------+
                            v
                     +-------------+
                     | Transaction |
                     +-------------+
                            ^
                            | uses (file I/O only)
                     +-------------+
                     | FileManager |
                     +-------------+
```

`Bank` is the only class allowed to link `Customer`, `Account`, and
`Transaction` together. `FileManager` is the only class that touches
`<fstream>`. `main.cpp` never sees a raw `Customer`/`Account` object being
mutated directly — it only calls `Bank` methods.

Why a central `Bank` class instead of putting logic in `main()`? Because
otherwise every menu handler would need to re-implement "does this
customer exist?", "is this amount valid?", and so on, and those checks
would inevitably drift out of sync between handlers. With one `Bank`
class, each rule is written and tested exactly once.

## 7. Class Design

### `Customer`
- **Purpose**: represent one bank customer's identity/contact info and
  which accounts they own.
- **Attributes**: `customerId`, `name`, `phone`, `email`, `address`,
  `accountNumbers` (list of accounts owned).
- **Methods**: getters; validated setters (`setName`, etc., reject empty
  strings); `addAccountNumber`; `displayInfo`; `serialize`/`deserialize`
  for file storage.
- **Relationships**: one `Customer` → many `Account`s.

### `Account`
- **Purpose**: hold one account's balance and status, and be the *only*
  code path that can change that balance.
- **Attributes**: `accountNumber`, `customerId`, `accountType`,
  `balanceInSubunits` (a `long long`, not a `double` — see §26/§Money
  Representation below), `status`, `creationDate`.
- **Methods**: `deposit`/`withdraw` (validate before mutating; return
  `false` and change nothing on failure), `credit`/`debit` (lower-level
  primitives used internally by `Bank::transferFunds`), `closeAccount`,
  `displayInfo`, `serialize`/`deserialize`.
- **Relationships**: one `Account` → one owning `Customer` (by
  `customerId`); one `Account` → many `Transaction`s.

### `Transaction`
- **Purpose**: an immutable record of one money movement.
- **Attributes**: `transactionId`, `type` (`enum class TransactionType`),
  `accountNumber`, `relatedAccount` (only set for transfers),
  `amountInSubunits`, `timestamp`, `description`.
- **Methods**: getters, `isCredit()` (decides the +/- sign shown),
  `display`/`displayShort`, `serialize`/`deserialize`.
- **Relationships**: belongs to exactly one `Account` (by
  `accountNumber`); a transfer produces two linked records (`TransferOut`
  on the source, `TransferIn` on the destination).

### `Bank` (central coordinating class)
- **Purpose**: own every `Customer`/`Account`/`Transaction`, generate
  unique IDs, and enforce every cross-object business rule.
- **Attributes**: `unordered_map<string, Customer> customers`,
  `unordered_map<string, Account> accounts`,
  `vector<Transaction> transactions`, a `FileManager`, and three ID
  counters.
- **Methods**: `createCustomer`, `createAccount`, `deposit`, `withdraw`,
  `transferFunds`, `getTransactionsForAccount`, `loadData`, `saveData`,
  plus lookup helpers.
- **Relationships**: composes/owns all other domain objects; the only
  class that cross-references them.

### `FileManager`
- **Purpose**: isolate every `std::ifstream`/`std::ofstream` call in one
  place, so persistence can change (e.g. to a database) without touching
  `Bank`'s business logic.
- **Methods**: `loadCustomers/Accounts/Transactions`,
  `saveCustomers/Accounts/Transactions`. Load functions tolerate a missing
  file (returns as if nothing was found) and skip (rather than abort on)
  a corrupted line, reporting the count of skipped lines to the caller.

### `Utils` (free functions, not a class)
- `splitString`, `currentTimestamp`, `formatMoney`,
  `parseMoneyToSubunits`, `readLineTrimmed`. Kept as free functions
  because they don't hold any state of their own — wrapping them in a
  class with only `static` methods would add ceremony without benefit.

## 8. Data Structures

- **`std::unordered_map<std::string, Customer>` / `<std::string, Account>`**:
  customers and accounts are looked up by ID far more often than they are
  iterated top-to-bottom, so a hash map gives average **O(1)** lookup/
  insert by key, versus **O(n)** for a linear scan through a `vector`. The
  key is the ID string; the hash function is the standard library's
  built-in `std::hash<std::string>`. Worst case is O(n) if many keys
  hashed to the same bucket, but that's not a realistic concern at this
  project's scale.
- **`std::vector<Transaction>`**: transactions are naturally an
  append-only, ordered log. A `vector` appends in amortized O(1) and
  keeps insertion order, which is exactly what "recent transactions"
  needs (walk backwards from the end).
- **`enum class TransactionType` / `AccountType` / `AccountStatus`**:
  scoped, strongly-typed enumerations instead of raw strings or `int`
  codes — see §31 discussion below.

## 9. Algorithms

- **Deposit**: find account by key → validate amount > 0 and account
  active → `balance += amount` → append `Transaction`.
- **Withdraw**: find account → validate amount > 0, account active,
  amount ≤ balance → `balance -= amount` → append `Transaction`. If any
  validation fails, the balance is untouched and no transaction is
  recorded.
- **Transfer**: validate *both* accounts exist, are distinct, are active,
  and the source has sufficient funds — **all before mutating anything**
  — then debit source, credit destination, and append two linked
  `Transaction` records. See §19/Transaction Atomicity below for why the
  order matters.
- **Recent transactions**: walk the `transactions` vector from the end
  backwards, collecting records whose `accountNumber` matches, stopping
  once `limit` matches are found (or reaching the start for "full
  history").

## 10. File Storage

Three pipe-delimited (`|`) text files, one record per line:

```
customers.txt:     customerId|name|phone|email|address|acc1,acc2,...
accounts.txt:      accountNumber|customerId|accountType|balanceInSubunits|status|creationDate
transactions.txt:  transactionId|type|accountNumber|relatedAccount|amountInSubunits|timestamp|description
```

`FileManager::load*` opens each file; if `ifstream::is_open()` is false,
that's treated as "no data yet" (first run), not an error. Each line is
handed to the matching class's `deserialize()`, which returns `false` on
a malformed line (e.g. a corrupted numeric field). Malformed lines are
skipped and counted, not treated as fatal. `FileManager::save*` truncates
and rewrites the whole file from the current in-memory state — simple and
correct for this project's scale, though it means a save is all-or-nothing
work each time (see Limitations).

## 11. Transaction Processing

Every successful deposit/withdrawal/transfer appends a `Transaction` with
a fresh unique ID (`TXN####`), a timestamp from `<ctime>`, and a
description. IDs are generated by simple monotonically increasing
counters that, on `loadData()`, are advanced past the highest ID already
found on disk — so restarting the program never reuses an ID that's
already saved.

## 12. Error Handling

Every `Bank` operation that can fail returns an `OperationResult { bool
success; std::string message; }` instead of throwing an exception or
printing directly — this keeps `Bank` UI-agnostic and keeps the failure
reason attached to the attempt that produced it. `main.cpp` is the only
place that prints these messages to the console. Handled cases include:
customer/account not found, non-positive or unparsable amounts,
insufficient funds, same source/destination account, closed accounts, and
an unopenable file on save. No internal exception type or raw errno is
ever shown to the user — messages are all plain English.

## 13. Security Considerations

This project is explicitly an **educational simulation**, not real banking
software. It has no authentication, so anyone running the executable can
view or move money in any account. Data files are unencrypted plain text.
There is no PIN/password system (documented as an *optional* enhancement
in the assignment, deliberately left out here to avoid adding
complexity that isn't load-bearing for the assignment's actual goals).

If a PIN system were added: **authentication** answers "who are you?"
(matching a supplied Customer ID + PIN), while **authorization** answers
"what are you allowed to do?" (e.g. can this authenticated customer see
account 1000000005, which they don't own?). A real implementation must
never store a PIN or password in plain text — it should store a salted
hash (e.g. bcrypt/argon2) and compare hashes, never the raw secret.

This project is not production-ready: no encryption at rest, no
concurrency control (two simultaneous processes writing the same file
would corrupt it), no audit-proof storage (a text file can be hand-edited
by anyone with disk access), and no real identity verification.

## 14. Testing

All 29 cases below were executed manually against the compiled binary
(`g++ -std=c++17 -Wall -Wextra`, zero warnings) prior to submission.

### Customer Tests
1. Create valid customer → **Pass**, returns `CUST1001`.
2. Create a second customer → **Pass**, returns `CUST1002` (no collision — IDs are counter-generated, so a literal "duplicate ID" attempt is not user-reachable; uniqueness is guaranteed by construction).
3. Create customer with empty name/phone → **Pass**, rejected with "Name and phone number are required."

### Account Tests
4. Create account for a valid, existing customer → **Pass**.
5. Create account for a nonexistent customer ID → **Pass**, rejected with "Customer not found."
6. Create two accounts for the same customer → **Pass**, both get distinct account numbers and both are listed under that customer.

### Deposit Tests
7. Deposit a valid positive amount → **Pass**, balance increases, transaction recorded.
8. Deposit `0` → **Pass**, rejected ("must be greater than zero"), balance unchanged.
9. Deposit a negative amount → **Pass**, rejected at input-parsing stage (`parseMoneyToSubunits` refuses a leading `-`) before it ever reaches the account.
10. Deposit into a nonexistent account → **Pass**, rejected with "Account not found."

### Withdrawal Tests
11. Withdraw a valid amount ≤ balance → **Pass**.
12. Withdraw more than the balance → **Pass**, rejected with "Insufficient funds. Available balance: GHS X.XX", balance unchanged.
13. Withdraw `0` → **Pass**, rejected.
14. Withdraw a negative amount → **Pass**, rejected at parsing.
15. Withdraw from a nonexistent account → **Pass**, rejected.

### Transfer Tests
16. Transfer a valid amount between two existing accounts → **Pass**.
17. Transfer more than the source balance → **Pass**, rejected, neither balance changes.
18. Transfer to a nonexistent destination → **Pass**, rejected with "Destination account not found."
19. Transfer from a nonexistent source → **Pass**, rejected with "Source account not found."
20. Transfer to the same account (source == destination) → **Pass**, rejected with "Source and destination accounts must be different."
21. Verify both balances after a successful transfer → **Pass**: source balance − amount, destination balance + amount, confirmed via `viewAccount`.

### Transaction Tests
22. Verify a deposit creates a transaction record → **Pass**, visible in history with type `DEPOSIT`.
23. Verify a withdrawal creates a transaction record → **Pass**, type `WITHDRAWAL`.
24. Verify a transfer creates transaction records on **both** accounts → **Pass**, `TRANSFER-OUT` on source, `TRANSFER-IN` on destination, cross-referencing each other's account number.
25. Verify transaction history displays correctly, most-recent-first → **Pass**.

### Persistence Tests
26. Create a customer and account → **Pass**.
27. Exit the program (option 11) → **Pass**, "All data saved" printed, files written.
28. Restart the program → **Pass**, "Data loaded: 1 customer(s), 2 account(s), ..." printed.
29. Verify the customer/account/transaction data still exists after restart → **Pass**, confirmed via `viewCustomer`/`viewAccount`/history.

### Additional Edge Cases Verified
- Invalid (non-numeric) menu choice → does not crash, prints "Invalid menu option."
- Balance exactly equal to withdrawal amount → succeeds, resulting balance is exactly `0`.
- Deposit amount `12.5.6` (malformed decimal) → rejected by `parseMoneyToSubunits`, re-prompts.
- Missing data files on first run → program starts normally with "Data loaded: 0 customer(s)...".

## 15. Complexity Analysis

| Operation | Complexity | Why |
|---|---|---|
| Customer lookup by ID | Average O(1) | `unordered_map` hash lookup |
| Account lookup by number | Average O(1) | `unordered_map` hash lookup |
| Deposit / Withdraw | Average O(1) | one lookup + O(1) mutation + O(1) append to transaction log |
| Transfer | Average O(1) | two lookups + O(1) mutations + two O(1) appends |
| Transaction history for one account | O(n) | n = total transactions ever recorded (must scan, since transactions aren't separately indexed by account) |
| Loading files at startup | O(c + a + t) | linear in the number of customer/account/transaction lines |
| Saving files | O(c + a + t) | linear rewrite of each file |

"Average O(1)" for the hash-map operations assumes a reasonably distributed
hash and load factor, which `std::unordered_map`'s default resizing
behavior maintains; it is **not** a worst-case guarantee (a pathological
hash collision could degrade to O(n)), but that is not a realistic concern
for this dataset. Transaction history is the one place this project is
genuinely O(n) rather than O(1): with heavier use, a
`std::unordered_map<std::string, std::vector<size_t>>` index from account
number to transaction indices would make per-account history O(k) in the
number of that account's own transactions instead of scanning everything —
listed under Future Improvements as a reasonable next step, not implemented
here because it isn't needed at this project's scale.

## 16. Limitations

- No authentication/authorization.
- Flat-file storage, rewritten in full on every save (fine at this scale;
  would not scale to a large bank).
- No concurrency control.
- Transaction history lookup is O(n) rather than indexed.
- No interest calculation, overdraft, or multi-currency support.

## 17. Future Improvements

- PIN/password-based authentication with hashed credential storage.
- `SavingsAccount`/`CurrentAccount` subclasses if their business rules
  (interest, overdraft limits) ever genuinely diverge.
- A real database (e.g. SQLite) instead of flat files, for atomic writes
  and indexed lookup.
- Per-account transaction indexing for O(k) history lookup.
- Scheduled interest and generated statements.

## 18. Conclusion

The system satisfies every official CodeAlpha Task 4 requirement — three
purposeful classes, customer/account management, deposits, withdrawals,
transfers, and reviewable transaction history — plus several clearly
labelled enhancements (persistent storage, an account statement view,
defensive money parsing). Every design choice above (integer money
representation, validate-before-mutate transfers, hash-map lookups,
`enum class` for transaction types) has a stated reason, so the author can
explain and defend the implementation rather than just submit it.

---

## Money Representation

Money is stored as `long long balanceInSubunits` — an integer count of the
smallest currency unit (pesewas/cents), e.g. `GHS 12.50` is stored
internally as `1250`. **`double` is deliberately avoided for balances.**

Floating-point numbers cannot represent most decimal fractions exactly in
binary (`0.1` has no exact `double` representation, for instance). Adding
and subtracting many small `double` amounts over time can accumulate tiny
rounding errors that eventually surface as a balance that's off by a
fraction of a cent — unacceptable for money, where a customer's statement
must always add up exactly. Storing an integer count of the smallest unit
sidesteps this entirely: integer addition/subtraction is exact. The
tradeoff is that all arithmetic must stay in subunits internally, and
conversion to/from a human-readable "12.50" string happens only at the
input (`parseMoneyToSubunits`) and output (`formatMoney`) boundaries —
which is exactly what those two `Utils` functions exist to isolate.

## Transaction Atomicity

A transfer is logically two steps: debit the source, credit the
destination. If the program performed the debit and then discovered the
destination was invalid (or crashed) before the credit happened, money
would simply vanish — debited from one account, never credited anywhere.
`Bank::transferFunds` avoids this by running **every** validation check
(both accounts exist, are distinct, are active, and the source has
sufficient funds) **before touching either balance**. Only after every
check has passed does it call `debit()` then `credit()`. Because this
project is single-threaded and in-memory, there is no window in which
another operation could interleave between the debit and the credit, so
this ordering is sufficient here (a real multi-user bank would additionally
need a database transaction/lock around both writes — noted under
Limitations).

## Recent Transactions — Definition

"Recent transactions" is defined as **the 5 most recently recorded
transactions belonging to the account being viewed**, most-recent-first.
This is implemented by walking `Bank`'s transaction log from the end
backwards and collecting matches for that account number, stopping once 5
are found. "Full history" (menu option 8) is the same walk with no cap.

---

## UML-Style Class Diagram

```
+---------------------------+
|         Customer          |
+---------------------------+
| - customerId: string      |
| - name, phone, email,     |
|   address: string         |
| - accountNumbers:         |
|     vector<string>        |
+---------------------------+
| + getCustomerId()         |
| + setName()/setPhone()/...|
| + addAccountNumber()      |
| + displayInfo()           |
| + serialize()/deserialize()|
+---------------------------+
             | owns (1 : many)
             v
+---------------------------+
|          Account          |
+---------------------------+
| - accountNumber: string   |
| - customerId: string      |
| - accountType: AccountType|
| - balanceInSubunits:      |
|     long long             |
| - status: AccountStatus   |
| - creationDate: string    |
+---------------------------+
| + deposit()/withdraw()    |
| + credit()/debit()        |
| + getBalanceInSubunits()  |
| + displayInfo()           |
| + serialize()/deserialize()|
+---------------------------+
             | generates (1 : many)
             v
+---------------------------+
|        Transaction        |
+---------------------------+
| - transactionId: string   |
| - type: TransactionType   |
| - accountNumber: string   |
| - relatedAccount: string  |
| - amountInSubunits:       |
|     long long             |
| - timestamp: string       |
| - description: string     |
+---------------------------+
| + isCredit()              |
| + display()/displayShort()|
| + serialize()/deserialize()|
+---------------------------+

+---------------------------+        +---------------------------+
|            Bank            |------> |        FileManager        |
+---------------------------+  uses  +---------------------------+
| - customers: unordered_map |        | - customersFile: string   |
| - accounts: unordered_map  |        | - accountsFile: string    |
| - transactions: vector     |        | - transactionsFile: string|
| - fileManager: FileManager |        +---------------------------+
| - next*Number: long long   |        | + load*()  + save*()      |
+---------------------------+        +---------------------------+
| + createCustomer()         |
| + createAccount()          |
| + deposit()/withdraw()     |
| + transferFunds()          |
| + getTransactionsForAccount()|
| + loadData()/saveData()    |
+---------------------------+
```

`Bank` composes (owns) collections of `Customer`, `Account`, and
`Transaction`, and delegates all file I/O to `FileManager`. One `Customer`
can own many `Account`s; each `Account` belongs to exactly one `Customer`.
One `Account` generates many `Transaction`s over its lifetime; a transfer
links two `Transaction` records (one per account) via `relatedAccount`.

---

## Pseudocode

### Create Customer
```
FUNCTION createCustomer(name, phone, email, address):
    IF name is empty OR phone is empty:
        RETURN failure "Name and phone number are required."
    id = generateNextCustomerId()      // e.g. "CUST1001", guaranteed unused
    store new Customer(id, name, phone, email, address)
    RETURN success, id
```

### Create Account
```
FUNCTION createAccount(customerId, accountType, initialDeposit):
    IF customerId does not exist:
        RETURN failure "Customer not found."
    IF initialDeposit < 0:
        RETURN failure "Initial deposit cannot be negative."
    accNum = generateNextAccountNumber()
    store new Account(accNum, customerId, accountType, initialDeposit)
    add accNum to that customer's account list
    IF initialDeposit > 0:
        record a DEPOSIT transaction for accNum
    RETURN success, accNum
```

### Deposit
```
FUNCTION deposit(accountNumber, amount):
    IF account not found: RETURN failure "Account not found."
    IF amount <= 0: RETURN failure "Invalid amount."
    IF account is closed: RETURN failure "Account is closed."
    account.balance += amount
    record a DEPOSIT transaction
    RETURN success
```

### Withdraw
```
FUNCTION withdraw(accountNumber, amount):
    IF account not found: RETURN failure "Account not found."
    IF amount <= 0: RETURN failure "Invalid amount."
    IF account is closed: RETURN failure "Account is closed."
    IF amount > account.balance: RETURN failure "Insufficient funds."
    account.balance -= amount
    record a WITHDRAWAL transaction
    RETURN success
```

### Transfer
```
FUNCTION transferFunds(fromAcc, toAcc, amount):
    IF fromAcc == toAcc: RETURN failure "Accounts must be different."
    IF fromAcc not found: RETURN failure "Source account not found."
    IF toAcc not found: RETURN failure "Destination account not found."
    IF amount <= 0: RETURN failure "Invalid amount."
    IF either account is closed: RETURN failure "Account is closed."
    IF amount > fromAcc.balance: RETURN failure "Insufficient funds."
    // --- only now mutate ---
    fromAcc.balance -= amount
    toAcc.balance += amount
    record TRANSFER-OUT transaction on fromAcc (linked to toAcc)
    record TRANSFER-IN transaction on toAcc (linked to fromAcc)
    RETURN success
```

### View Account
```
FUNCTION viewAccount(accountNumber):
    IF account not found: RETURN failure
    print account number, customer id, type, status, balance, creation date
    RETURN success
```

### View Transaction History
```
FUNCTION getTransactionsForAccount(accountNumber, limit):
    result = empty list
    FOR each transaction in allTransactions, walking from newest to oldest:
        IF transaction.accountNumber == accountNumber:
            append to result
            IF limit > 0 AND result has `limit` items: STOP
    RETURN result
```

### Load Data
```
FUNCTION loadData():
    read customers.txt line by line -> parse into Customer objects (skip bad lines)
    read accounts.txt line by line -> parse into Account objects (skip bad lines)
    read transactions.txt line by line -> parse into Transaction objects (skip bad lines)
    advance each ID counter past the highest ID found, so new IDs never collide
    report counts loaded / skipped
```

### Save Data
```
FUNCTION saveData():
    write every Customer to customers.txt (one line each)
    write every Account to accounts.txt
    write every Transaction to transactions.txt
    RETURN true only if all three writes succeeded
```

### Main Menu
```
LOOP:
    print menu (1-11)
    choice = read integer (treat non-numeric input as invalid)
    MATCH choice:
        1 -> run Create Customer flow
        2 -> run Create Account flow
        3 -> run View Customer flow
        4 -> run View Account flow
        5 -> run Deposit flow
        6 -> run Withdraw flow
        7 -> run Transfer flow
        8 -> run Full History flow
        9 -> run Recent Transactions flow
        10 -> run Account Statement flow
        11 -> save data and exit loop
        default -> print "Invalid menu option."
```

---

## System Flow (Textual Flowchart)

```
START
  |
  v
Load saved data (customers.txt, accounts.txt, transactions.txt)
  |
  v
Display menu <--------------------------------------------+
  |                                                        |
  v                                                        |
User selects an option                                     |
  |                                                        |
  +-- Create Customer -> validate -> generate ID -> save --+
  +-- Create Account  -> verify customer exists ->          |
  |                       generate account number -> save --+
  +-- Deposit  -> find account -> validate amount ->        |
  |                increase balance -> record transaction --+
  +-- Withdraw -> find account -> validate amount ->        |
  |                check balance -> decrease balance ->      |
  |                record transaction -----------------------+
  +-- Transfer -> find both accounts -> validate everything ->|
  |                update both balances -> record 2 txns ----+
  +-- View History / Recent / Statement -> display -----------+
  |
  +-- Exit
        |
        v
   Save all data
        |
        v
       END
```

---

## GitHub Setup

**Recommended repository name:** `CodeAlpha_BankingSystem`

**Folder structure to push:** exactly the `BankingSystem/` folder contents
shown in the README (source files, `README.md`, `TECHNICAL_REPORT.md`,
`.gitignore`). Do **not** commit the compiled binary or the runtime
`.txt` data files — `.gitignore` already excludes them.

**Commands:**
```bash
cd BankingSystem
git init
git add .
git commit -m "Task 4: Banking System - initial submission"
git branch -M main
git remote add origin https://github.com/<your-username>/CodeAlpha_BankingSystem.git
git push -u origin main
```

Do not commit any real personal data, PINs, or credentials — this project
has none by design, but the reminder stands for any future changes.

---

## Presentation Preparation — Mentor/Interviewer Q&A

**1. Why did you use classes?**
To model the domain directly: a `Customer` and an `Account` are naturally
separate things with separate data and rules, and classes let me bundle
each one's data with the operations that are allowed to touch it.

**2. Why are Customer, Account and Transaction separate classes?**
Each has a distinct responsibility and lifecycle: a customer's contact
details don't change when they make a deposit; an account's balance
changes constantly but a transaction, once created, never changes at all.
Splitting them keeps each class's rules simple and independent.

**3. What is encapsulation?**
Keeping an object's data `private` and only allowing it to be read or
changed through that object's own `public` methods, so the object can
enforce its own rules (e.g. an `Account` can refuse a negative deposit)
instead of trusting every caller to remember to check.

**4. Why is balance private?**
So nothing outside `Account` can set it to an invalid value directly
(`account.balance = -5000;` would not even compile). All changes must go
through `deposit()`/`withdraw()`/`credit()`/`debit()`, which validate
first.

**5. How does a deposit work?**
Find the account by number, check the amount is positive and the account
is active, add the amount to the balance, then append a `DEPOSIT`
`Transaction` record.

**6. How does withdrawal work?**
Same as deposit, plus one more check: the amount must not exceed the
current balance. If it does, nothing changes and the user sees an
"Insufficient funds" message with the available balance.

**7. How do you prevent overdrawing?**
`Account::withdraw()` and `Bank::withdraw()` both check
`amount > balance` before subtracting anything; if that's true, the
function returns `false`/a failure result and the balance is untouched.

**8. How does fund transfer work?**
Validate both accounts exist, are distinct, are active, and the source has
enough funds — all before changing anything — then debit the source,
credit the destination, and record two linked transactions (`TRANSFER-OUT`
and `TRANSFER-IN`).

**9. What happens if the destination account does not exist?**
The transfer is rejected with "Destination account not found." before
either balance is touched — no partial transfer can occur.

**10. How do you maintain transaction history?**
Every successful operation appends a `Transaction` object to a
`std::vector` owned by `Bank`. Each transaction is tied to an account by
its `accountNumber` field, so history for one account is just "every
transaction whose account number matches."

**11. How are account numbers generated?**
A counter inside `Bank` starts at `1000000001` and increments every time
a new account is created; on startup, the counter is advanced past the
highest account number already found on disk, so restarts never reuse a
number.

**12. How are customer IDs generated?**
Same idea: a counter produces `CUST1001`, `CUST1002`, etc., also advanced
past whatever's already saved when the program starts.

**13. Why did you choose vector/map?**
`unordered_map` for customers and accounts, because they're looked up by
ID far more often than iterated, giving average O(1) lookup. `vector` for
transactions, because they're an append-only, ordered log that's scanned
from the most recent end — exactly what a `vector` is good at.

**14. How is data stored?**
Three pipe-delimited text files (`customers.txt`, `accounts.txt`,
`transactions.txt`), one record per line, read/written entirely by a
dedicated `FileManager` class.

**15. What happens when the program restarts?**
`loadData()` reads all three files back into memory, skipping any
corrupted line rather than aborting, and advances the ID counters past
whatever it finds so new records never collide with old ones.

**16. How do you handle file errors?**
A missing file is treated as "no data yet," not an error. A file that
can't be opened for writing on save is reported to the user (`"could not
save ... file"`), rather than silently losing data or crashing.

**17. How do you prevent invalid amounts?**
Input parsing (`parseMoneyToSubunits`) rejects anything that isn't a
non-negative number (letters, a leading `-`, multiple decimal points), and
every `Bank` operation additionally rejects `amount <= 0` before it ever
reaches an account's balance.

**18. What happens during a failed transfer?**
Nothing changes. All validation happens before either account is
mutated, so a failed transfer leaves both balances and the transaction log
exactly as they were.

**19. How would you improve security?**
Add PIN/password authentication with salted, hashed credential storage;
encrypt data at rest; add per-session authorization so a customer can only
act on their own accounts; add audit logging that can't be edited by the
same process that writes normal data.

**20. Why isn't this production-ready banking software?**
No authentication, unencrypted flat-file storage, no concurrency control,
and no real database transactions/ACID guarantees — all fine for a
single-user learning project, all unacceptable for real money at scale.

**21. What OOP principles are demonstrated?**
Encapsulation (private balance, controlled mutators) and abstraction
(`deposit()`/`withdraw()`/`transferFunds()` hide *how* balances change
behind a simple public interface). Inheritance/polymorphism were
deliberately **not** used — `Savings` and `Current` accounts currently
behave identically except for a label, so a base/derived split would add
complexity without a real behavioral difference; it's noted as a natural
extension point if that ever changes.

**22. What is the difference between authentication and authorization?**
Authentication answers "who are you?" (proving identity, e.g. ID + PIN).
Authorization answers "what are you allowed to do, now that we know who
you are?" (e.g. can this customer see this specific account?).

**23. What would you change if this used a real database?**
Replace `FileManager`'s flat-file reads/writes with SQL queries against
tables for customers/accounts/transactions; wrap each transfer in an
actual database transaction so the debit+credit are atomic even across
crashes or concurrent access; let the database enforce uniqueness/foreign
keys instead of doing it by hand in `Bank`.

**24. How would you scale the application?**
Move to a client-server model: a shared database backend, a server
process enforcing the business rules, and multiple client sessions
talking to it over a network, with proper locking/transactions for
concurrent access — the current design assumes one process, one user, one
file set.

**25. What would you improve if you had another month?**
PIN-based authentication with hashed credentials, an indexed
per-account transaction lookup (O(k) instead of O(n)), and a real
database backend with proper transactional guarantees for transfers.

---

## Final Quality Check

- [x] Compiles cleanly: `g++ -std=c++17 -Wall -Wextra` — zero warnings, zero errors.
- [x] Customer management (create/view) — verified.
- [x] Account management (create/view) — verified.
- [x] Deposits — verified, including zero/negative rejection.
- [x] Withdrawals — verified, including insufficient-funds rejection.
- [x] Transfers — verified, including same-account and insufficient-funds rejection.
- [x] Transaction history (full + recent) — verified, correct order, correct linkage.
- [x] Balances update correctly and only on success — verified.
- [x] Invalid operations rejected without crashing or corrupting state — verified.
- [x] Data persists across restart — verified (see Testing, cases 26-29).
- [x] Each class has one clear responsibility — verified by design review above.
- [x] No unnecessary global variables — verified (all state lives in `Bank` or is
      passed explicitly).
- [x] Input validation and error handling — verified across all listed test cases.
- [x] Documentation matches the actual implementation — this report and the
      README were written against the final compiled code, not before it.
- [x] Satisfies the official CodeAlpha Task 4 requirements — see §4 Functional
      Requirements, all met, with enhancements clearly separated in the README.
