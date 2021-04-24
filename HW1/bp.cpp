/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdio.h>

enum ReturnStatus {SUCCESS = 0, FAILURE = -1};
enum Bimodal {MIN=-1 , SNT = 0, WNT, WT, ST , MAX};
class BP;
class BHR;

class BHR {
public:
    static bool isGlobalHist;
    static bool isGlobalTable;
    static int Shared;
    static unsigned historySize;
    static unsigned fsmState;
    bool valid;
    unsigned tag;
    unsigned target;
    unsigned local_history;
    Bimodal * fsm;
    BHR() : fsm(new Bimodal[1<<historySize]), valid(false), local_history(0) {
        for(int i = 0; i < 1 << historySize ; i++) {
            Bimodal default_bimodal_val = static_cast<Bimodal>(fsmState);
            if (default_bimodal_val <= MIN || default_bimodal_val >= MAX) {
                throw;
            }
            fsm[i] = default_bimodal_val;
        }


    }
    BHR(const BHR & bhr) = delete;
    BHR & operator=(const BHR & bhr) = delete;
    ~BHR() = default;
};

class BP {
public:
    static unsigned btbSize;
    static unsigned historySize;
    static unsigned tagSize;
    static unsigned fsmState;
    static bool isGlobalHist;
    static bool isGlobalTable;
    static int Shared;
    static BHR * BTB;
};


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared) {
	BP::btbSize = btbSize;
	BP::historySize = historySize;
	BP::tagSize = tagSize;
	BP::fsmState = fsmState;
	BP::isGlobalHist = isGlobalHist;
	BP::isGlobalTable = isGlobalTable;
	BP::Shared = Shared;
     try{
         BHR::historySize = historySize;
         BHR::fsmState = fsmState;
         BP::BTB = new BHR[btbSize];
     } catch(...) {

     }


	return SUCCESS;
}
/** Lior */
bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

 /** Shai Haim Yehezkel */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

