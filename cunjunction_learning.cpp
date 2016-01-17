#include "cunjunction_learning.h"

int main ()
{
	clock_t begin = clock();
	string myfile("train_mincover500.arff");
	const char* c_myfile = myfile.c_str();
	vector<boost::dynamic_bitset<> > featureBitSet;
	boost::dynamic_bitset<> classMask;
	vector<string> features;
	std::priority_queue<conjunction_max> max_heap;
	std::priority_queue<conjunction_min> min_heap;
	conjunction_max bestConjunction;
	boost::unordered_map<string, int> CLOSED;
	
	if(parseFile(c_myfile, &featureBitSet, &classMask, &features))
	{
		std::vector<conjunction_max> baseTerms = initPriorityQueue(&max_heap, &featureBitSet, &features, &classMask);		
		cout << "i: 0 size of priority queue BEFORE: " << max_heap.size() << endl;
		//generate all options
		bestConjunction = max_heap.top();
		cout << getPrintableConjunction(bestConjunction, &features) <<  " | cover PN " << bestConjunction.cover <<endl;
		for(int i = 0; i < 1000; ++i)
		{
			generateNewConjunctions(&max_heap, &featureBitSet, &CLOSED, &classMask, &baseTerms);
			checkQueueMaxLimit(&max_heap, &min_heap);
			//cout << "i: " << i << " size of priority queue: " << max_heap.size() << endl;
			bestConjunction = max_heap.top();
			cout << getPrintableConjunction(bestConjunction, &features) <<  " | cover PN " << bestConjunction.cover << " - i:" << i <<" |queue| " << max_heap.size() <<endl;
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
		//whichTerms.resize(featureBitSet->size(), false);
		//whichTerms.set(id, true);
		
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
		//to_string(best.whichTerms, whichTermsID);
	}
	int bitsetSize = best.toExpand.size();
	max_heap->pop();
	(*CLOSED)[whichTermsID]++;
	//cout << "bitseize: " << bitsetSize << endl;
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
				//(*CLOSED)[tohash]++;	//add to unorder_map
				
				//cout << "choose best: " << best.id << " with: " << baseTerms->at(i).id << " cover: "  << tmp.cover << endl;
				//cout << getPrintableConjunction(tmp, &features) <<  " | cover PN " << bestConjunction.cover
			}
			else
			{
				;//cout << "DUPLCITY\n";
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
	//cout << "key: " << key << endl;
	return key;
}
