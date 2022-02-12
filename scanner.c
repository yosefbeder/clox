#include "scanner.h"
#include <string.h>

void initScanner(Scanner* scanner, char source[]) {
    resetScanner(scanner);
    scanner->current = source;
    scanner->start = source;
}

Token popToken(Scanner* scanner, TokenType type) {
    Token token;

    token.type = type;
    token.line = scanner->line;
    token.start = scanner->start;
    token.length = scanner->current - scanner->start;

    return token;
}

Token errorToken(Scanner* scanner, char msg[]) {
    Token token;
    
    token.type = TOKEN_ERROR;
    token.line = scanner->line;
    token.start = msg;
    token.length = strlen(msg);

    return token;
}

char next(Scanner* scanner) {
    return *scanner->current++;
}

char peek(Scanner* scanner) {
    return *scanner->current;
}

char peekNext(Scanner* scanner) {
    return *(scanner->current + 1);
}

char lookback(Scanner* scanner) {
    return *(scanner->current - 1);
}

int atEnd(Scanner* scanner) {
    return peek(scanner) == '\0';
}

int match(Scanner* scanner, char expected) {
    if (atEnd(scanner)) return 0;

    if (peek(scanner) != expected) return 0;

    next(scanner);
    return 1;
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isAlphabet(char c) {
    return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z';
}

void skipWhitespace(Scanner* scanner) {
    while (1) {
        switch (peek(scanner)) {
            case ' ':
            case '\r':
            case '\t':
                next(scanner);
                scanner->start = scanner->current;
                break;
            case '\n':
                next(scanner);
                scanner->start = scanner->current;
                scanner->line++;
                break;
            case '/':
                if (peekNext(scanner) == '/') {
                    while (peek(scanner) != '\n' && !atEnd(scanner)) next(scanner);
                    break;
                } else return;
                break;
            default:
                return;
        }
    }
};

Token scanToken(Scanner* scanner) {
    skipWhitespace(scanner);

    scanner->start = scanner->current;

    if(atEnd(scanner)) {
        next(scanner);
        return popToken(scanner, TOKEN_EOF);
    }


    char c = next(scanner);

    // 15 15.5
    if (isDigit(c)) {
        while (isDigit(peek(scanner))) next(scanner);

        if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
            next(scanner);
            while (isDigit(peek(scanner))) next(scanner);
            return popToken(scanner, TOKEN_NUMBER);
        } else return popToken(scanner, TOKEN_NUMBER);
    }

    switch (c) {
        case '(':
            return popToken(scanner, TOKEN_LEFT_PAREN);
        case ')':
            return popToken(scanner, TOKEN_RIGHT_PAREN);
        case '{':
            return popToken(scanner, TOKEN_LEFT_BRACE);
        case '}':
            return popToken(scanner, TOKEN_RIGHT_BRACE);
        case ',':
            return popToken(scanner, TOKEN_COMMA);
        case '.':
            return popToken(scanner, TOKEN_DOT);
        case '-':
            return popToken(scanner, TOKEN_MINUS);
        case '+':
            return popToken(scanner, TOKEN_PLUS);
        case ';':
            return popToken(scanner, TOKEN_SEMICOLON);
        case '*':
            return popToken(scanner, TOKEN_STAR);
        case '/':
            return popToken(scanner, TOKEN_SLASH);
        case '!':
            return popToken(scanner, match(scanner, '=')? TOKEN_BANG_EQUAL: TOKEN_BANG);
        case '=':
            return popToken(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL: TOKEN_EQUAL);
        case '>':
            return popToken(scanner, match(scanner, '=')? TOKEN_GREATER_EQUAL:  TOKEN_GREATER);
        case '<':
            return popToken(scanner, match(scanner, '=')? TOKEN_LESS_EQUAL: TOKEN_LESS);
        default: {
            return errorToken(scanner, "Unexpected character");
        }
    }
}

void resetScanner(Scanner* scanner) {
    scanner->current = NULL;
    scanner->start = NULL;
    scanner->line = 1;
}

    // #define IS_IDENTIFIER_START(c) isAlphabet(c) || c == '_'
    // #define IS_IDENTIFIER_MID(c) IS_IDENTIFIER_START(c) || isDigit(c)

    // if (IS_IDENTIFIER_START(*scanner->current)) {
    //     char value[MAX_IDETNFIER_LENGTH];

    //     next(scanner);

    //     while (IS_IDENTIFIER_MID(*scanner->current)) {
    //         next(scanner);
    //     }

    //     return popToken(scanner, TOKEN_IDENTIFIER);
    // }

    // #undef IS_IDENTIFIER_MID
    // #undef IS_IDENTIFIER_START