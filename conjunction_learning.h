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
#include <limits.h>

using namespace std;
using namespace boost;

const int BITSET_SIZE = 100000;

const string HELP = "Conjunction positive rule learner - Frantisek Malinka\n\
Parameters:\n\
conjunction_learning --train FILE [--test FILE] [--max_rules UINT] [--max_iteration UINT] [--max_queue UINT]\n\
\t--train FILE - path to the ARFF training dataset\n\
\t--test FILE - path to the ARFF testing dataset\n\
\t--max_rules UINT - upper limit for number of rules. DEFAULT 10. IF 0 then cover all positive examples.\n\
\t--max_iteration UINT - upper limit for number of iterations during the procces of finding a rule. DEFAULT 100.\n\
\t--max_queue UINT - upper limit for number of candidates in a priority queue. DEFAULT 10000. IF 0 then queue is unlimited.\n\
\t--verbose - verbose mode\n\
\t--debug - For debug mode use --debug and --verbose\n\
";

struct conjunction_max
{
	int id;
	int cover;
	int P;
	int N;
	boost::dynamic_bitset<> examples;	//vector of coverage
	boost::dynamic_bitset<> toExpand;	//which terms can be expanded
	boost::dynamic_bitset<> whichTerms;	//which terms were used during contruction process
	int length;

	bool operator<(const conjunction_max& rhs) const
    {
        return cover < rhs.cover;
    }
};


struct conjunction_min
{
	int id;
	int cover;
	int P;
	int N;	
	boost::dynamic_bitset<> examples;	//vector of coverage
	boost::dynamic_bitset<> toExpand;	//which terms can be expanded
	boost::dynamic_bitset<> whichTerms;	//which terms were used during contruction process
	int length;
	
	bool operator<(const conjunction_min& rhs) const
    {
        return cover > rhs.cover;
    }	
};

struct statistics
{
	int TP;
	int FP;
	int FN;
	int TN;
	double ACC;
};

std::vector<conjunction_max> train();
void test(std::vector<conjunction_max> *rules);
bool parseCommandLineParametrs(int argc, char* argv[]);
int parseFile(const char* file, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, vector<string> *features);
int getdataPositionFomARFF(ifstream *myfile, int *featuresNumber, int *examplesNumber);
bool swap(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap);
bool swap(std::priority_queue<conjunction_min> *min_heap, std::priority_queue<conjunction_max> *max_heap);
void checkQueueMaxLimit(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap);
std::vector<conjunction_max> initPriorityQueue(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, vector<string> *features, boost::dynamic_bitset<> *classMask);
int countCover(boost::dynamic_bitset<> *example, boost::dynamic_bitset<> *classMask, int *P, int *N);
bool generateNewConjunctions(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, boost::unordered_map<string, int> *CLOSED, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *baseTerms);
string getPrintableConjunction(conjunction_max best, std::vector<string> *features);
string generateHashKey(boost::dynamic_bitset<> *best);
void eraseCoveredExamples(conjunction_max best, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask);
void countPN(boost::dynamic_bitset<> *bitset, int *P, int *N);
bool isBetter(conjunction_max *bestConjunction, conjunction_max *max_heap_top);
void printSettings();
int countTrue(boost::dynamic_bitset<> *bitset);
statistics evaluateDataset(vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *rules);
