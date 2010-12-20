/* Copyright (c) 2010 Guillaume Collet
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 */

#include "trident.h"
#include "options.h"
#include "scoring_matrix.h"

#include <cmath>
#include <fstream>
#include <algorithm>

#define MIN(x,y)  (x < y ? x : y)

/* Calculate the weight of sequence i in the msa 
 * by the formula from Henikoff & Henikoff (1994)
 * w_i = \frac{1}{L}\sum_{x=1}^{L}\frac{1}{k_x n_{x_i}}
 * We use these notations in the code below
 */
float 
TridStat :: calcSeqWeight(Msa & msa, int i)
{
	int x, seq;
	int k;								    /**< number of symbol types in a column */
	int n;								    /**< number of occurence of aa[i][x] in column x */
	int L = msa.getNcol();    /**< number of columns (i.e. length of the alignment) */
	int nseq = msa.getNseq(); /**< number of rows (i.e. number of sequences in the alignment) */
	float w;						    	/**< weight of sequence i */
	
	w = 0.0;
	for	(x = 0; x < L; ++x){
		k = msa.getNtype(x);
		/* Calculate the number of aa[i][x] in current column */
		n = 0;
		for(seq = 0; seq < nseq; ++seq){
		  if (msa.getSymbol(i, x) == msa.getSymbol(seq, x)){
				n++;
			}
		}
		w += (float) 1 / (float) (n * k); 
	}
	w /= (float) L;
	return w ;
}

void
TridStat :: calculateStatistic(Msa & msa)
{
	/* Init size */
	ncol = msa.getNcol();
	nseq = msa.getNseq();
	string alphabet = msa.getAlphabet();
	
	/* Calculate Sequence Weights */
	for (int seq(0); seq < nseq; ++seq){
		seq_weight.push_back(calcSeqWeight(msa,seq));
	}
	
	/* Print if verbose mode on */
	if (Options::Get().verbose){
		cout << "Seq weights :\n";
		for (int seq(0); seq < nseq; ++seq){
		  cout.width(10);
		  cout << seq_weight[seq];
			/*for (int col(0); col < ncol; ++col){
			 cout.width(10);
			 cout << msa.getSymbol(seq, col);	
			 }*/
			cout << "\n";
		}
		cout << "\n";
	}
	
	/* Calculate t(x) = \frac{\sum_{a=1}^{K}p_a log(p_a)}{log(min(N,K))}
	 *						p_a = \sum_{i \in \{i|s(i) = a\}} w_i
	 *						w_i = \frac{1}{L} \sum_{x=1}^{L}\frac{1}{K_x n_{x_i}}
	 */
	float lambda = 1.0 / log(MIN(alphabet.size(),nseq));
	
  for (int x(0); x < ncol; x++){
		t.push_back(0.0);
		for (int a(0); a < alphabet.size(); a++){
			float tmp_proba(0.0);
		  for (int j(0); j < nseq; j++){
				if(msa.getSymbol(j, x) == alphabet[a]){
					tmp_proba += seq_weight[j];
				}
			}
			if (tmp_proba != 0.0){
				t[x] -= tmp_proba * log(tmp_proba);
			}
		}
		t[x] *= lambda;
	}
	
	/* Calculate r(x) = \lambda_r \frac{1}{k_x}\sum_{a=1}^{k_x}|\bar{X}(x) - X_a|
	 *      \lambda_r = \frac{1}{\sqrt{20(max(M)-min(M))^2}}
	 *					  X_a = \left[ \begin{array}{c}M(a,a_1)\\M(a,a_2)\\.\\.\\.\\M(a,a_{20})\end{array}\right]
	 *							M is a normalized scoring matrix
	 */
	ScoringMatrix score_mat(Options::Get().score_matrix_path + "/blosum62.mat");
	int alph_size = score_mat.getAlphabetSize();
	string sm_alphabet = score_mat.getAlphabet();
	
	msa.fitToAlphabet(sm_alphabet);
	
	for (int x(0); x < ncol; x++){
		
		int ntype = msa.getNtype(x);
		string type_list = msa.getTypeList(x);
		
		int pos = type_list.find('-');
		if (pos < type_list.size()){
			type_list.erase(type_list.begin()+pos);
			ntype--;
		}
		if (ntype){
			/* Calculate Mean vector */
			vector<float> mean(alph_size, 0.0);
					for (int i(0); i < ntype; ++i)
					for (int a(0); a < alph_size; ++a)
					mean[a] += score_mat.normScore(sm_alphabet[a], type_list[i]);
			
			for (int a(0); a < alph_size; ++a)
				mean[a] /= ntype;
		
			/* Calculate Score */
			float lambda = sqrt(alph_size * (score_mat.getMax() - score_mat.getMin()) * (score_mat.getMax() - score_mat.getMin()));
			float tmp_score = 0.0;
			for (int i(0); i < ntype; ++i){
				vector<float> diff_vect;
				for(int a(0); a < alph_size; ++a){
					diff_vect.push_back(mean[a] - score_mat.normScore(sm_alphabet[a], type_list[i]));
				}
				tmp_score += normVect(diff_vect);
			}
			tmp_score /= ntype;
			tmp_score /= lambda;
			r.push_back(tmp_score);
		} else {
			r.push_back(1.0);
		}
	}

	/* Calculate g(x) = nb_gap / nb_seq 
	 * Represents the proportion of gaps in the column
	 */
	for (int x(0); x < ncol; x++){
		g.push_back((float) msa.getGap(x) / (float) nseq);
	}

	/* Print if verbose mode on */
	cout << "Score is based on trident score defined by Valdar (2002)\n";
	cout << "S = (1 - t)^a * (1 - r)^b * (1 - g)^c\n";
	cout << "t measures the entropy\n";
	cout << "r measures the residue similarity (based on a normalized substitution matrix)\n";
	cout << "g measures the gap frequencies\n";
	cout << "a = "<<Options::Get().factor_a<<"\n";
	cout << "b = "<<Options::Get().factor_b<<"\n";
	cout << "c = "<<Options::Get().factor_c<<"\n";
	
	ofstream file(Options::Get().output_name.c_str());
	if (!file.is_open()){
	  cerr << "Cannot open file " << Options::Get().output_name << "\n";
		exit(0);
	}
	for (int col(0); col < ncol; ++col){
	  file << pow((float) (1.0 - t[col]), Options::Get().factor_a) * pow((float) (1.0 - r[col]), Options::Get().factor_b) * pow((float) (1.0 - g[col]), Options::Get().factor_c) << "\n";
	}
}


float 
TridStat :: normVect(vector<float> vect){
	float score= 0.0;
	for(int i(0); i < vect.size(); ++i){
		score += vect[i] * vect[i];
	}
	return sqrt(score);
}

