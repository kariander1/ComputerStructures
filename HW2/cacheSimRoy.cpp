/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <stdint.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
//int index=0;
typedef struct Way {
    uint32_t tag;
    bool validBit;
    bool dirty;
    uint32_t LRU;
} Way;

typedef struct Cache{
    Way** L1;
    Way** L2;
    unsigned MemCyc, BSize, L1Size, L2Size, L1Assoc,
            L2Assoc, L1Cyc, L2Cyc, WrAlloc;
    int L1miss, L2miss, L1NumOfAccesses, L2NumOfAccesses, total;
} Cache;

Cache myCache;
int L1Levels;
int L2Levels;

/*
void //print_LRU_State2 ()
{
    int size = myCache.L2Assoc;
    int i;
    for (i=1 ; i<=size ; i++){
        cout << i << "(" ;
        if (myCache.L2[0][i-1].validBit == true)
        {
            cout << myCache.L2[0][i-1].LRU << ") " ;
        }
        else
        {
            cout << "X )" ;
        }
    }
    cout << endl;
} */

void update_cache_counters(int cache_id,bool isMiss)
{
    if(cache_id == 1)
    {
        myCache.L1NumOfAccesses++;
        myCache.L1miss+=isMiss;
    }else
    {
        myCache.L2NumOfAccesses++;
        myCache.L2miss+=isMiss;
    }
}

uint32_t create_mask(int starting_bit,int bits_num) {
    unsigned x=0, mask = 0;
    for (int i = starting_bit; i < bits_num + starting_bit; i++) {
        x++;
        x = x << i;
        mask = mask + x;
        x = 0;
    }
    return mask;
}

bool SearchCache (int cache_id, uint32_t cur_set, uint32_t cur_tag, Way* ret_way, int* way_num)
{
    if(cache_id == 1)
    {
        for (int i=1 ; i<=myCache.L1Assoc ; i++)
        {
            if (((myCache.L1[cur_set][i-1]).tag == cur_tag) && (myCache.L1[cur_set][i-1]).validBit)
            {
                *ret_way = myCache.L1[cur_set][i-1];
                *way_num = i-1;
                return true;
            }
        }
        return false;
    }
    else
    {
        for (int i=1 ; i<=myCache.L2Assoc ; i++)
        {
            if (((myCache.L2[cur_set][i-1]).tag == cur_tag) && (myCache.L2[cur_set][i-1]).validBit)
            {
                *ret_way = myCache.L2[cur_set][i-1];
                *way_num = i-1;
                return true;
            }
        }
        return false;
    }
}

void UpdateLRU (Way** cur_cache, uint32_t cur_set, unsigned limit, unsigned assoc)
{
    for (int i=1 ; i<=assoc ; i++)
    {
        if (cur_cache[cur_set][i-1].validBit)
        {
            if ((cur_cache[cur_set][i-1]).LRU == limit)
            {
                (cur_cache[cur_set][i-1]).LRU = 1;
            }
            else
            {
                if ((cur_cache[cur_set][i-1]).LRU < limit)
                    (cur_cache[cur_set][i-1]).LRU++;
            }
        }
        ////print_LRU_State2();
    }
}

void UpdateLRUforEvac (Way** cur_cache, uint32_t cur_set, unsigned limit, unsigned assoc)
{
    for (int i=1 ; i<=assoc ; i++)
    {
        if (cur_cache[cur_set][i-1].validBit)
        {
            if ((cur_cache[cur_set][i-1]).LRU > limit)
            {
                (cur_cache[cur_set][i-1]).LRU--;
            }
        }
    }
}

void InsertToSlot(Way** cur_cache, uint32_t cur_set, uint32_t cur_tag, int way_num, char operation, unsigned assoc)
{
    cur_cache[cur_set][way_num].tag = cur_tag;
    cur_cache[cur_set][way_num].validBit = true;
    cur_cache[cur_set][way_num].LRU = assoc;
    if (operation == 'r')
    {
        cur_cache[cur_set][way_num].dirty = false;
    }
    else //it's 'w'
    {
        cur_cache[cur_set][way_num].dirty = true;
    }
    UpdateLRU(cur_cache, cur_set, assoc, assoc);
}

Way* getLRU (Way** cur_cache, uint32_t cur_set, unsigned assoc, int* way_num)
{
    for (int i=1 ; i<=assoc ; i++)
    {
        //if (index==3 && cur_cache[cur_set][i-1].validBit){
            ////printf("LRU of %d is %d\n", i, cur_cache[cur_set][i-1].LRU);
            //cout << "LRU of " << i << " is " << cur_cache[cur_set][i-1].LRU << endl;
        //}
        if (cur_cache[cur_set][i-1].LRU == assoc) //here we assume all is valid and taken (no empty slot)
        {
            *way_num = i-1;
            return &cur_cache[cur_set][i-1]; //TODO: Nadav please check I did it right
        }
    }
    return NULL; //should never reach here
}
void InsertToL1 (uint32_t cur_set, uint32_t cur_tag, char operation)
{
    bool emptySlot = false;
    for (int i=1 ; i<=myCache.L1Assoc ; i++) //search for empty slot
    {
        if (!myCache.L1[cur_set][i-1].validBit) //true is there's empty slot
        {
            emptySlot = true;
            InsertToSlot(myCache.L1, cur_set, cur_tag, i-1, operation, myCache.L1Assoc); //it's also update LRU
            break;
        }
    }
    if (!emptySlot) //need to evac slot
    {
        //printf("EVAC in L1\n");
        int way_num;
        Way* way_to_evac = getLRU(myCache.L1, cur_set, myCache.L1Assoc, &way_num); //search for old one to evac
        if (way_to_evac->dirty) //it's dirty, need to update L2
        {
            //Find this line in L2: //TODO: Nadav please check the logic here
            uint32_t full_line = way_to_evac->tag;
            full_line = full_line << (int)log2(L1Levels);
            full_line = full_line + cur_set;
            uint32_t L2_set_mask = create_mask(0,log2(L2Levels));
            uint32_t L2_set = (full_line & L2_set_mask);
            uint32_t L2_tag = (full_line) >> (int)log2(L2Levels);
            Way L2_way;
            int way_num_L2;
            bool isFound = SearchCache(2, L2_set, L2_tag, &L2_way, &way_num_L2);
            if (isFound) //should always be true, cache is inclusive
            {
                //L2_way.LRU = myCache.L2Assoc;
                //L2_way.dirty = true;
                int current_LRU =  myCache.L2[L2_set][way_num_L2].LRU;
                //myCache.L2[L2_set][way_num_L2].LRU = myCache.L2Assoc;
                myCache.L2[L2_set][way_num_L2].dirty = true;
                UpdateLRU(myCache.L2, L2_set, current_LRU, myCache.L2Assoc);
            }
            else //should never reach here
            {
                //Error, should never reach here
            }
        }
        //After we update L2, we insert the new onr for L1:
        InsertToSlot(myCache.L1, cur_set, cur_tag, way_num, operation, myCache.L1Assoc); //it's also update LRU
    }
    //cout << "after insert to L1:" << endl;
    ////print_LRU_State2();
}

void InsertToL2 (uint32_t cur_set, uint32_t cur_tag, char operation)
{
    bool emptySlot = false;
    for (int i=1 ; i<=myCache.L2Assoc ; i++) //search for empty slot
    {
        if (!myCache.L2[cur_set][i-1].validBit) //true is there's empty slot
        {
            emptySlot = true;
            InsertToSlot(myCache.L2, cur_set, cur_tag, i-1, operation, myCache.L2Assoc); //it's also update LRU
            break;
        }
    }
    if (!emptySlot) //need to evac slot
    {
        //index++;
        //printf("EVAC in L2\n");
        int way_num_L2, way_num_L1;
        Way* way_to_evac = getLRU(myCache.L2, cur_set, myCache.L2Assoc, &way_num_L2); //search for old one to move
        //Need to remove this one from L1 because cache is inclusive:
        //Find this line in L1: //TODO: Nadav please check the logic here
        uint32_t full_line = way_to_evac->tag;
        full_line = full_line << (int)log2(L2Levels);
        full_line = full_line + cur_set;
        uint32_t L1_set_mask = create_mask(0,log2(L1Levels));
        uint32_t L1_set = (full_line & L1_set_mask);
        uint32_t L1_tag = (full_line) >> (int)log2(L1Levels);
        Way L1_way;
        bool isFound = SearchCache(1, L1_set, L1_tag, &L1_way, &way_num_L1);
        if (isFound) //should always be true, cache is inclusive
        {
            //L1_way.validBit=false; //it's not valid any more TODO: bad, I update here and not in struct!!!
            myCache.L1[L1_set][way_num_L1].validBit = false;
            //printf("EVAC in L1\n");
            int cur_LRU = L1_way.LRU;
            UpdateLRUforEvac(myCache.L1, L1_set, cur_LRU, myCache.L1Assoc);
        }
        else //should never reach here
        {
            //Error, should never reach here
        }
        InsertToSlot(myCache.L2, cur_set, cur_tag, way_num_L2, operation, myCache.L2Assoc); //also update LRU
    }
    //cout << "after insert to L2:" << endl;
    ////print_LRU_State2();
}

int main(int argc, char **argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments

    // File
    // Assuming it is the first argument
    char* fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
            L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = atoi(argv[i + 1]);
        } else if (s == "--bsize") {
            BSize = atoi(argv[i + 1]);
        } else if (s == "--l1-size") {
            L1Size = atoi(argv[i + 1]);
        } else if (s == "--l2-size") {
            L2Size = atoi(argv[i + 1]);
        } else if (s == "--l1-cyc") {
            L1Cyc = atoi(argv[i + 1]);
        } else if (s == "--l2-cyc") {
            L2Cyc = atoi(argv[i + 1]);
        } else if (s == "--l1-assoc") {
            L1Assoc = atoi(argv[i + 1]);
        } else if (s == "--l2-assoc") {
            L2Assoc = atoi(argv[i + 1]);
        } else if (s == "--wr-alloc") {
            WrAlloc = atoi(argv[i + 1]);
        } else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }

    //Set Arguments
    myCache.MemCyc=MemCyc;
    myCache.BSize=pow(2,BSize);
    myCache.L1Size=pow(2,L1Size);
    myCache.L2Size=pow(2,L2Size);
    myCache.L1Cyc=L1Cyc;
    myCache.L2Cyc=L2Cyc;
    myCache.L1Assoc=pow(2,L1Assoc);
    myCache.L2Assoc=pow(2,L2Assoc);
    myCache.WrAlloc=WrAlloc;
    myCache.L1miss=0;
    myCache.L2miss=0;
    myCache.L1NumOfAccesses=0;
    myCache.L2NumOfAccesses=0;
    myCache.total=0;

    //Init L1
    int L1TotalLines = myCache.L1Size/myCache.BSize;          //Num of blockes in cache
    L1Levels = L1TotalLines/myCache.L1Assoc;              //"Hight" of the cache
    uint32_t L1_set_mask = create_mask(BSize,log2(L1Levels));
    int size_to_shft_for_L1_tag =  BSize+log2(L1Levels);
    myCache.L1 = (Way **) new Way[L1Levels];
    for(int i =0; i < L1Levels;i++)
    {
        myCache.L1[i] = (Way*) new Way[myCache.L1Assoc];
        for(int j =0; j<myCache.L1Assoc; j++)
        {
            myCache.L1[i][j].validBit = false;
        }
    }
    //Init L2
    int L2TotalLines = myCache.L2Size/myCache.BSize;
    L2Levels = L2TotalLines/myCache.L2Assoc;
    myCache.L2 = (Way **) new Way[L2Levels];
    uint32_t L2_set_mask = create_mask(BSize,log2(L2Levels));
    int size_to_shft_for_L2_tag =  BSize+log2(L2Levels);
    for(int i =0; i < L2Levels;i++)
    {
        myCache.L2[i] = (Way*) new Way[myCache.L2Assoc];
        for(int j =0; j<myCache.L2Assoc; j++)
        {
            myCache.L2[i][j].validBit = false;
        }
    }
    //printf("Params: Block_Size_in_bits=%d\n", BSize);
    //printf("L1 is %d lines, with %d ways each\n", L1Levels, myCache.L1Assoc);
    //printf("L2 is %d lines, with %d ways each\n", L2Levels, myCache.L2Assoc);
    //Handle the trace file - read the accesses to cache
    while (getline(file, line)) {
        //printf("\n");
        myCache.total++; //using for calc avg time
        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;
        }

        // DEBUG - remove this line
        //cout << "operation: " << operation;

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        // DEBUG - remove this line
        //cout << ", address (hex)" << cutAddress;

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), NULL, 16);

        // DEBUG - remove this line
        //cout << " (dec) " << num << endl;
        uint32_t L1_set = (num & L1_set_mask)>>BSize;
        uint32_t L2_set = (num & L2_set_mask)>>BSize;
        uint32_t L1_tag = (num )>>size_to_shft_for_L1_tag;
        uint32_t L2_tag = (num )>>size_to_shft_for_L2_tag;

        Way L1_way;
        Way L2_way;
        int search_way_num;
        //cout << "before run" << endl;
        ////print_LRU_State2();
        if (operation == 'w') //if write
        {
            //check if exists in l1
            bool isInL1 = SearchCache(1, L1_set, L1_tag, &L1_way, &search_way_num);
            //if exists in L1:
            if (isInL1)
            {
                UpdateLRU(myCache.L1, L1_set, L1_way.LRU, myCache.L1Assoc); // update LRU
                myCache.L1[L1_set][search_way_num].dirty=true; //update L1 - dirty //TODO: fixed here
                //L1_way.dirty = true;
                update_cache_counters(1, false); //update L1 - hit++
                //printf("L1 Hit   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set, L2_tag, L2_set);
            }
            else //is NOT in L1
            {
                bool isInL2 = SearchCache(2, L2_set, L2_tag, &L2_way, &search_way_num); // check if exists in l2
                if (isInL2) //if exists in L2:
                {
                    UpdateLRU(myCache.L2, L2_set, L2_way.LRU, myCache.L2Assoc); //update L2 - LRU
                    if (myCache.WrAlloc == 1) //Yes allocate
                    {
                        InsertToL1(L1_set, L1_tag, operation); //insert to L1, include LRU, dirty
                    }
                    else //NO allocate
                    {
                        //L2_way.dirty=true; //L2 is dirty
                        myCache.L2[L2_set][search_way_num].dirty=true; //update L2 - dirty //TODO: fixed here
                    }
                    update_cache_counters(1, true);//update L1 - miss++
                    update_cache_counters(2, false);//update L2 - hit++
                    //printf("L1 Miss L2 Hit   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set, L2_tag,L2_set);
                }
                else //is NOT in L2
                {
                    if (myCache.WrAlloc == 1) //Yes allocate
                    {
                        InsertToL2(L2_set, L2_tag, operation); //insert to L2, include edit LRU, and not dirty TODO: it makes it dirty
                        InsertToL1(L1_set, L1_tag, operation); //insert to L1, include edit LRU, and dirty
                    }
                    update_cache_counters(1, true);//update L1 - miss++
                    update_cache_counters(2, true);//update L2 - miss++
                    //printf("L1 Miss L2 Miss   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set,L2_tag, L2_set);
                }
            }
        }

        //if read
        if (operation == 'r')
        {
            bool isInL1 = SearchCache(1, L1_set, L1_tag, &L1_way, &search_way_num); //check if exists in L1
            if (isInL1) //if exists in L1:
            {
                UpdateLRU(myCache.L1, L1_set, L1_way.LRU, myCache.L1Assoc);
                update_cache_counters(1, false);//update L1 - hit++
                //printf("L1 Hit   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set, L2_tag, L2_set);
            }
            else //NOT in L1
            {
                bool isInL2 = SearchCache(2, L2_set, L2_tag, &L2_way, &search_way_num); //check if exists in L2
                if (isInL2) //if exists in L2
                {
                    UpdateLRU(myCache.L2, L2_set, L2_way.LRU, myCache.L2Assoc); //update LRU in L2
                    InsertToL1(L1_set, L1_tag, operation); //insert to L1, include edit LRU //TODO I switched order here
                    update_cache_counters(1, true);//update L1 - miss++
                    update_cache_counters(2, false);//update L1 - hit++
                    //printf("L1 Miss L2 Hit   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set,L2_tag, L2_set);
                }
                else //Not in L2
                {
                    InsertToL2(L2_set, L2_tag, operation); //insert to L2, include edit LRU
                    InsertToL1(L1_set, L1_tag, operation); //insert to L1, include edit LRU
                    update_cache_counters(1, true);//update L1 - miss++
                    update_cache_counters(2, true);//update L1 - miss++
                    //printf("L1 Miss L2 Miss   |   L1_tag=%d L1_set=%d L2_tag=%d L2_set=%d\n", L1_tag, L1_set,L2_tag, L2_set);
                }
            }
        }
    }

    double L1MissRate = (double)(myCache.L1miss)/(myCache.L1NumOfAccesses);
    double L2MissRate = (double)(myCache.L2miss)/(myCache.L2NumOfAccesses);

    unsigned L1_hit = myCache.total - myCache.L1miss;
    unsigned L1_miss_and_L2_hit = myCache.total - myCache.L2miss - L1_hit;
    unsigned L1_miss_and_L2_miss = myCache.L2miss;
    unsigned total_time = L1_hit*myCache.L1Cyc + L1_miss_and_L2_hit*(myCache.L1Cyc + myCache.L2Cyc) +
            L1_miss_and_L2_miss*(myCache.L1Cyc + myCache.L2Cyc + myCache.MemCyc);
    double avgAccTime = (double)total_time/myCache.total;
    //TODO: free all memory allocated: L1 + L2

    //printf("L1: %d misses from %d accesses\n", myCache.L1miss, myCache.L1NumOfAccesses);
    //printf("L2: %d misses from %d accesses\n", myCache.L2miss, myCache.L2NumOfAccesses);
    //printf("total actios: %d\n", myCache.total);
    //printf("L1_hit: %d\n", L1_hit);
    //printf("L1_miss_and_L2_hit: %d\n", L1_miss_and_L2_hit);
    //printf("L1_miss_and_L2_miss: %d\n", L1_miss_and_L2_miss);
    //printf("total_time: %d\n", total_time);
    //printf("total time calc is:\n");
    //printf("   L1_cyc: %d, L2_cyc: %d, Mem_cyc: %d\n", myCache.L1Cyc, myCache.L2Cyc, myCache.MemCyc);
    //printf("   L1_hit is %d * %d = %.03f\n", L1_hit, myCache.L1Cyc, (double)L1_hit*myCache.L1Cyc);
    //printf("   L1_miss_and_L2_hit is %d * %d = %.03f\n", L1_miss_and_L2_hit, myCache.L1Cyc + myCache.L2Cyc,(double) (L1_miss_and_L2_hit*(myCache.L1Cyc + myCache.L2Cyc)));
    //printf("   L1_miss_and_L2_miss is %d * %d = %.03f\n", L1_miss_and_L2_miss, myCache.L1Cyc + myCache.L2Cyc + myCache.MemCyc, (double)(L1_miss_and_L2_miss*(myCache.L1Cyc + myCache.L2Cyc + myCache.MemCyc)));
    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}
