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
#include <boost/unordered_map.hpp>
#include <unordered_map>

using namespace std;
using namespace boost;

const int BITSET_SIZE = 100000;
const int QUEUE_LIMIT = 10000;

struct conjunction_max
{
	int id;
	int cover;	
	boost::dynamic_bitset<> examples;	//vector of coverage
	boost::dynamic_bitset<> toExpand;	//which terms can be expanded
	boost::dynamic_bitset<> whichTerms;	//which terms were used during contruction process

	bool operator<(const conjunction_max& rhs) const
    {
        return cover < rhs.cover;
    }
};


struct conjunction_min
{
	int id;
	int cover;	
	boost::dynamic_bitset<> examples;	//vector of coverage
	boost::dynamic_bitset<> toExpand;	//which terms can be expanded
	boost::dynamic_bitset<> whichTerms;	//which terms were used during contruction process
	
	bool operator<(const conjunction_min& rhs) const
    {
        return cover > rhs.cover;
    }	
};

int parseFile(const char* file, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, vector<string> *features);
int getdataPositionFomARFF(ifstream *myfile, int *featuresNumber, int *examplesNumber);
bool swap(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap);
bool swap(std::priority_queue<conjunction_min> *min_heap, std::priority_queue<conjunction_max> *max_heap);
void checkQueueMaxLimit(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap);
std::vector<conjunction_max> initPriorityQueue(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, vector<string> *features, boost::dynamic_bitset<> *classMask);
int countCover(boost::dynamic_bitset<> *example, boost::dynamic_bitset<> *classMask);
bool generateNewConjunctions(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, boost::unordered_map<string, int> *CLOSED, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *baseTerms);
string getPrintableConjunction(conjunction_max best, std::vector<string> *features);
string generateHashKey(boost::dynamic_bitset<> *best);
