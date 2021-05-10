/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <time.h>
#include <iomanip>
using std::cerr;
using std::cout;
using std::endl;
using std::FILE;
using std::ifstream;
using std::ostream;
using std::string;
using std::stringstream;
using std::vector;

#define NOT_USED (-1)	 // for when block hasn't been used yet
const bool DEBUG = false; // MAKE FALSE BEFORE SUBMISSION!
unsigned int pseudo_cycle = 0;

void logDebug(const string &text)
{
	if (DEBUG)
	{
		std::cout << text;
	}
}

/* I Don't think we need this
class CacheBlock_p
{
	time_t last_accessed_time;
	unsigned tag;

public:
	CacheBlock_p()
	{
		last_accessed_time = NOT_USED;
		tag = NOT_USED;
	}
	~CacheBlock_p() = default;
	time_t getTime()
	{
		return last_accessed_time;
	}
	void setTime()
	{
		time(&last_accessed_time);
	}
	unsigned getTag()
	{
		return tag;
	}
	void setTag(unsigned new_tag)
	{
		tag = new_tag;
	}
};
*/
typedef struct CacheBlock_p *CacheBlock;

class WayEntry_p // Way line (fully associative every way has one set)
{

public:
	bool valid;
	bool dirty;
	// No need to save set, since set is the index number
	//unsigned waze_num_log; //It's a joke, shai
	unsigned int tag;
	double last_accessed_time;
	unsigned long int address;
	//CacheBlock block;

	WayEntry_p(const unsigned &num_of_sets) : valid(false), dirty(false), tag(0), last_accessed_time(0), address(0){
																											 //blocks = new CacheBlock[waze_num_log]();
																										 };
	~WayEntry_p()
	{
		//delete[] blocks;
	}

	friend ostream &operator<<(ostream &os, const WayEntry_p &entry)
	{

		os << entry.valid << " " << std::setfill('0') << std::setw(4) << entry.last_accessed_time << " " << entry.dirty << " 0x" << std::setfill('0') << std::setw(8) << std::hex << entry.tag << ' ' << std::dec;
		return os;
	}
};
typedef struct WayEntry_p *WayEntry;
class Way_p
{
	unsigned int num_of_sets;

public:
	WayEntry *entries; // index number will be the set number
	Way_p(const unsigned &num_of_sets) : num_of_sets(num_of_sets)
	{
		entries = new WayEntry[num_of_sets];
		for (size_t i = 0; i < num_of_sets; i++)
		{
			entries[i] = new WayEntry_p(0);
		}
	}
	~Way_p()
	{
		for (size_t i = 0; i < num_of_sets; i++)
		{
			delete entries[i];
		}

		delete[] entries;
	}
	friend ostream &operator<<(ostream &os, const Way_p &way)
	{
		os << "Set V L    D Tag" << endl;
		for (size_t i = 0; i < way.num_of_sets; i++)
		{
			os << i << "   " << (*way.entries[i]) << endl;
		}
		return os;
	}
};
typedef struct Way_p *Way;

class CacheLevel
{
	unsigned int num_of_ways;
	unsigned num_of_blocks; // = cache_size/block_size
	unsigned num_of_sets;	// = num_of_blocks/num_of_ways
	unsigned tag_size;		// = 32 - (log2(num_of_sets)) - (log2(block_size))
	unsigned short cache_num;

	//unsigned int cache_byte_size;

	unsigned int block_size_log;
	Way *ways;

public:
	unsigned int num_of_cycles;
	double miss_count;
	double access_count;
	CacheLevel(const unsigned &cache_size_log, const unsigned &waze_num_log, const unsigned &num_of_cycles,
			   const unsigned &block_size_log, const unsigned &cache_num) : num_of_ways(pow(2, waze_num_log)),
																			num_of_blocks(pow(2, cache_size_log - block_size_log)),
																			num_of_sets(num_of_blocks / num_of_ways), // 0 associativeness is Direct-mapping
																			tag_size(32 - int(log2(num_of_sets)) - block_size_log),
																			num_of_cycles(num_of_cycles),
																			block_size_log(block_size_log),
																			cache_num(cache_num),
																			miss_count(0),
																			access_count(0)
	{
		ways = new Way[num_of_ways];
		for (size_t i = 0; i < num_of_ways; i++)
		{
			ways[i] = new Way_p(num_of_sets);
		}
	}
	~CacheLevel()
	{
		for (size_t i = 0; i < num_of_ways; i++)
		{
			delete ways[i];
		}
		delete[] ways;
	}
	friend ostream &operator<<(ostream &os, const CacheLevel &cache_level)
	{

		for (size_t i = 0; i < cache_level.num_of_ways; i++)
		{
			os << "		WAY " << i << endl;
			os << (*cache_level.ways[i]);
		}
		return os;
	}

	// Check if block associated with given tag exists in SET
	WayEntry blockExists(const unsigned long int &address)
	{
		const unsigned int set = getSet(address);
		const unsigned int tag = getTag(address);

		// Search all ways in the specific set for the requested tag
		for (int i = 0; i < num_of_ways; i++)
		{
			if (DEBUG)
			{
				std::cout << "L" << cache_num << " Looking for address " << address << " in way N." << i;
				std::cout << " With Set N." << set << " Tag " << tag << endl;
			}
			WayEntry entry = ways[i]->entries[set];
			if (entry->valid && entry->tag == tag)
			{
				// HIT!
				return entry;
			}
		}
		// MISS!
		return NULL;
	}
	/*
	bool isBlockInSet(unsigned tag)
	{
		
		for (int i = 0; i < num_of_ways; i++)
		{
			if (ways[i]->entries getTag() == tag)
			{
				return true;
			}
		}
		return false;
	}
*/
	// Insert new block to SET according to LRU rules.

	WayEntry getEntryForNewBlock(const unsigned int &address)
	{
		const unsigned int set = getSet(address);
		const unsigned int tag = getTag(address);

		WayEntry entry_to_update;
		unsigned int last_block_time = __UINT32_MAX__;
		for (int i = 0; i < num_of_ways; i++)
		{
			WayEntry entry = ways[i]->entries[set];

			if (!entry->valid)
			{
				// Found an unused way
				//blocks[i]->setTime();
				//blocks[i]->setTag(tag);
				//entry->tag = tag;
				//entry->valid = true;
				//entry->last_accessed_time= time(NULL);
				return entry; // Return NULL since Cache doesn't need to do anything with line evicted
			}
			else
			{
				if (entry->last_accessed_time < last_block_time)
				{
					//current block time < last block time
					last_block_time = entry->last_accessed_time;
					entry_to_update = entry;
				}
			}
		}
		// update last block time and tag
		// Might need to evict block!

		//	entry_to_update->last_accessed_time = pseudo_cycle;
		//	entry_to_update->tag = tag;
		return entry_to_update;
	}

	// update an existing block associated with given tag
	/*
	void updateExistingBlock(unsigned tag)
	{
		for (int i = 0; i < waze_num_log; i++)
		{
			if (blocks[i]->getTag() == tag)
			{
				blocks[i]->setTime();
				return;
			}
		}
	}
	*/
	// Get matching set according to memory address
	unsigned int getSet(unsigned long int address)
	{

		// 	block_size_log is logarithmic, and represent the num of bits reserved for offset
		address = address >> (block_size_log);
		const unsigned int mask = (1 << int(log2(num_of_sets))) - 1;

		return (address & mask);
	}
	// Get tag from address
	unsigned getTag(unsigned long int address)
	{
		unsigned offset_and_set_bits = 32 - tag_size;
		return (address >> offset_and_set_bits);
	}
};

enum WritePolicy
{
	WRITE_ERROR_MIN,
	NO_WRITE_ALLOCATE = 0,
	WRITE_ALLOCATE = 1,
	WRITE_ERROR_MAX
};
class Cache
{

	WritePolicy write_policy;
	//unsigned write_policy;

public:
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;
	double t_access;
	unsigned MemCyc;
	CacheLevel L1;
	CacheLevel L2;

	Cache(const unsigned &MemCyc, const unsigned &BSize, const unsigned &L1Size, const unsigned &L2Size,
		  const unsigned &L1Assoc, const unsigned &L2Assoc, const unsigned &L1Cyc,
		  const unsigned &L2Cyc, const unsigned &WrAlloc) : t_access(0), MemCyc(MemCyc),
															write_policy(static_cast<WritePolicy>(WrAlloc)), L1MissRate(0), L2MissRate(0),
															avgAccTime(0),
															L1(CacheLevel(L1Size, L1Assoc, L1Cyc, BSize, 1)),
															L2(CacheLevel(L2Size, L2Assoc, L2Cyc, BSize, 2))
	{
	}
	~Cache() = default; // Will call d'tor automatically of cache levels

	WayEntry fetchToL1(const unsigned long int &address)
	{
		WayEntry l1_victim = L1.getEntryForNewBlock(address);
		if (l1_victim->valid)
		{
			// Entry was valid there!!
			// Need to update L2 with new data
			WayEntry entry_l2 = L2.blockExists(l1_victim->address);
			if (l1_victim->dirty)
			{
				// Due to inclusiveness
				std::cout << "L1 Victim is dirty! Change record in L2 to dirty" << std::endl ;
				entry_l2->dirty = true;
			}
		}
		// If entry was invalid OR entry was valid but not dirty, we'll get here immediately
		l1_victim->valid = true;
		l1_victim->tag = L1.getTag(address);
		l1_victim->last_accessed_time = pseudo_cycle;
		l1_victim->address = address;
		l1_victim->dirty = false;
		return l1_victim;
	}
	WayEntry fetchBlockFromMemory(const unsigned long int &address)
	{
		// Fetch block from memory to L2 and L1

		WayEntry l1_victim = fetchToL1(address);
		WayEntry l2_victim = L2.getEntryForNewBlock(address);
		if (l2_victim->valid)
		{
			// There was a valid entry in L2!
			// Need to evict it to memory..
			// First check if the line is dirty in L1

			if (DEBUG)
			{
				std::cout << "L2 Found Victim! Searching if block exists in L1" << std::endl ;
			}
			WayEntry entry_l1 = L1.blockExists(l2_victim->address);
			if (entry_l1)
			{
					if (DEBUG)
					{
						std::cout << "Evicted block from L2 Exists in L1, invalidate it" << std::endl ;
					}
				// Oh dear.. in L1 the line that is about to get evicted exists and is valid, ivalidate it
				// And update L2 with the data from L1
				entry_l1->valid = false;

				// If we are about to update L2 with the new data, should access time update?
				// I think not..
			}
		}

		l2_victim->valid = true;
		l2_victim->tag = L2.getTag(address);
		l2_victim->last_accessed_time = pseudo_cycle;
		l2_victim->address = address;
		l2_victim->dirty = false;
		// Should change to dirty as well?

		// From this stage, the entry in L2 is updated, should update also L1:
		return l1_victim;
	}
	void cacheRead(const unsigned long int &address)
	{
		t_access += L1.num_of_cycles;
		L1.access_count++;
		WayEntry requested_entry = L1.blockExists(address);
		if (!requested_entry)
		{
			if (DEBUG)
			{
				std::cout << "L1 Read Miss!" << std::endl ;
			}
			// L1 Read Miss! Try next level
			L1.miss_count++;
			t_access += L2.num_of_cycles;
			L2.access_count++;
			requested_entry = L2.blockExists(address);
			if (!requested_entry)
			{
				if (DEBUG)
				{
					std::cout << "L2 Read Miss!" << std::endl ;
				}
				// L2 Read Miss! Access memory
				L2.miss_count++;
				t_access += MemCyc;
				fetchBlockFromMemory(address);
			}
			else
			{
				if (DEBUG)
				{
					std::cout << "L2 HIT!" << std::endl ;
				}
				// L2 HIT!
				// This updates L1 with cache line

				fetchToL1(address);
				requested_entry->last_accessed_time = pseudo_cycle;
			}
		}
		else
		{
			if (DEBUG)
			{
				std::cout << "L1 HIT!" << std::endl ;
			}
			// L1 HIT!
			requested_entry->last_accessed_time = pseudo_cycle;
		}
		return;
	}
	void cacheWrite(const unsigned long int &address)
	{
		// Always access L1
		t_access += L1.num_of_cycles;
		L1.access_count++;
		WayEntry requested_entry = L1.blockExists(address);
		if (!requested_entry)
		{
			if (DEBUG)
			{
				std::cout << "L1 Write Miss!" << std::endl ;
			}
			L1.miss_count++;
			// L1 Write Miss! Try next level
			t_access += L2.num_of_cycles;
			L2.access_count++;
			requested_entry = L2.blockExists(address);
			if (!requested_entry)
			{
				if (DEBUG)
				{
					std::cout << "L2 Write Miss!" << std::endl ;
				}
				// L2 Write Miss! Access memory
				L2.miss_count++;
				t_access += MemCyc;
				if (write_policy == WRITE_ALLOCATE)
				{
					fetchBlockFromMemory(address)->dirty = true;
				}
			}
			else
			{

				if (DEBUG)
				{
					std::cout << "L2 HIT!" << std::endl ;
				}
				// L2 HIT! We are in write-back policy, make entry dirty in L2
				// This block didn't exist in L1, so no need to perform any updates
				//
				if (write_policy == NO_WRITE_ALLOCATE)
				{
					requested_entry->dirty = true;
				}
				else
				{
					// Write block to L1 as well
					fetchToL1(address)->dirty = true;
				}
				requested_entry->last_accessed_time = pseudo_cycle;
			}
		}
		else
		{
			if (DEBUG)
			{
				std::cout << "L1 HIT!" << std::endl ;
			}
			// L1 HIT! We are in write-back policy, make entry dirty
			requested_entry->dirty = true;
			requested_entry->last_accessed_time = pseudo_cycle;
		}
		return;
	}
	friend ostream &operator<<(ostream &os, const Cache &cache)
	{
		os << "-----------------------------------------------------------------" << endl;
		os << "Cache L1" << endl
		   << endl;
		os << cache.L1 << endl
		   << endl;
		;
		os << "-----------------------------------------------------------------" << endl;
		os << "Cache L2" << endl
		   << endl;
		os << cache.L2 << endl;
		os << "-----------------------------------------------------------------" << endl;
		return os;
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
	string line = "";
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
		string address = "";
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
		case 'r':
			cache.cacheRead(num); // Lior
			break;
		case 'w':
			cache.cacheWrite(num); // Shai
			break;
		}

		if (DEBUG)
		{
			std::cout << cache;
		}
		pseudo_cycle++;
	}

	printf("L1miss=%.03f ", cache.L1.miss_count / cache.L1.access_count);
	printf("L2miss=%.03f ", cache.L2.miss_count / cache.L2.access_count);
	printf("AccTimeAvg=%.03f\n", cache.t_access / cache.L1.access_count);

	return 0;
}
