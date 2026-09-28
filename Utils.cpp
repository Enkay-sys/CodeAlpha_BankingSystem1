#include "Utils.h"
#include <sstream>
#include <ctime>
#include <cctype>
#include <iostream>
#include <iomanip>

std::vector<std::string> splitString(const std::string& text, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    std::istringstream stream(text);
    while (std::getline(stream, current, delimiter)) {
        result.push_back(current);
    }
    // std::getline with a delimiter drops a trailing empty field
    // (e.g. "a|b|" would yield ["a","b"] instead of ["a","b",""]).
    // We restore it when the original text actually ended with the delimiter,
    // since our file format sometimes relies on a trailing empty field
    // (e.g. a customer that owns zero accounts).
    if (!text.empty() && text.back() == delimiter) {
        result.push_back("");
    }
    return result;
}

std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string formatMoney(long long amountInSubunits) {
    bool negative = amountInSubunits < 0;
    long long value = negative ? -amountInSubunits : amountInSubunits;
    long long whole = value / 100;
    long long sub = value % 100;

    std::ostringstream oss;
    if (negative) oss << '-';
    oss << whole << '.' << (sub < 10 ? "0" : "") << sub;
    return oss.str();
}

bool parseMoneyToSubunits(const std::string& text, long long& outSubunits) {
    if (text.empty()) return false;

    size_t i = 0;
    // Reject a leading '-' outright: this banking system never accepts a
    // user-entered negative amount (deposits/withdrawals/transfers all
    // validate for amount > 0 elsewhere, but we also refuse to *parse*
    // a negative number in the first place).
    if (text[i] == '-') return false;
    if (text[i] == '+') i++; // tolerate a leading '+', ignore it

    std::string wholePart;
    std::string fracPart;
    bool seenDot = false;

    for (; i < text.size(); ++i) {
        char c = text[i];
        if (c == '.') {
            if (seenDot) return false; // second '.' -> invalid, e.g. "12.5.6"
            seenDot = true;
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        if (seenDot) fracPart += c;
        else wholePart += c;
    }

    if (wholePart.empty() && fracPart.empty()) return false;
    if (fracPart.size() > 2) return false; // more precision than a subunit allows

    // Pad/truncate fractional part to exactly 2 digits.
    while (fracPart.size() < 2) fracPart += '0';

    if (wholePart.empty()) wholePart = "0";

    try {
        long long whole = std::stoll(wholePart);
        long long sub = std::stoll(fracPart);
        outSubunits = whole * 100 + sub;
    } catch (...) {
        return false; // e.g. number too large for long long
    }
    return true;
}

std::string readLineTrimmed() {
    std::string line;
    std::getline(std::cin, line);
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end = line.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return line.substr(start, end - start + 1);
}
