/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <utility>
#include<bits/stdc++.h>
#include <algorithm>


using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

typedef enum {NO_WRITE_ALLOCATE, WRITE_ALLOCATE} policy;
typedef enum {L1, L2} cache_name;
typedef enum {DOESNT_EXIST = 0 , EXIST = 1, DIRTY = 2} data_status;


/**
 * the following class simulates how the work of a cache
 */
class Cache {
public:
    unsigned size_of_cache;
    unsigned associative_level;
    unsigned block_size;
    unsigned num_of_rows; // size_of_cache /(block_size * associative_level)
    int access_count = 0;
    std::vector<std::vector<std::pair<unsigned, data_status>>> data; //pair : tag, status
    std::vector<std::vector<unsigned>> LRU_vector; //each vector represents a line of cache. each inner vector represents a cell (depends on associative level)

    Cache(unsigned cache_size, unsigned associative_level, unsigned block_size) : size_of_cache(pow(2, cache_size)),
                                                                                  associative_level(
                                                                                          pow(2, associative_level)),
                                                                                  block_size(pow(2, block_size)) {
        unsigned num_of_blocks = size_of_cache / this->block_size;
        num_of_rows = num_of_blocks / this->associative_level;
        data.resize(num_of_rows, std::vector<std::pair<unsigned, data_status>>(this->associative_level,
                                                                               std::make_pair(0, DOESNT_EXIST)));
        LRU_vector.resize(num_of_rows, std::vector<unsigned int>(0, 0));
    }

    ~Cache() = default;

    Cache(Cache &other) = default;

    bool in_cache(unsigned long int address, std::pair<unsigned, data_status> *return_pair, bool is_tag);

    bool add(unsigned long int address, const unsigned *LRU_address, bool L1);

    void changeToX(unsigned long int address, data_status new_data_status, bool is_address);

    bool checkIfDirty(unsigned long int address);

    /* LRU functions */
    // NOTE: back of vector = most used. front of vector = least used
    unsigned *LRUgetLeastRecentlyUsed(unsigned long address);

    void LRUupdate(unsigned long address, bool is_address);

    void LRUremove(unsigned long tag);

    void LRUprint();
};

bool Cache::in_cache(unsigned long int address, std::pair<unsigned, data_status>* return_pair, bool is_tag = false) {
    if (return_pair == nullptr) return false;

    unsigned long int tag ; // get the upper bits of the address to check with tag
    unsigned set;
    if (!is_tag) { // if address
        unsigned int offset_size = log2(this->block_size);
        tag = address >> offset_size; // get the upper bits of the address to check with tag
        set = tag%this->num_of_rows;
    }
    else {
        set = address%this->num_of_rows;
        tag = address;
    }
    unsigned int num_of_bits_in_set = log2(num_of_rows);
    tag = tag >> num_of_bits_in_set;

    for (unsigned int i = 0; i < this->associative_level ; ++i) {
        if ((this->data[set][i].first == tag) && (this->data[set][i].second != DOESNT_EXIST)) {
            *return_pair = this->data[set][i];
            return true;
        }
    }
    return_pair = nullptr;
    return false;
}

// returns id evacuation is needed
bool Cache::add(unsigned long address, const unsigned* LRU_address, bool L1 = true) {
    bool need_evac = false;
    bool need_to_replace_LRU_address = true;
    unsigned int offset_size = log2(this->block_size); // 8 is the num of bits in byte
    unsigned long int tag = address >> offset_size; // get the upper bits of the address to check with tag
    unsigned set = tag%this->num_of_rows;
    unsigned int num_of_bits_in_set = log2(num_of_rows);
    tag = tag >> num_of_bits_in_set;

    std::pair<unsigned int, data_status> pair_of_LRU;

    for (auto& data_address :  this->data[set]) { // cannot have both -> if lru_address == 0 ( does not exist) then will not enter due to data_address.second == EXIST
        // need to replace LRU_address with address  todo: add check if LRU exists
        // need to find a free location in N-ways
        if (data_address.second == DOESNT_EXIST && !this->in_cache(address,&pair_of_LRU)) { // add the data to the available spot
            data_address.first = tag;
            data_address.second = EXIST;
            need_evac = false;
            need_to_replace_LRU_address = false;
            break;
        }
    }
    if(need_to_replace_LRU_address && LRU_address != nullptr) { // free location not found -> need to replace with LRU_address
        unsigned long int LRU_address_tag = *LRU_address >> num_of_bits_in_set;
        for (auto &data_address :  this->data[set]) {
            if (data_address.first == LRU_address_tag && data_address.second != DOESNT_EXIST) {
                need_evac = (data_address.second == DIRTY) || (!L1 && data_address.second != DOESNT_EXIST);
                data_address.first = tag;
                data_address.second = EXIST;
                LRUremove(*LRU_address);
                break;
            }
        }
    }
    return need_evac;
}

/**
 * function changes the status of a given adress to the given status (Exist, doesnt_exist or dirty)
 * @param address
 * @param new_data_status
 */
void Cache::changeToX(unsigned long address, data_status new_data_status, bool is_address = true) {
    unsigned long int tag;
    unsigned set;
    if (is_address) {
        unsigned int offset_size = log2(this->block_size); // 8 is the num of bits in byte
        tag = address >> offset_size; // get the upper bits of the address to check with tag
        set = tag%this->num_of_rows;
    } else {
        tag = address;
    }
    unsigned int num_of_bits_in_set = log2(num_of_rows);
    tag = tag >> num_of_bits_in_set;

    for (unsigned int i = 0; i < this->associative_level ; ++i) {
        if ( (this->data[set][i].first == tag) && (this->data[set][i].second != DOESNT_EXIST) ) {
            this->data[set][i].second = new_data_status;
            break;
        }
    }
}

bool Cache::checkIfDirty(unsigned long address) {
    unsigned int offset_size = log2(this->block_size);
    unsigned long int tag = address >> offset_size; // get the upper bits of the address to check with tag
    unsigned set = tag%this->num_of_rows;
    unsigned int num_of_bits_in_set = log2(num_of_rows);
    tag = tag >> num_of_bits_in_set;

    for (unsigned int i = 0; i < this->associative_level ; ++i) {
        if (this->data[set][i].first == tag) {
            return (this->data[set][i].second == DIRTY);
        }
    }
    return false;
}

/**
 * returns address of least recently used value
 * @param address
 * @return
 */
unsigned* Cache::LRUgetLeastRecentlyUsed(unsigned long address) { //TODO: make sure returns from right end of the vector
    unsigned int offset_size = log2(this->block_size); // 8 is the num of bits in byte
    unsigned long int tag = address >> offset_size; // get the upper bits of the address to check with tag
    unsigned set = tag%this->num_of_rows;

    if (LRU_vector.at(set).empty()) {
        return nullptr;
    }
    return new unsigned(LRU_vector.at(set).at(0));
}

void Cache::LRUremove(unsigned long tag) {

//    unsigned int offset_size = log2(this->block_size); // 8 is the num of bits in byte
//    unsigned long int tag = address >> offset_size; // get the upper bits of the address to check with tag
    std::vector<unsigned> new_LRU_vector;
    unsigned set = tag%this->num_of_rows;

    auto itr = LRU_vector.at(set).begin();
    for (; itr != LRU_vector.at(set).end() ; ++itr) {
        if (*itr != tag) {
            new_LRU_vector.push_back(*itr);
        }
    }
    LRU_vector.at(set) = new_LRU_vector;
}

/**
 * checks if given address exists in LRU table.
 * @param address
 * @return
 */
void Cache::LRUupdate(unsigned long address, bool is_address) {
    unsigned set;
    unsigned long int tag;
    std::vector<unsigned> new_LRU_vector;
    if (is_address) {
        unsigned int offset_size = log2(this->block_size);
        tag = address >> offset_size; // get the upper bits of the address to check with tag
        set = tag%this->num_of_rows;
        //unsigned int num_of_bits_in_set = log2(num_of_rows);
        //tag = tag >> num_of_bits_in_set; //todo : check with other functions that its ok!!!!!
    }
    else {
        set = address%this->num_of_rows;
        tag = address;
    }

    //bool exist_in_vector = false;
    auto itr = LRU_vector.at(set).begin();
    for (; itr != LRU_vector.at(set).end() ; ++itr) {
        if (*itr != tag) {
            new_LRU_vector.push_back(*itr);
        }
    }
    //this for loop does the same, while using index instead of iterators
    /*int index;
    for (index = 0; index < LRU_vector->at(set).size(); ++index) {//search in the relevant line
        if (LRU_vector->at(set).at(index) == address) {
            exist_in_vector = true;
            break;
        }
    }*/
    //now index is the location of the corresponding col.
    LRU_vector.at(set) = new_LRU_vector;

    LRU_vector.at(set).push_back(tag);
}

void Cache::LRUprint() {

    for (unsigned int i = 0; i <LRU_vector.size() ; ++i) {

        if (!LRU_vector[i].empty()){
            cout << "set " << i << ":";
            cout << endl;

        } else continue;

        for (unsigned int j = 0; j <LRU_vector[i].size() ; ++j) {
            cout << "    0x" << std::hex << LRU_vector[i][j];
        }
        cout << endl;
    }
}

/**
 * this class manages the interaction with the memory.
 * it contains two field of class 'Cache', each one represents a cache.
 */
class Memory {
public:
    Cache* L1_cache;
    Cache* L2_cache;
    policy cache_policy;
    unsigned access_count_L1 = 0;
    unsigned miss_count_L1 = 0;
    unsigned access_count_L2 = 0;
    unsigned miss_count_L2 = 0;
    unsigned access_count_mem = 0;
    unsigned dram_cycles;
    unsigned L1_cycles;
    unsigned L2_cycles;
    unsigned block_size;
    std::vector< std::pair<unsigned, data_status> > LRU;
    Memory(unsigned MemCyc,unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
           unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc) :
           dram_cycles(MemCyc), L1_cycles(L1Cyc),L2_cycles(L2Cyc), block_size(pow(2,BSize)){ // c'tor
        L1_cache = new Cache(L1Size, L1Assoc, BSize);
        L2_cache = new Cache(L2Size, L2Assoc, BSize);
        cache_policy = WrAlloc ? WRITE_ALLOCATE : NO_WRITE_ALLOCATE;
    }
    ~Memory() {
        delete L1_cache;
        delete L2_cache;
    }
    Memory(Memory& other) = default;
    void calc_operation(unsigned long int address, const char op);
};

void Memory::calc_operation(unsigned long int address, char op) {
    //unsigned data_location = address % (this->block_size);
    std::pair <unsigned, data_status> returned_pair;
    access_count_L1++; // add the time to access L1 cache
    if (this->L1_cache->in_cache(address, &returned_pair) == true) {
        L1_cache->LRUupdate(address, true);
        if(op == 'w') { // need to specify as dirty
            L1_cache->changeToX(address, DIRTY);
        }
    }
    else { // not is L1 cache -> check in L2
        this->miss_count_L1++;
        access_count_L2++;
        if (this->L2_cache->in_cache(address, &returned_pair) == true) { // in L2
            if (((op == 'w') && (this->cache_policy == WRITE_ALLOCATE)) || op == 'r'){ // need to add address to L1
                unsigned* LRU_address;
                LRU_address = L1_cache->LRUgetLeastRecentlyUsed(address);
                if (L1_cache->add(address, LRU_address) &&  (LRU_address != nullptr)){ // if need to evict old address the remove is in add
                    L2_cache->LRUupdate(*LRU_address, false);
                    //L1_cache->LRUremove(*LRU_address);
                    L2_cache->changeToX(address, DIRTY); // basically we don't check dirty of L2 so don't need!!
                }
                if (LRU_address != nullptr) {
                    delete LRU_address; // delete old L1 adress
                }
                if (op == 'w') {
                    L1_cache->changeToX(address, DIRTY);
                }
                if (op == 'r') {
                    L1_cache->changeToX(address, EXIST);
                    L2_cache->changeToX(address, EXIST);
                }
                L1_cache->LRUupdate(address, true);
                L2_cache->LRUupdate(address, true);

            }
            else { // ((op == 'w') && (this->cache_policy == NO_WRITE_ALLOCATE))
                L2_cache->LRUupdate(address, true);
                L2_cache->changeToX(address, DIRTY);
            }
        }
        else { // not in L2
            miss_count_L2++;
            access_count_mem++;
            if (((op == 'w') && (this->cache_policy == WRITE_ALLOCATE)) || op == 'r'){ // need to add address to L1 and L2
                unsigned* LRU_address;
                //////////////////////// search in L2 ///////////////////////////
                LRU_address = L2_cache->LRUgetLeastRecentlyUsed(address);// allocate new address
                if (L2_cache->add(address, LRU_address, false) && (LRU_address != nullptr)) { // if need to remove LRU address
                    if (L1_cache->in_cache(*LRU_address,&returned_pair, true )) {
                        L1_cache->changeToX(*LRU_address, DOESNT_EXIST, false);
                        L1_cache->LRUremove(*LRU_address);
                    }
                }
                if (LRU_address != nullptr) {
                    delete LRU_address; // delete old L2 adress
                }
                L2_cache->LRUupdate(address, true);
                //////////////////////// search in L1 ///////////////////////////
                LRU_address = L1_cache->LRUgetLeastRecentlyUsed(address); // allocate new address
                if (L1_cache->add(address, LRU_address) && (LRU_address != nullptr)){ // if need to evict old address
                    L2_cache->LRUupdate(*LRU_address, false);

                }
                if (LRU_address != nullptr) {
                    delete LRU_address; // delete old L1 adress
                }
                L1_cache->LRUupdate(address, true);
                if (op == 'w') { // modify L1 cache
                    L1_cache->changeToX(address, DIRTY);
                }
            }
             //else :  // ((op == 'w') && (this->cache_policy == NO_WRITE_ALLOCATE))
             // go to mem and update the data
             // do not update LRU at all
        }
    }
    /*cout << "L1 cache : " << endl;
    L1_cache->LRUprint();
    cout << "L2 cache : " << endl;
    L2_cache->LRUprint();*/

}

int main(int argc, char **argv) {
    //std::cout << "number of arguments: " << argc << std::endl;
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
    string address;
    char operation = 0; // read (R) or write (W)
    unsigned long int num = 0;
    auto cpu_mem = new Memory(MemCyc,BSize, L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc,WrAlloc );
    double L1MissRate = 0;
    double L2MissRate = 0;
    double avgAccTime = 0;

	while (getline(file, line)) {

		stringstream ss(line);

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


		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;

        cpu_mem->calc_operation(num, operation);

         avgAccTime = double (cpu_mem->access_count_L1*cpu_mem->L1_cycles + cpu_mem->access_count_L2*cpu_mem->L2_cycles
            +cpu_mem->access_count_mem*cpu_mem->dram_cycles)/double (cpu_mem->access_count_L1);

    if (cpu_mem->access_count_L1 != 0) {
        L1MissRate = double(cpu_mem->miss_count_L1) / double(cpu_mem->access_count_L1);
    }
    //std::cout << "L1 Miss rate: " << L1MissRate << std::endl;

    if (cpu_mem->access_count_L2 != 0){
        L2MissRate = double(cpu_mem->miss_count_L2) / double(cpu_mem->access_count_L2);
    }
    //std::cout << "L2 Miss rate: " << L2MissRate << std::endl;

    printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);
    
	}

    avgAccTime = double (cpu_mem->access_count_L1*cpu_mem->L1_cycles + cpu_mem->access_count_L2*cpu_mem->L2_cycles
            +cpu_mem->access_count_mem*cpu_mem->dram_cycles)/double (cpu_mem->access_count_L1);

    if (cpu_mem->access_count_L1 != 0) {
        L1MissRate = double(cpu_mem->miss_count_L1) / double(cpu_mem->access_count_L1);
    }
    //std::cout << "L1 Miss rate: " << L1MissRate << std::endl;

    if (cpu_mem->access_count_L2 != 0){
        L2MissRate = double(cpu_mem->miss_count_L2) / double(cpu_mem->access_count_L2);
    }
    //std::cout << "L2 Miss rate: " << L2MissRate << std::endl;

    printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	delete cpu_mem;
	return 0;
}