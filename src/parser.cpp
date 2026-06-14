#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokenList) : tokens(tokenList), current(0) {}

std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    while (!isAtEnd()) {
        try {
            program->body.push_back(declaration());
        } catch (const ParseError& error) {
            // Very simple error recovery: synchronize to next statement boundary
            std::cerr << "Syntax Error: " << error.what() << std::endl;
            while (!isAtEnd() && peek().type != TokenType::SEMICOLON) {
                advance();
            }
            if (!isAtEnd()) advance(); // consume the semicolon
        }
    }
    return program;
}

// --- Helpers ---
Token Parser::peek() { return tokens[current]; }
Token Parser::previous() { return tokens[current - 1]; }
bool Parser::isAtEnd() { return peek().type == TokenType::EOF_TOKEN; }

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::checkNext(TokenType type) {
    if (isAtEnd() || current + 1 >= tokens.size()) return false;
    return tokens[current + 1].type == type;
}

bool Parser::isArrowFunction() {
    if (check(TokenType::IDENTIFIER) && checkNext(TokenType::ARROW)) return true;
    if (check(TokenType::LPAREN)) {
        size_t temp = current;
        while (tokens[temp].type != TokenType::RPAREN && tokens[temp].type != TokenType::EOF_TOKEN) {
            temp++;
        }
        if (tokens[temp].type == TokenType::RPAREN && temp + 1 < tokens.size() && tokens[temp + 1].type == TokenType::ARROW) {
            return true;
        }
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw ParseError(message + " at line " + std::to_string(peek().line));
}

// --- Statements ---
std::shared_ptr<Statement> Parser::declaration() {
    if (match({TokenType::LET, TokenType::CONST})) return varDeclaration();
    if (match({TokenType::FUNCTION})) return functionDeclaration();
    return statement();
}

std::shared_ptr<Statement> Parser::varDeclaration() {
    bool isConst = previous().type == TokenType::CONST;
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name");

    std::shared_ptr<Expression> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = expression();
    }

    match({TokenType::SEMICOLON}); // optional semicolon
    return std::make_shared<VariableDeclaration>(isConst, name.value, initializer);
}

std::shared_ptr<Statement> Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect function name");
    consume(TokenType::LPAREN, "Expect '(' after function name");
    
    std::vector<std::string> parameters;
    bool hasRest = false;
    if (!check(TokenType::RPAREN)) {
        do {
            if (match({TokenType::SPREAD})) {
                parameters.push_back(consume(TokenType::IDENTIFIER, "Expect rest parameter name").value);
                hasRest = true;
                break;
            }
            parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name").value);
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters");
    
    consume(TokenType::LBRACE, "Expect '{' before function body");
    auto body = block();
    return std::make_shared<FunctionDeclaration>(name.value, parameters, body, hasRest);
}

std::shared_ptr<Statement> Parser::statement() {
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::FOR})) return forStatement();
    if (match({TokenType::RETURN})) return returnStatement();
    if (match({TokenType::LBRACE})) return block();
    
    return expressionStatement();
}

std::shared_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'if'");
    auto condition = expression();
    consume(TokenType::RPAREN, "Expect ')' after if condition");

    auto consequent = statement();
    std::shared_ptr<Statement> alternate = nullptr;
    if (match({TokenType::ELSE})) {
        alternate = statement();
    }

    return std::make_shared<IfStatement>(condition, consequent, alternate);
}

std::shared_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'while'");
    auto condition = expression();
    consume(TokenType::RPAREN, "Expect ')' after while condition");
    auto body = statement();
    return std::make_shared<WhileStatement>(condition, body);
}

std::shared_ptr<Statement> Parser::forStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'for'");

    std::shared_ptr<Statement> init = nullptr;
    if (match({TokenType::SEMICOLON})) {
        init = nullptr;
    } else if (match({TokenType::LET, TokenType::CONST})) {
        init = varDeclaration(); // varDeclaration consumes semicolon
    } else {
        init = expressionStatement(); // expressionStatement consumes semicolon
    }

    std::shared_ptr<Expression> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition");

    std::shared_ptr<Expression> update = nullptr;
    if (!check(TokenType::RPAREN)) {
        update = expression();
    }
    consume(TokenType::RPAREN, "Expect ')' after for clauses");

    auto body = statement();

    return std::make_shared<ForStatement>(init, condition, update, body);
}

std::shared_ptr<Statement> Parser::returnStatement() {
    std::shared_ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    match({TokenType::SEMICOLON});
    return std::make_shared<ReturnStatement>(value);
}

std::shared_ptr<BlockStatement> Parser::block() {
    std::vector<std::shared_ptr<Statement>> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RBRACE, "Expect '}' after block");
    return std::make_shared<BlockStatement>(statements);
}

std::shared_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();
    match({TokenType::SEMICOLON}); // optional
    return std::make_shared<ExpressionStatement>(expr);
}

// --- Expressions ---
std::shared_ptr<Expression> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expression> Parser::assignment() {
    auto expr = logicalOr();

    if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN, TokenType::MULTIPLY_ASSIGN, TokenType::DIVIDE_ASSIGN})) {
        Token equals = previous();
        auto value = assignment();

        if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
            return std::make_shared<AssignmentExpression>(id->name, value, equals.type);
        }
        
        throw ParseError("Invalid assignment target");
    }

    return expr;
}

std::shared_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match({TokenType::LOGICAL_OR})) {
        Token op = previous();
        auto right = logicalAnd();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();
    while (match({TokenType::LOGICAL_AND})) {
        Token op = previous();
        auto right = equality();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    while (match({TokenType::EQUAL, TokenType::STRICT_EQUAL, TokenType::NOT_EQUAL, TokenType::STRICT_NOT_EQUAL})) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::comparison() {
    auto expr = term();
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
        Token op = previous();
        auto right = term();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::term() {
    auto expr = factor();
    while (match({TokenType::MINUS, TokenType::PLUS})) {
        Token op = previous();
        auto right = factor();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::factor() {
    auto expr = power();
    while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
        Token op = previous();
        auto right = power();
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::power() {
    auto expr = unary();
    while (match({TokenType::POWER})) {
        Token op = previous();
        auto right = unary(); // Power is right-associative in true JS, but this works for basic cases
        expr = std::make_shared<BinaryExpression>(expr, op.type, right);
    }
    return expr;
}

std::shared_ptr<Expression> Parser::unary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS, TokenType::TYPEOF})) {
        Token op = previous();
        auto right = unary();
        return std::make_shared<UnaryExpression>(op.type, right);
    }
    return call();
}

std::shared_ptr<Expression> Parser::call() {
    auto expr = primary();

    while (true) {
        if (match({TokenType::LPAREN})) {
            std::vector<std::shared_ptr<Expression>> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expect ')' after arguments");
            expr = std::make_shared<CallExpression>(expr, arguments);
        } else if (match({TokenType::DOT})) {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'");
            expr = std::make_shared<MemberExpression>(expr, name.value);
        } else {
            break;
        }
    }

    return expr;
}

std::shared_ptr<Expression> Parser::functionExpression() {
    std::string name = "";
    if (check(TokenType::IDENTIFIER)) {
        name = advance().value;
    }
    consume(TokenType::LPAREN, "Expect '(' after function");
    
    std::vector<std::string> parameters;
    bool hasRest = false;
    if (!check(TokenType::RPAREN)) {
        do {
            if (match({TokenType::SPREAD})) {
                parameters.push_back(consume(TokenType::IDENTIFIER, "Expect rest parameter name").value);
                hasRest = true;
                break;
            }
            parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name").value);
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters");
    
    consume(TokenType::LBRACE, "Expect '{' before function body");
    auto body = block();
    return std::make_shared<FunctionExpression>(name, parameters, body, hasRest);
}

std::shared_ptr<Expression> Parser::arrowFunction(bool hasParens) {
    std::vector<std::string> parameters;
    bool hasRest = false;
    
    if (hasParens) {
        if (!check(TokenType::RPAREN)) {
            do {
                if (match({TokenType::SPREAD})) {
                    parameters.push_back(consume(TokenType::IDENTIFIER, "Expect rest parameter name").value);
                    hasRest = true;
                    break;
                }
                parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name").value);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expect ')' after arrow function parameters");
    } else {
        parameters.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name").value);
    }
    
    consume(TokenType::ARROW, "Expect '=>' after parameters");
    
    std::shared_ptr<BlockStatement> body;
    if (check(TokenType::LBRACE)) {
        consume(TokenType::LBRACE, "Expect '{' before arrow function body");
        body = block();
    } else {
        // Single expression body. Wrap it in a return statement.
        auto expr = expression();
        auto ret = std::make_shared<ReturnStatement>(expr);
        std::vector<std::shared_ptr<Statement>> stmts = {ret};
        body = std::make_shared<BlockStatement>(stmts);
    }
    
    return std::make_shared<ArrowFunctionExpression>(parameters, body, hasRest);
}

std::shared_ptr<Expression> Parser::primary() {
    if (isArrowFunction()) {
        bool hasParens = check(TokenType::LPAREN);
        if (hasParens) advance(); // consume LPAREN
        return arrowFunction(hasParens);
    }
    
    if (match({TokenType::NEW})) {
        auto expr = call();
        if (auto callExpr = std::dynamic_pointer_cast<CallExpression>(expr)) {
            return std::make_shared<NewExpression>(callExpr->callee, callExpr->arguments);
        } else if (auto idExpr = std::dynamic_pointer_cast<Identifier>(expr)) {
            return std::make_shared<NewExpression>(idExpr, std::vector<std::shared_ptr<Expression>>());
        }
        throw ParseError("Expect constructor call after 'new'");
    }

    if (match({TokenType::FUNCTION})) {
        return functionExpression();
    }

    if (match({TokenType::BOOLEAN_FALSE})) return std::make_shared<BooleanLiteral>(false);
    if (match({TokenType::BOOLEAN_TRUE})) return std::make_shared<BooleanLiteral>(true);
    
    // Null and Undefined handled via strings for now or add them to AST. Let's return Identifier for undefined.
    if (match({TokenType::NULL_LIT})) return std::make_shared<Identifier>("null");
    if (match({TokenType::UNDEFINED})) return std::make_shared<Identifier>("undefined");

    if (match({TokenType::NUMBER})) {
        return std::make_shared<NumberLiteral>(std::stod(previous().value));
    }

    if (match({TokenType::STRING})) {
        return std::make_shared<StringLiteral>(previous().value);
    }

    if (match({TokenType::IDENTIFIER})) {
        return std::make_shared<Identifier>(previous().value);
    }

    if (match({TokenType::LBRACKET})) {
        std::vector<std::shared_ptr<Expression>> elements;
        if (!check(TokenType::RBRACKET)) {
            do {
                if (match({TokenType::SPREAD})) {
                    elements.push_back(std::make_shared<SpreadElement>(expression()));
                } else {
                    elements.push_back(expression());
                }
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACKET, "Expect ']' after array elements");
        return std::make_shared<ArrayLiteral>(elements);
    }

    if (match({TokenType::LBRACE})) {
        std::vector<Property> properties;
        if (!check(TokenType::RBRACE)) {
            do {
                std::string key;
                if (check(TokenType::IDENTIFIER) || check(TokenType::STRING)) {
                    key = advance().value;
                } else {
                    throw ParseError("Expect property name");
                }
                consume(TokenType::COLON, "Expect ':' after property name");
                auto val = expression();
                properties.push_back({key, val});
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RBRACE, "Expect '}' after object properties");
        return std::make_shared<ObjectLiteral>(properties);
    }

    if (match({TokenType::LPAREN})) {
        auto expr = expression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return expr;
    }

    throw ParseError("Expect expression at '" + peek().value + "'");
}
