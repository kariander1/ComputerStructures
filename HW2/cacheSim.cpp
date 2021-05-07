/* 046267 Computer Architecture - Spring 2021 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <math.h>
using std::cerr;
using std::cout;
using std::endl;
using std::FILE;
using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

class CacheBlock_p
{
	time_t last_accessed_time;
	// TODO c'tor & d'tor !
	//vector<CacheBlock>::iterator pos_in_vector;
};
typedef struct CacheBlock_p *CacheBlock;
class Way
{
	CacheBlock *blocks;

	// TODO think of a data structure for this
	//std::vector<CacheBlock> least_recently_used_blocks; // Sorted such that the least used is always at the back

	// TODO c'tor & d'tor !
};

class CacheLevel
{
	// num_of_blocks = cache_size/block_size
	// num_of_sets = num_of_blocks/num_of_ways
	// tag_size = 32 - (log2(num_of_sets)) - (log2(block_size))
	unsigned int cache_byte_size;
	unsigned int association;
	unsigned int num_of_cycles; // Mask for keeping history in correct size
	unsigned int block_byte_size;

public:
};
enum WritePolicy
{
	NO_WRITE_ALLOCATE = 0,
	WRITE_ALLOCATE = 1
};
class Cache
{

	WritePolicy write_policy;
	CacheLevel L1;
	CacheLevel L2;

public:
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;
	Cache(const unsigned &MemCyc, const unsigned &BSize, const unsigned &L1Size, const unsigned &L2Size,
		  const unsigned &L1Assoc, const unsigned &L2Assoc, const unsigned &L1Cyc,
		  const unsigned &L2Cyc, const unsigned &WrAlloc)
	{
		// TODO initialzize cache levels
	}
	~Cache() = delete;

	void cacheRead();
	void cacheWrite();
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
			cache.cacheRead(); // Lior
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
