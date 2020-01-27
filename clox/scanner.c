#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

void initScanner(Scanner* scanner, const char* source) {
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAtEnd(Scanner* scanner) {
    return *scanner->current == '\0';
}

static char peek(Scanner* scanner) {
    return *scanner->current;
}

static char peekNext(Scanner* scanner) {
    if (isAtEnd(scanner)) {
        return '\0';
    }
    return scanner->current[1];
}

static char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

static bool match(Scanner* scanner, char expected) {
    if (isAtEnd(scanner)) {
        return false;
    }
    if (*scanner->current != expected) {
        return false;
    }
    scanner->current++;
    return true;
}

static void skipWhitespace(Scanner* scanner) {
    while (1) {
        char c = peek(scanner);
        switch(c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '/':
                if (peekNext(scanner) == '/') {
                    while(peek(scanner) != '\n' && !isAtEnd(scanner)) {
                        advance(scanner);
                    }
                }
                else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static void makeToken(Scanner* scanner, Token* token, TokenType type) {
    token->type = type;
    token->start = scanner->start;
    token->length = (int)(scanner->current - scanner->start);
    token->line = scanner->line;
}

static void errorToken(Scanner* scanner, Token* token, const char* message) {
    token->type = TOKEN_ERROR;
    token->start = message;
    token->length = (int)strlen(message);
    token->line = scanner->line;
}

#define MAKE_TOKEN(type) makeToken(scanner, token, (type))
#define ERROR_TOKEN(message) errorToken(scanner, token, (message))
#define MATCH(expected) match(scanner, (expected))

static void string(Scanner* scanner, Token* token) {
    while (peek(scanner) != '"' && !isAtEnd(scanner)) {
        if (peek(scanner) == '\n') {
            scanner->line++;
        }
        advance(scanner);
    }

    if (isAtEnd(scanner)) {
        ERROR_TOKEN("Unterminated string.");
        return;
    }

    advance(scanner);
    MAKE_TOKEN(TOKEN_STRING);
    return;
}

static void number(Scanner* scanner, Token* token) {
    while(isDigit(peek(scanner))) {
        advance(scanner);
    }

    // Look for decimal part
    if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
        // Consume the '.'
        advance(scanner);

        while(isDigit(peek(scanner))) {
            advance(scanner);
        }
    }
    MAKE_TOKEN(TOKEN_NUMBER);
    return;
}

static TokenType checkKeyword(Scanner* scanner, size_t start, size_t length,
        const char* rest, TokenType type) {
    if (scanner->current - scanner->start == (int)(start + length) &&
            memcmp(scanner->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Scanner* scanner) {
    switch (scanner->start[0]) {
        case 'a': return checkKeyword(scanner, 1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(scanner, 1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(scanner, 1, 3, "lse", TOKEN_ELSE);
        case 'i': return checkKeyword(scanner, 1, 1, "f", TOKEN_IF);
        case 'f':                                                     
        if (scanner->current - scanner->start > 1) {                  
            switch (scanner->start[1]) {                               
                case 'a': return checkKeyword(scanner,
                    2, 3, "lse", TOKEN_FALSE);
                case 'o': return checkKeyword(scanner, 2, 1, "r", TOKEN_FOR);    
                case 'u': return checkKeyword(scanner, 2, 1, "n", TOKEN_FUN);    
            }                                                         
        }                                                           
        break;
        case 'n': return checkKeyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(scanner, 1, 4, "uper", TOKEN_SUPER);
        case 't':                                                   
        if (scanner->current - scanner->start > 1) {                
            switch (scanner->start[1]) {                             
            case 'h': return checkKeyword(scanner, 2, 2, "is", TOKEN_THIS);
            case 'r': return checkKeyword(scanner, 2, 2, "ue", TOKEN_TRUE);
            }                                                       
        }                                                         
        break; 
        case 'v': return checkKeyword(scanner, 1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static void identifier(Scanner* scanner, Token* token) {
    while(isAlpha(peek(scanner)) || isDigit(peek(scanner))) {
        advance(scanner);
    }

    MAKE_TOKEN(identifierType(scanner));
    return;
}

void scanToken(Scanner* scanner, Token* token) {
    skipWhitespace(scanner);

    scanner->start = scanner->current;

    if (isAtEnd(scanner)) {
        MAKE_TOKEN(TOKEN_EOF);
        return;
    }

    char c = advance(scanner);

    if (isAlpha(c)) {
        identifier(scanner, token);
        return;
    }

    if (isDigit(c)) {
        number(scanner, token);
        return;
    }

    switch (c) {
        case '(': MAKE_TOKEN(TOKEN_LEFT_PAREN); return;
        case ')': MAKE_TOKEN(TOKEN_RIGHT_PAREN); return;
        case '{': MAKE_TOKEN(TOKEN_LEFT_BRACE); return;
        case '}': MAKE_TOKEN(TOKEN_RIGHT_BRACE); return;
        case ';': MAKE_TOKEN(TOKEN_SEMICOLON); return;
        case ',': MAKE_TOKEN(TOKEN_COMMA); return;
        case '.': MAKE_TOKEN(TOKEN_DOT); return;
        case '-': MAKE_TOKEN(TOKEN_MINUS); return;
        case '+': MAKE_TOKEN(TOKEN_PLUS); return;
        case '/': MAKE_TOKEN(TOKEN_SLASH); return;
        case '*': MAKE_TOKEN(TOKEN_STAR); return;
        case '!':
            MAKE_TOKEN(MATCH('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
            return;
        case '=':
            MAKE_TOKEN(MATCH('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
            return;
        case '<':
            MAKE_TOKEN(MATCH('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
            return;
        case '>':
            MAKE_TOKEN(MATCH('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
            return;
        case '"':
            string(scanner, token);
            return;
    }

    ERROR_TOKEN("Unexpected character.");
    return;
}

#undef MAKE_TOKEN
#undef ERROR_TOKEN
#undef MATCH