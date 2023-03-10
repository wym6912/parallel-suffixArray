// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This is a parallel version of the algorithm described in
//  Juha Karkkainen and Peter Sanders.
//  Simple linear work suffix array construction.
//  Proc. ICALP 2003.  pp 943
// It includes code for finding the LCP
//   Written by Guy Blelloch and Julian Shun

#include <iostream>
#include "sequence.h"
#include "blockRadixSort.h"
#include "parallel.h"
#include "gettime.h"
#include "merge.h"
#include "utils.h"
#include "rangeMin.h"
using namespace std;

typedef pair<uintT,uintT> uintPair;

// Radix sort a pair of integers based on first element
void radixSortPair(uintPair *A, long long n, long long m) {
  intSort::iSort(A, n, m, utils::firstF<uintT,uintT>());
}

inline bool leq(uintT a1, uintT a2,   uintT b1, uintT b2) {
  return(a1 < b1 || (a1 == b1 && a2 <= b2)); 
}                                                  

inline bool leq(uintT a1, uintT a2, uintT a3, uintT b1, uintT b2, uintT b3) {
  return(a1 < b1 || (a1 == b1 && leq(a2, a3, b2, b3))); 
}

struct compS {
  uintT* _s;
  uintT* _s12;
  compS(uintT* s, uintT* s12) : _s(s), _s12(s12) {}
  bool operator () (intT i, intT j) {
    if (i%3 == 1 || j%3 == 1) 
      return leq(_s[i],_s12[i+1], _s[j],_s12[j+1]);
    else
      return leq(_s[i],_s[i+1],_s12[i+2], _s[j],_s[j+1],_s12[j+2]);
  }
};

struct mod3is1 { bool operator() (intT i) {return i%3 == 1;}};

inline intT computeLCP(uintT* LCP12, uintT* rank, myRMQ & RMQ,
    intT j, intT k, uintT* s, uintT n){
 
    intT rank_j=rank[j]-2;
    intT rank_k=rank[k]-2;
  if(rank_j > rank_k) {swap(rank_j,rank_k);} //swap for RMQ query

  intT l = ((rank_j == rank_k-1) ? LCP12[rank_j]
	   : LCP12[RMQ.query(rank_j,rank_k-1)]);

  intT lll = (l<<1)+l;
  if (s[j+lll] == s[k+lll]) {
    if (s[j+lll+1] == s[k+lll+1]) return lll + 2;
    else return lll + 1;
  } 
  return lll;
}
#ifdef _CALC_TIME
timer radixTime;
timer mergeTime;
timer LCPtime;
#endif

// This recursive version requires s[n]=s[n+1]=s[n+2] = 0
// K is the maximum value of any element in s
pair<uintT*,uintT*> suffixArrayRec(uintT* s, long long n, long long K, bool findLCPs) {
  ++n;
  intT n0=(n+2)/3, n1=(n+1)/3, n12=n-n0;
  uintPair *C = (uintPair *) malloc(n12*sizeof(uintPair));

  uintT bits = utils::log2Up(K);
  // if 3 chars fit into a uintT then just do one radix sort
  if (bits+bits+bits <= 8*sizeof(uintT)) {
    parallel_for (intT i=0; i < n12; i++) {
      intT j = 1+i+(i>>1);
      C[i].first = (s[j] << 2*bits) + (s[j+1] << bits) + s[j+2];
      C[i].second = j;}
#ifdef _CALC_TIME
    radixTime.start();
#endif
    radixSortPair(C, n12, ((intT) 1) << 3*bits);
#ifdef _CALC_TIME
    radixTime.stop();
#endif

  // otherwise do 3 radix sorts, one per char
  } else {
    parallel_for (intT i=0; i < n12; i++) {
      intT j = 1 + i + (i >> 1);
      C[i].first = s[j+2]; 
      C[i].second = j;}
#ifdef _CALC_TIME
    radixTime.start();
#endif
    // radix sort based on 3 chars
    radixSortPair(C, n12, K);
    parallel_for (intT i=0; i < n12; i++) C[i].first = s[C[i].second+1];
    radixSortPair(C, n12, K);
    parallel_for (intT i=0; i < n12; i++) C[i].first = s[C[i].second];
    radixSortPair(C, n12, K);
#ifdef _CALC_TIME
    radixTime.stop();
#endif
  }

  // copy sorted results into sorted12
  uintT* sorted12 = newA(uintT,n12); 
  parallel_for (intT i=0; i < n12; i++) sorted12[i] = C[i].second;
  free(C);

  // generate names based on 3 chars
  uintT* name12 = newA(uintT,n12);
  parallel_for (uintT i = 1;  i < n12;  i++) {
    if (s[sorted12[i]] != s[sorted12[i-1]] 
	|| s[sorted12[i]+1] != s[sorted12[i-1]+1] 
	|| s[sorted12[i]+2] != s[sorted12[i-1]+2]) 
      name12[i] = 1;
    else name12[i] = 0;
  }
  name12[0] = 1;
  sequence::scanI(name12, name12, n12, utils::addF<uintT>(), (uintT)0);
  intT names = name12[n12-1];
  
  pair<uintT*,uintT*> SA12_LCP;
  uintT* SA12;
  uintT* LCP12 = NULL;
  // recurse if names are not yet unique
  if (names < n12) {
    uintT* s12  = newA(uintT, n12 + 3);  
    s12[n12] = s12[n12+1] = s12[n12+2] = 0;

    // move mod 1 suffixes to bottom half and and mod 2 suffixes to top
    parallel_for (intT i= 0; i < n12; i++)
      if (sorted12[i]%3 == 1) s12[sorted12[i]/3] = name12[i];
      else s12[sorted12[i]/3+n1] = name12[i];
    free(name12);  free(sorted12);

    SA12_LCP = suffixArrayRec(s12, n12, names+1, findLCPs); 
    SA12 = SA12_LCP.first;
    LCP12 = SA12_LCP.second;
    free(s12);

    // restore proper indices into original array
    parallel_for (intT i = 0;  i < n12;  i++) {
      intT l = SA12[i];
      SA12[i] = (l<n1) ? 3*l+1 : 3*(l-n1)+2;
    }
  } else {
    free(name12); // names not needed if we don't recurse
    SA12 = sorted12; // suffix array is sorted array
    if (findLCPs) {
      LCP12 = newA(uintT, n12+3);
      parallel_for(intT i=0; i<n12+3; i++)
	LCP12[i] = 0; //LCP's are all 0 if not recursing
    }
  }

  // place ranks for the mod12 elements in full length array
  // mod0 locations of rank will contain garbage
  uintT* rank  = newA(uintT, n + 3);
  rank[n]=1; rank[n+1] = 0;
  parallel_for (intT i = 0;  i < n12;  i++) {
      if(SA12[i] < n) rank[SA12[i]] = i + 2;
  }

  
  // stably sort the mod 0 suffixes 
  // uses the fact that we already have the tails sorted in SA12
  uintT* s0  = newA(uintT, n0);
  intT x = sequence::filter(SA12, s0, n12, mod3is1());
  uintPair *D = (uintPair *) malloc(n0*sizeof(uintPair));
  D[0].first = s[n-1]; D[0].second = n-1;
  parallel_for (intT i=0; i < x; i++) {
    D[i+n0-x].first = s[s0[i]-1]; 
    D[i+n0-x].second = s0[i]-1;}
#ifdef _CALC_TIME
  radixTime.start();
#endif
  radixSortPair(D,n0, K);
#ifdef _CALC_TIME
  radixTime.stop();
#endif
  uintT* SA0  = s0; // reuse memory since not overlapping
  parallel_for (intT i=0; i < n0; i++) SA0[i] = D[i].second;
  free(D);

  compS comp(s,rank);
  uintT o = (n%3 == 1) ? 1 : 0;
  uintT *SA = newA(uintT,n); 
#ifdef _CALC_TIME
  mergeTime.start();
#endif
  merge(SA0+o, n0-o, SA12+1-o, n12+o-1, SA, comp);
#ifdef _CALC_TIME
  mergeTime.stop();
#endif
  free(SA0); free(SA12);
  uintT* LCP = NULL;


  //get LCP from LCP12
  if(findLCPs){
    LCP = newA(uintT, n);  
    LCP[n-1] = LCP[n-2] = 0; 
#ifdef _CALC_TIME
    LCPtime.start();
#endif
    myRMQ RMQ(LCP12, n12+3); //simple rmq
    parallel_for(intT i=0;i<n-2;i++){
      intT j = SA[i];
      intT k = SA[i+1];
      volatile uintT CLEN = 16;
      volatile intT ii;
      for (ii=0; ii < CLEN; ii++) 
	if (s[j+ii] != s[k+ii]) break;
      if (ii != CLEN) LCP[i] = ii;
      else {
      	if (j%3 != 0 && k%3 != 0)  
	  LCP[i] = computeLCP(LCP12, rank, RMQ, j, k, s, n); 
	else if (j%3 != 2 && k%3 != 2)
	  LCP[i] = 1 + computeLCP(LCP12, rank, RMQ, j+1, k+1, s, n);
	else 
	  LCP[i] = 2 + computeLCP(LCP12, rank, RMQ, j+2, k+2, s, n);
	  }
    }
#ifdef _CALC_TIME
    LCPtime.stop();
#endif
    free(LCP12);
  }
  free(rank);
  return make_pair(SA, LCP);
}

pair<uintT*,uintT*> suffixArray(unsigned char* s, uintT n, bool findLCPs) {
  // following line is used to fool icpc into starting the scheduler
  if (n < 0)  printf("ouch");
  uintT *ss = newA(uintT, n+3); 
  ss[n] = ss[n+1] = ss[n+2] = 0;
  parallel_for (intT i=0; i < n; i++) ss[i] = ((uintT) s[i])+1;
  intT k = 1 + sequence::reduce(ss, n, utils::maxF<uintT>());
#ifdef _CALC_TIME
  radixTime.clear();
  mergeTime.clear();
  LCPtime.clear();
  pair<uintT*,uintT*> SA_LCP = suffixArrayRec(ss, n, k, findLCPs);
  cout << "Radix sort time: " << radixTime.total() << endl;
  cout << "Merge time: " << mergeTime.total() << endl;
  cout << "LCP time: " << LCPtime.total() << endl;
#else
  pair<uintT*, uintT*> SA_LCP = suffixArrayRec(ss, n, k, findLCPs);
#endif
  free(ss);
  return SA_LCP;
}

#ifdef _WINDLL
#define dlltype(x) __declspec(dllexport) x __stdcall
#define dllcall(x) x __stdcall
#else
#define dlltype(x) x
#define dllcall(x) x
#endif

dlltype(uintT*) suffixArray(unsigned char* s, uintT n) {
  return suffixArray(s, n, false).first;
}

dlltype(uintT*) LCP(unsigned char* s, uintT n){
    return suffixArray(s, n, true).second;
}
