#include <vector>
#include <stdexcept>
#include <cmath>
#include "calclib/calclib.hpp"

using namespace std::string_literals;

bool Token::operator==(const Token &rhs) const {
    return (this->type == rhs.type || rhs.type == Token_type::e_none) && this->value == rhs.value;
}

Token Token::fromLexertk(const lexertk::token& lexertk_token) {
    Token token;
    token.type = lexertk_token.type;
    if (lexertk_token.type == Token_type::e_number){
        token.value = std::stod(lexertk_token.value);
    } else {
        token.value = lexertk_token.value;
    }
    return token;
}

double calcLib::add(double lhs, double rhs) {
    return lhs + rhs;
}

double calcLib::sub(double lhs, double rhs) {
    return lhs - rhs;
}

double calcLib::mul(double lhs, double rhs) {
    return lhs * rhs;
}

double calcLib::div(double lhs, double rhs) {
    return lhs / rhs;
}

double calcLib::sin(double n) {
    return std::sin(n);
}

double calcLib::cos(double n) {
    return std::cos(n);
}

double calcLib::tan(double n) {
    return std::tan(n);
}

double calcLib::sqrt(double num) {
    return std::sqrt(num);
}

double calcLib::mod(double lhs, double rhs) {
    double div = lhs/rhs;
    double intpart;
    modf(div, &intpart);
    return (div-intpart) * rhs;
}

double calcLib::root(double degree, double num) {
    return pow(num, (1 / degree));
}

double calcLib::log(double base, double num) {
    return log(num) / log(base);
}

double calcLib::log(double num) {
    return std::log10(num);
}

double calcLib::pow(double base, double exponent) {
    return std::pow(base, exponent);
}

double calcLib::factorial(double n) {
    double intpart;
    if (n < 0 || modf(n, &intpart) != 0.0){
        return NAN;
    }
    if (n > 100){
        return INFINITY;
    }
    if (n <= 1){
        return 1;
    }
    return n * factorial(n-1);
}

int calcLib::parseEquation(const std::string &expression, std::vector<Token> &outTokens){
    lexertk::generator generator;

    if (!generator.process(expression))
    {
        std::cout << "Failed to lex: " << expression << std::endl;
        return 1;
    }

    lexertk::helper::bracket_checker bc;
    bc.process(generator);

    if (!bc.result())
    {
        std::cout << "Failed Bracket Checker!" << std::endl;
        return 1;
    }

    lexertk::helper::commutative_inserter ci;
    ci.process(generator);

    while(!generator.finished()){
        outTokens.emplace_back(Token::fromLexertk(generator.next_token()));
    }

    return 0;
}

double calcLib::calculateBinaryOperation(double lhs, double rhs, const Token& operation){
    switch(operation.type){
        case Token_type::e_add:
            return calcLib::add(lhs, rhs);
        case Token_type::e_sub:
            return calcLib::sub(lhs, rhs);
        case Token_type::e_mul:
            return calcLib::mul(lhs, rhs);
        case Token_type::e_div:
            return calcLib::div(lhs, rhs);
        case Token_type::e_pow:
            return calcLib::pow(lhs, rhs);
        default:
            if (std::get<std::string>(operation.value) == "%"){
                return mod(lhs,rhs);
            }
    }
    throw std::invalid_argument(std::string("Invalid operation: " + std::get<std::string>(operation.value)));
}

double calcLib::calculateUnaryOperation(double num, const Token& operation){
    switch(operation.type){
        case Token_type::e_add:
            return num;
        case Token_type::e_sub:
            return -num;
        default:
            if (std::get<std::string>(operation.value) == "!"){
                return factorial(num);
            }
    }
    throw std::invalid_argument(std::string("Invalid operation: " + std::get<std::string>(operation.value)));
}

void calcLib::solveBinaryOperation(std::vector<Token> &tokens, const Token& operation, bool reverse){
    while(true){
        std::vector<Token>::iterator token;
        if (reverse){
            auto reverse_token = std::find_if(tokens.rbegin(), tokens.rend(), [&](const Token &token){return token == operation;});
            if (reverse_token == tokens.rend()){break;}
            token = reverse_token.base()-1; //Get as forward iterator
        } else {
            token = std::find_if(tokens.begin(), tokens.end(), [&](const Token &token){return token == operation;});
            if (token == tokens.end()){break;}
        }

        auto previous = token-1;
        auto next = token+1;
        if (previous->type == Token_type::e_number && next->type == Token_type::e_number){
            previous->value = calculateBinaryOperation(std::get<double>(previous->value),
                                                       std::get<double>(next->value), operation);
            tokens.erase(token);
            tokens.erase(token);
        } else {
            throw std::invalid_argument("Non number tokens around operators");
        }
    }
}

void calcLib::solveUnaryPlusMinus(std::vector<Token> &tokens){
    for(auto token = tokens.begin(); token != tokens.end(); token++){
        if (token->type == Token_type::e_sub || token->type == Token_type::e_add){
            auto previous = token-1;
            auto next = token+1;
            if ((previous->type != Token_type::e_number && previous->type != Token_type::e_rbracket) && next->type == Token_type::e_number){
                next->value = calculateUnaryOperation(std::get<double>(next->value), *token);
                tokens.erase(token);
            }
        }
    }
}

static std::map<std::string, double> variables{
        {"pi", M_PI},
        {"e", M_E},
        {"ans", 0}
};

void calcLib::solveConstants(std::vector<Token> &tokens, const Token& constant){
    for(auto &token : tokens) {
        if (token == constant){
            token = Token{Token_type::e_number, variables.at(std::get<std::string>(token.value))};
        }
    }
}

std::string calcLib::solveEquation(const std::string &expression) {
    try {
        std::vector<Token> tokens;
        int result = parseEquation(expression, tokens);
        if (result == 1){
            return "Syntax error";
        }
        solveConstants(tokens, Token{Token_type::e_symbol, "e"});
        solveConstants(tokens, Token{Token_type::e_symbol, "pi"});
        solveConstants(tokens, Token{Token_type::e_symbol, "ans"});
        solveUnaryPlusMinus(tokens);
        solveFunctions(tokens);
        solveUnaryPlusMinus(tokens); // To solve 2*-sin(-2) we need to run twice.
        solveRightAssociativeUnary(tokens, Token{Token_type::e_none, "!"});
        solveBinaryOperation(tokens, Token{Token_type::e_none, "%"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_pow, "^"}, true);
        solveBinaryOperation(tokens, Token{Token_type::e_mul, "*"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_div, "/"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_add, "+"},false);
        solveBinaryOperation(tokens, Token{Token_type::e_sub, "-"},false);
        if (tokens[0].type != Token_type::e_number){
            return "Err";
        }
        variables.at("ans") = std::get<double>(tokens[0].value);
        return std::to_string(std::get<double>(tokens[0].value));
    } catch(std::invalid_argument &err) {
        return "Err";
    }
}

void calcLib::solveRightAssociativeUnary(std::vector<Token>& tokens, const Token& operation) {
    while(true){
        auto token = std::find_if(tokens.begin(), tokens.end(), [&](const Token &token){return token == operation;});
        if (token == tokens.end() || token == tokens.begin()){break;}
        auto previous = token-1;
        if (previous->type == Token_type::e_number){
            previous->value = calculateUnaryOperation(std::get<double>(previous->value), *token);
            tokens.erase(token);
        }
    }
}

double calcLib::calculateFunction(const std::vector<Token>& parameters, const Token& function) {
    if (parameters.size() == 1){
        Token param = parameters.at(0);
        double num = std::get<double>(param.value);
        if(std::get<std::string>(function.value) == "sin") {
            return calcLib::sin(num);
        } else if(std::get<std::string>(function.value) == "cos") {
            return calcLib::cos(num);
        } else if(std::get<std::string>(function.value) == "tan") {
            return calcLib::tan(num);
        } else if(std::get<std::string>(function.value) == "sqrt") {
            return calcLib::sqrt(num);
        } else if(std::get<std::string>(function.value) == "root") {
            return calcLib::sqrt(num);
        } else if(std::get<std::string>(function.value) == "log") {
            return calcLib::log(num);
        }
    } else if (parameters.size() == 2){
        Token param = parameters.at(0);
        Token param2 = parameters.at(1);
        if(std::get<std::string>(function.value) == "root") {
            return calcLib::root(std::get<double>(param.value), std::get<double>(param2.value));
        } else if(std::get<std::string>(function.value) == "log") {
            return calcLib::log(std::get<double>(param.value), std::get<double>(param2.value));
        }
    }
    throw std::invalid_argument("Invalid function");
}

void calcLib::solveFunctions(std::vector<Token>& tokens){
    for(auto token = tokens.begin()+1; token != tokens.end(); token++) {
        auto functionName = token - 1;
        if (functionName->type == Token_type::e_symbol && token->type == Token_type::e_lbracket){
            auto next = token+1;
            auto rbrace = next+1;
            std::vector<Token> parameters;
            if (next->type == Token_type::e_number && (rbrace->type == Token_type::e_rbracket || rbrace->type == Token_type::e_colon)){
                while (next->type != Token_type::e_rbracket){
                parameters.push_back(*next);
                    next +=1;
                    if (next->type == Token_type::e_colon){
                        next +=1;
                    }
                }
                rbrace = next;
                tokens.erase(token, rbrace+1);
                *functionName = Token{Token_type::e_number, calculateFunction(parameters, *functionName)};
                token = tokens.begin();
            } else {
                throw std::invalid_argument("Function end not found");
            }
        }
    }
}
