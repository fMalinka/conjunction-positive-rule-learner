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
int VERBOSE = 0;
int DEBUG = 0;
int ROC_FLAG = 0;
string ROC;


int main(int argc, char* argv[])
{
	if(!parseCommandLineParametrs(argc, argv))
	{
		cerr << HELP;
		return EXIT_FAILURE;
	}
	else
		printSettings();

	//TRAIN
	std::vector<conjunction_max> bestConjunctionS = train();
	if(bestConjunctionS.empty())
		return EXIT_FAILURE;
	
	//TEST
	if(TEST_FLAG)
	{
		test(&bestConjunctionS);
	}

	return EXIT_SUCCESS;		
}

std::vector<conjunction_max> train()
{
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
		vector<boost::dynamic_bitset<> > featureBitSet_orig = featureBitSet;
		boost::dynamic_bitset<> classMask_orig = classMask;
		for(int irule = 0; irule < RULES_LIMIT; ++irule)
		{
			std::vector<conjunction_max> baseTerms = initPriorityQueue(&max_heap, &featureBitSet, &features, &classMask);
			int Pclass; int Nclass;
			countPN(&classMask, &Pclass, &Nclass);
			
			if(Pclass == 0)	//nothing to search
				break;
			
			cout << "[RULE " << irule+1 << "]" <<" [" << Pclass << "/" << Nclass << "|" << classMask.size() << "] POS/NEG|TOTAL" <<endl;
			
			//generate all options
			bestConjunction = max_heap.top();
			if(VERBOSE)
			{
				cout << "\tBest initial: " << getPrintableConjunction(max_heap.top(), &features) <<  " | cover ["  <<max_heap.top().P << "/" << max_heap.top().N << "|" << max_heap.top().cover << "]";
				if(DEBUG)
					{
						string q;
						if(QUEUE_UNLIMITED)
							q = "UNLIMITED";
						else
							q = to_string(QUEUE_LIMIT);
						cout << " QUEUE [" << max_heap.size()  << "| " << q << "]";
					}
					 cout << endl;
			}
				
			for(int i = 0; i < ITERATE_LIMIT; ++i)
			{
				if(max_heap.empty())
					break;
				
				generateNewConjunctions(&max_heap, &featureBitSet, &CLOSED, &classMask, &baseTerms);
				if(!QUEUE_UNLIMITED)
					checkQueueMaxLimit(&max_heap, &min_heap);
					
				//find the best rule with MIN coveradge and MIN negative examples
				conjunction_max candidate = max_heap.top();
				if(isBetter(&bestConjunction, &candidate))
					bestConjunction = max_heap.top();
					
				if(VERBOSE)
				{
					cout << "\ti:" << i+1 << " " << getPrintableConjunction(candidate, &features) <<  " | cover ["  <<candidate.P << "/" << candidate.N << "|" << candidate.cover << "]";
					if(DEBUG)
					{
						string q;
						if(QUEUE_UNLIMITED)
							q = "UNLIMITED";
						else
							q = to_string(QUEUE_LIMIT);
						cout << " QUEUE [" << max_heap.size()  << "| " << q << "]";
					}
					 cout << endl;
				}
			}
			//if(bestConjunction.cover == 0)
			//{
			//	break;
			//}
			cout << "BEST: "  << getPrintableConjunction(bestConjunction, &features) <<  " | cover ["  <<bestConjunction.P << "/" << bestConjunction.N << "|" << bestConjunction.cover << "]" << endl << endl;
			eraseCoveredExamples(bestConjunction, &featureBitSet, &classMask);
			while(!max_heap.empty())
				max_heap.pop();
			bestConjunctionS.push_back(bestConjunction);
			CLOSED.clear();
			if(classMask.empty())
				break;
		}
		
		//statistics
		statistics traindataset = evaluateDataset(TRAIN_MODE, &featureBitSet_orig, &classMask_orig, &bestConjunctionS);
		cout << "=== Error on training data ===" << endl << endl;
		cout << "TP " << traindataset.TP << "\t FP " << traindataset.FP << endl;
		cout << "FN " << traindataset.FN << "\t TN " << traindataset.TN << endl;
		double acc = (traindataset.TP+traindataset.TN)/ double (traindataset.FP+traindataset.FN+traindataset.TP+traindataset.TN);
		cout.precision(5);
		cout << "ACC " << acc << endl;
	}
	else
	{
		cerr << "Unable to read file " << endl;
		return bestConjunctionS;
	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	cout  << "Time taken to build model: " << elapsed_secs  << "sec."<< endl << endl;
	return bestConjunctionS;
}

void test(std::vector<conjunction_max> *rules)
{
	string myfile(TEST);
	const char* c_myfile = myfile.c_str();
	vector<boost::dynamic_bitset<> > featureBitSet;
	boost::dynamic_bitset<> classMask;
	vector<string> features;

	
	if(parseFile(c_myfile, &featureBitSet, &classMask, &features))
	{
		statistics test = evaluateDataset(TEST_MODE, &featureBitSet, &classMask, rules);
		cout << "=== Error on test data ===" << endl << endl;
		cout << "TP " << test.TP << "\t FP " << test.FP << endl;
		cout << "FN " << test.FN << "\t TN " << test.TN << endl;
		double acc = (test.TP+test.TN)/ double (test.FP+test.FN+test.TP+test.TN);
		cout.precision(5);
		cout << "ACC " << acc << endl;
		
		//for(int i = 0; i < test.confidence.size(); ++i)
		//{
		//	cout << test.confidence[i] << endl;
		//}
		
		//RFILE
		if(ROC_FLAG)
			generateRFile(&featureBitSet, &classMask, rules, &test);
		
	}
	else
		cerr << "Unable to read file" << endl;
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
					return INT_MAX;
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
			else if(std::string(argv[i]) == "--verbose")
			{
				VERBOSE = 1;
			}
			else if(std::string(argv[i]) == "--debug")
			{
				DEBUG = 1;
			}
			else if(std::string(argv[i]) == "--generateRFile")
			{
				ROC_FLAG = 1;
				ROC = std::string(argv[i+1]);
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
		
		//cout << "Parsing..." << endl;
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
		tmp_min.P = tmp.P;
		tmp_min.N = tmp.N;
		tmp_min.examples = tmp.examples;
		tmp_min.toExpand = tmp.toExpand;
		tmp_min.whichTerms = tmp.whichTerms;
		tmp_min.length = tmp.length;
		
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
		tmp_max.P = tmp.P;
		tmp_max.N = tmp.N;
		tmp_max.examples = tmp.examples;
		tmp_max.toExpand = tmp.toExpand;
		tmp_max.whichTerms = tmp.whichTerms;
		tmp_max.length = tmp.length;
		
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
		int P; int N;
		int C = countCover(&(*ibit), classMask, &P, &N);
		conjunction_max tmp = {.id = id, .cover = C, .P = P, .N = N, .examples = (*ibit), .toExpand = toExpand, .whichTerms = whichTerms, .length = 1};
		if(C != 0 && P != 0 && N != 0)
		{
			max_heap->push(tmp);
			
			
		}
		baseTerms.push_back(tmp);
		++id;		 
	}
	return baseTerms;
}
int countCover(boost::dynamic_bitset<> *example, boost::dynamic_bitset<> *classMask, int *P, int *N)
{
	*P = 0; *N = 0;
	boost::dynamic_bitset<> negMask = ~(*classMask);
	boost::dynamic_bitset<> tmp;
	tmp = *example;	
	tmp &= (*classMask); //POS bitset
	for(int i = 0; i < tmp.size(); ++i)
	{
		if(tmp.test(i))
			++(*P);
	}
	tmp = *example;
	tmp &= negMask;	//NEG bitset
	for(int i = 0; i < tmp.size(); ++i)
	{
		if(tmp.test(i))
			++(*N);
	}
	return (*P)-(*N);
}

bool generateNewConjunctions(std::priority_queue<conjunction_max> *max_heap, vector<boost::dynamic_bitset<> > *featureBitSet, boost::unordered_map<string, int> *CLOSED, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *baseTerms)
{
	conjunction_max best = max_heap->top();
	string whichTermsID = generateHashKey(&(best.whichTerms));
	
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
				int Ptmp; int Ntmp;
				int C = countCover(&(newExample), classMask, &Ptmp, &Ntmp);
				conjunction_max tmp = {.id = -1, .cover = C, .P = Ptmp, .N = Ntmp, .examples = newExample, .toExpand = newToExpand, .whichTerms = newWhichTerms, .length = best.length+1};
				//dont push conjunction with zero P, N, cover
				if(C =! 0 && Ptmp != 0 && Ntmp != 0)
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

void countPN(boost::dynamic_bitset<> *bitset, int *P, int *N)
{
	*P = 0; *N = 0;
	for(int i = 0; i < bitset->size(); ++i)
	{
		if(bitset->test(i))
			++(*P);
		else
			++(*N);
	}
}

bool isBetter(conjunction_max *bestConjunction, conjunction_max *max_heap_top)
{	
	if(bestConjunction->cover < max_heap_top->cover)
		return true;
	else if(bestConjunction->cover > max_heap_top->cover)
		return false;
	else // bestConjunction->cover == max_heap_top->cover
	{
		if(bestConjunction->N > max_heap_top->N)
			return false;
		else if(bestConjunction->N < max_heap_top->N)
			return true;
		else //(bestConjunction->N == max_heap_top->N
		{
			if(bestConjunction->length < max_heap_top->length)
				return true;
			else
				return false;
		}
	}
	return false;
}

void printSettings()
{
	cout << "[train " << TRAIN  << "]" << endl;
	if(TEST_FLAG)
		cout << "[test " << TEST << "]" << endl;
	cout << "[max_rules " << RULES_LIMIT << "]" << endl;
	if(ITERATE_LIMIT == INT_MAX)
		cout << "[max_iteration UNLIMITED]" << endl;
	else
		cout << "[max_iteration " << ITERATE_LIMIT << "]" << endl;
	if(QUEUE_UNLIMITED)
		cout << "[max_queue UNLIMITED]" << endl;
	else
		cout << "[max_queue " << QUEUE_LIMIT << "]" << endl;
	if(VERBOSE)
		cout << "[VERBOSE mode]" << endl;
	if(DEBUG)
		cout << "[DEBUG mode]" << endl;
	if(ROC_FLAG)
		cout << "[generateRFile " << ROC << "]" << endl;
	cout << endl;
}

int countTrue(boost::dynamic_bitset<> *bitset)
{
	int count = 0;
	for(int i = 0; i < bitset->size(); ++i)
	{
		if(bitset->test(i))
			++count;
	}
	return count;
}

statistics evaluateDataset(MODE mod, vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *rules)
{
	boost::dynamic_bitset<> sumCoverage;
	vector<boost::dynamic_bitset<> > constructedRules;
	sumCoverage.resize(classMask->size(), false);
	for(std::vector<conjunction_max>::iterator irule = rules->begin(); irule != rules->end(); ++irule)
	{
		boost::dynamic_bitset<> res;
		res.resize(classMask->size(), true);
		//create a rule
		 for(int ibit = 0; ibit < (*irule).whichTerms.size(); ++ibit)
		 {
			 if((*irule).whichTerms.test(ibit))
				res &= (*featureBitSet)[ibit];
		 }
		//res &= *classMask;
		if(mod == TRAIN_MODE)
		{
			boost::dynamic_bitset<> acc_res = res;
			acc_res &= *classMask;
			(*irule).acc = countTrue(&acc_res) / double (countTrue(&res));
		}
		else
			constructedRules.push_back(res);
			
		sumCoverage |= res;
		
		cout.precision(5);
		cout << "ACC " << (*irule).acc << endl;
	}
	
	std::vector<double> confidence;
	if(mod == TEST_MODE)
	{
		for(int ibit = 0; ibit < sumCoverage.size(); ++ibit)
		{
			int count = 0;
			double sum = 0.0;
			for(int irule = 0; irule < constructedRules.size(); ++irule)
			{
				if(sumCoverage.test(ibit) && constructedRules[irule].test(ibit))
				{
					sum += rules->at(irule).acc;
					count++;
				}
			}
			double conf;
			if(count == 0)
				conf = 0;
			else
				conf = sum/ double(count);
			confidence.push_back(conf);			
		}
	}
	
	boost::dynamic_bitset<> TP = sumCoverage;
	boost::dynamic_bitset<> FP = sumCoverage;
	boost::dynamic_bitset<> TN = ~sumCoverage;
	boost::dynamic_bitset<> FN = sumCoverage;
	
	sumCoverage &= *classMask;
	
	TP &= *classMask; //count true
	TN &= ~(*classMask);//count true
	
	boost::dynamic_bitset<> FPtmp = FP;
	FPtmp ^= *classMask;
	FP &=  FPtmp; //count true
	
	FN ^= *classMask;
	FN &=  *classMask; //count true
	
	statistics dataset = {.TP = countTrue(&TP), .FP = countTrue(&FP), .FN = countTrue(&FN), .TN = countTrue(&TN), .ACC = 0, .confidence = confidence};
	dataset.ACC = (dataset.TP+dataset.TN)/ double (dataset.FP+dataset.FN+dataset.TP+dataset.TN);
	return dataset;
}

void generateRFile(vector<boost::dynamic_bitset<> > *featureBitSet, boost::dynamic_bitset<> *classMask, std::vector<conjunction_max> *rules, statistics *test)
{
	string rtext = "if(!library(ROCR, logical.return = TRUE))\
					{\
					  install.packages(ROCR)\
					}\\n";
					
	boost::dynamic_bitset<> sumCoverage;
	sumCoverage.resize(classMask->size(), false);
	for(std::vector<conjunction_max>::iterator irule = rules->begin(); irule != rules->end(); ++irule)
	{
		boost::dynamic_bitset<> res;
		res.resize(classMask->size(), true);
		//create a rule
		 for(int ibit = 0; ibit < (*irule).whichTerms.size(); ++ibit)
		 {
			 if((*irule).whichTerms.test(ibit))
				res &= (*featureBitSet)[ibit];
		 }
		//res &= *classMask;			
		sumCoverage |= res;
	}
	
	string predictors;// = "pr <- c(";
	string labels;// = "lb <- c(";
	
	for(int i = 0; i < sumCoverage.size(); ++i)
	{
		predictors += to_string(test->confidence.at(i));
		predictors += ",";
			
		if(classMask->test(i))
			labels += "1,";
		else
			labels += "0,";
	}
	predictors.pop_back();
	//predictors += ")";
	labels.pop_back();
	//labels += ")";
	
	//rtext += predictors + "\n";
	//rtext += labels + "\n";
	
	rtext +=	"png()\
pred <- prediction( ROCR.simple$predictions, ROCR.simple$labels)\
perf <- performance(pred,\"tpr\",\"fpr\")\
plot(perf)\
dev.off()\
\
\
#auc\
perf <- performance(pred, measure = \"auc\", x.measure = \"cutoff\")\
cat(\"AUC =\",deparse(as.numeric(perf@y.values)),\"\n\")\
\
#acc\
perf <- performance(pred, measure = \"acc\", x.measure = \"cutoff\")\
max(perf@y.values[[1]])";
	
	//write to file
	std::ofstream out(ROC);
	out << rtext;
	out.close();
	
	std::ofstream outData(ROC+"_data");
	outData << predictors;
	outData << endl;
	outData << labels;
	outData << endl;
	outData.close();
	
	
}
