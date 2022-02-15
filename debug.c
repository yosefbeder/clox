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

char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_LEFT_PAREN:
            return "LEFT_PAREN";
            break;
        case TOKEN_RIGHT_PAREN:
            return "RIGHT_PAREN";
            break;
        case TOKEN_LEFT_BRACE:
            return "LEFT_BRACE";
            break;
        case TOKEN_RIGHT_BRACE:
            return "RIGHT_BRACE";
            break;
        case TOKEN_COMMA:
            return "COMMA";
            break;
        case TOKEN_DOT:
            return "DOT";
            break;
        case TOKEN_MINUS:
            return "MINUS";
            break;
        case TOKEN_PLUS:
            return "PLUS";
            break;
        case TOKEN_SEMICOLON:
            return "SEMICOLON";
            break;
        case TOKEN_SLASH:
            return "SLASH";
            break;
        case TOKEN_STAR:
            return "STAR";
            break;
            // One or two character tokens.
        case TOKEN_BANG:
            return "BANG";
            break;
        case TOKEN_BANG_EQUAL:
            return "BANG_EQUAL";
            break;
        case TOKEN_EQUAL:
            return "EQUAL";
            break;
        case TOKEN_EQUAL_EQUAL:
            return "EQUAL_EQUAL";
            break;
        case TOKEN_GREATER:
            return "GREATER";
            break;
        case TOKEN_GREATER_EQUAL:
            return "GREATER_EQUAL";
            break;
        case TOKEN_LESS:
            return "LESS";
            break;
        case TOKEN_LESS_EQUAL:
            return "LESS_EQUAL";
            break;
            // Literals.
        case TOKEN_IDENTIFIER:
            return "IDENTIFIER";
            break;
        case TOKEN_STRING:
            return "STRING";
            break;
        case TOKEN_NUMBER:
            return "NUMBER";
            break;
        case TOKEN_TEMPLATE_HEAD:
            return "TEMPLATE_HEAD";
            break;
        case TOKEN_TEMPLATE_MIDDLE:
            return "TEMPLATE_MIDDLE";
            break;
        case TOKEN_TEMPLATE_TAIL:
            return "TEMPLATE_TAIL";
            break;
            // Keywords.
        case TOKEN_AND:
            return "AND";
            break;
        case TOKEN_CLASS:
            return "CLASS";
            break;
        case TOKEN_ELSE:
            return "ELSE";
            break;
        case TOKEN_FALSE:
            return "FALSE";
            break;
        case TOKEN_FOR:
            return "FOR";
            break;
        case TOKEN_FUN:
            return "FUN";
            break;
        case TOKEN_IF:
            return "IF";
            break;
        case TOKEN_NIL:
            return "NIL";
            break;
        case TOKEN_OR:
            return "OR";
            break;
        case TOKEN_RETURN:
            return "RETURN";
            break;
        case TOKEN_SUPER:
            return "SUPER";
            break;
        case TOKEN_THIS:
            return "THIS";
            break;
        case TOKEN_TRUE:
            return "TRUE";
            break;
        case TOKEN_VAR:
            return "VAR";
            break;
        case TOKEN_WHILE:
            return "WHILE";
            break;
        case TOKEN_EOF:
            return "EOF";
            break;
    }
}

void printToken(Scanner* scanner, Token *token)
{
    int pos[2];

    getTokenPos(scanner, token, pos);

    printf("%5d:%-5d ", pos[0], pos[1]);

    printf("%-20s", tokenTypeToString(token->type));

    printf(" ");
    printf("('%.*s')\n", token->length, token->start);
}