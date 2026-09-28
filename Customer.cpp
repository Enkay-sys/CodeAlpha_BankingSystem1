#include "Customer.h"
#include "Utils.h"
#include <iostream>
#include <sstream>

Customer::Customer() : customerId(""), name(""), phone(""), email(""), address("") {}

Customer::Customer(const std::string& customerId,
                    const std::string& name,
                    const std::string& phone,
                    const std::string& email,
                    const std::string& address)
    : customerId(customerId), name(name), phone(phone), email(email), address(address) {}

std::string Customer::getCustomerId() const { return customerId; }
std::string Customer::getName() const { return name; }
std::string Customer::getPhone() const { return phone; }
std::string Customer::getEmail() const { return email; }
std::string Customer::getAddress() const { return address; }
const std::vector<std::string>& Customer::getAccountNumbers() const { return accountNumbers; }

bool Customer::setName(const std::string& newName) {
    if (newName.empty()) return false;
    name = newName;
    return true;
}

bool Customer::setPhone(const std::string& newPhone) {
    if (newPhone.empty()) return false;
    phone = newPhone;
    return true;
}

bool Customer::setEmail(const std::string& newEmail) {
    if (newEmail.empty()) return false;
    email = newEmail;
    return true;
}

bool Customer::setAddress(const std::string& newAddress) {
    if (newAddress.empty()) return false;
    address = newAddress;
    return true;
}

void Customer::addAccountNumber(const std::string& accountNumber) {
    accountNumbers.push_back(accountNumber);
}

void Customer::displayInfo() const {
    std::cout << "----------------------------------------\n";
    std::cout << "Customer ID   : " << customerId << "\n";
    std::cout << "Name          : " << name << "\n";
    std::cout << "Phone         : " << phone << "\n";
    std::cout << "Email         : " << email << "\n";
    std::cout << "Address       : " << address << "\n";
    std::cout << "Accounts Owned: ";
    if (accountNumbers.empty()) {
        std::cout << "(none)";
    } else {
        for (size_t i = 0; i < accountNumbers.size(); ++i) {
            std::cout << accountNumbers[i];
            if (i + 1 < accountNumbers.size()) std::cout << ", ";
        }
    }
    std::cout << "\n----------------------------------------\n";
}

// File record layout (fields separated by '|'):
// customerId|name|phone|email|address|acc1,acc2,acc3
std::string Customer::serialize() const {
    std::ostringstream oss;
    oss << customerId << '|' << name << '|' << phone << '|' << email << '|' << address << '|';
    for (size_t i = 0; i < accountNumbers.size(); ++i) {
        oss << accountNumbers[i];
        if (i + 1 < accountNumbers.size()) oss << ',';
    }
    return oss.str();
}

bool Customer::deserialize(const std::string& line, Customer& outCustomer) {
    std::vector<std::string> fields = splitString(line, '|');
    if (fields.size() < 5) return false; // corrupted record -- caller should skip it

    outCustomer.customerId = fields[0];
    outCustomer.name = fields[1];
    outCustomer.phone = fields[2];
    outCustomer.email = fields[3];
    outCustomer.address = fields[4];
    outCustomer.accountNumbers.clear();

    if (fields.size() >= 6 && !fields[5].empty()) {
        std::vector<std::string> accs = splitString(fields[5], ',');
        for (const auto& a : accs) outCustomer.accountNumbers.push_back(a);
    }
    return true;
}
