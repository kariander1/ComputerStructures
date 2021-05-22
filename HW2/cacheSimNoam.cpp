/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ostream>
#include <bits/stdc++.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

typedef long long Tag;

static int MEM_DELAY = 0;

class LRU {
    std::list<int> keys_list; // keys of cache
    std::unordered_map<int, std::list<int>::iterator> ref_map; // references of key in cache
    int associativeness; // maximum capacity of cache

public:
    LRU(int);
    void updateLRU(int);
	int getLeastRecentlyUsed();
	int remove(int way);
	void printLRU();
};

LRU::LRU(int ways) {
    associativeness = (1 << ways);
    for (int i = 0; i < associativeness; i++) {
        keys_list.push_front(i);
        //ref_map[i] = keys_list.begin();
    }
}

void LRU::updateLRU(int n) {
    /*if (ref_map.find(n) == ref_map.end()) { // not in cache
        // cache is full
        if (keys_list.size() == associativeness) {
            // delete least recently used element
            int last = keys_list.back();
            keys_list.pop_back();
            ref_map.erase(last);
        }
    }
    else // in cache
    */
    // update keys_list, ref_map
    keys_list.erase(std::find(keys_list.begin(), keys_list.end(), n));
    keys_list.push_front(n);
    //ref_map[n] = keys_list.begin();
}

int LRU::getLeastRecentlyUsed() {
	return keys_list.back();
}

void LRU::printLRU() {
    for (auto it = keys_list.begin(); it != keys_list.end(); it++)
        std::cout << (*it) << " ";
    std::cout << std::endl;
}

int LRU::remove(int way) {
    keys_list.erase(std::find(keys_list.begin(), keys_list.end(), way));
    keys_list.push_back(way);
    return way;
}

class Cache {
    std::vector<Tag> tag_array;
    int size_of_cache; // in bits
    int size_of_block; // in bits
    int assoc_type; // in bits
    int access_time;
    bool write_allocate;
    int miss_count;
    int access_count;
    Cache* aboveMe;
    Cache* belowMe;
    int layer;
	std::vector<LRU> cacheLRU;
    std::vector<int> dirty_array;
    int updateCache(uint32_t mem);
    void updateLRU(int set, int way);
    int getLowestLRU(int index);
    Tag calc_tag(uint32_t mem);
    int calc_set(uint32_t mem);
    int calc_index(int way, int set);
    bool contains(uint32_t mem);
    int find(uint32_t mem);
    void remove(uint32_t mem);
    void mark_dirty(uint32_t mem);

public:
    Cache(int size_of_cache, int size_of_block, int assoc_type, int access_time, bool write_allocate, int layer);
    void setAbove(Cache* above) {aboveMe = above;}
    void setBelow(Cache* below) {belowMe = below;}
    int getMissCount() {return miss_count;}
    int getAccessCount() {return access_count;}
    int cacheRead(uint32_t mem);
    int cacheWrite(uint32_t mem, bool update_count);
    friend std::ostream& operator<<(std::ostream& os, Cache cache);
};

bool Cache::contains(uint32_t mem) {
    return (find(mem) != -1);
}

int Cache::updateCache(uint32_t mem) {
    // todo: Noam
    int set = calc_set(mem);
    int way = getLowestLRU(set);
    Tag tag = calc_tag(mem);
    int index = calc_index(way, set);
    if ((layer == 2) && (tag == 248) && (index == 4)) {
        set++;
        set--;
    }
    if (tag_array[index] != -1) {
        uint32_t to_kick_out = ((tag_array[index] << (size_of_cache - assoc_type)) + set) << size_of_block;
        if (aboveMe != nullptr && aboveMe->contains(to_kick_out)) {/////////////////////////////////
            aboveMe->remove(to_kick_out);
        }
        if (belowMe != nullptr && belowMe->contains(to_kick_out) && dirty_array[index] == 1) {/////////////////////////////////
            belowMe->cacheWrite(to_kick_out, false);
        }
    }
    dirty_array[index] = 0;
    tag_array[index] = tag;
    return way;
}

int Cache::find(uint32_t mem) {
    int set = calc_set(mem);////////////////////
    Tag tag = calc_tag(mem);
    for (int i = 0; i < (1 << assoc_type); i++) {
        int index = calc_index(i, set);
        if (tag_array[index] == tag) {
            return i;
        }
    }
    return -1;
}

void Cache::remove(uint32_t mem) {
    int set = calc_set(mem);
    int way = find(mem);
    if (way == -1) {
        return;
    }
    int index = calc_index(way, set);
    if (dirty_array[index] && belowMe != nullptr) {
        belowMe->cacheWrite(mem, false);
    }
    dirty_array[index] = 0;
    tag_array[index] = -1;
    cacheLRU[set].remove(way);
}

Cache::Cache(int size_of_cache, int size_of_block, int assoc_type, int access_time, bool write_allocate, int layer) :
    size_of_cache(size_of_cache), size_of_block(size_of_block), assoc_type(assoc_type),
    access_time(access_time), write_allocate(write_allocate), miss_count(0), access_count(0),
    aboveMe(nullptr), belowMe(nullptr), layer(layer) {
    tag_array = std::vector<Tag>((1 << (size_of_cache - size_of_block)), -1);
	dirty_array = std::vector<int>((1 << (size_of_cache - size_of_block)), 0);
    cacheLRU = std::vector<LRU>((1 << (size_of_cache - size_of_block - assoc_type)), assoc_type);
}


int Cache::getLowestLRU(int index) {
	return cacheLRU[index].getLeastRecentlyUsed();
}

void Cache::updateLRU(int set, int way) {
    cacheLRU[set].updateLRU(way);
}

int Cache::cacheRead(uint32_t mem) {
    // todo: Noam
    // try to read from mem
    int set = calc_set(mem);
    //int tag = calc_tag(mem);
    int time;
    access_count++;
    int way = find(mem);
    if (mem == 248) {
        access_count++;
        access_count--;
    }
    if (way != -1) {
        std::cout << layer << " R Hit\n";
        updateLRU(set, way);
        return access_time;
    } else { // not found
        std::cout << layer << " R Miss\n";
        miss_count++;
        way = updateCache(mem);
        updateLRU(set, way);
        if (belowMe == nullptr) {
            time = access_time + MEM_DELAY;
        } else {
            time = access_time + belowMe->cacheRead(mem);
        }
        return time;
    }
}

void Cache::mark_dirty(uint32_t mem) {
    int set = calc_set(mem);
    int way = find(mem);
    if (way == -1) {
        return;
    }
    int index = calc_index(way, set);
	dirty_array[index] = 1;
}

int Cache::cacheWrite(uint32_t mem, bool update_count) {
    // try to write from mem
    if (update_count) {
        access_count++;
    }
    int set = calc_set(mem);
    int way = find(mem);
    if (mem == 0x000000f8) {
        access_count = access_count;
    }
    int time;
    if (way != -1) {
        std::cout << layer << " W Hit\n";
		mark_dirty(mem);
        updateLRU(set, way);
        return access_time;
	}
    else {
        std::cout << layer << " W Miss\n";
        if (update_count) {
            miss_count++;
        }
        if (write_allocate) {
            way = updateCache(mem);
            mark_dirty(mem);
            updateLRU(set, way);
            if (belowMe == nullptr) {
                time = access_time + MEM_DELAY;
            } else {
                time = access_time + belowMe->cacheRead(mem);
            }
            return time;
        }
        else{ // write no alloc
            //this should not add time
            mark_dirty(mem);
            if (belowMe != nullptr) {
                belowMe->cacheWrite(mem, true);
            }
            return access_time;
        }
	}
}

int Cache::calc_set(uint32_t mem) {
    return (mem >> size_of_block) % (1 << (size_of_cache - size_of_block - assoc_type));
}

Tag Cache::calc_tag(uint32_t mem) {
    return mem >> (size_of_cache - assoc_type);
}

int Cache::calc_index(int way, int set) {
    return (way << (size_of_cache - size_of_block - assoc_type)) + set;
}

std::ostream& operator<<(std::ostream& os, Cache cache) {
    os << "###########################################\n";
    os << "Cache layer " << cache.layer << " :" << std::endl;
    for (int set = 0 ; set < (1 << (cache.size_of_cache - cache.size_of_block - cache.assoc_type)); set++) {
        os << "Set: " << set;
        for (int way = 0 ; way < (1 << cache.assoc_type); way++) {
            os << " Way: " << way << " " << std::hex << cache.tag_array[cache.calc_index(way, set)];
            //cache.cacheLRU[set].printLRU();
        }
        os << std::endl;
    }
    return os;
}

int main(int argc, char **argv) {
	if (argc < 19) {
		std::cerr << "Not enough arguments" << std::endl;
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
   double L1MissRate;
	double L2MissRate;
    double avgAccTimeL2;
	double avgAccTime;
    int accumulated_time = 0;
	MEM_DELAY = MemCyc;
    Cache L1(L1Size, BSize, L1Assoc, L1Cyc, WrAlloc, 1);
    Cache L2(L2Size, BSize, L2Assoc, L2Cyc, WrAlloc, 2);
    L1.setBelow(&L2);
    L2.setAbove(&L1);

    while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			std::cout << "Command Format error" << std::endl;
			return 0;
		}
		
		// DEBUG - remove this line
		std::cout << "operation: " << operation;

		std::string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		std::cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		std::cout << " (dec) " << num << std::endl;
		
		// todo accumulated_time += read / write
        accumulated_time += (operation == 'r') ? L1.cacheRead((uint32_t)num) : L1.cacheWrite((uint32_t) num, true);
	    //std::cout << L1;
        //std::cout << L2;
         L1MissRate = ((double)L1.getMissCount()) / (double)L1.getAccessCount();
    L2MissRate = ((double)L2.getMissCount()) / (double)L2.getAccessCount();
    avgAccTimeL2 =  (double)L2Cyc + (double)MEM_DELAY * L2MissRate;
    avgAccTime = (double)L1Cyc +  avgAccTimeL2 * L1MissRate;
            printf("L1miss=%.06f ", L1MissRate);
            printf("L2miss=%.06f ", L2MissRate);
            printf("AccTimeAvg=%.06f\n", avgAccTime);
    }

 

    L1MissRate = ((double)L1.getMissCount()) / (double)L1.getAccessCount();
    L2MissRate = ((double)L2.getMissCount()) / (double)L2.getAccessCount();
    avgAccTimeL2 =  (double)L2Cyc + (double)MEM_DELAY * L2MissRate;
    avgAccTime = (double)L1Cyc +  avgAccTimeL2 * L1MissRate;
    printf("L1miss=%.06f ", L1MissRate);
	printf("L2miss=%.06f ", L2MissRate);
	printf("AccTimeAvg=%.06f\n", avgAccTime);
	return 0;
}
