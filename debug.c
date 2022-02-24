#include "debug.h"

char* opCodeToString(OpCode opCode) {
    switch (opCode) {
        case OP_RETURN: return "RETURN";
        case OP_CONSTANT: return "CONSTANT";
        case OP_NEGATE: return "NEGATE";
        case OP_ADD: return "ADD";
        case OP_SUBTRACT: return "SUBTRACT";
        case OP_MULTIPLY: return "MULTIPLY";
        case OP_DIVIDE: return "DIVIDE";
        case OP_OR: return "OR";
        case OP_AND: return "AND";
        case OP_EQUAL: return "EQUAL";
        case OP_NOT_EQUAL: return "NOT_EQUAL";
        case OP_GREATER: return "GREATER";
        case OP_GREATER_OR_EQUAL: return "GREATER_OR_EQUAL";
        case OP_LESS: return "LESS";
        case OP_LESS_OR_EQUAL: return "LESS_OR_EQUAL";
        case OP_BANG: return "BANG";
        case OP_GET_GLOBAL: return "GET_GLOBAL";
        case OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
        case OP_ASSIGN_GLOBAL: return "ASSIGN_GLOBAL";
        case OP_NIL: return "NIL";
        case OP_POP: return "POP";
    }
}

void disassembleChunk(Chunk *chunk)
{
    int i = 0;

    while (i < chunk->count)
    {
        uint8_t byte = chunk->code[i];
        int pos[2];

        getTokenPos(pos, &chunk->tokenArr.tokens[i]);

        printf("%04d | ", i);

        printf("%5d:%-5d ", pos[0], pos[1]);

        if (byte == OP_CONSTANT || byte == OP_DEFINE_GLOBAL || byte == OP_GET_GLOBAL || byte == OP_ASSIGN_GLOBAL) {
            //TODO print the constant
            printf("%s", opCodeToString(byte));

            if (byte != OP_CONSTANT) {
                printf(" (%s)", AS_STRING((&chunk->constants.values[chunk->code[i + 1]]))->chars);
            }
            
            putchar('\n');

            i += 2;
        } else {
            printf("%s\n", opCodeToString(byte));
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
        case TOKEN_ERROR:
            return "ERROR";
            break;
        case TOKEN_EOF:
            return "EOF";
            break;
    }
}

void printToken(Token *token)
{
    int pos[2];

    getTokenPos(pos, token);

    printf("%5d:%-5d ", pos[0], pos[1]);

    printf("%-20s", tokenTypeToString(token->type));

    printf(" ");
    printf("('%.*s')", token->length, token->start);

    if (token->type == TOKEN_ERROR) {
        printf(" %s\n", token->errorMsg);
    } else {
        putchar('\n');
    }
}