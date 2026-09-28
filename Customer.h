#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <vector>

// ============================================================
// Customer
// ------------------------------------------------------------
// Represents a single bank customer. A Customer does NOT store
// money directly -- money lives on Account objects. A Customer
// simply holds personal/contact details plus a list of account
// numbers that belong to them (a customer may own more than one
// account, e.g. one Savings + one Current account).
//
// All data members are private (encapsulation). Other code can
// only read or change them through the public methods below,
// which lets this class control what a "valid" Customer looks
// like (for example, you cannot set an empty name from outside).
// ============================================================
class Customer {
private:
    std::string customerId;                 // e.g. "CUST1001" (unique, assigned by Bank)
    std::string name;
    std::string phone;
    std::string email;
    std::string address;
    std::vector<std::string> accountNumbers; // accounts owned by this customer

public:
    // Constructors ------------------------------------------------
    Customer(); // default constructor, needed so Customer can live inside
                // containers like std::unordered_map<std::string, Customer>
    Customer(const std::string& customerId,
             const std::string& name,
             const std::string& phone,
             const std::string& email,
             const std::string& address);

    // Getters (read-only access to private data) ------------------
    std::string getCustomerId() const;
    std::string getName() const;
    std::string getPhone() const;
    std::string getEmail() const;
    std::string getAddress() const;
    const std::vector<std::string>& getAccountNumbers() const;

    // Setters (controlled updates) ---------------------------------
    // Returns false (and changes nothing) if the new value is invalid,
    // e.g. an empty string. This is basic input validation living
    // inside the class itself -- part of "abstraction": the caller
    // does not need to know the validation rule, just the outcome.
    bool setName(const std::string& newName);
    bool setPhone(const std::string& newPhone);
    bool setEmail(const std::string& newEmail);
    bool setAddress(const std::string& newAddress);

    // Account association -------------------------------------------
    void addAccountNumber(const std::string& accountNumber);

    // Display ----------------------------------------------------
    void displayInfo() const;

    // Serialization (used by FileManager for persistence) ----------
    // Converts this Customer into one line of a text file.
    std::string serialize() const;
    // Reconstructs a Customer from a line previously produced by
    // serialize(). Returns true on success.
    static bool deserialize(const std::string& line, Customer& outCustomer);
};

#endif // CUSTOMER_H
