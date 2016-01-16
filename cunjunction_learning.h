#include <iostream>
#include <fstream>
#include <string>
#include <boost/tokenizer.hpp>
#include <regex>
#include <stdlib.h>
#include <vector>
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <ctime>
#include <queue>
#include <functional>

using namespace std;
using namespace boost;

const int BITSET_SIZE = 100000;
const int QUEUE_LIMIT = 3;

struct conjunction_max
{
	int id;
	int cover;

	bool operator<(const conjunction_max& rhs) const
    {
        return cover < rhs.cover;
    }
};


struct conjunction_min
{
	int id;
	int cover;
	
	bool operator<(const conjunction_min& rhs) const
    {
        return cover > rhs.cover;
    }	
};


std::priority_queue<conjunction_max> max_heap;
std::priority_queue<conjunction_min> min_heap;

int parseFile(const char* file, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, vector<string> *features);
int getdataPositionFomARFF(ifstream *myfile, int *featuresNumber, int *examplesNumber);
bool swap(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap);
bool swap(std::priority_queue<conjunction_min> *min_heap, std::priority_queue<conjunction_max> *max_heap);
void checkQueueMaxLimit(std::priority_queue<conjunction_max> *max_heap);
