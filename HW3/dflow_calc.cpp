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

const int none = -1; 
struct node
{
    unsigned int opcode;
    int dep1;
    int dep2;
    int max_depth;
    unsigned int latency;
    node() : dep1(none),dep2(none),max_depth(none) {};
};

class DataFlow
{
    

public:
    node *dependencies;
    unsigned int *regs;
    bool *exit_nodes;
    unsigned int num_of_insts;
    unsigned int max_depth;
    

    DataFlow(const unsigned int numOfInsts) :   dependencies(new node[numOfInsts]),
                                                regs(new unsigned int[numOfInsts]),
                                                exit_nodes(new bool[numOfInsts]),
                                                num_of_insts(numOfInsts)
    {
        for (unsigned int i = 0; i < numOfInsts; i++)
        {
            regs[i] = none;
            exit_nodes[i] = true;
        }
    }
    ~DataFlow()
    {
        delete[] dependencies;
        delete[] regs;
        delete[] exit_nodes;
    }    
    void InitData(const unsigned int opsLatency[], const InstInfo progTrace[])
    {
        
        for (unsigned int i = 0; i < num_of_insts; i++)
        {
            // Get instruction #i
            InstInfo current_inst = progTrace[i];

            // Update opcode
            dependencies[i].opcode = current_inst.opcode;
            dependencies[i].latency = opsLatency[current_inst.opcode];
            // Get dependencies index from regs array
            int dependency1 = regs[current_inst.src1Idx];
            int dependency2 = regs[current_inst.src2Idx];

            // Update dependencies (can be none)
            dependencies[i].dep1 = dependency1;
            dependencies[i].dep2 = dependency2;

            // Update exit node (by its exit array)
            exit_nodes[dependency1] = dependency1 == none;
            exit_nodes[dependency2] = dependency2 == none;

            // Update regs array depedency
            regs[current_inst.dstIdx] = i;
        }

        // Update max depth for each node (starting from exit nodes)
        for (unsigned int i = 0; i < num_of_insts; i++)
        {
            if(exit_nodes[i])
            {
                unsigned int max_temp = getDepthRecursive(i);
                max_depth = max_depth > max_temp ? max_depth : max_temp;
            }
        }
        
    }
    int getDepthRecursive(const int node_index)
    {
        if(node_index == none)
        {
            return 0;
        }
        node current_node = dependencies[node_index];
        if(current_node.max_depth !=none)
        {
            // We've calculated its max_depth
            return current_node.max_depth;
        }
        
        int max_depth = current_node.latency;

        // If there are no dependecies, then the max will be zero
        int max1 = getDepthRecursive(current_node.dep1);
        int max2 = getDepthRecursive(current_node.dep2);

        current_node.max_depth = max_depth + (max1>max2 ? max1 : max2);
        return current_node.max_depth;
    }

};
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    //DO_IF_DEBUG(cout<< sizeof(int *););

    ProgCtx data_flow = new DataFlow(numOfInsts);

    
    return data_flow;
}

void freeProgCtx(ProgCtx ctx)
{
    delete (DataFlow *)ctx;   
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    node inst_node = ((DataFlow *)ctx)->dependencies[theInst];
    return inst_node.max_depth - inst_node.latency;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    node inst_node = ((DataFlow *)ctx)->dependencies[theInst];
    *src1DepInst = inst_node.dep1;
    *src2DepInst = inst_node.dep2;
    return 0;
}

int getProgDepth(ProgCtx ctx)
{
    
    return ((DataFlow *)ctx)->max_depth;
}

