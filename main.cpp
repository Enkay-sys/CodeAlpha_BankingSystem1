// ============================================================
// CodeAlpha C++ Internship - Task 4: Banking System
// ------------------------------------------------------------
// main.cpp is intentionally "thin": it only starts the program,
// loads data, shows the menu, reads the user's choice, calls the
// matching Bank method, prints the OperationResult, and saves data
// on exit. All banking rules live in Bank/Account/Customer/Transaction.
// ============================================================
#include <iostream>
#include <limits>
#include "Bank.h"
#include "Utils.h"

// Reads a menu choice as an int. Returns -1 (an always-invalid choice)
// if the user typed something that is not a plain integer, instead of
// letting cin enter a broken/failed state or crashing.
int readMenuChoice() {
    std::string line = readLineTrimmed();
    if (line.empty()) return -1;
    for (char c : line) {
        if (!isdigit(static_cast<unsigned char>(c))) return -1;
    }
    try {
        return std::stoi(line);
    } catch (...) {
        return -1;
    }
}

// Prompts for a monetary amount until the user enters something valid,
// or types "cancel" to back out. Returns false if cancelled.
bool promptAmount(const std::string& label, long long& outSubunits) {
    while (true) {
        std::cout << label << " (or 'cancel'): ";
        std::string text = readLineTrimmed();
        if (text == "cancel") return false;
        if (parseMoneyToSubunits(text, outSubunits)) return true;
        std::cout << "Invalid amount. Please enter a positive number like 250 or 250.50.\n";
    }
}

void printMenu() {
    std::cout << "\n========================================\n";
    std::cout << "              BANKING SYSTEM\n";
    std::cout << "========================================\n";
    std::cout << " 1. Create Customer\n";
    std::cout << " 2. Create Account\n";
    std::cout << " 3. View Customer\n";
    std::cout << " 4. View Account\n";
    std::cout << " 5. Deposit\n";
    std::cout << " 6. Withdraw\n";
    std::cout << " 7. Transfer Funds\n";
    std::cout << " 8. View Full Transaction History (one account)\n";
    std::cout << " 9. View Recent Transactions (last 5, one account)\n";
    std::cout << "10. Account Statement\n";
    std::cout << "11. Exit\n";
    std::cout << "========================================\n";
    std::cout << "Enter your choice: ";
}

void handleCreateCustomer(Bank& bank) {
    std::cout << "\n-- Create Customer --\n";
    std::cout << "Full name: ";
    std::string name = readLineTrimmed();
    std::cout << "Phone number: ";
    std::string phone = readLineTrimmed();
    std::cout << "Email: ";
    std::string email = readLineTrimmed();
    std::cout << "Address: ";
    std::string address = readLineTrimmed();

    std::string newId;
    OperationResult result = bank.createCustomer(name, phone, email, address, newId);
    if (result.success) {
        std::cout << "\n" << result.message << "\n";
        std::cout << "Customer ID: " << newId << "\n";
    } else {
        std::cout << "\nError: " << result.message << "\n";
    }
}

void handleCreateAccount(Bank& bank) {
    std::cout << "\n-- Create Account --\n";
    std::cout << "Customer ID: ";
    std::string customerId = readLineTrimmed();

    if (!bank.customerExists(customerId)) {
        std::cout << "\nError: Customer not found.\n";
        return;
    }

    std::cout << "Account type (1 = Savings, 2 = Current): ";
    int typeChoice = readMenuChoice();
    AccountType type = (typeChoice == 2) ? AccountType::Current : AccountType::Savings;

    long long initialDeposit = 0;
    std::cout << "Initial deposit amount (0 is allowed): ";
    std::string text = readLineTrimmed();
    if (!text.empty() && !parseMoneyToSubunits(text, initialDeposit)) {
        std::cout << "\nInvalid amount entered; defaulting initial deposit to 0.\n";
        initialDeposit = 0;
    }

    std::string newAccNum;
    OperationResult result = bank.createAccount(customerId, type, initialDeposit, newAccNum);
    if (result.success) {
        std::cout << "\n" << result.message << "\n\n";
        bank.viewAccount(newAccNum);
    } else {
        std::cout << "\nError: " << result.message << "\n";
    }
}

void handleViewCustomer(Bank& bank) {
    std::cout << "\n-- View Customer --\n";
    std::cout << "Customer ID: ";
    std::string customerId = readLineTrimmed();
    std::cout << "\n";
    if (!bank.viewCustomer(customerId)) {
        std::cout << "Customer not found.\n";
    }
}

void handleViewAccount(Bank& bank) {
    std::cout << "\n-- View Account --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    std::cout << "\n";
    if (!bank.viewAccount(accNum)) {
        std::cout << "Account not found.\n";
    }
}

void handleDeposit(Bank& bank) {
    std::cout << "\n-- Deposit --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    if (!bank.accountExists(accNum)) {
        std::cout << "\nError: Account not found.\n";
        return;
    }
    long long amount;
    if (!promptAmount("Enter amount to deposit", amount)) return;

    OperationResult result = bank.deposit(accNum, amount);
    if (result.success) {
        const Account* a = bank.findAccount(accNum);
        std::cout << "\n" << result.message << "\n";
        std::cout << "Amount deposited: GHS " << formatMoney(amount) << "\n";
        std::cout << "New balance: GHS " << formatMoney(a->getBalanceInSubunits()) << "\n";
    } else {
        std::cout << "\nTransaction failed. " << result.message << "\n";
    }
}

void handleWithdraw(Bank& bank) {
    std::cout << "\n-- Withdraw --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    if (!bank.accountExists(accNum)) {
        std::cout << "\nError: Account not found.\n";
        return;
    }
    long long amount;
    if (!promptAmount("Enter amount to withdraw", amount)) return;

    OperationResult result = bank.withdraw(accNum, amount);
    if (result.success) {
        const Account* a = bank.findAccount(accNum);
        std::cout << "\n" << result.message << "\n";
        std::cout << "Amount withdrawn: GHS " << formatMoney(amount) << "\n";
        std::cout << "Remaining balance: GHS " << formatMoney(a->getBalanceInSubunits()) << "\n";
    } else {
        std::cout << "\nTransaction failed. " << result.message << "\n";
    }
}

void handleTransfer(Bank& bank) {
    std::cout << "\n-- Transfer Funds --\n";
    std::cout << "From account: ";
    std::string from = readLineTrimmed();
    std::cout << "To account: ";
    std::string to = readLineTrimmed();

    long long amount;
    if (!promptAmount("Enter amount to transfer", amount)) return;

    OperationResult result = bank.transferFunds(from, to, amount);
    if (result.success) {
        std::cout << "\n" << result.message << "\n";
        std::cout << "GHS " << formatMoney(amount) << " transferred from " << from
                  << " to " << to << ".\n";
    } else {
        std::cout << "\nTransfer failed. " << result.message << "\n";
    }
}

void handleFullHistory(Bank& bank) {
    std::cout << "\n-- Full Transaction History --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    if (!bank.accountExists(accNum)) {
        std::cout << "\nError: Account not found.\n";
        return;
    }
    auto txns = bank.getTransactionsForAccount(accNum, -1); // -1 = no cap
    std::cout << "\n";
    if (txns.empty()) {
        std::cout << "No transactions found for this account.\n";
        return;
    }
    for (const auto& t : txns) t.display();
}

void handleRecentTransactions(Bank& bank) {
    std::cout << "\n-- Recent Transactions (last 5) --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    if (!bank.accountExists(accNum)) {
        std::cout << "\nError: Account not found.\n";
        return;
    }
    // "Recent" is defined as the 5 most recently recorded transactions for
    // this account (see technical report, "Recent Transactions").
    auto txns = bank.getTransactionsForAccount(accNum, 5);
    std::cout << "\n";
    if (txns.empty()) {
        std::cout << "No transactions found for this account.\n";
        return;
    }
    for (const auto& t : txns) t.displayShort();
}

void handleStatement(Bank& bank) {
    std::cout << "\n-- Account Statement (enhancement) --\n";
    std::cout << "Account number: ";
    std::string accNum = readLineTrimmed();
    const Account* acc = bank.findAccount(accNum);
    if (!acc) {
        std::cout << "\nError: Account not found.\n";
        return;
    }
    const Customer* cust = bank.findCustomer(acc->getCustomerId());

    std::cout << "\n====================================\n";
    std::cout << "          ACCOUNT STATEMENT\n";
    std::cout << "====================================\n";
    std::cout << "Account Number : " << acc->getAccountNumber() << "\n";
    std::cout << "Customer       : " << (cust ? cust->getName() : "(unknown)") << "\n";
    std::cout << "Account Type   : " << accountTypeToString(acc->getAccountType()) << "\n";
    std::cout << "Current Balance: GHS " << formatMoney(acc->getBalanceInSubunits()) << "\n";
    std::cout << "------------------------------------\n";
    std::cout << "Transaction History:\n";
    std::cout << "------------------------------------\n";
    auto txns = bank.getTransactionsForAccount(accNum, -1);
    if (txns.empty()) {
        std::cout << "(no transactions yet)\n";
    } else {
        for (const auto& t : txns) t.displayShort();
    }
    std::cout << "====================================\n";
}

int main() {
    Bank bank("customers.txt", "accounts.txt", "transactions.txt");
    bank.loadData();

    bool running = true;
    while (running) {
        printMenu();
        int choice = readMenuChoice();

        switch (choice) {
            case 1: handleCreateCustomer(bank); break;
            case 2: handleCreateAccount(bank); break;
            case 3: handleViewCustomer(bank); break;
            case 4: handleViewAccount(bank); break;
            case 5: handleDeposit(bank); break;
            case 6: handleWithdraw(bank); break;
            case 7: handleTransfer(bank); break;
            case 8: handleFullHistory(bank); break;
            case 9: handleRecentTransactions(bank); break;
            case 10: handleStatement(bank); break;
            case 11:
                running = false;
                break;
            default:
                std::cout << "\nInvalid menu option. Please enter a number from 1 to 11.\n";
                break;
        }
    }

    std::cout << "\nSaving data...\n";
    if (bank.saveData()) {
        std::cout << "All data saved. Goodbye!\n";
    } else {
        std::cout << "Some data may not have been saved. Please check file permissions.\n";
    }
    return 0;
}
