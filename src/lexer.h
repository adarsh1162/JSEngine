#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

// Define all possible token types in JavaScript
enum class TokenType {
    // Keywords
    LET, CONST, VAR, FUNCTION, IF, ELSE, FOR, WHILE, DO, SWITCH, RETURN, TYPEOF, NEW,
    TRY, CATCH, FINALLY, THROW, THIS, BREAK, CONTINUE, CASE, DEFAULT_KW, IN_KW, OF_KW,
    CLASS, EXTENDS, SUPER, ASYNC, AWAIT,

    // Data Types / Literals
    NUMBER, STRING, BOOLEAN_TRUE, BOOLEAN_FALSE, NULL_LIT, UNDEFINED,
    TEMPLATE_LITERAL, REGEX_LITERAL,
    IDENTIFIER,

    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN,
    INCREMENT, DECREMENT, QUESTION,
    BITWISE_AND, BITWISE_OR, BITWISE_XOR, BITWISE_NOT, LEFT_SHIFT, RIGHT_SHIFT, UNSIGNED_RIGHT_SHIFT,
    EQUAL, STRICT_EQUAL, NOT_EQUAL, STRICT_NOT_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    POWER, // **

    // Punctuation
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, DOT, SEMICOLON, COLON,
    SPREAD, // ...

    // Arrow Function
    ARROW, // =>

    // End of File
    EOF_TOKEN,
    
    // Unknown or Invalid token
    ILLEGAL
};

// Represents a single token
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, std::string v, int l, int c) : type(t), value(v), line(l), column(c) {}
};

// The Lexer class responsible for breaking JS code into Tokens
class Lexer {
private:
    std::string source;
    size_t position;
    int line;
    int column;
    char currentChar;

    void advance();
    char peek(int offset = 1);
    void skipWhitespace();
    void skipComments();

    Token nextToken();
    Token readNumber();
    Token readString(char quoteType);
    Token readTemplateLiteral();
    Token readIdentifierOrKeyword();

public:
    TokenType lastTokenType = TokenType::ILLEGAL;
    explicit Lexer(const std::string& sourceCode);
    std::vector<Token> tokenize();
};

// Helper for debugging
std::string tokenTypeToString(TokenType type);

#endif // LEXER_H
