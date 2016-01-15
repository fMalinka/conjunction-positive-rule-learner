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
		for(vector<boost::dynamic_bitset<> >::iterator ibit = featureBitSet.begin(); ibit != featureBitSet.end();  ++ibit)
		{
			cout << "size: " << (*ibit).size() << endl;
		}
		for(vector<string>::iterator ibit = features.begin(); ibit != features.end();  ++ibit)
		{
			cout << "size: " << (*ibit) << endl;
		}
		cout << classMask.size()  << endl;
		string x;
		boost::to_string(classMask, x);
		cout << classMask.test(180) << endl;
		cout << classMask.test(181) << endl;
		cout << classMask.test(179) << endl;
		cout << classMask.test(182) << endl;
		cout << "bit: " << featureBitSet.size() << endl;
		cout << "feat: " << features.size() << endl;
		for(int i = 0; i < 304; ++i)
		{
			for(int ii = 0; ii < featureBitSet.size(); ++ii)
			{
				if(featureBitSet[ii].test(i))
					cout <<	"'+',";
				else
					cout <<	"'-',";
			}
			if(classMask.test(i))
			cout <<	"'+'";
				else
					cout <<	"'-'";			
			cout << endl;
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

int getdataPositionFomARFF(ifstream *myfile, int *featuresNumber, int *examplesNumber)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(",");
	string line;
	int dataPos = 0;
	
	while(myfile->good())
	{
		getline(*myfile, line);
		//iterate to data
		if (std::regex_match (line, std::regex("@data") ))
		{
			dataPos = myfile->tellg();
			getline(*myfile, line);
			tokenizer tokens(line, sep);
			for(tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
				++(*featuresNumber);
			while(myfile->good())
			{
				getline(*myfile, line);
				++(*examplesNumber);
			}
		}
	}
	cout << "Number of examples: " << *examplesNumber << ", Number of features: " << *featuresNumber << endl;
	return dataPos;
}
