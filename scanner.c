#include "scanner.h"
#include <string.h>

void getTokenPos(int pos[2], Token *token)
{
    int i;

    i = pos[1] = 0;
    pos[0] = 1;

    while (1)
    {
        if (token->source[i] == '\n')
            pos[0]++;

        if ((token->source + i) == token->start)
            break;

        i++;
    }

    while (i >= 0 && token->source[i] != '\n')
    {
        pos[1]++;
        i--;
    }
}

void initScanner(Scanner *scanner, char source[])
{
    scanner->source = source;
    scanner->current = source;
    scanner->start = source;
    scanner->stringDepth = 0;
}

static Token popToken(Scanner *scanner, TokenType type)
{
    Token token;

    token.type = type;
    token.start = scanner->start;
    token.length = scanner->current - scanner->start;
    token.source = scanner->source;
    token.errorMsg = NULL;

    return token;
}

static Token errorToken(Scanner *scanner, char msg[])
{
    Token token = popToken(scanner, TOKEN_ERROR);

    token.errorMsg = msg;

    return token;
}

static char next(Scanner *scanner)
{
    return *scanner->current++;
}

static char peek(Scanner *scanner)
{
    return *scanner->current;
}

static char peekNext(Scanner *scanner)
{
    return *(scanner->current + 1);
}

static char lookback(Scanner *scanner)
{
    return *(scanner->current - 1);
}

static bool atEnd(Scanner *scanner)
{
    return peek(scanner) == '\0';
}

bool match(Scanner *scanner, char expected)
{
    if (atEnd(scanner))
        return false;

    if (peek(scanner) != expected)
        return false;

    next(scanner);
    return true;
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool isAlphabet(char c)
{
    return c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z';
}

void skipWhitespace(Scanner *scanner)
{
    while (true)
    {
        switch (peek(scanner))
        {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            next(scanner);
            scanner->start = scanner->current;
            break;
        case '/':
            if (peekNext(scanner) == '/')
            {
                while (peek(scanner) != '\n' && !atEnd(scanner))
                    next(scanner);
                break;
            }
            else
                return;
            break;
        default:
            return;
        }
    }
};

/*
    params:
        * start: the number of already matchd characters.
        * rest: the rest of the characters that should be matched
*/
static bool checkKeyword(Scanner *scanner, int start, char rest[])
{
    int i = 0;

    while (rest[i] != '\0')
    {
        if (*(scanner->start + start + i) == rest[i++])
            continue;
        else
            return false;
    }

    return (scanner->start + start + i - scanner->current == 0) ? true : false;
}

static TokenType identifierType(Scanner *scanner)
{
    switch (*scanner->start)
    {
    case 'c':
        if (checkKeyword(scanner, 1, "lass"))
            return TOKEN_CLASS;
        else if (checkKeyword(scanner, 1, "ontinue"))
            return TOKEN_CONTINUE;
        break;
    case 'e':
        if (checkKeyword(scanner, 1, "lse"))
            return TOKEN_ELSE;
        if (checkKeyword(scanner, 1, "xtends"))
            return TOKEN_EXTENDS;
        break;
    case 'f':
        switch (*(scanner->start + 1))
        {
        case 'a':
            if (checkKeyword(scanner, 2, "lse"))
                return TOKEN_FALSE;
            break;
        case 'o':
            if (checkKeyword(scanner, 2, "r"))
                return TOKEN_FOR;
            break;
        case 'u':
            if (checkKeyword(scanner, 2, "n"))
                return TOKEN_FUN;
            break;
        default:
            return TOKEN_IDENTIFIER;
        }
    case 'i':
        if (checkKeyword(scanner, 1, "f"))
            return TOKEN_IF;
        break;
    case 'n':
        if (checkKeyword(scanner, 1, "il"))
            return TOKEN_NIL;
        break;
    case 'r':
        if (checkKeyword(scanner, 1, "eturn"))
            return TOKEN_RETURN;
        break;
    case 's':
        if (checkKeyword(scanner, 1, "uper"))
            return TOKEN_SUPER;
        break;
    case 't':
        switch (*(scanner->start + 1))
        {
        case 'h':
            if (checkKeyword(scanner, 2, "is"))
                return TOKEN_THIS;
            break;
        case 'r':
            if (checkKeyword(scanner, 2, "ue"))
                return TOKEN_TRUE;
            break;
        default:
            return TOKEN_IDENTIFIER;
        }
    case 'v':
        if (checkKeyword(scanner, 1, "ar"))
            return TOKEN_VAR;
        break;
    case 'w':
        if (checkKeyword(scanner, 1, "hile"))
            return TOKEN_WHILE;
        break;
    default:;
    }

    return TOKEN_IDENTIFIER;
}

Token scanToken(Scanner *scanner)
{
    skipWhitespace(scanner);

    scanner->start = scanner->current;

    if (atEnd(scanner))
    {
        next(scanner);
        return popToken(scanner, TOKEN_EOF);
    }

    char c = next(scanner);

    if (isDigit(c))
    {
        while (isDigit(peek(scanner)))
            next(scanner);

        if (peek(scanner) == '.' && isDigit(peekNext(scanner)))
        {
            next(scanner);
            while (isDigit(peek(scanner)))
                next(scanner);
            return popToken(scanner, TOKEN_NUMBER);
        }
        else
            return popToken(scanner, TOKEN_NUMBER);
    }

    if (isAlphabet(c) || c == '_')
    {
        char peeked_c;

        while (isAlphabet(peeked_c = peek(scanner)) || isDigit(peeked_c) || peeked_c == '_')
            next(scanner);

        return popToken(scanner, identifierType(scanner));
    }

    switch (c)
    {
    case '(':
        return popToken(scanner, TOKEN_LEFT_PAREN);
    case ')':
        return popToken(scanner, TOKEN_RIGHT_PAREN);
    case '{':
        return popToken(scanner, TOKEN_LEFT_BRACE);
    case '}':
        if (scanner->stringDepth)
        {
            while (true)
            {
                if (atEnd(scanner))
                    return errorToken(scanner, "Template didn't get terminated");

                if (peek(scanner) == '"')
                {
                    next(scanner);
                    scanner->stringDepth--;
                    return popToken(scanner, TOKEN_TEMPLATE_TAIL);
                }

                if (peek(scanner) == '$' && peekNext(scanner) == '{')
                {
                    next(scanner);
                    next(scanner);
                    return popToken(scanner, TOKEN_TEMPLATE_MIDDLE);
                }

                next(scanner);
            };
        }
        else
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
    case '?':
        return popToken(scanner, TOKEN_QUESTION_MARK);
    case ':':
        return popToken(scanner, TOKEN_COLON);
    case '!':
        return popToken(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return popToken(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '>':
        return popToken(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<':
        return popToken(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '"':
        while (true)
        {
            if (atEnd(scanner))
                return errorToken(scanner, "Unterminated string");

            char c = peek(scanner);

            if (c == '"')
            {
                next(scanner);
                return popToken(scanner, TOKEN_STRING);
            }

            if (c == '$' && peekNext(scanner) == '{')
            {
                next(scanner);
                next(scanner);
                scanner->stringDepth++;
                return popToken(scanner, TOKEN_TEMPLATE_HEAD);
            }

            next(scanner);
        }
    case '&':
    case '|':
        if (match(scanner, c))
            return popToken(scanner, c == '&' ? TOKEN_AND : TOKEN_OR);
    default:
        return errorToken(scanner, "Unexpected character");
    }
}

Token virtualToken(TokenType type, char *content)
{
    Token token;
    token.type = type;
    token.start = content;
    token.length = strlen(content);
    return token;
}

void resetScanner(Scanner *scanner)
{
    scanner->source = NULL;
    scanner->current = NULL;
    scanner->start = NULL;
    scanner->stringDepth = 0;
}