#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Customer.h"
#include "Account.h"
#include "Transaction.h"

// ============================================================
// FileManager
// ------------------------------------------------------------
// Isolates ALL direct file-stream (ifstream/ofstream) code in one
// place. Bank never touches std::ifstream/std::ofstream itself --
// it just asks FileManager to "load everything" or "save everything".
//
// Why put this in its own class instead of inside Bank?
//   - Single Responsibility: Bank decides *what* the data means and
//     enforces banking rules; FileManager only knows *how* to get
//     that data on and off disk. If we later swapped plain text files
//     for, say, a CSV or SQLite backend, only this class would change.
//   - It also makes the "missing file" / "corrupted line" handling
//     easy to find and test in one location.
// ============================================================
class FileManager {
private:
    std::string customersFile;
    std::string accountsFile;
    std::string transactionsFile;

public:
    FileManager(const std::string& customersFile,
                const std::string& accountsFile,
                const std::string& transactionsFile);

    // Each load function appends the records it successfully parses into
    // the given container and returns the count of *skipped* (corrupted)
    // lines, so the caller can report data-quality issues without crashing.
    // A missing file is not an error: it simply means "nothing to load yet"
    // (e.g. first run of the program), so these return 0 skipped lines and
    // leave the containers empty in that case.
    int loadCustomers(std::unordered_map<std::string, Customer>& customers) const;
    int loadAccounts(std::unordered_map<std::string, Account>& accounts) const;
    int loadTransactions(std::vector<Transaction>& transactions) const;

    // Each save function OVERWRITES its file with the current in-memory
    // state. Returns false if the file could not be opened for writing
    // (e.g. permissions problem) -- the caller decides how to report that.
    bool saveCustomers(const std::unordered_map<std::string, Customer>& customers) const;
    bool saveAccounts(const std::unordered_map<std::string, Account>& accounts) const;
    bool saveTransactions(const std::vector<Transaction>& transactions) const;
};

#endif // FILEMANAGER_H
