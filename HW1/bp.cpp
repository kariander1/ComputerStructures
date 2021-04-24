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
    unsigned tag;
    unsigned target;
    unsigned local_history;
    Bimodal * fsm;
    BHR(unsigned historySize, unsigned tagSize, unsigned  fsmState) : fsm(new Bimodal[1<<historySize]) {
        for(int i = 0; i < 1 << historySize ; i++) {
            fsm[i] = static_cast<Bimodal>(fsmState);
            
        }

    }
    BHR(const BHR & bhr) {}
    BHR & operator=(const BHR & bhr) {}
    ~BHR() {}
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
};


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared) {
				/*
	BP::btbSize = btbSize;
	BP::historySize = historySize;
	BP::tagSize = tagSize;
	BP::fsmState = fsmState;
	BP::isGlobalHist = isGlobalHist;
	BP::isGlobalTable = isGlobalTable;
	BP::Shared = Shared;

*/


	return SUCCESS;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

