#include "debug.h"

void disassembleChunk(Chunk *chunk)
{
    int i = 0;

    while (i < chunk->count - 1)
    {
        uint8_t byte = chunk->code[i];
        int line = getLine(&chunk->lineArr, i);
        int prevLine = getLine(&chunk->lineArr, i - 1);

        printf("%04d | ", i);

        if (i == 0 || prevLine != line)
        {
            printf("%04d | ", line);
        }
        else
        {
            printf("//// | ");
        }

        if (byte == OP_RETURN)
        {
            printf("OP_RETURN\n");
            i++;
        }

        if (byte == OP_CONSTANT)
        {
            int constant_i = chunk->code[i + 1];

            printf("OP_CONSTANT %d (%lf)\n", constant_i, chunk->constants.values[constant_i]);
            i += 2;
        }

        if (byte == OP_NEGATE)
        {
            printf("OP_NEGATE\n");
            i++;
        }
        if (byte == OP_ADD)
        {
            printf("OP_ADD\n");
            i++;
        }
        if (byte == OP_SUBTRACT)
        {
            printf("OP_SUBTRACT\n");
            i++;
        }
        if (byte == OP_MULTIPLY)
        {
            printf("OP_MULTIPLY\n");
            i++;
        }
        if (byte == OP_DIVIDE)
        {
            printf("OP_DIVIDE\n");
            i++;
        }
    }
}

void printToken(Token *token, int prevLine)
{
    if (prevLine != token->line)
    {
        printf("%04d | ", token->line);
    }
    else
    {
        printf("//// | ");
    }

    if (token->type == TOKEN_ERROR) {
        printf("Error: %.*s\n", token->length, token->start);
        return;
    }

    switch (token->type)
    {
        // Single-character tokens.
        case TOKEN_LEFT_PAREN:
            printf("TOKEN_LEFT_PAREN");
            break;
        case TOKEN_RIGHT_PAREN:
            printf("TOKEN_RIGHT_PAREN");
            break;
        case TOKEN_LEFT_BRACE:
            printf("TOKEN_LEFT_BRACE");
            break;
        case TOKEN_RIGHT_BRACE:
            printf("TOKEN_RIGHT_BRACE");
            break;
        case TOKEN_COMMA:
            printf("TOKEN_COMMA");
            break;
        case TOKEN_DOT:
            printf("TOKEN_DOT");
            break;
        case TOKEN_MINUS:
            printf("TOKEN_MINUS");
            break;
        case TOKEN_PLUS:
            printf("TOKEN_PLUS");
            break;
        case TOKEN_SEMICOLON:
            printf("TOKEN_SEMICOLON");
            break;
        case TOKEN_SLASH:
            printf("TOKEN_SLASH");
            break;
        case TOKEN_STAR:
            printf("TOKEN_STAR");
            break;
            // One or two character tokens.
        case TOKEN_BANG:
            printf("TOKEN_BANG");
            break;
        case TOKEN_BANG_EQUAL:
            printf("TOKEN_BANG_EQUAL");
            break;
        case TOKEN_EQUAL:
            printf("TOKEN_EQUAL");
            break;
        case TOKEN_EQUAL_EQUAL:
            printf("TOKEN_EQUAL_EQUAL");
            break;
        case TOKEN_GREATER:
            printf("TOKEN_GREATER");
            break;
        case TOKEN_GREATER_EQUAL:
            printf("TOKEN_GREATER_EQUAL");
            break;
        case TOKEN_LESS:
            printf("TOKEN_LESS");
            break;
        case TOKEN_LESS_EQUAL:
            printf("TOKEN_LESS_EQUAL");
            break;
            // Literals.
        case TOKEN_IDENTIFIER:
            printf("TOKEN_IDENTIFIER");
            break;
        case TOKEN_STRING:
            printf("TOKEN_STRING");
            break;
        case TOKEN_NUMBER:
            printf("TOKEN_NUMBER");
            break;
            // Keywords.
        case TOKEN_AND:
            printf("TOKEN_AND");
            break;
        case TOKEN_CLASS:
            printf("TOKEN_CLASS");
            break;
        case TOKEN_ELSE:
            printf("TOKEN_ELSE");
            break;
        case TOKEN_FALSE:
            printf("TOKEN_FALSE");
            break;
        case TOKEN_FOR:
            printf("TOKEN_FOR");
            break;
        case TOKEN_FUN:
            printf("TOKEN_FUN");
            break;
        case TOKEN_IF:
            printf("TOKEN_IF");
            break;
        case TOKEN_NIL:
            printf("TOKEN_NIL");
            break;
        case TOKEN_OR:
            printf("TOKEN_OR");
            break;
        case TOKEN_PRINT:
            printf("TOKEN_PRINT");
            break;
        case TOKEN_RETURN:
            printf("TOKEN_RETURN");
            break;
        case TOKEN_SUPER:
            printf("TOKEN_SUPER");
            break;
        case TOKEN_THIS:
            printf("TOKEN_THIS");
            break;
        case TOKEN_TRUE:
            printf("TOKEN_TRUE");
            break;
        case TOKEN_VAR:
            printf("TOKEN_VAR");
            break;
        case TOKEN_WHILE:
            printf("TOKEN_WHILE");
            break;
        case TOKEN_EOF:
            printf("TOKEN_EOF");
            break;
    }

    printf(" ");
    printf("('%.*s')\n", token->length, token->start);
}