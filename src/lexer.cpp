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

Token Lexer::readTemplateLiteral() {
    std::string result;
    int startCol = column;
    advance(); // skip opening backtick

    while (currentChar != '\0' && currentChar != '`') {
        if (currentChar == '\\') {
            advance(); // Skip backslash
            if (currentChar == 'n') result += '\n';
            else if (currentChar == 't') result += '\t';
            else if (currentChar == '\\') result += '\\';
            else if (currentChar == '`') result += '`';
            else result += currentChar;
        } else {
            result += currentChar;
        }
        advance();
    }
    
    if (currentChar == '`') {
        advance(); // skip closing backtick
    }
    
    return Token(TokenType::TEMPLATE_LITERAL, result, line, startCol);
}

// Removed readRegexLiteral since it's inline in nextToken()

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
    else if (result == "var") type = TokenType::VAR;
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
    else if (result == "try") type = TokenType::TRY;
    else if (result == "catch") type = TokenType::CATCH;
    else if (result == "finally") type = TokenType::FINALLY;
    else if (result == "throw") type = TokenType::THROW;
    else if (result == "this") type = TokenType::THIS;
    else if (result == "break") type = TokenType::BREAK;
    else if (result == "continue") type = TokenType::CONTINUE;
    else if (result == "case") type = TokenType::CASE;
    else if (result == "default") type = TokenType::DEFAULT_KW;
    else if (result == "in") type = TokenType::IN_KW;
    else if (result == "of") type = TokenType::OF_KW;
    else if (result == "class") type = TokenType::CLASS;
    else if (result == "extends") type = TokenType::EXTENDS;
    else if (result == "super") type = TokenType::SUPER;
    else if (result == "async") type = TokenType::ASYNC;
    else if (result == "await") type = TokenType::AWAIT;

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

        if (currentChar == '`') {
            return readTemplateLiteral();
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
            if (peek() == '>') {
                if (peek(2) == '>') {
                    advance(); advance(); advance();
                    return Token(TokenType::UNSIGNED_RIGHT_SHIFT, ">>>", line, startCol);
                }
                advance(); advance();
                return Token(TokenType::RIGHT_SHIFT, ">>", line, startCol);
            }
            if (peek() == '=') {
                advance(); advance();
                return Token(TokenType::GREATER_EQUAL, ">=", line, startCol);
            }
            advance();
            return Token(TokenType::GREATER, ">", line, startCol);
        }

        if (currentChar == '<') {
            if (peek() == '<') {
                advance(); advance();
                return Token(TokenType::LEFT_SHIFT, "<<", line, startCol);
            }
            if (peek() == '=') {
                advance(); advance();
                return Token(TokenType::LESS_EQUAL, "<=", line, startCol);
            }
            advance();
            return Token(TokenType::LESS, "<", line, startCol);
        }

        if (currentChar == '&') {
            if (peek() == '&') {
                advance(); advance();
                return Token(TokenType::LOGICAL_AND, "&&", line, startCol);
            }
            advance();
            return Token(TokenType::BITWISE_AND, "&", line, startCol);
        }

        if (currentChar == '|') {
            if (peek() == '|') {
                advance(); advance();
                return Token(TokenType::LOGICAL_OR, "||", line, startCol);
            }
            advance();
            return Token(TokenType::BITWISE_OR, "|", line, startCol);
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
                if (currentChar == '+') { advance(); return Token(TokenType::INCREMENT, "++", line, startCol); }
                if (currentChar == '=') { advance(); return Token(TokenType::PLUS_ASSIGN, "+=", line, startCol); }
                return Token(TokenType::PLUS, "+", line, startCol);
            case '-': 
                if (currentChar == '-') { advance(); return Token(TokenType::DECREMENT, "--", line, startCol); }
                if (currentChar == '=') { advance(); return Token(TokenType::MINUS_ASSIGN, "-=", line, startCol); }
                return Token(TokenType::MINUS, "-", line, startCol);
            case '/': 
                if (lastTokenType == TokenType::ILLEGAL || lastTokenType == TokenType::ASSIGN ||
                    lastTokenType == TokenType::LPAREN || lastTokenType == TokenType::LBRACKET ||
                    lastTokenType == TokenType::LBRACE || lastTokenType == TokenType::COMMA ||
                    lastTokenType == TokenType::SEMICOLON || lastTokenType == TokenType::COLON ||
                    lastTokenType == TokenType::RETURN || lastTokenType == TokenType::PLUS ||
                    lastTokenType == TokenType::MINUS || lastTokenType == TokenType::MULTIPLY ||
                    lastTokenType == TokenType::DIVIDE || lastTokenType == TokenType::MODULO ||
                    lastTokenType == TokenType::EQUAL || lastTokenType == TokenType::STRICT_EQUAL ||
                    lastTokenType == TokenType::NOT_EQUAL || lastTokenType == TokenType::STRICT_NOT_EQUAL ||
                    lastTokenType == TokenType::LOGICAL_AND || lastTokenType == TokenType::LOGICAL_OR ||
                    lastTokenType == TokenType::LOGICAL_NOT || lastTokenType == TokenType::QUESTION) {
                    
                    // We must step BACK to before the '/' to let readRegexLiteral consume it properly
                    // Actually, we already advanced past '/'. We need to adjust readRegexLiteral to not consume it again.
                    // Let's just adjust readRegexLiteral to assume '/' is already consumed, EXCEPT it needs to read the content.
                    // Wait, our readRegexLiteral assumes currentChar is at the start of the content inside the slash.
                    // So we can just call readRegexLiteral() here!
                    // Wait, readRegexLiteral() calls advance() to skip opening slash. Since we already advanced, we shouldn't advance again!
                    // Let's fix readRegexLiteral so it doesn't advance initially if we call it from here, or we pass a flag.
                    // Actually, I'll just change the logic here to read the regex content directly or inline it.
                    std::string regexContent = "/";
                    while (currentChar != '\0' && currentChar != '/') {
                        if (currentChar == '\\') {
                            regexContent += currentChar;
                            advance();
                        }
                        if (currentChar == '\0') break;
                        regexContent += currentChar;
                        advance();
                    }
                    if (currentChar == '/') {
                        regexContent += currentChar;
                        advance(); // skip closing slash
                    }
                    // Read flags
                    while (isalpha(currentChar)) {
                        regexContent += currentChar;
                        advance();
                    }
                    return Token(TokenType::REGEX_LITERAL, regexContent, line, startCol);
                }
                
                if (currentChar == '=') { advance(); return Token(TokenType::DIVIDE_ASSIGN, "/=", line, startCol); }
                return Token(TokenType::DIVIDE, "/", line, startCol);
            case '%': return Token(TokenType::MODULO, "%", line, startCol);
            case '^': return Token(TokenType::BITWISE_XOR, "^", line, startCol);
            case '~': return Token(TokenType::BITWISE_NOT, "~", line, startCol);
            case '(': return Token(TokenType::LPAREN, "(", line, startCol);
            case ')': return Token(TokenType::RPAREN, ")", line, startCol);
            case '{': return Token(TokenType::LBRACE, "{", line, startCol);
            case '}': return Token(TokenType::RBRACE, "}", line, startCol);
            case '[': return Token(TokenType::LBRACKET, "[", line, startCol);
            case ']': return Token(TokenType::RBRACKET, "]", line, startCol);
            case ',': return Token(TokenType::COMMA, ",", line, startCol);
            case ';': return Token(TokenType::SEMICOLON, ";", line, startCol);
            case ':': return Token(TokenType::COLON, ":", line, startCol);
            case '?': return Token(TokenType::QUESTION, "?", line, startCol);
        }

        return Token(TokenType::ILLEGAL, std::string(1, c), line, startCol);
    }

    return Token(TokenType::EOF_TOKEN, "", line, column);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    lastTokenType = TokenType::ILLEGAL;
    Token tok = nextToken();
    while (tok.type != TokenType::EOF_TOKEN) {
        tokens.push_back(tok);
        lastTokenType = tok.type;
        tok = nextToken();
    }
    tokens.push_back(tok); // Push EOF
    return tokens;
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LET: return "LET";
        case TokenType::CONST: return "CONST";
        case TokenType::VAR: return "VAR";
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
        case TokenType::TRY: return "TRY";
        case TokenType::CATCH: return "CATCH";
        case TokenType::FINALLY: return "FINALLY";
        case TokenType::THROW: return "THROW";
        case TokenType::THIS: return "THIS";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::INCREMENT: return "INCREMENT";
        case TokenType::DECREMENT: return "DECREMENT";
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
        case TokenType::QUESTION: return "QUESTION";
        case TokenType::SPREAD: return "SPREAD";
        case TokenType::ARROW: return "ARROW";
        case TokenType::EOF_TOKEN: return "EOF_TOKEN";
        case TokenType::ILLEGAL: return "ILLEGAL";
        default: return "UNKNOWN";
    }
}
