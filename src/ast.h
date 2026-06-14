#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "lexer.h"

// Base node for all AST nodes
class Evaluator;
class JSValue;

class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Base class for expressions
class Expression : public ASTNode {
public:
    virtual std::shared_ptr<JSValue> eval(Evaluator* evaluator) = 0;};

// Base class for statements
class Statement : public ASTNode {
public:
    virtual void execute(Evaluator* evaluator) = 0;};

// --- Expressions ---

class NumberLiteral : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    double value;
    explicit NumberLiteral(double v) : value(v) {}
};

class StringLiteral : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::string value;
    explicit StringLiteral(const std::string& v) : value(v) {}
};

class BooleanLiteral : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    bool value;
    explicit BooleanLiteral(bool v) : value(v) {}
};

class TemplateLiteralExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::vector<std::shared_ptr<Expression>> parts; // either StringLiteral or some evaluated expression
    explicit TemplateLiteralExpression(const std::vector<std::shared_ptr<Expression>>& p) : parts(p) {}
};

class RegexLiteralExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::string pattern;
    std::string flags;
    RegexLiteralExpression(const std::string& p, const std::string& f) : pattern(p), flags(f) {}
};

class Identifier : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::string name;
    explicit Identifier(const std::string& n) : name(n) {}
};

class SuperExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    SuperExpression() {}
};

class BinaryExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> left;
    TokenType op;
    std::shared_ptr<Expression> right;

    BinaryExpression(std::shared_ptr<Expression> l, TokenType o, std::shared_ptr<Expression> r)
        : left(l), op(o), right(r) {}
};

class AssignmentExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> left;
    TokenType op;
    std::shared_ptr<Expression> value;

    AssignmentExpression(std::shared_ptr<Expression> l, std::shared_ptr<Expression> v, TokenType o = TokenType::ASSIGN)
        : left(l), op(o), value(v) {}
};

class CallExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> arguments;

    CallExpression(std::shared_ptr<Expression> c, const std::vector<std::shared_ptr<Expression>>& args)
        : callee(c), arguments(args) {}
};

class NewExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> arguments;

    NewExpression(std::shared_ptr<Expression> c, const std::vector<std::shared_ptr<Expression>>& args)
        : callee(c), arguments(args) {}
};

class UnaryExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    TokenType op;
    std::shared_ptr<Expression> argument;

    UnaryExpression(TokenType o, std::shared_ptr<Expression> arg) : op(o), argument(arg) {}
};

class MemberExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> object;
    std::shared_ptr<Expression> property;
    bool computed;

    MemberExpression(std::shared_ptr<Expression> obj, std::shared_ptr<Expression> prop, bool comp = false)
        : object(obj), property(prop), computed(comp) {}
};

class UpdateExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    TokenType op;
    std::shared_ptr<Expression> argument;
    bool isPrefix;

    UpdateExpression(TokenType o, std::shared_ptr<Expression> arg, bool pre)
        : op(o), argument(arg), isPrefix(pre) {}
};

class ThisExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;
};

class ConditionalExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> test;
    std::shared_ptr<Expression> consequent;
    std::shared_ptr<Expression> alternate;

    ConditionalExpression(std::shared_ptr<Expression> t, std::shared_ptr<Expression> c, std::shared_ptr<Expression> a)
        : test(t), consequent(c), alternate(a) {}
};

class ArrayLiteral : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
std::vector<std::shared_ptr<Expression>> elements;
    explicit ArrayLiteral(const std::vector<std::shared_ptr<Expression>>& elems) : elements(elems) {}
};

class BlockStatement;

class FunctionExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::string name; // Can be empty for anonymous
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> parameters;
    std::shared_ptr<BlockStatement> body;
    bool hasRest;

    FunctionExpression(const std::string& n, const std::vector<std::pair<std::string, std::shared_ptr<Expression>>>& params,
                       std::shared_ptr<BlockStatement> b, bool rest = false)
        : name(n), parameters(params), body(b), hasRest(rest) {}
};

class ArrowFunctionExpression : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> parameters;
    std::shared_ptr<BlockStatement> body;
    bool hasRest;

    ArrowFunctionExpression(const std::vector<std::pair<std::string, std::shared_ptr<Expression>>>& params,
                            std::shared_ptr<BlockStatement> b, bool rest = false)
        : parameters(params), body(b), hasRest(rest) {}
};

class SpreadElement : public Expression {
public:
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

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
    std::shared_ptr<JSValue> eval(Evaluator* evaluator) override;

public:
    std::vector<Property> properties;
    explicit ObjectLiteral(const std::vector<Property>& props) : properties(props) {}
};

// --- Statements ---

class ExpressionStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> expression;
    explicit ExpressionStatement(std::shared_ptr<Expression> expr) : expression(expr) {}
};

class VariableDeclaration : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    bool isConst;
    bool isVar;
    std::string name;
    std::shared_ptr<Expression> initializer;

    VariableDeclaration(bool c, bool v, const std::string& n, std::shared_ptr<Expression> init)
        : isConst(c), isVar(v), name(n), initializer(init) {}
};

class DestructuringDeclaration : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    bool isConst;
    bool isVar;
    std::vector<std::string> names;
    std::shared_ptr<Expression> initializer;
    bool isArray;

    DestructuringDeclaration(bool c, bool v, const std::vector<std::string>& n, std::shared_ptr<Expression> init, bool isArr = false)
        : isConst(c), isVar(v), names(n), initializer(init), isArray(isArr) {}
};

class BlockStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::vector<std::shared_ptr<Statement>> statements;
    explicit BlockStatement(const std::vector<std::shared_ptr<Statement>>& stmts) : statements(stmts) {}
};

class MethodDefinition : public ASTNode {
public:
    std::string name;
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> parameters;
    bool hasRest;
    std::shared_ptr<BlockStatement> body;
    bool isAsync;

    MethodDefinition(const std::string& n, const std::vector<std::pair<std::string, std::shared_ptr<Expression>>>& params,
                     bool rest, std::shared_ptr<BlockStatement> b, bool async = false)
        : name(n), parameters(params), hasRest(rest), body(b), isAsync(async) {}
};

class ClassDeclaration : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::string name;
    std::shared_ptr<Expression> superClass; // Can be null
    std::vector<std::shared_ptr<MethodDefinition>> methods;

    ClassDeclaration(const std::string& n, std::shared_ptr<Expression> superC, const std::vector<std::shared_ptr<MethodDefinition>>& m)
        : name(n), superClass(superC), methods(m) {}
};

class IfStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> consequent;
    std::shared_ptr<Statement> alternate; // Can be null

    IfStatement(std::shared_ptr<Expression> cond, std::shared_ptr<Statement> cons, std::shared_ptr<Statement> alt = nullptr)
        : condition(cond), consequent(cons), alternate(alt) {}
};

class WhileStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> body;

    WhileStatement(std::shared_ptr<Expression> cond, std::shared_ptr<Statement> b)
        : condition(cond), body(b) {}
};

class DoWhileStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Statement> body;
    std::shared_ptr<Expression> condition;

    DoWhileStatement(std::shared_ptr<Statement> b, std::shared_ptr<Expression> cond)
        : body(b), condition(cond) {}
};

class ForStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Statement> init; // Can be VariableDeclaration or ExpressionStatement
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> update;
    std::shared_ptr<Statement> body;

    ForStatement(std::shared_ptr<Statement> i, std::shared_ptr<Expression> cond,
                 std::shared_ptr<Expression> upd, std::shared_ptr<Statement> b)
        : init(i), condition(cond), update(upd), body(b) {}
};

class ForInStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Statement> left; // VariableDeclaration or Identifier (ExpressionStatement)
    std::shared_ptr<Expression> right;
    std::shared_ptr<Statement> body;

    ForInStatement(std::shared_ptr<Statement> l, std::shared_ptr<Expression> r, std::shared_ptr<Statement> b)
        : left(l), right(r), body(b) {}
};

class ForOfStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Statement> left; // VariableDeclaration or Identifier (ExpressionStatement)
    std::shared_ptr<Expression> right;
    std::shared_ptr<Statement> body;

    ForOfStatement(std::shared_ptr<Statement> l, std::shared_ptr<Expression> r, std::shared_ptr<Statement> b)
        : left(l), right(r), body(b) {}
};

class SwitchCase {
public:
    std::shared_ptr<Expression> test; // nullptr means 'default'
    std::vector<std::shared_ptr<Statement>> consequent;
    SwitchCase(std::shared_ptr<Expression> t, const std::vector<std::shared_ptr<Statement>>& cons)
        : test(t), consequent(cons) {}
};

class SwitchStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> discriminant;
    std::vector<SwitchCase> cases;

    SwitchStatement(std::shared_ptr<Expression> disc, const std::vector<SwitchCase>& cs)
        : discriminant(disc), cases(cs) {}
};

class FunctionDeclaration : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::string name;
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> parameters;
    std::shared_ptr<BlockStatement> body;
    bool hasRest;

    FunctionDeclaration(const std::string& n, const std::vector<std::pair<std::string, std::shared_ptr<Expression>>>& params,
                        std::shared_ptr<BlockStatement> b, bool rest = false)
        : name(n), parameters(params), body(b), hasRest(rest) {}
};

class ReturnStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> argument;
    explicit ReturnStatement(std::shared_ptr<Expression> arg = nullptr) : argument(arg) {}
};

class BreakStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    BreakStatement() {}
};

class ContinueStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
ContinueStatement() {}
};

class ThrowStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<Expression> argument;

    explicit ThrowStatement(std::shared_ptr<Expression> arg) : argument(arg) {}
};

class TryStatement : public Statement {
public:
    void execute(Evaluator* evaluator) override;

public:
    std::shared_ptr<BlockStatement> block;
    std::shared_ptr<BlockStatement> handler;
    std::string param;
    std::shared_ptr<BlockStatement> finalizer; // Can be null

    TryStatement(std::shared_ptr<BlockStatement> b, std::shared_ptr<BlockStatement> h, const std::string& p, std::shared_ptr<BlockStatement> f = nullptr)
        : block(b), handler(h), param(p), finalizer(f) {}
};

// Root of the AST
class Program : public ASTNode {
public:
    std::vector<std::shared_ptr<Statement>> body;
};

#endif // AST_H
