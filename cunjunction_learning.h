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

using namespace std;
using namespace boost;

const int BITSET_SIZE = 100000;

int parseFile(const char* file, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, vector<string> *features);
int getdataPositionFomARFF(ifstream *myfile, int *featuresNumber, int *examplesNumber);
//int parseData(ifstream *myfile, vector<string> *features, *featureExample);
