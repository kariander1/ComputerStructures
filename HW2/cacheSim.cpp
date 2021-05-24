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

const bool DEBUG_ENABLE = false; // MAKE FALSE BEFORE SUBMISSION!
unsigned int pseudo_cycle = 0;	// Will represent the simulation cycles for LRU calculation.
								// Higher pseudo_cycle means that a block was updated more recently

#define DEBUG(command)                    \
	do                                    \
	{                                     \
		/* safely invoke a system call */ \
		if (DEBUG_ENABLE)                 \
		{                                 \
			command                       \
		}                                 \
	} while (0)

class WayEntry_p // Way line (fully associative every way has one set)
{

public:
	bool valid;
	bool dirty;
	// No need to save set, since set is the index number
	unsigned int tag;
	double last_accessed_time; // In units of pseudo cycles
	unsigned long int address; // Save the actual address that was requested for debugging purposes
							   // and for making it easier to access different levels later

	WayEntry_p(const unsigned &num_of_sets)
		: valid(false),
		  dirty(false),
		  tag(0),
		  last_accessed_time(0),
		  address(0){};

	// Print for debugging purposes
	friend ostream &operator<<(ostream &os, const WayEntry_p &entry)
	{

		os << entry.valid << " " << std::setfill('0') << std::setw(4) << entry.last_accessed_time << " "
		   << entry.dirty << " 0x" << std::setfill('0') << std::setw(8)
		   << std::hex << entry.tag << " (0x" << std::setfill('0') << std::setw(8) << entry.address << std::dec << ')';
		return os;
	}
};
typedef struct WayEntry_p *WayEntry;
class Way_p
{

	unsigned int num_of_sets; // Number of sets in way

public:
	WayEntry *entries; // Entries, set number is index number
	Way_p(const unsigned &num_of_sets) : num_of_sets(num_of_sets)
	{
		// Allocate all sets
		entries = new WayEntry[num_of_sets];
		for (size_t i = 0; i < num_of_sets; i++)
		{
			entries[i] = new WayEntry_p(0);
		}
	}
	~Way_p()
	{
		// Destruct all sets
		for (size_t i = 0; i < num_of_sets; i++)
		{
			delete entries[i];
		}

		delete[] entries;
	}
	// Print for debugging purposes
	friend ostream &operator<<(ostream &os, const Way_p &way)
	{
		os << "Set V LRU  D Tag        (Address)" << endl;
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
	unsigned int num_of_ways;	 // = num of ways in cache level, 2^(LXAssoc)
	unsigned num_of_blocks;		 // = cache_size/block_size
	unsigned num_of_sets;		 // = num_of_blocks/num_of_ways
	unsigned tag_size;			 // = 32 - (log2(num_of_sets)) - (log2(block_size))
	unsigned short cache_num;	 // Cache number for printing in debug
	unsigned int block_size_log; //  Block size logarithmic scale
	Way *ways;					 // Ways in cache level

public:
	unsigned int num_of_cycles;
	double miss_count;
	double access_count;
	CacheLevel(const unsigned &cache_size_log, const unsigned &waze_num_log, const unsigned &num_of_cycles,
			   const unsigned &block_size_log, const unsigned &cache_num)
		: num_of_ways(pow(2, waze_num_log)),
		  num_of_blocks(pow(2, cache_size_log - block_size_log)), // Calc in powers of 2
		  num_of_sets(num_of_blocks / num_of_ways),				  // 0 associativeness is Direct-mapping
		  tag_size(32 - int(log2(num_of_sets)) - block_size_log),
		  num_of_cycles(num_of_cycles),
		  block_size_log(block_size_log),
		  cache_num(cache_num),
		  miss_count(0),
		  access_count(0)
	{
		// Allocate ways
		ways = new Way[num_of_ways];
		for (size_t i = 0; i < num_of_ways; i++)
		{
			ways[i] = new Way_p(num_of_sets);
		}
	}
	~CacheLevel()
	{
		// Destruct all ways
		for (size_t i = 0; i < num_of_ways; i++)
		{
			delete ways[i];
		}
		delete[] ways;
	}
	// Print for debugging purposes
	friend ostream &operator<<(ostream &os, const CacheLevel &cache_level)
	{

		for (size_t i = 0; i < cache_level.num_of_ways; i++)
		{
			os << "		WAY " << i << endl;
			os << (*cache_level.ways[i]);
		}
		return os;
	}

	//blockExists: Check if block associated with given address exists and return it
	WayEntry blockExists(const unsigned long int &address)
	{
		// Get set & tag associated will current address
		const unsigned int set = getSet(address);
		const unsigned int tag = getTag(address);

		// Search all ways in the specific set for the requested tag
		for (int i = 0; i < num_of_ways; i++)
		{

			// Print for debugging
			DEBUG(std::cout << "L" << cache_num << " Looking for address 0x" <<std::hex << address << " in way N." << i;
				  std::cout << " With Set N." << set << " Tag " << tag << endl;);

			// Foreach entry, fetch the relevant entry with the set number
			WayEntry entry = ways[i]->entries[set];

			// If block is valig and the tag equals, then this is a requested block
			if (entry->valid && entry->tag == tag)
			{
				// HIT!
				return entry;
			}
		}
		// MISS!
		return NULL;
	}

	//getEntryForNewBlock: Get an entry to allocate for a new address.
	// Returns the entry that is the victim or an empty entry that can be allocated
	WayEntry getEntryForNewBlock(const unsigned int &address)
	{
		// Get set & tag associated will current address
		const unsigned int set = getSet(address);
		const unsigned int tag = getTag(address);
		unsigned int last_block_time = __UINT32_MAX__;
		WayEntry entry_to_update;

		for (int i = 0; i < num_of_ways; i++)
		{
			// Foreqch way, check the current entry in the set associated with the address
			WayEntry entry = ways[i]->entries[set];

			if (!entry->valid)
			{
				// Found an unused block!
				return entry;
			}
			else
			{
				// Check if current block is used later
				if (entry->last_accessed_time < last_block_time)
				{
					//current block time < last block time
					last_block_time = entry->last_accessed_time;
					entry_to_update = entry;
				}
			}
		}

		return entry_to_update;
	}
	// Get matching set according to memory address
	unsigned int getSet(unsigned long int address)
	{

		// 	block_size_log is logarithmic, and represent the num of bits reserved for offset
		address = address >> (block_size_log);
		const unsigned int mask = (1 << int(log2(num_of_sets))) - 1;

		// Apply mask
		return (address & mask);
	}
	// Get tag from address
	unsigned getTag(unsigned long int address)
	{
		// Shift right according to num of bits reserved to set and offset bits
		unsigned offset_and_set_bits = 32 - tag_size;
		return (address >> offset_and_set_bits);
	}
};

// Enumrable for Write-Miss policy
enum WritePolicy
{
	NO_WRITE_ALLOCATE = 0,
	WRITE_ALLOCATE = 1,
};

// Class representating the whole cache memory with both levels
class Cache
{

	WritePolicy write_policy;

public:
	double t_access_total; // Total access time in cache
	unsigned MemCyc;	   // Cycles we need to access memory
	CacheLevel L1;		   // L1 cache
	CacheLevel L2;		   // L2 cache

	Cache(const unsigned &MemCyc, const unsigned &BSize, const unsigned &L1Size, const unsigned &L2Size,
		  const unsigned &L1Assoc, const unsigned &L2Assoc, const unsigned &L1Cyc,
		  const unsigned &L2Cyc, const unsigned &WrAlloc)
		: t_access_total(0), MemCyc(MemCyc),
		  write_policy(static_cast<WritePolicy>(WrAlloc)),
		  L1(CacheLevel(L1Size, L1Assoc, L1Cyc, BSize, 1)),
		  L2(CacheLevel(L2Size, L2Assoc, L2Cyc, BSize, 2))
	{
	}
	// Special handling of fetching a block to L1. Return the entry that was created due to this operation
	WayEntry fetchToL1(const unsigned long int &address)
	{
		// Find a victim/new block for the new address
		WayEntry l1_victim = L1.getEntryForNewBlock(address);
		if (l1_victim->valid)
		{
			// Entry was valid there!!
			// Need to update L2 with new data
			WayEntry entry_l2 = L2.blockExists(l1_victim->address);
			if (l1_victim->dirty)
			{
				// Due to inclusiveness
				DEBUG(std::cout << "L1 Victim is dirty! Change record in L2 to dirty and updating LRU" << std::endl;);

				entry_l2->dirty = true;
				entry_l2->last_accessed_time = pseudo_cycle++;
			}
		}
		// If entry was invalid we'll get here immediately, perform here the actual getching of the new block:
		l1_victim->valid = true;
		l1_victim->tag = L1.getTag(address);
		l1_victim->last_accessed_time = pseudo_cycle++;
		l1_victim->address = address;
		l1_victim->dirty = false;
		return l1_victim;
	}
	// fetchBlockFromMemory: fetch a new memory block from memory to both L1 & L2
	// Return the L1 victim/block that was evicted and allocated
	WayEntry fetchBlockFromMemory(const unsigned long int &address)
	{
		// Fetch block from memory to L2 and L1

		// First fetch the block to L1
		WayEntry l1_victim = fetchToL1(address);

		// Find a victim/new block to L2:
		WayEntry l2_victim = L2.getEntryForNewBlock(address);
		if (l2_victim->valid)
		{
			// There was a valid entry in L2!
			// Need to evict it to memory..
			// First check if the line is dirty in L1
			DEBUG(std::cout << "L2 Found Victim! "<< *l2_victim<<" Searching if block exists in L1" << std::endl;);

			// Search the victim in L1 cache:
			WayEntry entry_l1 = L1.blockExists(l2_victim->address);
			if (entry_l1)
			{
				// Oh dear.. in L1 the line that is about to get evicted from L2 exists and is valid, ivalidate it
				DEBUG(std::cout << "Evicted block from L2 Exists in L1, invalidate it" << std::endl;);
				entry_l1->valid = false;
			}
		}

		// Perform the actual fetch
		l2_victim->valid = true;
		l2_victim->tag = L2.getTag(address);
		l2_victim->last_accessed_time = pseudo_cycle++;
		l2_victim->address = address;
		l2_victim->dirty = false;

		return l1_victim;
	}
	// cacheRead: perform a cacheRead operation
	void cacheRead(const unsigned long int &address)
	{
		// We access L1 first, so increment with its cycles
		t_access_total += L1.num_of_cycles;
		L1.access_count++;

		// Search block in L1
		WayEntry requested_entry = L1.blockExists(address);
		if (!requested_entry)
		{
			// L1 Read Miss! Try next level
			DEBUG(std::cout << "L1 Read Miss!" << std::endl;);
			L1.miss_count++;

			// About to access L2, increment its cycles
			t_access_total += L2.num_of_cycles;
			L2.access_count++;

			// Search for block in L2:
			requested_entry = L2.blockExists(address);
			if (!requested_entry)
			{
				// L2 Read Miss! Access memory
				DEBUG(std::cout << "L2 Read Miss!" << std::endl;);
				L2.miss_count++;
				t_access_total += MemCyc;
				// Need to fetch block to both levels:
				fetchBlockFromMemory(address);
			}
			else
			{
				// L2 HIT!
				// This updates L1 with cache line
				DEBUG(std::cout << "L2 HIT!" << std::endl;);

				// There was a HIT in L2, therfore we need to copy the block to L1 as well
				fetchToL1(address);
				// Hit in L2, we update the LRU
				requested_entry->last_accessed_time = pseudo_cycle++;
			}
		}
		else
		{
			DEBUG(std::cout << "L1 HIT!" << std::endl;);

			// L1 HIT!
			// Update LRU:
			requested_entry->last_accessed_time = pseudo_cycle++;
		}
		return;
	}
	// cacheWrite: perform a cacheWrite operation
	void cacheWrite(const unsigned long int &address)
	{
		// Always access L1
		t_access_total += L1.num_of_cycles;
		L1.access_count++;

		// Search or block in L1:
		WayEntry requested_entry = L1.blockExists(address);
		if (!requested_entry)
		{
			// L1 Write Miss! Try next level
			DEBUG(std::cout << "L1 Write Miss!" << std::endl;);
			L1.miss_count++;
			t_access_total += L2.num_of_cycles;
			L2.access_count++;

			// Search block in L2:
			requested_entry = L2.blockExists(address);
			if (!requested_entry)
			{
				// L2 Write Miss! Access memory
				DEBUG(std::cout << "L2 Write Miss!" << std::endl;);
				L2.miss_count++;
				t_access_total += MemCyc;

				// If we are in WRITE_ALLOCATE then we should fetch block to both levels,
				// and mark entry as dirty in L1 since we write to it
				if (write_policy == WRITE_ALLOCATE)
				{
					fetchBlockFromMemory(address)->dirty = true;
				}
			}
			else
			{
				// L2 HIT! We are in write-back policy, make entry dirty in L2
				// This block didn't exist in L1, so no need to perform any updates
				DEBUG(std::cout << "L2 HIT!" << std::endl;);

				// If we are in NO_WRITE_ALLOCATE policy then we don't bring the block to L1, therefore L2 is dirty
				if (write_policy == NO_WRITE_ALLOCATE)
				{
					requested_entry->dirty = true;
				}
				else
				{
					// Write block to L1 as well
					fetchToL1(address)->dirty = true;
				}
				// update LRU
				requested_entry->last_accessed_time = pseudo_cycle++;
			}
		}
		else
		{
			// L1 HIT! We are in write-back policy, make entry dirty
			DEBUG(std::cout << "L1 HIT!" << std::endl;);
			requested_entry->dirty = true;
			requested_entry->last_accessed_time = pseudo_cycle++;
		}
		return;
	}
	// Print for debug
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
		DEBUG(cout << "operation: " << operation;);

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		DEBUG(cout << ", address (hex)" << cutAddress;);

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		DEBUG(cout << " (dec) " << num << endl;);

		switch (operation)
		{
		case 'r':
			cache.cacheRead(num);
			break;
		case 'w':
			cache.cacheWrite(num);
			break;
		}

		DEBUG(std::cout << cache;);
		DEBUG(printf("L1miss=%.03f ", cache.L1.miss_count / cache.L1.access_count););
		DEBUG(printf("L2miss=%.03f ", cache.L2.miss_count / cache.L2.access_count););
		DEBUG(printf("AccTimeAvg=%.03f\n", cache.t_access_total / cache.L1.access_count););
		
	}

	printf("L1miss=%.03f ", cache.L1.miss_count / cache.L1.access_count);
	printf("L2miss=%.03f ", cache.L2.miss_count / cache.L2.access_count);
	printf("AccTimeAvg=%.03f\n", cache.t_access_total / cache.L1.access_count);

	return 0;
}
