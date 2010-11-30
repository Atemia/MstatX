/*
 *  msa.cpp
 *  MstatX++
 *
 *  Created by COLLET Guillaume on 08/11/10.
 *
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>

#include "msa.h"
#include "options.h"

using namespace std;


/**************************************************************
 * This constructor of a multiple alignment reads the 
 * multiple alignment in a multi-fasta format.
 * Once read, the multiple alignment is analysed to find
 * the alphabet used, the number of gaps and the entropy of 
 * each column, and the frequency of each amino acid type.
 **************************************************************/
Msa :: Msa(string fname)
{
	/* Open file */
	if (Options::Get().verbose){
		cout << "Read Multiple Alignment in " << fname << "\n";
	}
	ifstream file(fname.c_str());
	if (!file.good()){
	  cerr << "Cannot open file " << fname << "\n";
		exit(0);
	}
	int nb_seq_limits =500;
	/* Read file */
	string s, tmp_seq;
	while (file.good() && mali_seq.size() < nb_seq_limits){
		getline(file,s);
		if (s[0] == '>'){
			if (mali_name.size() != 0){
			  mali_seq.push_back(tmp_seq);	
			}
			if (mali_seq.size() < nb_seq_limits){
  			mali_name.push_back(s.substr(1, s.find_first_of(' ') - 1));
			}
			tmp_seq.clear();
		} else {
			tmp_seq = tmp_seq + s;
		}
	}
	if (mali_seq.size() < nb_seq_limits){
	  mali_seq.push_back(tmp_seq);
	}
	
	nseq = mali_name.size();
	ncol = mali_seq[0].size();
	cout << "nb seq = "<<nseq<<" -- nb col = "<<ncol<<"\n";
	
	/* Change all mali.seq in upper case*/
	for (int i(0); i < nseq; ++i){
		for(int j(0); j < ncol; ++j){
			mali_seq[i][j] = toupper(mali_seq[i][j]);
		}
	}
	
	/* Analyse the multiple alignment */
	defineAlphabet();
	countGap();
	countFreq();
	countType();
	countEntropy();
	
	
	/* Print if verbose mode */
	if (Options::Get().verbose){
		cout << "\nAlphabet :\n";
	  for(int i(0); i < alphabet.size(); ++i){
			cout << alphabet[i] << ";";
		}
		cout << "\n";
		cout << "\nMultiple Alignment :\n";
	  for(int i(0); i <nseq; ++i){
			cout << mali_seq[i] << "\n";
		}
		cout << "\nAA Frequencies :\n";
		for (int i(0); i < aa_freq.size(); ++i){
			cout << aa_freq[i] << ";";
		}
		cout << "\n";
		cout << "\nGap counts :\n";
		for (int i(0); i < gap_counts.size(); ++i){
			cout << gap_counts[i] << ";";
		}
		cout << "\n";
		cout << "\nAA Entropy :\n";
		for (int i(0); i < entropy.size(); ++i){
			if (gap_counts[i] < nseq/10){
		  	cout << entropy[i] << ";";
			} else {
				cout << "-12.0;";
			}
		}
		cout << "\n";
		cout << "\nAA Types :\n";
		for (int i(0); i < nb_type.size(); ++i){
				cout << nb_type[i] << ";";
		}
		cout << "\n";
	}
}


/**************************************************************
 * countGap() calculate the number of gaps in each column
 **************************************************************/
void
Msa :: countGap(){
	for(int col(0); col < ncol; ++col){
		int gap = 0;
		for(int row(0); row < nseq; ++row){
			if (mali_seq[row][col] == '-' || mali_seq[row][col] == ' '){
				gap++;
			}
		}
		gap_counts.push_back(gap);
	}
}


/**************************************************************
 * countFreq() calculates the frequency of each amino acid
 * type in the overall multiple alignment
 **************************************************************/
void
Msa :: countFreq(){
	int total = 0;
	vector<int> tmp_freq(alphabet.size(), 0);
	
	aa_freq = vector<float>(alphabet.size());
	/* Count the number of each amino acid type defined in alphabet */
	for(int col(0); col < ncol; ++col){
		for(int row(0); row < nseq; ++row){
			if (mali_seq[row][col] != '-' && mali_seq[row][col] != ' '){
				total++;
			}
			int pos = alphabet.find(mali_seq[row][col]);
			if (pos >= alphabet.size()){
			  cerr << "error : symbol is not in the alphabet\n";
				exit(0);
			}
			tmp_freq[pos]++;
		}
	}

	/* Divide by the total */
	for (int i(0); i < aa_freq.size(); ++i){
		aa_freq[i] = (float) tmp_freq[i] ;
		aa_freq[i] /= (float) total;
	}
}

/**************************************************************
 * countType() calculates the number of different types in
 * each column of the alignment
 **************************************************************/
void
Msa :: countType(){
	string aa_types;
	for(int col(0); col < ncol; ++col){
		aa_types.clear();
		for(int row(0); row < nseq; ++row){
			if (aa_types.find(mali_seq[row][col]) >= aa_types.size()){
			  aa_types.push_back(mali_seq[row][col]);
			}
		}
		aa_type_list.push_back(aa_types);
		nb_type.push_back(aa_types.size());
	}
}

/**************************************************************
 * defineAlphabet() reads the multiple alignment to
 * determine all the symbols used in
 **************************************************************/
void
Msa :: defineAlphabet(){
	alphabet.clear();
	for(int col(0); col < ncol; ++col){
		for(int row(0); row < nseq; ++row){
			if (alphabet.find(mali_seq[row][col]) >= alphabet.size()){
				alphabet.push_back(mali_seq[row][col]);
			}
		}
	}
}


/**************************************************************
 * getFreq(aa) returns the frequency of amino acid aa
 * in the overall multiple alignment
 **************************************************************/
float 
Msa :: getFreq(char aa){
  int pos = getAaPos(aa);
	if (pos == -1){
	  cerr << "Error, symbol not in alphabet\n";
		exit(0);
	}
	return aa_freq[pos];
}


/**************************************************************
 * getAaPos(aa) returns the index of aa in alphabet
 **************************************************************/
int
Msa :: getAaPos(char aa){
	if (alphabet.find(aa) >= alphabet.size()){
		return -1;
	}
	return alphabet.find(aa);
};


/**************************************************************
 * getGap(col) returns the number of gap in column col
 **************************************************************/
int
Msa :: getGap(int col){
	return gap_counts[col];
}


/**************************************************************
 * countEntropy calculates the entropy of each column
 * by the following formula (Normalized Shannon Entropy):
 * entropy[col] = - \frac{1}{log(K)} \sum_{a=1}^{K}p_a log(p_a)
 * with 
 * K = alphabet length
 * p_a = probability to see amino acid of type a in the column
 * p_a = frequency of amino acid a in the column (nb_a / nseq)
 **************************************************************/
void 
Msa :: countEntropy(){
	entropy = vector<float>(ncol,0.0);
 
  for(int col(0); col < ncol; ++col){
		vector<float> lfreq(alphabet.size(), 0.0);
		for(int row(0); row < nseq; ++row){
			lfreq[getAaPos(mali_seq[row][col])] += 1.0;
		}
		for (int i(0); i < lfreq.size(); ++i){
		  lfreq[i] /= (float) nseq;
			if (lfreq[i] > 0.0){
				if (lfreq[i] == 1.0){
				  entropy[col] == 0.0;	
				} else {
				  entropy[col] -= lfreq[i] * log(lfreq[i]);	
				}
			}
		}
		entropy[col] /= log(alphabet.size()-1); /* -1 because gaps are in the alphabet */
	}
}


/**************************************************************
 * isInclude(alph1) returns true if the alphabet of the
 * multiple alignment is include in the alphabet alph1
 **************************************************************/
bool
Msa :: isInclude(string alph1){
  for (int i(0); i < alphabet.size(); ++i){
		if (alph1.find(alphabet[i]) >= alph1.size() && alphabet[i] != '-' && alphabet[i] != ' '){
		  return false;	
		}
	}
	return true;
}

string 
Msa :: getCol(int col)
{
  string column;
	for (int i(0); i < nseq; ++i){
		column.push_back(mali_seq[i][col]);
	}
	return column;
}