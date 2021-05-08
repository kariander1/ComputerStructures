/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <time.h>
using std::cerr;
using std::cout;
using std::endl;
using std::FILE;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

#define NOT_USED (-1) // for when block hasn't been used yet

class CacheBlock_p
{
	time_t last_accessed_time;
	unsigned tag;
public:
    CacheBlock_p() {
        last_accessed_time = NOT_USED;
        tag = NOT_USED;
    }
    ~CacheBlock_p() = default;
    time_t getTime() {
        return last_accessed_time;
    }
    void setTime() {
        time(&last_accessed_time);
    }
    unsigned getTag() {
        return tag;
    }
    void setTag(unsigned new_tag) {
        tag = new_tag;
    }
};
typedef struct CacheBlock_p *CacheBlock;
class Set
{
    unsigned waze_num; //It's a joke, shay
	CacheBlock *blocks;
public:
    Set(unsigned waze_num) : waze_num(waze_num) {
        blocks = new CacheBlock[waze_num]();
    };
    ~Set() {
        delete[] blocks;
    }
    // Insert new block to SET according to LRU rules.
    void insertNewBlock(unsigned set, unsigned tag) {
        unsigned last_block_index = 0;
        time_t last_block_time = NOT_USED;
        for(int i = 0; i < waze_num; i++) {
            time_t current_block_time = blocks[i]->getTime();
            if(current_block_time == NOT_USED) {
                // Found an unused way
                blocks[i]->setTime();
                blocks[i]->setTag(tag);
                return;
            }
            else {
                if(difftime(current_block_time, last_block_time) > 0) {
                    //current block time < last block time
                    last_block_time = current_block_time;
                    last_block_index = i;
                }
            }
        }
        // update last block time and tag
        blocks[last_block_index]->setTime();
        blocks[last_block_index]->setTag(tag);
    }
    // Check if block associated with given tag exists in SET
    bool isBlockInSet(unsigned tag) {
        for(int i = 0; i < waze_num; i++) {
            if(blocks[i]->getTag() == tag) {
                return true;
            }
        }
        return false;
    }
    // update an existing block associated with given tag
    void updateExistingBlock(unsigned tag) {
        for(int i = 0; i < waze_num; i++) {
            if(blocks[i]->getTag() == tag) {
                blocks[i]->setTime();
                return;
            }
        }
    }
};

class CacheLevel_p
{
	unsigned num_of_blocks; // = cache_size/block_size
	unsigned num_of_sets; // = num_of_blocks/num_of_ways
	unsigned tag_size; // = 32 - (log2(num_of_sets)) - (log2(block_size))
	unsigned int cache_byte_size;
	unsigned int waze_num;
	unsigned int num_of_cycles; // Mask for keeping history in correct size
	unsigned int block_byte_size;

public:
    CacheLevel_p(unsigned  cache_byte_size, unsigned waze_num, unsigned num_of_cycles, unsigned block_byte_size) :
        cache_byte_size(cache_byte_size), waze_num(waze_num), num_of_cycles(num_of_cycles), block_byte_size(block_byte_size) {
        num_of_blocks = cache_byte_size/block_byte_size;
        num_of_sets = num_of_blocks/waze_num;
        tag_size = 30 - int(log2(num_of_sets)) - int(log2(block_byte_size));
    }
    ~CacheLevel_p() = default;
    // Get matching set according to memory address
    unsigned getSet(unsigned address) {
        unsigned offset_bits = int(log2(block_byte_size));
        address = address >> (offset_bits + 2);
        unsigned mask = 1 << (int(log2(num_of_sets) - 1));
        return (address & mask);
    }
    // Get tag from address
    unsigned getTag(unsigned address) {
        unsigned offset_and_set_bits = 32 - tag_size;
        return (address >> offset_and_set_bits);
    }
} ;
typedef class CacheLevel_p * CacheLevel;

enum WritePolicy
{
    WRITE_ERROR_MIN,
	NO_WRITE_ALLOCATE = 0,
	WRITE_ALLOCATE = 1,
    WRITE_ERROR_MAX
};
class Cache
{

	//WritePolicy write_policy;
	unsigned write_policy;
	CacheLevel L1;
	CacheLevel L2;

public:
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;
	unsigned  MemCyc;
	unsigned Bsize;
	unsigned L1Size;
	unsigned L2Size;
	unsigned L1Assoc;
	unsigned L2Assoc;
	unsigned L1Cyc;
	unsigned L2Cyc;
	Cache(const unsigned &MemCyc, const unsigned &BSize, const unsigned &L1Size, const unsigned &L2Size,
		  const unsigned &L1Assoc, const unsigned &L2Assoc, const unsigned &L1Cyc,
		  const unsigned &L2Cyc,   const unsigned &WrAlloc) :
		  MemCyc(MemCyc), Bsize(BSize), L1Size(L1Size), L2Size(L2Size), L1Assoc(L1Assoc), L2Assoc(L2Assoc), L1Cyc(L1Cyc),  L2Cyc(L2Cyc), write_policy(WrAlloc),  L1MissRate(0), L2MissRate(0), avgAccTime(0)
	{
		L1 = new CacheLevel_p(BSize, L1Assoc, L1Cyc, L1Size);
		L2 = new CacheLevel_p(BSize, L2Assoc, L2Cyc, L2Size);
	}
	~Cache() {
	    delete L1;
	    delete L2;
	}

	void cacheRead(unsigned long int num) {
	    return;
	}
	void cacheWrite() {
	    return;
	}
};
int main(int argc, char **argv)
{

	if (argc < 19)
	{
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char *fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good())
	{
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			 L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2)
	{
		string s(argv[i]);
		if (s == "--mem-cyc")
		{
			MemCyc = atoi(argv[i + 1]);
		}
		else if (s == "--bsize")
		{
			BSize = atoi(argv[i + 1]);
		}
		else if (s == "--l1-size")
		{
			L1Size = atoi(argv[i + 1]);
		}
		else if (s == "--l2-size")
		{
			L2Size = atoi(argv[i + 1]);
		}
		else if (s == "--l1-cyc")
		{
			L1Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-cyc")
		{
			L2Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l1-assoc")
		{
			L1Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-assoc")
		{
			L2Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--wr-alloc")
		{
			WrAlloc = atoi(argv[i + 1]);
		}
		else
		{
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	Cache cache(MemCyc, BSize, L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc);
	while (getline(file, line))
	{

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address))
		{
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		switch (operation)
		{
		case 'R':
			cache.cacheRead(num); // Lior
			break;
		case 'W':
			cache.cacheWrite(); // Shai
			break;
		}
	}

	printf("L1miss=%.03f ", cache.L1MissRate);
	printf("L2miss=%.03f ", cache.L2MissRate);
	printf("AccTimeAvg=%.03f\n", cache.avgAccTime);

	return 0;
}
