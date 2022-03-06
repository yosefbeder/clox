#include "debug.h"
#include "object.h"

char* opCodeToString(OpCode opCode) {
    switch (opCode) {
        case OP_RETURN: return "RETURN";
        case OP_CONSTANT: return "CONSTANT";
        case OP_NEGATE: return "NEGATE";
        case OP_ADD: return "ADD";
        case OP_SUBTRACT: return "SUBTRACT";
        case OP_MULTIPLY: return "MULTIPLY";
        case OP_DIVIDE: return "DIVIDE";
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
        case OP_GET_LOCAL: return "GET_LOCAL";
        case OP_DEFINE_LOCAL: return "DEFINE_LOCAL";
        case OP_ASSIGN_LOCAL: return "ASSIGN_LOCAL";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_BACKWARDS: return "JUMP_BACKWARDS";
        case OP_NIL: return "NIL";
        case OP_POP: return "POP";
        case OP_CALL: return "CALL";
        default:;
    }
}

int noOperands(Chunk* chunk, int offset) {
    printf("%s\n", opCodeToString(chunk->code[offset]));
    return offset + 1;
}

int stringOperand(Chunk* chunk, int offset) {
    printf("%s (%s)\n", opCodeToString(chunk->code[offset]), AS_STRING((&chunk->constants.values[chunk->code[offset + 1]]))->chars);

    return offset + 2;
}

int u8Operand(Chunk* chunk, int offset) {
    printf("%s (%d)\n", opCodeToString(chunk->code[offset]), chunk->code[offset + 1]);

    return offset + 2;
}

void disassembleChunk(Chunk *chunk, char* name)
{
    printf("=== %s ===\n", name);

    for (int offset = 0; offset < chunk->count;) {
        OpCode opCode = chunk->code[offset];
        Token token = chunk->tokenArr.tokens[offset];

        int pos[2];
        getTokenPos(pos, &token);

        printf("%04d | %5d:%-5d ", offset, pos[0], pos[1]);

        switch (opCode) {
            case OP_RETURN:
            case OP_NEGATE:
            case OP_ADD:
            case OP_SUBTRACT:
            case OP_MULTIPLY:
            case OP_DIVIDE:
            case OP_EQUAL:
            case OP_NOT_EQUAL:
            case OP_GREATER:
            case OP_GREATER_OR_EQUAL:
            case OP_LESS:
            case OP_LESS_OR_EQUAL:
            case OP_BANG:
            case OP_POP:
            case OP_NIL:
            case OP_DEFINE_LOCAL:
                offset = noOperands(chunk, offset);
                break;
            case OP_GET_GLOBAL:
            case OP_DEFINE_GLOBAL:
            case OP_ASSIGN_GLOBAL:
                offset = stringOperand(chunk, offset);
                break;
            case OP_GET_LOCAL:
            case OP_ASSIGN_LOCAL:
            case OP_CONSTANT:
            case OP_JUMP_IF_FALSE:
            case OP_JUMP_IF_TRUE:
            case OP_JUMP:
            case OP_JUMP_BACKWARDS:
            case OP_CALL:
                offset = u8Operand(chunk, offset);
                break;
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