#include "lexer.h"
#include <cctype>
#include <unordered_map>
#include <iostream>

Lexer::Lexer(const std::string& sourceCode) 
    : source(sourceCode), position(0), line(1), column(1) {
    if (!source.empty()) {
        currentChar = source[0];
    } else {
        currentChar = '\0';
    }
}

void Lexer::advance() {
    position++;
    column++;
    if (position >= source.length()) {
        currentChar = '\0';
    } else {
        currentChar = source[position];
        // Note: Line counting is handled in advance when seeing \n
    }
}

char Lexer::peek(int offset) {
    if (position + offset >= source.length()) {
        return '\0';
    }
    return source[position + offset];
}

void Lexer::skipWhitespace() {
    while (currentChar != '\0' && isspace(currentChar)) {
        if (currentChar == '\n') {
            line++;
            column = 0; // Will become 1 on next advance if we did manual advance, but let's handle properly
        }
        advance();
    }
}

void Lexer::skipComments() {
    while (currentChar == '/') {
        if (peek() == '/') {
            // Single line comment
            while (currentChar != '\0' && currentChar != '\n') {
                advance();
            }
            skipWhitespace();
        } else if (peek() == '*') {
            // Multi-line comment
            advance(); // skip /
            advance(); // skip *
            while (currentChar != '\0' && !(currentChar == '*' && peek() == '/')) {
                if (currentChar == '\n') {
                    line++;
                    column = 0;
                }
                advance();
            }
            if (currentChar != '\0') {
                advance(); // skip *
                advance(); // skip /
            }
            skipWhitespace();
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    std::string result;
    int startCol = column;
    bool hasDot = false;

    while (currentChar != '\0' && (isdigit(currentChar) || (currentChar == '.' && !hasDot))) {
        if (currentChar == '.') {
            hasDot = true;
        }
        result += currentChar;
        advance();
    }
    return Token(TokenType::NUMBER, result, line, startCol);
}

Token Lexer::readString(char quoteType) {
    std::string result;
    int startCol = column;
    advance(); // skip opening quote

    while (currentChar != '\0' && currentChar != quoteType) {
        if (currentChar == '\\') {
            advance(); // Skip backslash
            // Simple escape sequence handling
            if (currentChar == 'n') result += '\n';
            else if (currentChar == 't') result += '\t';
            else if (currentChar == '\\') result += '\\';
            else if (currentChar == '"') result += '"';
            else if (currentChar == '\'') result += '\'';
            else result += currentChar;
        } else {
            result += currentChar;
        }
        advance();
    }
    
    if (currentChar == quoteType) {
        advance(); // skip closing quote
    }
    
    return Token(TokenType::STRING, result, line, startCol);
}

Token Lexer::readIdentifierOrKeyword() {
    std::string result;
    int startCol = column;

    while (currentChar != '\0' && (isalnum(currentChar) || currentChar == '_' || currentChar == '$')) {
        result += currentChar;
        advance();
    }

    // Keyword matching
    TokenType type = TokenType::IDENTIFIER;
    if (result == "let") type = TokenType::LET;
    else if (result == "const") type = TokenType::CONST;
    else if (result == "function") type = TokenType::FUNCTION;
    else if (result == "if") type = TokenType::IF;
    else if (result == "else") type = TokenType::ELSE;
    else if (result == "for") type = TokenType::FOR;
    else if (result == "while") type = TokenType::WHILE;
    else if (result == "do") type = TokenType::DO;
    else if (result == "switch") type = TokenType::SWITCH;
    else if (result == "return") type = TokenType::RETURN;
    else if (result == "true") type = TokenType::BOOLEAN_TRUE;
    else if (result == "false") type = TokenType::BOOLEAN_FALSE;
    else if (result == "null") type = TokenType::NULL_LIT;
    else if (result == "undefined") type = TokenType::UNDEFINED;
    else if (result == "typeof") type = TokenType::TYPEOF;
    else if (result == "new") type = TokenType::NEW;

    return Token(type, result, line, startCol);
}

Token Lexer::nextToken() {
    while (currentChar != '\0') {
        skipWhitespace();
        skipComments();
        
        if (currentChar == '\0') break;

        // Ensure we didn't just hit EOF after skipping whitespace/comments
        if (isspace(currentChar)) continue; 

        int startCol = column;

        if (isalpha(currentChar) || currentChar == '_' || currentChar == '$') {
            return readIdentifierOrKeyword();
        }

        if (isdigit(currentChar)) {
            return readNumber();
        }

        if (currentChar == '"' || currentChar == '\'') {
            return readString(currentChar);
        }

        // Multi-character Operators
        if (currentChar == '=') {
            if (peek() == '=') {
                if (peek(2) == '=') {
                    advance(); advance(); advance();
                    return Token(TokenType::STRICT_EQUAL, "===", line, startCol);
                }
                advance(); advance();
                return Token(TokenType::EQUAL, "==", line, startCol);
            }
            if (peek() == '>') {
                advance(); advance();
                return Token(TokenType::ARROW, "=>", line, startCol);
            }
            advance();
            return Token(TokenType::ASSIGN, "=", line, startCol);
        }

        if (currentChar == '!') {
            if (peek() == '=') {
                if (peek(2) == '=') {
                    advance(); advance(); advance();
                    return Token(TokenType::STRICT_NOT_EQUAL, "!==", line, startCol);
                }
                advance(); advance();
                return Token(TokenType::NOT_EQUAL, "!=", line, startCol);
            }
            advance();
            return Token(TokenType::LOGICAL_NOT, "!", line, startCol);
        }

        if (currentChar == '>') {
            if (peek() == '=') {
                advance(); advance();
                return Token(TokenType::GREATER_EQUAL, ">=", line, startCol);
            }
            advance();
            return Token(TokenType::GREATER, ">", line, startCol);
        }

        if (currentChar == '<') {
            if (peek() == '=') {
                advance(); advance();
                return Token(TokenType::LESS_EQUAL, "<=", line, startCol);
            }
            advance();
            return Token(TokenType::LESS, "<", line, startCol);
        }

        if (currentChar == '&' && peek() == '&') {
            advance(); advance();
            return Token(TokenType::LOGICAL_AND, "&&", line, startCol);
        }

        if (currentChar == '|' && peek() == '|') {
            advance(); advance();
            return Token(TokenType::LOGICAL_OR, "||", line, startCol);
        }

        if (currentChar == '*') {
            if (peek() == '*') {
                advance(); advance();
                return Token(TokenType::POWER, "**", line, startCol);
            }
            if (peek() == '=') {
                advance(); advance();
                return Token(TokenType::MULTIPLY_ASSIGN, "*=", line, startCol);
            }
            advance();
            return Token(TokenType::MULTIPLY, "*", line, startCol);
        }

        if (currentChar == '.') {
            if (peek() == '.' && peek(2) == '.') {
                advance(); advance(); advance();
                return Token(TokenType::SPREAD, "...", line, startCol);
            }
            // If it's a decimal starting with dot (like .5)
            if (isdigit(peek())) {
                return readNumber();
            }
            advance();
            return Token(TokenType::DOT, ".", line, startCol);
        }

        // Single Character Tokens and Compound Assignments
        char c = currentChar;
        advance();
        switch (c) {
            case '+': 
                if (currentChar == '=') { advance(); return Token(TokenType::PLUS_ASSIGN, "+=", line, startCol); }
                return Token(TokenType::PLUS, "+", line, startCol);
            case '-': 
                if (currentChar == '=') { advance(); return Token(TokenType::MINUS_ASSIGN, "-=", line, startCol); }
                return Token(TokenType::MINUS, "-", line, startCol);
            case '/': 
                if (currentChar == '=') { advance(); return Token(TokenType::DIVIDE_ASSIGN, "/=", line, startCol); }
                return Token(TokenType::DIVIDE, "/", line, startCol);
            case '%': return Token(TokenType::MODULO, "%", line, startCol);
            case '(': return Token(TokenType::LPAREN, "(", line, startCol);
            case ')': return Token(TokenType::RPAREN, ")", line, startCol);
            case '{': return Token(TokenType::LBRACE, "{", line, startCol);
            case '}': return Token(TokenType::RBRACE, "}", line, startCol);
            case '[': return Token(TokenType::LBRACKET, "[", line, startCol);
            case ']': return Token(TokenType::RBRACKET, "]", line, startCol);
            case ',': return Token(TokenType::COMMA, ",", line, startCol);
            case ';': return Token(TokenType::SEMICOLON, ";", line, startCol);
            case ':': return Token(TokenType::COLON, ":", line, startCol);
        }

        return Token(TokenType::ILLEGAL, std::string(1, c), line, startCol);
    }

    return Token(TokenType::EOF_TOKEN, "", line, column);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token tok = nextToken();
    while (tok.type != TokenType::EOF_TOKEN) {
        tokens.push_back(tok);
        tok = nextToken();
    }
    tokens.push_back(tok); // Push EOF
    return tokens;
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LET: return "LET";
        case TokenType::CONST: return "CONST";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::FOR: return "FOR";
        case TokenType::WHILE: return "WHILE";
        case TokenType::DO: return "DO";
        case TokenType::SWITCH: return "SWITCH";
        case TokenType::RETURN: return "RETURN";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::BOOLEAN_TRUE: return "BOOLEAN_TRUE";
        case TokenType::BOOLEAN_FALSE: return "BOOLEAN_FALSE";
        case TokenType::NULL_LIT: return "NULL_LIT";
        case TokenType::UNDEFINED: return "UNDEFINED";
        case TokenType::TYPEOF: return "TYPEOF";
        case TokenType::NEW: return "NEW";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::MODULO: return "MODULO";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case TokenType::DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::STRICT_EQUAL: return "STRICT_EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::STRICT_NOT_EQUAL: return "STRICT_NOT_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::LOGICAL_AND: return "LOGICAL_AND";
        case TokenType::LOGICAL_OR: return "LOGICAL_OR";
        case TokenType::LOGICAL_NOT: return "LOGICAL_NOT";
        case TokenType::POWER: return "POWER";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::SPREAD: return "SPREAD";
        case TokenType::ARROW: return "ARROW";
        case TokenType::EOF_TOKEN: return "EOF_TOKEN";
        case TokenType::ILLEGAL: return "ILLEGAL";
        default: return "UNKNOWN";
    }
}
