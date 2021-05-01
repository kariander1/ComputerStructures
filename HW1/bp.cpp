/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <bitset>
#include <stack>

using std::bitset;
using std::ostream;
using std::endl;
using std::cout;
enum ReturnStatus
{
    SUCCESS = 0,
    FAILURE = -1
};

enum BimodalState
{
    BIMODAL_MIN = -1,
    SNT = 0,
    WNT,
    WT,
    ST,
    BIMODAL_MAX
};

enum ShareStatus
{
    NOT_USING_SHARE = 0,
    USING_SHARE_LSB = 1,
    USING_SHARE_MID = 2
};

// Finite state machine for branch prediction.
class BimodalFSM_p
{
    
    BimodalState state;

public:
    BimodalFSM_p(unsigned fsmState) : state(static_cast<BimodalState>(fsmState)) {}
    void update(const bool &taken)
    {
        state = static_cast<BimodalState>(int(state) + (taken ? 1 : -1));
        state = state >= BIMODAL_MAX ? ST : state;
        state = state <= BIMODAL_MIN ? SNT : state;
    }
   void set_state(const unsigned int &state)
   {
       this->state =static_cast<BimodalState>(state);
   }
    BimodalState getState()
    {
        return state;
    }
      friend ostream& operator<<(ostream& os, const BimodalFSM_p& bimodal_fsm)
      {

          switch (bimodal_fsm.state)
          {
          case 0:
              os << "SNT";
              break;
          case 1:
              os << "WNT";
              break;
          case 2:
              os << "WT";
              break;
          case 3:
              os << "ST";
              break;
          default:
              break;
          }
          return os;
      }
};
typedef struct BimodalFSM_p *BimodalFSM;

class FSM_table
{
    private:
        unsigned int default_state;
        unsigned int num_of_fsm;
        BimodalFSM *fsm;

    public:
        FSM_table(const unsigned &historySize, const unsigned &fsmState) 
                            : default_state(fsmState), num_of_fsm(1 << historySize)
        {
            fsm = new BimodalFSM[num_of_fsm];
            reset(true);
        }
        ~FSM_table()
        {
            for (size_t i = 0; i < num_of_fsm; i++)
            {
                delete fsm[i];
            }            
            delete fsm;
        }
        void update(const unsigned int &index,const bool &taken)
        {
            fsm[index]->update(taken);
        }
        const BimodalState getStateByIndex(const unsigned int &index) const
        {
            return fsm[index]->getState();
        }
        void reset(const bool &allocate=false)
        {
            for (unsigned int i = 0; i < num_of_fsm; i++)
            {
                if(allocate)
                {
                    fsm[i] = new BimodalFSM_p(default_state);
                }
                
                fsm[i]->set_state(default_state);
            }
        }
        friend ostream& operator<<(ostream& os, const FSM_table& table)
        {
            os << '{';
            for (unsigned int i = 0; i < table.num_of_fsm; i++)
            {
                os << ' ' << i << ':' << *table.fsm[i] << ' ';
            }
            os << '}';
            return os;
        }
};
// Branch History Register. For history only.
class BHR
{
    unsigned int history_size;
    unsigned int history_record; // History
    unsigned int history_mask;   // Mask for keeping history in correct size

public:
    BHR(const unsigned historySize) : history_size(historySize), history_mask((1 << historySize) - 1) {}
    void update(const bool &taken)
    {
        this->history_record =this->history_record  << 1;     // Shift left by one space
        this->history_record += taken; // add 0/1 according to taken/not taken

        // Cap the value to be less than history_mask
        this->history_record = history_record & history_mask;
    }
    const unsigned int getHistory()
    {
        return history_record;
    }
    void setHistory(const unsigned int history)
    {
        this->history_record=history;
    }
     friend ostream& operator<<(ostream& os, const BHR& bhr)
     {
         unsigned int copy = bhr.history_record;
         std::stack<int> flipped;
         for (size_t i = 0; i < bhr.history_size; i++)
         {
             flipped.push(copy%2);
             
             copy/=2;
         }
         while (!flipped.empty())
         {
             os << flipped.top();
             flipped.pop();
         }
         
         return os;
     }
};

// An entry in the BTB table.
// Encapsulates:
// - Tag of branch
// - Target to jump to in case of taken
// - Valid bit
// - Prediction history
// - prediction FSM machine
class BTBentry_p
{
public:
    bool valid;
    unsigned tag;
    unsigned target;
    BHR local_BHR;
    FSM_table local_table;

    BTBentry_p(const unsigned historySize, const unsigned &fsmState) 
        : valid(false), local_BHR(historySize),local_table(historySize,fsmState) {}
    
        friend ostream& operator<<(ostream& os, const BTBentry_p& entry)
        {
            std::cout << entry.valid << ' '<< std::setfill('0') << std::setw(32) << std::hex <<   entry.tag<< ' ';
            std::cout << std::setfill('0') << std::setw(32) << std::hex <<   entry.target << ' ';
            std::cout << entry.local_BHR << ' ' << entry.local_table;
            return os;
        }
    
};
typedef struct BTBentry_p *BTBentry;

// Full prediction table
class BTB
{

private:
    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned entry_index_mask; // For calculating tag
    unsigned fsmState;
    BHR global_BHR;
    FSM_table global_table;
    bool isGlobalHist;
    bool isGlobalTable;
    BTBentry *entries;
    int Shared;
    int update_counter;
    int flush_counter;

public:
  
    BTB(const unsigned &btbSize,
        const unsigned &historySize,
        const unsigned &tagSize,
        const unsigned &fsm_state,
        const bool &isGlobalHist,
        const bool &isGlobalTable,
        const int &Shared)
        : btbSize(btbSize),
          historySize(historySize),
          tagSize(tagSize),
          entry_index_mask((1 << ((int)log2(btbSize))) - 1),
          fsmState(fsm_state),
          global_BHR(historySize),
          global_table(historySize,fsm_state),
          isGlobalHist(isGlobalHist),
          isGlobalTable(isGlobalTable),
          Shared(Shared),
          update_counter(0),
          flush_counter(0)
    {
        entries = new BTBentry[btbSize]; // Allocate array of entries
        for (size_t i = 0; i < btbSize; i++)
        {
            // Allocate each entry
            entries[i] = new BTBentry_p(historySize, fsm_state);
        }       
      
    }

    ~BTB()
    {
        for (size_t i = 0; i < btbSize; i++)
        {
            delete entries[i];
        }
        
        delete entries;
    }

    /**
     * returns history masked according to type of SHARE
     */
    unsigned int getSharedHistory(unsigned int history, const uint32_t &pc)
    {
        if (Shared == NOT_USING_SHARE)
        {
            // Regular history
            return history;
        }
        // Create mask in the form of 00..011..1 with (history size) amount of 1's
        unsigned int history_mask = (1 << (historySize)) - 1;
        uint32_t masked_pc = pc;
        if (Shared == USING_SHARE_LSB)
        {
            // XOR of history with pc from bit 2
            masked_pc = masked_pc >> 2;
        }
        else
        { // Shared == USING_SHARE_MID
            // XOR of history with pc from bit 16
            masked_pc = masked_pc >> 16;
        }
        // Sample (history size) amount of bits from pc and perform XOR with history
        return (history ^ (masked_pc & history_mask));
    }
    const unsigned getTag(const uint32_t &address)
    {
        return (address >> (2 + (int)log2(btbSize))) & ((1 << tagSize) - 1);
    }
    const unsigned getEntryByIndex(const uint32_t &address)
    {
        return  ((address >> 2) & entry_index_mask);
    }
    void update(const uint32_t &pc, const uint32_t &targetPc, const bool &taken, uint32_t pred_dst)
    {
        
       


        // Update counters
        update_counter++;
        if ((taken && targetPc != pred_dst) || (!taken && pred_dst!=(pc+4)) )
        {
            flush_counter++;
        }
        
       
        // Determine entry according to PC        
        BTBentry entry = this->entries[getEntryByIndex(pc)];
        const unsigned int current_tag = getTag(pc);
        
        // Reset the entry if not valid (=never set before) or it conflicts
        if ((!entry->valid) || (entry->tag != current_tag))
        {            
            entry->tag = current_tag;
            entry->target = targetPc;
            entry->local_BHR.setHistory(0);
            entry->local_table.reset();
            
        }
        // Always set this since after this update the entry will have valid data
        entry->valid = true; 
        if(taken)
        {
            entry->target = targetPc;
        }

        // From here the entry is guaranteed to be valid, whether new or not
        
       

        // Determine which history will be used for updating tables
        BHR hist_for_update = isGlobalHist ? global_BHR : entry->local_BHR;
        
        // For updating global history consider XOR/LSB operations:
        unsigned int masked_history = getSharedHistory(hist_for_update.getHistory(),pc);
        this->global_table.update(masked_history,taken);
        entry->local_table.update(hist_for_update.getHistory(),taken);
        
        
         // Update BHR values of entry and global, even if not using one of them
        this->global_BHR.update(taken);
        entry->local_BHR.update(taken);    


        //cout << *this;
        
    }

    // Predicts branch conclusion based on tag's history in Fetch stage
    bool predict(const uint32_t &pc, uint32_t *dst)
    {
        // Convert pc to index
        const unsigned index = getEntryByIndex(pc);
        const unsigned int current_tag = getTag(pc);
        // Load BHR with index
        BTBentry entry = this->entries[index];
        if (entry->valid && current_tag == entry->tag)
        {
            // index exists

            // Load local/global history and FSM table according to BTB flags
            unsigned int history_index = isGlobalHist ? global_BHR.getHistory() : entry->local_BHR.getHistory();

            // Get fsm table by reference so it win't be deleted at d'tor at the end of block
            FSM_table &fsm_table = isGlobalTable ? global_table : entry->local_table;
            if (isGlobalTable)
            {
                // XOR history with pc
                history_index = getSharedHistory(history_index, pc);
            }
            // Load branch prediction from Bipodal FSM
            BimodalState bi_state = fsm_table.getStateByIndex(history_index);
            if (bi_state == WT || bi_state == ST)
            {
                // Branch taken
                *dst = entry->target;
                return true;
            }
        }
        // Branch not taken OR no corresponding tag
        *dst = pc + 4;
        return false;
    }
      friend ostream& operator<<(ostream& os, const BTB& btb)
    {
        std::cout << "Global BHR :" <<endl;
        cout<< btb.global_BHR << endl <<endl;

        std::cout << "Global Table :" <<endl;
        cout<< btb.global_table << endl <<endl;

        for (size_t i = 0; i < btb.btbSize; i++)
        {
            cout<< i << " : " << *btb.entries[i] <<endl;
        }
        return os;
    }
    void updateStats(SIM_stats *curStats)
    {
        curStats->br_num = update_counter;
        curStats->flush_num = flush_counter;

        const unsigned int valid_bit_size = btbSize;
        const unsigned int tag_bit_size = btbSize*tagSize;
        const unsigned int target_bit_size = 30 * btbSize;
        const unsigned int history_bit_size = isGlobalHist ? historySize : historySize*btbSize ;

        const unsigned int table_size = (2 << historySize);
        const unsigned int table_bit_size = isGlobalTable ? table_size : table_size*btbSize;

        curStats->size = valid_bit_size+tag_bit_size+target_bit_size+history_bit_size+table_bit_size;
    }
};
static BTB *BP_BTB;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared)
{
    BP_BTB = new BTB(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);

    return SUCCESS;
}
/** Lior V=dvIR*/
bool BP_predict(uint32_t pc, uint32_t *dst)
{
    return BP_BTB->predict(pc, dst);
}

/** Shai Haim Yehezkel */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
    BP_BTB->update(pc, targetPc, taken, pred_dst);
    //std::cout << BP_BTB;
}

void BP_GetStats(SIM_stats *curStats)
{
    BP_BTB->updateStats(curStats);
    delete BP_BTB;
}
