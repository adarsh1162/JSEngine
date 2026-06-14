#include "ast.h"
#include "evaluator.h"

void ContinueStatement::execute(Evaluator* evaluator) {
    evaluator->execContinueStatement(this);
}
void ExpressionStatement::execute(Evaluator* evaluator) {
    evaluator->execExpressionStatement(this);
}
void ThrowStatement::execute(Evaluator* evaluator) {
    evaluator->execThrowStatement(this);
}
void ReturnStatement::execute(Evaluator* evaluator) {
    evaluator->execReturnStatement(this);
}
void BlockStatement::execute(Evaluator* evaluator) {
    evaluator->execBlockStatement(this);
}
void IfStatement::execute(Evaluator* evaluator) {
    evaluator->execIfStatement(this);
}
void DoWhileStatement::execute(Evaluator* evaluator) {
    evaluator->execDoWhileStatement(this);
}
void ForOfStatement::execute(Evaluator* evaluator) {
    evaluator->execForOfStatement(this);
}
void SwitchStatement::execute(Evaluator* evaluator) {
    evaluator->execSwitchStatement(this);
}
void DestructuringDeclaration::execute(Evaluator* evaluator) {
    evaluator->execDestructuringDeclaration(this);
}
void ClassDeclaration::execute(Evaluator* evaluator) {
    evaluator->execClassDeclaration(this);
}
void TryStatement::execute(Evaluator* evaluator) {
    evaluator->execTryStatement(this);
}
void WhileStatement::execute(Evaluator* evaluator) {
    evaluator->execWhileStatement(this);
}
void BreakStatement::execute(Evaluator* evaluator) {
    evaluator->execBreakStatement(this);
}
void FunctionDeclaration::execute(Evaluator* evaluator) {
    evaluator->execFunctionDeclaration(this);
}
void ForStatement::execute(Evaluator* evaluator) {
    evaluator->execForStatement(this);
}
void VariableDeclaration::execute(Evaluator* evaluator) {
    evaluator->execVariableDeclaration(this);
}
void ForInStatement::execute(Evaluator* evaluator) {
    evaluator->execForInStatement(this);
}
std::shared_ptr<JSValue> ArrayLiteral::eval(Evaluator* evaluator) {
    return evaluator->evalArrayLiteral(this);
}
std::shared_ptr<JSValue> NumberLiteral::eval(Evaluator* evaluator) {
    return evaluator->evalNumberLiteral(this);
}
std::shared_ptr<JSValue> ThisExpression::eval(Evaluator* evaluator) {
    return evaluator->evalThisExpression(this);
}
std::shared_ptr<JSValue> UnaryExpression::eval(Evaluator* evaluator) {
    return evaluator->evalUnaryExpression(this);
}
std::shared_ptr<JSValue> BinaryExpression::eval(Evaluator* evaluator) {
    return evaluator->evalBinaryExpression(this);
}
std::shared_ptr<JSValue> TemplateLiteralExpression::eval(Evaluator* evaluator) {
    return evaluator->evalTemplateLiteralExpression(this);
}
std::shared_ptr<JSValue> ObjectLiteral::eval(Evaluator* evaluator) {
    return evaluator->evalObjectLiteral(this);
}
std::shared_ptr<JSValue> BooleanLiteral::eval(Evaluator* evaluator) {
    return evaluator->evalBooleanLiteral(this);
}
std::shared_ptr<JSValue> ConditionalExpression::eval(Evaluator* evaluator) {
    return evaluator->evalConditionalExpression(this);
}
std::shared_ptr<JSValue> RegexLiteralExpression::eval(Evaluator* evaluator) {
    return evaluator->evalRegexLiteralExpression(this);
}
std::shared_ptr<JSValue> NewExpression::eval(Evaluator* evaluator) {
    return evaluator->evalNewExpression(this);
}
std::shared_ptr<JSValue> AssignmentExpression::eval(Evaluator* evaluator) {
    return evaluator->evalAssignmentExpression(this);
}
std::shared_ptr<JSValue> MemberExpression::eval(Evaluator* evaluator) {
    return evaluator->evalMemberExpression(this);
}
std::shared_ptr<JSValue> FunctionExpression::eval(Evaluator* evaluator) {
    return evaluator->evalFunctionExpression(this);
}
std::shared_ptr<JSValue> Identifier::eval(Evaluator* evaluator) {
    return evaluator->evalIdentifier(this);
}
std::shared_ptr<JSValue> ArrowFunctionExpression::eval(Evaluator* evaluator) {
    return evaluator->evalArrowFunctionExpression(this);
}
std::shared_ptr<JSValue> CallExpression::eval(Evaluator* evaluator) {
    return evaluator->evalCallExpression(this);
}
std::shared_ptr<JSValue> UpdateExpression::eval(Evaluator* evaluator) {
    return evaluator->evalUpdateExpression(this);
}
std::shared_ptr<JSValue> SpreadElement::eval(Evaluator* evaluator) {
    return evaluator->evalSpreadElement(this);
}
std::shared_ptr<JSValue> SuperExpression::eval(Evaluator* evaluator) {
    return evaluator->evalSuperExpression(this);
}
std::shared_ptr<JSValue> StringLiteral::eval(Evaluator* evaluator) {
    return evaluator->evalStringLiteral(this);
}
