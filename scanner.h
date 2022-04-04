#ifndef clox_scanner_h
#define clox_scanner_h

#include "common.h"

#define MAX_IDETNFIER_LENGTH 31

typedef struct
{
    char *source;
    char *start;
    char *current;
    int stringDepth;
} Scanner;

typedef enum
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    TOKEN_QUESTION_MARK,
    TOKEN_COLON,
    // One or two character tokens.
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_TEMPLATE_HEAD,
    TOKEN_TEMPLATE_MIDDLE,
    TOKEN_TEMPLATE_TAIL,
    // Keywords.
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_CONTINUE,
    TOKEN_EXTENDS,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;
    char *source;
    char *start;
    int length;
    char *errorMsg;
} Token;

void getTokenPos(int[2], Token *);

void initScanner(Scanner *, char[]);

Token scanToken(Scanner *);

Token virtualToken(TokenType, char *);

void resetScanner(Scanner *);

#endif