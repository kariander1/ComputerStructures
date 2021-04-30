/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdio.h>
#include <math.h>

enum ReturnStatus
{
    SUCCESS = 0,
    FAILURE = -1
};
class BimodalFSM_p
{
    enum BimodalState
    {
        BIMODAL_MIN = -1,
        SNT = 0,
        WNT,
        WT,
        ST,
        BIMODAL_MAX
    };
    BimodalState default_state;
    BimodalState state;

public:
    BimodalFSM_p(unsigned fsmState) : state(static_cast<BimodalState>(fsmState)), default_state(static_cast<BimodalState>(fsmState)) {}
    void update(const bool &taken)
    {
        state = static_cast<BimodalState>(int(state) + (taken ? 1 : -1));
        state = state >= BIMODAL_MAX ? ST : state;
        state = state <= BIMODAL_MIN ? SNT : state;
    }
    void reset()
    {
        state = default_state;
    }
};
typedef struct BimodalFSM_p *BimodalFSM;

class BHR
{
    unsigned int history_record;
    unsigned int history_mask;

public:
    BHR(const unsigned &historySize) : history_mask((1 << historySize)-1) {}
    void update(const bool &taken)
    {
        history_record << 1;     // Shift left by one space
        history_record += taken; // add 0/1 according to taken/not taken

        // Cap the value to be less than history_mask
        history_record = history_record & history_mask;
    }
    const unsigned int getHistory()
    {
        return history_record;
    }
};

class BTBentry_p
{
public:
    bool valid;
    unsigned tag;
    unsigned target;
    BHR localBHR;
    BimodalFSM *fsm;
    unsigned int num_of_fsm;
    BTBentry_p(const unsigned &historySize,const unsigned &fsmState) : valid(false), num_of_fsm(1 << historySize), localBHR(historySize)
    {
        fsm = new BimodalFSM[1 << historySize];
        for (size_t i = 0; i < num_of_fsm; i++)
        {
            fsm[i] =new BimodalFSM_p(fsmState);
        }
        
    }
    ~BTBentry_p()
    {
        delete[] fsm;
    }
    /*
    BTBentry() : fsm(new Bimodal[1<<historySize]), valid(false), local_history(0) {
        for(int i = 0; i < 1 << historySize ; i++) {
            Bimodal default_bimodal_val = static_cast<Bimodal>(fsmState);
            if (default_bimodal_val <= MIN || default_bimodal_val >= MAX) {
                throw;
            }
            fsm[i] = default_bimodal_val;
        }


    }
    BTBentry(const BTBentry & BTBentry) = delete;
    BTBentry & operator=(const BTBentry & BTBentry) = delete;
    ~BTBentry() = default;
    */
};
typedef struct BTBentry_p *BTBentry;
class BTB
{

private:
    unsigned btbSize;
    unsigned tagSize;
    unsigned index_mask;
    unsigned fsmState;
    BHR publicBHR;
    BimodalFSM_p sharedTable;
    bool isGlobalHist;
    bool isGlobalTable;
    BTBentry *entires;

public:
    BTB(const unsigned &btbSize,
        const unsigned &historySize,
        const unsigned &tagSize,
        const unsigned &fsm_state,
        const bool &isGlobalHist,
        const bool &isGlobalTable)
        : btbSize(btbSize),
          tagSize(tagSize),
          index_mask((1<<(32-2-tagSize))-1),
          fsmState(fsm_state),
          publicBHR(historySize),
          sharedTable(BimodalFSM_p(fsm_state)),
          isGlobalHist(isGlobalHist),
          isGlobalTable(isGlobalTable)
    {
        entires = new BTBentry[btbSize]; // Allocate array of entries
        for (size_t i = 0; i < btbSize; i++)
        {
            // Allocate each entry
            entires[i] = new BTBentry_p(historySize,fsm_state);
        }
    }
    ~BTB()
    {
        delete[] entires;
    }
    void update(const uint32_t &pc,const uint32_t &targetPc, const bool &taken, uint32_t pred_dst)
    {
        // Update global & shared:
        const bool correct_pred = (targetPc == pred_dst);
        this->publicBHR.update(correct_pred);
        this->sharedTable.update(correct_pred);

        const unsigned index = ((pc >> 2) & index_mask) % btbSize;
        BTBentry entry = this->entires[index];
        const unsigned int tag_mask =((1 << tagSize) - 1);
        const unsigned int current_tag = (pc >> (2 + (int)log2(btbSize))) & tag_mask;
        if (!entry->valid || entry->tag != current_tag)
        {
            entry->tag = current_tag;
            entry->target = targetPc;
            entry->localBHR = 0;
            for (int i = 0; i < entry->num_of_fsm; i++)
            {
                entry->fsm[i]->reset();
            }            
        }
        // Now entry is the relevant branch
      
        entry->localBHR.update(correct_pred);
        entry->fsm[entry->localBHR.getHistory()]->update(correct_pred);
        entry->valid = true; // Always set this
    }
};

static BTB *BP_BTB;

/*
class BP {
public:
    static unsigned btbSize;
    static unsigned historySize;
    static unsigned tagSize;
    static unsigned fsmState;
    static bool isGlobalHist;
    static bool isGlobalTable;
    static int Shared;
    static BTBentry * BTB;
};
*/
static unsigned btbSize;
static unsigned historySize;
static unsigned tagSize;
static unsigned fsmState;
static bool isGlobalHist;
static bool isGlobalTable;
static int Shared;
//static BTBentry *BTB;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared)
{
    BP_BTB = new BTB(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable);
    /*
    btbSize = btbSize;
    historySize = historySize;
    tagSize = tagSize;
    fsmState = fsmState;
    isGlobalHist = isGlobalHist;
    isGlobalTable = isGlobalTable;
    Shared = Shared;
    */
    try
    {
        //BTBentry::historySize = historySize;
        //BTBentry::fsmState = fsmState;
        //BP::BTB = new BTBentry[btbSize];
    }
    catch (...)
    {
    }

    return SUCCESS;
}
/** Lior V=dvIR*/
bool BP_predict(uint32_t pc, uint32_t *dst)
{
    return false;
}

/** Shai Haim Yehezkel */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
    BP_BTB->update(pc, targetPc, taken,pred_dst);
}

void BP_GetStats(SIM_stats *curStats)
{
    return;
}
