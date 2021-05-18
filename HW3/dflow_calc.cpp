#include <iostream>
#include "dflow_calc.h"

using std::cout;
#ifdef DEBUG
#define DO_IF_DEBUG(command)            \
    do                                  \
    { /* safely invoke a system call */ \
        command                         \
    } while (0)
#else
#define DO_IF_DEBUG(command)      \
    do                            \
    {                             \
        /* empty intentionally */ \
    } while (0)
#endif

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    DO_IF_DEBUG(cout << "Hello" << std::endl;);
    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx)
{
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    return -1;
}

int getProgDepth(ProgCtx ctx)
{
    return 0;
}
