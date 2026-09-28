#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// Small free (non-member) helper functions shared by multiple classes.
// Keeping them here avoids duplicating the same code in Customer.cpp,
// Account.cpp and Transaction.cpp.

// Splits `text` on every occurrence of `delimiter`, returning the pieces
// in order. Used to parse the '|' and ',' separated lines in our data files.
std::vector<std::string> splitString(const std::string& text, char delimiter);

// Returns the current date/time as "YYYY-MM-DD HH:MM:SS", using only the
// standard C++ <ctime> facilities (no external date library needed).
std::string currentTimestamp();

// Converts a whole-currency-unit + subunit amount (e.g. GHS 12.50) that is
// stored internally as an integer number of pesewas/cents (1250) into a
// display string "12.50". We store money as an integer number of the
// smallest currency unit (see Account.h) to avoid floating-point rounding
// errors; this function is only used for *printing*.
std::string formatMoney(long long amountInSubunits);

// Parses a user-typed amount such as "12.50" or "12" into an integer number
// of subunits (1250 / 1200). Returns false if the text is not a valid,
// non-negative monetary amount (rejects things like "abc", "-5", "12.5.6").
bool parseMoneyToSubunits(const std::string& text, long long& outSubunits);

// Reads a full line of user input safely and trims surrounding whitespace.
std::string readLineTrimmed();

#endif // UTILS_H
