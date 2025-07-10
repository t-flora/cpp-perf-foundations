#pragma once
#include <string>

namespace calculator {
    // Template declarations only
    template <typename T, typename U>
    auto add(T a, U b) -> decltype(a + b){ // this solution takes the return type from adding distinct numeric types
        return a + b;
    };

    template <typename T, typename U>
    auto subtract(T a, U b) -> decltype(a - b){
        return a - b;
    };

    template <typename T, typename U>
    auto multiply(T a, U b) -> decltype(a * b){
        return a * b;
    };

    template <typename T, typename U>
    auto divide(T a, U b) -> decltype(a / b){
        if (b == 0) {
            throw std::runtime_error("Division by zero");
        }
        return a / b;
    };


    // Helper function to detect if string represents an integer
    inline bool isInteger(const std::string& str) {
        if (str.empty()) return false;
        
        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') start = 1;
        
        for (size_t i = start; i < str.length(); ++i) {
            if (!std::isdigit(str[i])) return false;
        }
        return true;
    }


    // Helper function to parse expression and determine types
    inline bool parseExpression(const std::string& line, std::string& leftStr, char& op, std::string& rightStr) {
        // Find operator position (skip first character in case of negative numbers)
        size_t pos = line.find_first_of("+-*/", 1);
        if (pos == std::string::npos) return false;
        
        leftStr = line.substr(0, pos);
        op = line[pos];
        rightStr = line.substr(pos + 1);
        
        // Trim whitespace
        leftStr.erase(0, leftStr.find_first_not_of(" \t"));
        leftStr.erase(leftStr.find_last_not_of(" \t") + 1);
        rightStr.erase(0, rightStr.find_first_not_of(" \t"));
        rightStr.erase(rightStr.find_last_not_of(" \t") + 1);
        
        return !leftStr.empty() && !rightStr.empty();
    }
}
