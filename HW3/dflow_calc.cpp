#include <iostream>
#include "dflow_calc.h"

using std::cout;

// Macros used for DEBUG (if compiled with DDEBUG flag)
#ifdef DEBUG
#define DO_IF_DEBUG(command) \
    do                       \
    {                        \
        command              \
    } while (0)
#else
#define DO_IF_DEBUG(command)      \
    do                            \
    {                             \
        /* empty intentionally */ \
    } while (0)
#endif

const short NUM_OF_REGS = 32;   // Number of regs in HW
const int NONE = -1;            // Const representing an error or a non-existent value

// Struct representing a flow chart node of an instruction
struct node
{
    unsigned int opcode;    // Opcode of command
    int dep1;               // Inst. # of 1st dependency
    int dep2;               // Inst. # of 2nd dependency
    int max_depth;          // The max depth of the node from Entry node (calculated in init)
    unsigned int latency;   // Latency of inst.
    node() : dep1(NONE), dep2(NONE), max_depth(NONE){}; // C'tor with values
};

// Class DataFlow - Representing the entire flow chart of dependencies
class DataFlow
{

public:
    node *dependencies;             // Array of dependecy nodes
    unsigned int regs[NUM_OF_REGS]; // Array of regs
    bool *exit_nodes;               // Boolean array to represent which instructions are connected to exit
    unsigned int num_of_insts;      // Number of instructions in program
    int max_depth;                  // Max depth of the entire program (entry->exit, calculated in init)

    // C'tor with inits in initializer list
    DataFlow(const unsigned int numOfInsts, const unsigned int opsLatency[], const InstInfo progTrace[])
        : dependencies(new node[numOfInsts]), exit_nodes(new bool[numOfInsts]), num_of_insts(numOfInsts),
          max_depth(NONE)
    {
        // Init regs array to NONE
        for (unsigned int i = 0; i < NUM_OF_REGS; i++)
        {
            regs[i] = NONE;
        }
        // Init exit nodes so that every inst. node is connected to exit
        for (unsigned int i = 0; i < num_of_insts; i++)
        {
            exit_nodes[i] = true;
        }
        // Perform actual initialization of program
        InitData(opsLatency, progTrace);
    }
    // D'tor to release resources
    ~DataFlow()
    {
        delete[] dependencies;
        delete[] exit_nodes;
    }
    // Init data function to perform the actual nodes and depths calculations
    void InitData(const unsigned int opsLatency[], const InstInfo progTrace[])
    {
        // Foreach instruction given:
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
            if (dependency1 != NONE)
            {
                DO_IF_DEBUG(cout << "Node N." << dependency1 << " is not from exit" << std::endl;);
                exit_nodes[dependency1] = false;
            }
            if (dependency2 != NONE)
            {
                DO_IF_DEBUG(cout << "Node N." << dependency2 << " is not from exit" << std::endl;);
                exit_nodes[dependency2] = false;
            }

            // Update regs array dependency
            regs[current_inst.dstIdx] = i;
        }
        // For debug purposes (won't be compiled):
        DO_IF_DEBUG(
            cout << "Exit nodes are: ";
            for (unsigned int i = 0; i < num_of_insts; i++)
            {
                if (exit_nodes[i])
                {
                    cout << i << ' ';
                }
            } cout
            << std::endl;

        );
        // Update max depth for each node (starting from exit nodes)
        for (unsigned int i = 0; i < num_of_insts; i++)
        {
            // Start updating max_depth from the exit nodes
            // Foreach node connected to exit represented with "true" in exit array
            if (exit_nodes[i])
            {
                // Call the recursive function
                int max_temp = getDepthRecursive(i);
                // update max depth of program if it is larger
                max_depth = max_depth > max_temp ? max_depth : max_temp;
            }
        }
    }
    // getDepthRecursive - perform a recursive update of the max depth foreach node in path
    int getDepthRecursive(const int node_index)
    {
        // Stop condition - reaching a NONE node
        if (node_index == NONE)
        {
            return 0;
        }
        // Fetch current node struct
        node &current_node = dependencies[node_index];

        // If max_depth is not NONE, then we have calculated max_depth already
        if (current_node.max_depth != NONE)
        {
            return current_node.max_depth;
        }
        // Init the node max_depth to be the latency of the current inst.
        int max_depth = current_node.latency;

        // If there are no dependecies, then the max will be zero
        int max1 = getDepthRecursive(current_node.dep1);
        int max2 = getDepthRecursive(current_node.dep2);

        // Determine max_depth according to max_depths of dependecy nodes
        current_node.max_depth = max_depth + (max1 > max2 ? max1 : max2);
        return current_node.max_depth;
    }
};
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    // Init data_flow to NULL
    ProgCtx data_flow = PROG_CTX_NULL;
    try
    {
        // Try using the c'tor of new data flow type
        data_flow = new DataFlow(numOfInsts, opsLatency, progTrace);
    }
    catch (const std::exception &e)
    {
        // If there's an error, we shall return NULL
        data_flow = PROG_CTX_NULL;
    }
    return data_flow;
}

void freeProgCtx(ProgCtx ctx)
{
    // Call d'tor
    delete (DataFlow *)ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    // Fetch data_flow instance
    DataFlow *data_flow = (DataFlow *)ctx;

    // If the "theInst" is out of range, return an error (-1)
    if (theInst >= data_flow->num_of_insts)
    {
        return NONE;
    }
    // Fetch the inst node
    node inst_node = data_flow->dependencies[theInst];

    // If there a NONE value, then the depth is zero, otherwise return the saved value
    return inst_node.max_depth == NONE ? 0 : inst_node.max_depth - inst_node.latency;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    // Fetch data_flow instance
    DataFlow *data_flow = (DataFlow *)ctx;

    // If the "theInst" is out of range, return an error (-1)    
    if (theInst >= data_flow->num_of_insts)
    {
        return NONE;
    }
    // Fetch the inst node
    node inst_node = data_flow->dependencies[theInst];

    // Update the given pointers to the dependencies (they can be NONE)
    *src1DepInst = inst_node.dep1;
    *src2DepInst = inst_node.dep2;
    return 0;
}

int getProgDepth(ProgCtx ctx)
{
    // Return the max depth of the program
    return ((DataFlow *)ctx)->max_depth;
}
