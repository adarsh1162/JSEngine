#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <vector>
#include <stdexcept>
#include <memory>
#include <string>

// The Parser implements Recursive Descent Parsing
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;

    // Helper methods
    Token peek();
    Token previous();
    Token advance();
    bool isAtEnd();
    bool check(TokenType type);
    bool checkNext(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    bool isArrowFunction();

    // Grammar - Expressions (Precedence from lowest to highest)
    std::shared_ptr<Expression> expression();
    std::shared_ptr<Expression> assignment();
    std::shared_ptr<Expression> logicalOr();
    std::shared_ptr<Expression> logicalAnd();
    std::shared_ptr<Expression> bitwiseOr();
    std::shared_ptr<Expression> bitwiseXor();
    std::shared_ptr<Expression> bitwiseAnd();
    std::shared_ptr<Expression> equality();
    std::shared_ptr<Expression> comparison();
    std::shared_ptr<Expression> shift();
    std::shared_ptr<Expression> term();
    std::shared_ptr<Expression> factor();
    std::shared_ptr<Expression> power();
    std::shared_ptr<Expression> unary();
    std::shared_ptr<Expression> call();
    std::shared_ptr<Expression> primary();
    std::shared_ptr<Expression> functionExpression();
    std::shared_ptr<Expression> arrowFunction(bool hasParens);

    // Grammar - Statements
    std::shared_ptr<Statement> declaration();
    std::shared_ptr<Statement> statement();
    std::shared_ptr<Statement> classDeclaration();
    std::shared_ptr<MethodDefinition> methodDefinition();
    std::shared_ptr<Statement> varDeclaration();
    std::shared_ptr<Statement> functionDeclaration();
    std::shared_ptr<Statement> ifStatement();
    std::shared_ptr<Statement> whileStatement();
    std::shared_ptr<Statement> doWhileStatement();
    std::shared_ptr<Statement> forStatement();
    std::shared_ptr<Statement> switchStatement();
    std::shared_ptr<Statement> tryStatement();
    std::shared_ptr<Statement> throwStatement();
    std::shared_ptr<Statement> returnStatement();
    std::shared_ptr<BlockStatement> block();
    std::shared_ptr<Statement> expressionStatement();

public:
    explicit Parser(const std::vector<Token>& tokenList);
    std::shared_ptr<Program> parse();
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

#endif // PARSER_H
