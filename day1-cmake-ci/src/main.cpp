#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <limits>
#include "calculator.hpp"


int main() {
    std::cout << "Calculator Demo\n";
    std::cout << "======================================\n";
    std::cout << "Use +, -, *, / to perform calculations\n";
    std::cout << "Format: number operator number (e.g., 2.5 + 3 or 10 / 2)\n";
    std::cout << "Enter 'q' to exit\n\n";

    std::string line;
    
    while (true) {
        std::cout << "Calculator> ";
        std::getline(std::cin, line);

        if (line.empty()) {
            std::cout << "Please enter an expression or 'q' to quit\n";
            continue;
        }
        
        if (line == "q" || line == "quit" || line == "exit") {
            std::cout << "Exiting...\n";
            break;
        }

        std::string leftStr, rightStr;
        char op;
        
        if (!calculator::parseExpression(line, leftStr, op, rightStr)) {
            std::cout << "Invalid format. Use: number operator number\n";
            continue;
        }

        try {
            // Type-flexible parsing: use integers when possible, otherwise doubles
            if (calculator::isInteger(leftStr) && calculator::isInteger(rightStr)) {
                int a = std::stoi(leftStr);
                int b = std::stoi(rightStr);
                
                switch (op) {
                    case '+':
                        std::cout << calculator::add(a, b) << "\n";
                        break;
                    case '-':
                        std::cout << calculator::subtract(a, b) << "\n";
                        break;
                    case '*':
                        std::cout << calculator::multiply(a, b) << "\n";
                        break;
                    case '/':
                        if (b == 0) {
                            std::cout << "Error: Division by zero\n";
                        } else {
                            // For integer division, show result with type info
                            auto result = calculator::divide(a, b);
                            std::cout << result << " (integer division)\n";
                        }
                        break;
                    default:
                        std::cout << "Unknown operator: " << op << "\n";
                        break;
                }
            } else {
                // At least one operand is a decimal number
                double a = std::stod(leftStr);
                double b = std::stod(rightStr);
                
                switch (op) {
                    case '+':
                        std::cout << calculator::add(a, b) << "\n";
                        break;
                    case '-':
                        std::cout << calculator::subtract(a, b) << "\n";
                        break;
                    case '*':
                        std::cout << calculator::multiply(a, b) << "\n";
                        break;
                    case '/':
                        if (b == 0.0) {
                            std::cout << "Error: Division by zero\n";
                        } else {
                            std::cout << calculator::divide(a, b) << "\n";
                        }
                        break;
                    default:
                        std::cout << "Unknown operator: " << op << "\n";
                        break;
                }
            }
        } catch (const std::invalid_argument& e) {
            std::cout << "Error: Invalid number format\n";
        } catch (const std::out_of_range& e) {
            std::cout << "Error: Number out of range\n";
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}
