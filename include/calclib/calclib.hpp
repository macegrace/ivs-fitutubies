#pragma once

#include <iostream>
#include <lib/lexertk/lexertk.hpp>
#include <vector>
#include <variant>

using Token_type = lexertk::token::token_type;

/**
 * Custom token holding value in double for Token_type::e_number and in std::string for everything else.
 */
class Token{
public:
    Token_type type;
    std::variant<double,std::string> value;
    static Token fromLexertk(const lexertk::token &lexertk_token);
    inline bool operator==(const Token& rhs) const;
};

namespace calcLib {
    double add(double lhs, double rhs);
    double sub(double lhs, double rhs);
    double mul(double lhs, double rhs);
    double div(double lhs, double rhs);
    double mod(double lhs, double rhs);
    double sin(double n);
    double cos(double n);
    double tan(double n);
    double sqrt(double n);
    double pow(double base, double exponent);
    double root(double degree, double num);
    double log(double base, double num);
    double log(double num);
    double factorial(double n);
    std::string solveEquation(const std::string &expression);

    int parseEquation(const std::string &expression, std::vector<Token> &outTokens);
    double calculateBinaryOperation(double lhs, double rhs, const Token& operation);
    double calculateUnaryOperation(double num, const Token& operation);
    double calculateFunction(const std::vector<Token> &parameters, const Token &function);
    void solveBinaryOperation(std::vector<Token> &tokens, const Token& operation, bool reverse);
    void solveUnaryPlusMinus(std::vector<Token> &tokens);
    void solveRightAssociativeUnary(std::vector<Token> &vector, const Token &operation);
    void solveFunctions(std::vector<Token> &tokens);
    void solveConstants(std::vector<Token> &tokens, const Token &constant);
}
