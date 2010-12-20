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

#include "jensen.h"
#include "options.h"

#include <cmath>
#include <fstream>
#include <algorithm>

#define MIN(x,y)  (x < y ? x : y)

JensenStat :: ~JensenStat()
{
	if (proba != NULL){
		for (int i(0); i < ncol; ++i) {
			if (proba[i] != NULL){
				free (proba[i]);	
			}
		}
		free(proba);
	}
}

/* Calculate the weight of sequence i in the msa 
 * by the formula from Henikoff & Henikoff (1994)
 * w_i = \frac{1}{L}\sum_{x=1}^{L}\frac{1}{k_x n_{x_i}}
 * We use these notations in the code below
 */
double 
JensenStat :: calcSeqWeight(Msa & msa, int i)
{
	int x, seq;
	int k;								    /**< number of symbol types in a column */
	int n;								    /**< number of occurence of aa[i][x] in column x */
	int L = msa.getNcol();    /**< number of columns (i.e. length of the alignment) */
	int nseq = msa.getNseq(); /**< number of rows (i.e. number of sequences in the alignment) */
	double w;						    	/**< weight of sequence i */
	
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
		w += (double) 1 / (double) (n * k); 
	}
	w /= (double) L;
	return w ;
}

void
JensenStat :: calculateStatistic(Msa & msa)
{
	/* Init size */
	ncol = msa.getNcol();
	nseq = msa.getNseq();
	string alphabet = msa.getAlphabet();
	
	/* Allocate proba array */
	proba = (double **) calloc(ncol, sizeof(double*));
	if (proba == NULL){
		fprintf(stderr,"Cannot Allocate probability matrix\n");
		exit(0);
	}
	for (int i(0); i < ncol; i++){
		proba[i] = (double *) calloc(alphabet.size(), sizeof(double));
		if (proba[i] == NULL){
			fprintf(stderr,"Cannot Allocate probability submatrix\n");
			exit(0);
		}
	}
	
	/* Calculate Sequence Weights */
	for (int seq(0); seq < nseq; ++seq){
		seq_weight.push_back(calcSeqWeight(msa,seq));
	}
	
	/* Print if verbose mode on */
	if (Options::Get().verbose){
		cout << "Seq weights :\n";
		for (int seq(0); seq < nseq; ++seq){
		  cout.width(10);
		  cout << seq_weight[seq] << "\n";
		}
		cout << "\n";
	}
	
	/* Calculate aa proba by columns */
	double lambda = 1.0 / log(MIN(alphabet.size(),nseq));
	
  for (int x(0); x < ncol; ++x){
		int nb_abs = 0;
		for (int a(0); a < alphabet.size(); a++){
		  for (int j(0); j < nseq; ++j){
				if(msa.getSymbol(j, x) == alphabet[a]){
					proba[x][a] += seq_weight[j];
				}
			}
			if (proba[x][a] == 0.0){
				proba[x][a] = pow(10.0,-6.0);
				nb_abs++;
			}
		}
		/* reduce by the pseudo counts in order to have sum-of-proba = 1 */
		double pseudo_counts = (double) nb_abs * pow(10.0,-6.0) / (double) (alphabet.size() - nb_abs);
		for (int a(0); a < alphabet.size(); a++){
			if (proba[x][a] > pow(10.0,-6.0)){
				proba[x][a] -= pseudo_counts;
			}
		}
	}
	
	/* Calculate conservation score by columns */
	for (int x(0); x < ncol; ++x){
		for (int a(0); a < alphabet.size(); a++){
		}
	}
	
	cout << "\nScore is based on Jensen-Shannon measure\n";
	cout << "S = λ R(p,r) + (1 - λ) R(q,r)\n\n";
	return;
	/* Print Conservation score in output file */
	ofstream file(Options::Get().output_name.c_str());
	if (!file.is_open()){
	  cerr << "Cannot open file " << Options::Get().output_name << "\n";
		exit(0);
	}
	for (int col(0); col < ncol; ++col){
	  file << (1.0 - col_cons[col]) * (1 - ((double) msa.getGap(col) / (double) nseq)) << "\n";
	}
	file.close();
}
