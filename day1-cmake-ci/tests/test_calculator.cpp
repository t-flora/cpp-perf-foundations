#include <cassert>
#include <iostream>
#include <cmath>
#include "../src/calculator.hpp"

// Helper function for floating point comparison
bool approx_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

void test_basic_operations() {
    std::cout << "Testing basic operations...\n";
    
    // Test addition
    assert(calculator::add(2, 3) == 5);
    assert(calculator::add(-5, 3) == -2);
    assert(calculator::add(0, 0) == 0);
    assert(approx_equal(calculator::add(2.5, 3.7), 6.2));
    
    // Test subtraction
    assert(calculator::subtract(5, 3) == 2);
    assert(calculator::subtract(-5, -3) == -2);
    assert(calculator::subtract(0, 5) == -5);
    assert(approx_equal(calculator::subtract(2.5, 1.5), 1.0));
    
    // Test multiplication
    assert(calculator::multiply(4, 3) == 12);
    assert(calculator::multiply(-4, 3) == -12);
    assert(calculator::multiply(0, 100) == 0);
    assert(approx_equal(calculator::multiply(2.5, 2.0), 5.0));
    
    // Test division
    assert(calculator::divide(10, 2) == 5);
    assert(calculator::divide(-10, 2) == -5);
    assert(calculator::divide(0, 5) == 0);
    assert(approx_equal(calculator::divide(7.5, 2.5), 3.0));
    
    std::cout << "✅ Basic operations passed!\n";
}

void test_mixed_types() {
    std::cout << "Testing mixed-type operations...\n";
    
    // Test int + double
    auto result1 = calculator::add(5, 3.7);
    assert(approx_equal(result1, 8.7));
    
    // Test float + int
    auto result2 = calculator::add(2.5f, 3);
    assert(approx_equal(result2, 5.5f, 1e-6f));
    
    // Test double * int
    auto result3 = calculator::multiply(2.5, 4);
    assert(approx_equal(result3, 10.0));
    
    std::cout << "✅ Mixed-type operations passed!\n";
}

void test_helper_functions() {
    std::cout << "Testing helper functions...\n";
    
    // Test isInteger
    assert(calculator::isInteger("123") == true);
    assert(calculator::isInteger("-456") == true);
    assert(calculator::isInteger("+789") == true);
    assert(calculator::isInteger("0") == true);
    assert(calculator::isInteger("123.45") == false);
    assert(calculator::isInteger("abc") == false);
    assert(calculator::isInteger("") == false);
    
    // Test parseExpression
    std::string left, right;
    char op;
    
    assert(calculator::parseExpression("5+3", left, op, right) == true);
    assert(left == "5" && op == '+' && right == "3");
    
    assert(calculator::parseExpression("10 - 7", left, op, right) == true);
    assert(left == "10" && op == '-' && right == "7");
    
    assert(calculator::parseExpression("-5+3", left, op, right) == true);
    assert(left == "-5" && op == '+' && right == "3");
    
    assert(calculator::parseExpression("5", left, op, right) == false);
    assert(calculator::parseExpression("", left, op, right) == false);
    
    std::cout << "✅ Helper functions passed!\n";
}

void test_edge_cases() {
    std::cout << "Testing edge cases...\n";
    
    // Test integer division behavior
    assert(calculator::divide(7, 3) == 2);  // Integer division truncates
    assert(calculator::divide(-7, 3) == -2);
    
    // Test mixed division preserves precision
    assert(approx_equal(calculator::divide(7.0, 3), 2.333333, 1e-5));
    assert(approx_equal(calculator::divide(7, 3.0), 2.333333, 1e-5));
    
    // Test floating point precision
    assert(approx_equal(calculator::add(0.1, 0.2), 0.3));
    assert(approx_equal(calculator::subtract(1.0, 0.9), 0.1));
    
    std::cout << "✅ Edge cases passed!\n";
}

int main() {
    std::cout << "🧪 Running Calculator Tests\n";
    std::cout << "============================\n";
    
    try {
        test_basic_operations();
        test_mixed_types();
        test_helper_functions();
        test_edge_cases();
        
        std::cout << "\n🎉 All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}