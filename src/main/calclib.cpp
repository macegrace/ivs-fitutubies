#include <list>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <cmath>
#include <sstream>
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
    if (rhs == 0){
        throw std::overflow_error("Division by zero");
    }
    return lhs / rhs;
}

#define M_DEGREE M_PI/180
double calcLib::sin(double num) {
    return std::sin(num*M_DEGREE);
}

double calcLib::cos(double num) {
    return std::cos(num*M_DEGREE);
}

double calcLib::tan(double num) {
    if (num == 90 || num == -90){
        throw std::overflow_error("Division by zero");
    }
    return std::tan(num*M_DEGREE);
}

double calcLib::sqrt(double num) {
    return root(2, num);
}

double calcLib::mod(double lhs, double rhs) {
    double div = calcLib::div(lhs, rhs);
    double intpart;
    modf(div, &intpart);
    return (div-intpart) * rhs;
}

double calcLib::root(double degree, double num) {
    if (num < 0){
        throw std::overflow_error("root: Undefined for argument domain");
    }
    return pow(num, calcLib::div(1,degree));
}

double calcLib::log(double base, double num) {
    return calcLib::div(log(num),log(base));
}

double calcLib::log(double num) {
    return std::log10(num);
}

double calcLib::pow(double base, double exponent) {
    return std::pow(base, exponent);
}

double calcLib::factorial(double num) {
    double intpart;
    if (num < 0 || modf(num, &intpart) != 0.0){
        return NAN;
    }
    if (num > 100){
        return INFINITY;
    }
    if (num <= 1){
        return 1;
    }
    return num * factorial(num - 1);
}

int calcLib::parseEquation(std::string_view expression, std::list<Token> &outTokens){
    lexertk::generator generator;

    if (!generator.process(expression.data()))
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

    // generator.nextToken() returns invalid pointer after ~40 tokens.
    for (std::size_t i = 0; i < generator.size(); ++i)
    {
        lexertk::token t = generator[i];
        outTokens.push_back(Token::fromLexertk(t));
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

void calcLib::solveBinaryOperation(std::list<Token> &tokens, const Token& operation, bool reverse){
    while(true){
        std::list<Token>::iterator token;
        if (reverse){
            auto reverse_token = std::find_if(tokens.rbegin(), tokens.rend(), [&](const Token &token){return token == operation;});
            if (reverse_token == tokens.rend()){break;}
            token = std::prev(reverse_token.base()); //Get as forward iterator
        } else {
            token = std::find_if(tokens.begin(), tokens.end(), [&](const Token &token){return token == operation;});
            if (token == tokens.end()){break;}
        }

        auto previous = std::prev(token);
        auto next = std::next(token);
        if (previous->type == Token_type::e_number && next->type == Token_type::e_number){
            previous->value = calculateBinaryOperation(std::get<double>(previous->value),
                                                       std::get<double>(next->value), operation);
            tokens.erase(token, std::next(next));
        } else {
            throw std::invalid_argument("Non number tokens around operators");
        }
    }
}

void calcLib::solveUnaryPlusMinus(std::list<Token> &tokens){
    for(auto token = tokens.begin(); token != tokens.end(); token++){
        if (token->type == Token_type::e_sub || token->type == Token_type::e_add){
            auto previous = std::prev(token);
            auto next = std::next(token);
            if ((previous->type != Token_type::e_number && previous->type != Token_type::e_rbracket
            && !(*previous == Token{Token_type::e_none, "!"})) && next->type == Token_type::e_number){
                next->value = calculateUnaryOperation(std::get<double>(next->value), *token);
                tokens.erase(token++);
            }
        }
    }
}

void calcLib::solveVariable(std::list<Token> &tokens, const Token& variable){
    for(auto &token : tokens) {
        if (token == variable){
            token = Token{Token_type::e_number, variables.at(std::get<std::string>(token.value))};
        }
    }
}

std::string calcLib::formatResult(double result){
    std::ostringstream ostringstream;
    ostringstream.precision(precision);
    ostringstream << std::fixed;
    ostringstream << result;
    std::string string_num = ostringstream.str();
    if (format == ResultFormat::variable){
        if (string_num.find('.')){
            while(*(string_num.end()-1) == '0'){
                string_num.erase(string_num.end()-1, string_num.end());
            }
        }
        if(*(string_num.end()-1) == '.'){
            string_num.erase(string_num.end()-1, string_num.end());
        }
    }
    return string_num;
}

std::string calcLib::solveEquation(std::string expression) {
    try {
        std::replace(expression.begin(), expression.end(), ',', '.');
        std::list<Token> tokens;
        int result = parseEquation(expression, tokens);
        if (result == 1){
            return "Syntax error";
        }
        if (tokens.empty()){
            return formatResult(variables.at("ans"));
        }
        solveVariable(tokens, Token{Token_type::e_symbol, "e"});
        solveVariable(tokens, Token{Token_type::e_symbol, "pi"});
        solveVariable(tokens, Token{Token_type::e_symbol, "ans"});
        solveUnaryPlusMinus(tokens);
        solveFunctions(tokens);
        solveUnaryPlusMinus(tokens); // To solve 2*-sin(-2) we need to run twice.
        solveLeftAssociativeUnary(tokens, Token{Token_type::e_none, "!"});
        solveBinaryOperation(tokens, Token{Token_type::e_none, "%"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_pow, "^"}, true);
        solveBinaryOperation(tokens, Token{Token_type::e_div, "/"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_mul, "*"}, false);
        solveBinaryOperation(tokens, Token{Token_type::e_sub, "-"},false);
        solveBinaryOperation(tokens, Token{Token_type::e_add, "+"},false);
        if (tokens.begin()->type != Token_type::e_number || tokens.empty() || tokens.size() > 1){
            return "Err";
        }
        variables.at("ans") = std::get<double>(tokens.begin()->value);
        return formatResult(std::get<double>(tokens.begin()->value));
    } catch(std::invalid_argument &err) {
        return "Err";
    } catch(std::overflow_error &err) {
        return err.what();
    } catch(...) {
        return "Unhandled error in library";
    }
}

void calcLib::solveLeftAssociativeUnary(std::list<Token>& tokens, const Token& operation) {
    while(true){
        auto token = std::find_if(tokens.begin(), tokens.end(), [&](const Token &token){return token == operation;});
        if (token == tokens.end() || token == tokens.begin()){break;}
        auto previous = std::prev(token);
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

void calcLib::solveFunctions(std::list<Token>& tokens){
    for(auto token = std::next(tokens.begin()); token != tokens.end(); token++) {
        auto functionName = std::prev(token);
        if (functionName->type == Token_type::e_symbol && token->type == Token_type::e_lbracket){
            auto next = std::next(token);
            auto rbrace = std::next(next);
            std::vector<Token> parameters;
            if (next->type == Token_type::e_number && (rbrace->type == Token_type::e_rbracket || rbrace->type == Token_type::e_colon)){
                while (next->type != Token_type::e_rbracket){
                parameters.push_back(*next);
                    next++;
                    if (next->type == Token_type::e_colon){
                        next++;
                    }
                }
                rbrace = next;
                tokens.erase(token, std::next(rbrace));
                *functionName = Token{Token_type::e_number, calculateFunction(parameters, *functionName)};
                token = tokens.begin();
            } else {
                throw std::invalid_argument("Function end not found");
            }
        }
    }
}

calcLib::calcLib(ResultFormat format, size_t precision){
    variables = std::map<std::string, double>{
            {"pi", M_PI},
            {"e", M_E},
            {"ans", 0}
    };
    this->format = format;
    this->precision = precision;
}

calcLib::calcLib() : calcLib(ResultFormat::variable, 8){
}
