#include "common.h"
#include "debug.h"
#include "chunk.h"

int main()
{
    Chunk chunk;

    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);
    diassembleChunk(&chunk);

    return 0;
}