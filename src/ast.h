#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "lexer.h"

// Base node for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Base class for expressions
class Expression : public ASTNode {};

// Base class for statements
class Statement : public ASTNode {};

// --- Expressions ---

class NumberLiteral : public Expression {
public:
    double value;
    explicit NumberLiteral(double v) : value(v) {}
};

class StringLiteral : public Expression {
public:
    std::string value;
    explicit StringLiteral(const std::string& v) : value(v) {}
};

class BooleanLiteral : public Expression {
public:
    bool value;
    explicit BooleanLiteral(bool v) : value(v) {}
};

class Identifier : public Expression {
public:
    std::string name;
    explicit Identifier(const std::string& n) : name(n) {}
};

class BinaryExpression : public Expression {
public:
    std::shared_ptr<Expression> left;
    TokenType op;
    std::shared_ptr<Expression> right;

    BinaryExpression(std::shared_ptr<Expression> l, TokenType o, std::shared_ptr<Expression> r)
        : left(l), op(o), right(r) {}
};

class AssignmentExpression : public Expression {
public:
    std::string name;
    std::shared_ptr<Expression> value;

    AssignmentExpression(const std::string& n, std::shared_ptr<Expression> v)
        : name(n), value(v) {}
};

class CallExpression : public Expression {
public:
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> arguments;

    CallExpression(std::shared_ptr<Expression> c, const std::vector<std::shared_ptr<Expression>>& args)
        : callee(c), arguments(args) {}
};

class MemberExpression : public Expression {
public:
    std::shared_ptr<Expression> object;
    std::string property;

    MemberExpression(std::shared_ptr<Expression> obj, const std::string& prop)
        : object(obj), property(prop) {}
};

class ArrayLiteral : public Expression {
public:
    std::vector<std::shared_ptr<Expression>> elements;
    explicit ArrayLiteral(const std::vector<std::shared_ptr<Expression>>& elems) : elements(elems) {}
};

class BlockStatement;

class FunctionExpression : public Expression {
public:
    std::string name; // Can be empty for anonymous functions
    std::vector<std::string> parameters;
    std::shared_ptr<BlockStatement> body;

    FunctionExpression(const std::string& n, const std::vector<std::string>& params, std::shared_ptr<BlockStatement> b)
        : name(n), parameters(params), body(b) {}
};

class ArrowFunctionExpression : public Expression {
public:
    std::vector<std::string> parameters;
    std::shared_ptr<BlockStatement> body;

    ArrowFunctionExpression(const std::vector<std::string>& params, std::shared_ptr<BlockStatement> b)
        : parameters(params), body(b) {}
};

class SpreadElement : public Expression {
public:
    std::shared_ptr<Expression> argument;
    explicit SpreadElement(std::shared_ptr<Expression> arg) : argument(arg) {}
};

struct Property {
    std::string key;
    std::shared_ptr<Expression> value;
};

class ObjectLiteral : public Expression {
public:
    std::vector<Property> properties;
    explicit ObjectLiteral(const std::vector<Property>& props) : properties(props) {}
};

// --- Statements ---

class ExpressionStatement : public Statement {
public:
    std::shared_ptr<Expression> expression;
    explicit ExpressionStatement(std::shared_ptr<Expression> expr) : expression(expr) {}
};

class VariableDeclaration : public Statement {
public:
    bool isConst;
    std::string name;
    std::shared_ptr<Expression> initializer;

    VariableDeclaration(bool c, const std::string& n, std::shared_ptr<Expression> init)
        : isConst(c), name(n), initializer(init) {}
};

class BlockStatement : public Statement {
public:
    std::vector<std::shared_ptr<Statement>> statements;
    explicit BlockStatement(const std::vector<std::shared_ptr<Statement>>& stmts) : statements(stmts) {}
};

class IfStatement : public Statement {
public:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> consequent;
    std::shared_ptr<Statement> alternate; // Can be null

    IfStatement(std::shared_ptr<Expression> cond, std::shared_ptr<Statement> cons, std::shared_ptr<Statement> alt = nullptr)
        : condition(cond), consequent(cons), alternate(alt) {}
};

class WhileStatement : public Statement {
public:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> body;

    WhileStatement(std::shared_ptr<Expression> cond, std::shared_ptr<Statement> b)
        : condition(cond), body(b) {}
};

class ForStatement : public Statement {
public:
    std::shared_ptr<Statement> init; // usually VariableDeclaration or ExpressionStatement
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> update;
    std::shared_ptr<Statement> body;

    ForStatement(std::shared_ptr<Statement> i, std::shared_ptr<Expression> c, std::shared_ptr<Expression> u, std::shared_ptr<Statement> b)
        : init(i), condition(c), update(u), body(b) {}
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::shared_ptr<BlockStatement> body;

    FunctionDeclaration(const std::string& n, const std::vector<std::string>& params, std::shared_ptr<BlockStatement> b)
        : name(n), parameters(params), body(b) {}
};

class ReturnStatement : public Statement {
public:
    std::shared_ptr<Expression> argument; // Can be null

    explicit ReturnStatement(std::shared_ptr<Expression> arg = nullptr) : argument(arg) {}
};

// Root of the AST
class Program : public ASTNode {
public:
    std::vector<std::shared_ptr<Statement>> body;
};

#endif // AST_H
