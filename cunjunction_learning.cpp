#include "cunjunction_learning.h"

int main ()
{
	clock_t begin = clock();
	string myfile("train_mincover500.arff");
	const char* c_myfile = myfile.c_str();
	vector<boost::dynamic_bitset<> > featureBitSet;
	boost::dynamic_bitset<> classMask;
	vector<string> features;
	if(parseFile(c_myfile, &featureBitSet, &classMask, &features))
	{
		
		conjunction_max a2;
		a2.cover = 2;
		conjunction_max a4;
		a4.cover = 4;
		conjunction_max a3;
		a3.cover = 3;
		conjunction_max a1;
		a1.cover = 1;
		
		max_heap.push(a1);
		max_heap.push(a2);
		max_heap.push(a3);
		max_heap.push(a4);
		
		cout << "max size: " << max_heap.size() << endl;
		checkQueueMaxLimit(&max_heap);
		cout << "max size: " << max_heap.size() << endl;		
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
		
		max_heap->push(tmp_max);
		min_heap->pop();
		;
	}
}

void checkQueueMaxLimit(std::priority_queue<conjunction_max> *max_heap)
{
	int heap_size = max_heap->size();
	if(heap_size > QUEUE_LIMIT)
	{
		int toDelete = heap_size - QUEUE_LIMIT;
		swap(max_heap, &min_heap);
		for(int i = 0; i <= toDelete; ++i)
		{
			if(!min_heap.empty())
				min_heap.pop();
		}
		swap(&min_heap, max_heap);
	}
}
