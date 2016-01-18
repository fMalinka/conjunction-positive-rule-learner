#include "conjunction_learning.h"

//Global parametrs with DEFAULTs values
string TRAIN;
bool TRAIN_FLAG = 0;
string TEST;
bool TEST_FLAG = 0;
int QUEUE_LIMIT = 10000;
bool QUEUE_UNLIMITED = false;
int RULES_LIMIT = 10;
int ITERATE_LIMIT = 100;


int main(int argc, char* argv[])
{
	if(!parseCommandLineParametrs(argc, argv))
	{
		cerr << HELP;
		return EXIT_FAILURE;
	}
	else
	{
		cout << "[train " << TRAIN  << "]" << endl;
		if(TEST_FLAG)
			cout << "[test " << TEST << "]" << endl;
		cout << "[max_rules " << RULES_LIMIT << "]" << endl;
		cout << "[max_iteration " << ITERATE_LIMIT << "]" << endl;
		if(QUEUE_UNLIMITED)
			cout << "[max_queue UNLIMITED]" << endl;
		else
			cout << "[max_queue " << QUEUE_LIMIT << "]" << endl;
		cout << endl;
	}
	
	clock_t begin = clock();
	string myfile(TRAIN);
	const char* c_myfile = myfile.c_str();
	vector<boost::dynamic_bitset<> > featureBitSet;
	boost::dynamic_bitset<> classMask;
	vector<string> features;
	std::priority_queue<conjunction_max> max_heap;
	std::priority_queue<conjunction_min> min_heap;
	std::vector<conjunction_max> bestConjunctionS;
	boost::unordered_map<string, int> CLOSED;
	
	if(parseFile(c_myfile, &featureBitSet, &classMask, &features))
	{
		conjunction_max bestConjunction;
		for(int irule = 0; irule < RULES_LIMIT; ++irule)
		{
			std::vector<conjunction_max> baseTerms = initPriorityQueue(&max_heap, &featureBitSet, &features, &classMask);		
			cout << "[RULE " << irule+1 << "]" <<" [" << classMask.size() << "]" <<endl;
			//generate all options
			bestConjunction = max_heap.top();
			//cout << getPrintableConjunction(bestConjunction, &features) <<  " | cover PN " << bestConjunction.cover <<endl;
			for(int i = 0; i < ITERATE_LIMIT; ++i)
			{
				if(max_heap.empty())
					break;
				
				generateNewConjunctions(&max_heap, &featureBitSet, &CLOSED, &classMask, &baseTerms);
				if(!QUEUE_UNLIMITED)
					checkQueueMaxLimit(&max_heap, &min_heap);
				if(bestConjunction.cover <= max_heap.top().cover)
					bestConjunction = max_heap.top();
				//cout << getPrintableConjunction(bestConjunction, &features) <<  " | cover PN " << bestConjunction.cover << " - i:" << i <<" |queue| " << max_heap.size() <<endl;
			}
			cout << getPrintableConjunction(bestConjunction, &features) <<  " | cover PN " << bestConjunction.cover <<endl;
			eraseCoveredExamples(bestConjunction, &featureBitSet, &classMask);
			while(!max_heap.empty())
				max_heap.pop();
			bestConjunctionS.push_back(bestConjunction);
			CLOSED.clear();
			if(classMask.empty())
				break;
		}
		
	}
	else
	{
		cerr << "Unable to read file " << endl;
		return EXIT_FAILURE;
	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	cout  << "Elapsed time: " << elapsed_secs << endl;
	return EXIT_SUCCESS;		
}

bool parseCommandLineParametrs(int argc, char* argv[])
{
	if(argc <= 2)
		return false;
	else
	{
		for(int i = 0; i < argc; ++i)
		{
			if(std::string(argv[i]) == "--train")
			{
				TRAIN_FLAG = true;
				TRAIN = std::string(argv[i+1]);
			}
			else if(std::string(argv[i]) == "--test")
			{
				TEST_FLAG = true;
				TEST = std::string(argv[i+1]);
			}
			else if(std::string(argv[i]) == "--max_rules")
			{
				int rules = atoi(argv[i+1]);
				if(rules != 0)
					RULES_LIMIT = rules;
				else
					return false;
			}
			else if(std::string(argv[i]) == "--max_iteration")
			{
				int iterate = atoi(argv[i+1]);
				if(iterate != 0)
					ITERATE_LIMIT = iterate;
				else
					return false;
			}
			else if(std::string(argv[i]) == "--max_queue")
			{
				int queue = atoi(argv[i+1]);
				if(queue != 0)
					QUEUE_LIMIT = queue;
				else
					QUEUE_UNLIMITED = true;
			}
		}
		if(!TRAIN_FLAG)
			return false;
	}
	return true;
}

int parseFile(const char* file, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, vector<string> *features)
{
	ifstream myfile(file);
	if(myfile.is_open())
	{
		//boost separetor
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep(",");
		long int lineNumber = 0;
		int lineCollumn = 0;
		string line;
		
		cout << "Parsing..." << endl;
 		while(myfile.good())
		{
			getline(myfile, line);
			//iterate to data
			if (std::regex_match(line, std::regex("@data") ))
			{
				for(vector<boost::dynamic_bitset<> >::iterator ibit = featureBitSet->begin(); ibit != featureBitSet->end();  ++ibit)
					(*ibit).resize(BITSET_SIZE, false);
				getline(myfile, line);
				int resize_circle = 1;
				while(myfile.good())
				{
					if(lineNumber % (BITSET_SIZE*resize_circle) == 0)
					{
						++resize_circle;
						for(vector<boost::dynamic_bitset<> >::iterator ibit = featureBitSet->begin(); ibit != featureBitSet->end();  ++ibit)
							(*ibit).resize(BITSET_SIZE*resize_circle, false);
					}
					tokenizer tokens(line, sep);
					int lineCollumn = 0;
					for(tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
					{		
						if(*tok_iter == "'+'")
						{
							(*featureBitSet)[lineCollumn].set(lineNumber, true);							
						}
						++lineCollumn;
					}
					getline(myfile, line);
					++lineNumber;
				}
				for(vector<boost::dynamic_bitset<> >::iterator ibit = featureBitSet->begin(); ibit != featureBitSet->end();  ++ibit)
							(*ibit).resize(lineNumber, false);
				//class
				*classMask = featureBitSet->back();
				featureBitSet->pop_back();
				features->pop_back();
			}
			else
			{
				std::smatch m;
				std::regex e("@attribute\\s+'([^']+)'");   // matches words beginning by "sub"
				std::regex_search(line, m, e);
				if(m.size() >=1)
				{
					features->push_back(m[1]);
					featureBitSet->push_back(boost::dynamic_bitset<>());
					++lineCollumn;
				}
			}
		}		
	}
	else
	{
		cerr << "Unable to read file " << file << endl;
		return false;
	}
	return true;
}

bool swap(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap)
{
	while(!max_heap->empty())
	{
		conjunction_max tmp = max_heap->top();
		conjunction_min tmp_min;
		
		tmp_min.id = tmp.id;
		tmp_min.cover = tmp.cover;
		tmp_min.examples = tmp.examples;
		tmp_min.toExpand = tmp.toExpand;
		tmp_min.whichTerms = tmp.whichTerms;
		
		min_heap->push(tmp_min);
		max_heap->pop();
		;
	}
}

bool swap(std::priority_queue<conjunction_min> *min_heap, std::priority_queue<conjunction_max> *max_heap)
{
	while(!min_heap->empty())
	{
		conjunction_min tmp = min_heap->top();
		conjunction_max tmp_max;
		
		tmp_max.id = tmp.id;
		tmp_max.cover = tmp.cover;
		tmp_max.examples = tmp.examples;
		tmp_max.toExpand = tmp.toExpand;
		tmp_max.whichTerms = tmp.whichTerms;
		
		max_heap->push(tmp_max);
		min_heap->pop();
		;
	}
}

void checkQueueMaxLimit(std::priority_queue<conjunction_max> *max_heap, std::priority_queue<conjunction_min> *min_heap)
{
	int heap_size = max_heap->size();
	if(heap_size > QUEUE_LIMIT)
	{
		int toDelete = heap_size - QUEUE_LIMIT;
		swap(max_heap, min_heap);
		for(int i = 0; i <= toDelete; ++i)
		{
			if(!min_heap->empty())
				min_heap->pop();
		}
		swap(min_heap, max_heap);
	}
}

std::vector<conjunction_max> initPriorityQueue(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, vector<string> *features, boost::dynamic_bitset<> *classMask)
{
	int id = 0;
	std::vector<conjunction_max> baseTerms;
	for(vector<boost::dynamic_bitset<> >::iterator ibit = featureBitSet->begin(); ibit != featureBitSet->end(); ++ibit)
	{
		boost::dynamic_bitset<> toExpand;
		toExpand.resize(featureBitSet->size(), true);
		toExpand.set(id, false);
		boost::dynamic_bitset<> whichTerms = ~toExpand;		
		
		conjunction_max tmp = {.id = id, .cover = countCover(&(*ibit), classMask), .examples = (*ibit), .toExpand = toExpand, .whichTerms = whichTerms};
		max_heap->push(tmp);
		++id;
		baseTerms.push_back(tmp);
		 
	}
	return baseTerms;
}
int countCover(boost::dynamic_bitset<> *example, boost::dynamic_bitset<> *classMask)
{
	int P = 0, N = 0;
	boost::dynamic_bitset<> negMask = ~(*classMask);
	boost::dynamic_bitset<> tmp;
	tmp = *example;	
	tmp &= (*classMask); //POS bitset
	for(int i = 0; i < tmp.size(); ++i)
	{
		if(tmp.test(i))
			++P;
	}
	tmp = *example;
	tmp &= negMask;	//NEG bitset
	for(int i = 0; i < tmp.size(); ++i)
	{
		if(tmp.test(i))
			++N;
	}
	return P-N;
}

bool generateNewConjunctions(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, boost::unordered_map<string, int> *CLOSED, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *baseTerms)
{
	//typedef boost::unordered_map<boost::dynamic_bitset<>, int> generated_CLOSED;
	conjunction_max best = max_heap->top();
	string whichTermsID = generateHashKey(&(best.whichTerms));
	//to_string(best.whichTerms, whichTermsID);
	
	//iterate until you find the best non-duplicate conjunction of terms
	while((*CLOSED)[whichTermsID])
	{
		if(max_heap->empty())
			return false;
		max_heap->pop();
		best = max_heap->top();
		whichTermsID = generateHashKey(&(best.whichTerms));
	}
	int bitsetSize = best.toExpand.size();
	max_heap->pop();
	(*CLOSED)[whichTermsID]++;
	for(int i = 0; i < bitsetSize; ++i)
	{
		//candidate is found, create new conjunstion
		if(best.toExpand.test(i))
		{
			//best = first
			//basedTerms->at(i) second			
			boost::dynamic_bitset<> newWhichTerms = best.whichTerms;
			newWhichTerms |= baseTerms->at(i).whichTerms;
			//was it duplicated?
			string tohash = generateHashKey(&newWhichTerms);
			//to_string(newWhichTerms, tohash);
			if(!((*CLOSED)[tohash]))
			{
				boost::dynamic_bitset<> newExample = best.examples;
				newExample &= baseTerms->at(i).examples;
				
				boost::dynamic_bitset<> newToExpand = ~newWhichTerms;
				
				conjunction_max tmp = {.id = -1, .cover = countCover(&(newExample), classMask), .examples = newExample, .toExpand = newToExpand, .whichTerms = newWhichTerms};
				max_heap->push(tmp);
			}			 
		}
	} 	
}

string getPrintableConjunction(conjunction_max best, std::vector<string> *features)
{
	int size = best.toExpand.size();
	string rules = "";
	vector<string> rule;
	for(int i = 0; i < size; ++i)
	{		
		if(best.whichTerms.test(i))
			rule.push_back(features->at(i));
	}
	for(vector<string>::iterator irule = rule.begin(); irule != rule.end(); ++irule)
	{
		rules += *irule;
		if(irule + 1 != rule.end())
			rules += " \u2227 ";
	}
	
	rules += " => 1";
	return rules;
}

string generateHashKey(boost::dynamic_bitset<> *best)
{
	int size = best->size();
	string key = "";
	for(int i = 0; i < size; ++i)
	{
		if(best->test(i))
		{
			key += to_string(i) +",";
		}
	}
	return key;
}

void eraseCoveredExamples(conjunction_max best, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask)
{
	boost::dynamic_bitset<> newClassMask;
	vector<boost::dynamic_bitset<> > newFeatureBit;
	newClassMask.resize(best.examples.size(), false);
	int new_size = 0;
	for(int ibit = 0; ibit < best.examples.size(); ++ibit)
	{
		if(!(best.examples.test(ibit)))
		{
			newClassMask.set(new_size, (*classMask).test(ibit));
			++new_size;
		}
	}
	newClassMask.resize(new_size);
	*classMask = newClassMask;
	
	for(int i = 0; i < featureBitSet->size(); ++i)
	{		
		//cout << "featurebit: " << i << endl;
		boost::dynamic_bitset<> newFBit;
		newFBit.resize(best.examples.size(), false);
		int new_size = 0;
		for(int ibit = 0; ibit < best.examples.size(); ++ibit)
		{
			//cout << "bit: " << ibit << endl;
			if(!(best.examples.test(ibit)))
			{
				newFBit.set(new_size, (*featureBitSet)[i].test(ibit));
				++new_size;
			}
		}
		newFBit.resize(new_size);
		newFeatureBit.push_back(newFBit);
	}
	*featureBitSet = newFeatureBit;
}
