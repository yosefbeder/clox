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
            printf("LEFT_PAREN");
            break;
        case TOKEN_RIGHT_PAREN:
            printf("RIGHT_PAREN");
            break;
        case TOKEN_LEFT_BRACE:
            printf("LEFT_BRACE");
            break;
        case TOKEN_RIGHT_BRACE:
            printf("RIGHT_BRACE");
            break;
        case TOKEN_COMMA:
            printf("COMMA");
            break;
        case TOKEN_DOT:
            printf("DOT");
            break;
        case TOKEN_MINUS:
            printf("MINUS");
            break;
        case TOKEN_PLUS:
            printf("PLUS");
            break;
        case TOKEN_SEMICOLON:
            printf("SEMICOLON");
            break;
        case TOKEN_SLASH:
            printf("SLASH");
            break;
        case TOKEN_STAR:
            printf("STAR");
            break;
            // One or two character tokens.
        case TOKEN_BANG:
            printf("BANG");
            break;
        case TOKEN_BANG_EQUAL:
            printf("BANG_EQUAL");
            break;
        case TOKEN_EQUAL:
            printf("EQUAL");
            break;
        case TOKEN_EQUAL_EQUAL:
            printf("EQUAL_EQUAL");
            break;
        case TOKEN_GREATER:
            printf("GREATER");
            break;
        case TOKEN_GREATER_EQUAL:
            printf("GREATER_EQUAL");
            break;
        case TOKEN_LESS:
            printf("LESS");
            break;
        case TOKEN_LESS_EQUAL:
            printf("LESS_EQUAL");
            break;
            // Literals.
        case TOKEN_IDENTIFIER:
            printf("IDENTIFIER");
            break;
        case TOKEN_STRING:
            printf("STRING");
            break;
        case TOKEN_NUMBER:
            printf("NUMBER");
            break;
            // Keywords.
        case TOKEN_AND:
            printf("AND");
            break;
        case TOKEN_CLASS:
            printf("CLASS");
            break;
        case TOKEN_ELSE:
            printf("ELSE");
            break;
        case TOKEN_FALSE:
            printf("FALSE");
            break;
        case TOKEN_FOR:
            printf("FOR");
            break;
        case TOKEN_FUN:
            printf("FUN");
            break;
        case TOKEN_IF:
            printf("IF");
            break;
        case TOKEN_NIL:
            printf("NIL");
            break;
        case TOKEN_OR:
            printf("OR");
            break;
        case TOKEN_RETURN:
            printf("RETURN");
            break;
        case TOKEN_SUPER:
            printf("SUPER");
            break;
        case TOKEN_THIS:
            printf("THIS");
            break;
        case TOKEN_TRUE:
            printf("TRUE");
            break;
        case TOKEN_VAR:
            printf("VAR");
            break;
        case TOKEN_WHILE:
            printf("WHILE");
            break;
        case TOKEN_EOF:
            printf("EOF");
            break;
    }

    printf(" ");
    printf("('%.*s')\n", token->length, token->start);
}