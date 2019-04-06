// DEVELOPMENT HISTORY:
// 2.74: Ground only relevant SCCs, as grounding layer was taking
// too long in some problems.

/*
 Copyright (c) 2017 Odinaldo Rodrigues.
 King's College London.

 Compile with
   g++ -std=c++11 -c grounder.cpp
   g++ -std=c++11 -c eqargsolver-2.2.cpp
   g++ -o eqargsolver-2.2 eqargsolver-2.2.o grounder.o

 EqArgSolver is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 v2.2: This version can be used in decision and enumeration problems
 for the grounded, complete, preferred and stable semantics.

 Computation is done via decomposition of the network into layers,
 which are then successively processed. The outline of execution is
 more or less as follows.

  1. The graph is divided into SCCs and arranged into layers
  2. The trivial SCCs in a layer are combined and jointly processed.
     Each non-trivial SCC in a layer is processed individually
     in ascending layer order.
  3. Each layer is conditioned by a partial solution for
     the previous layers. The conditioning values are propagated using a
     grounding mechanism based on the discrete Gabbay-Rodrigues iteration
     schema. The grounding resolves all nodes in trivial SCCs of
     that layer and possibly some nodes in the non-trivial SCCs.
  4. However, some nodes in the non-trivial SCCs may still be left
     undecided. Finding legal values in the set {0,1} for these nodes
     give rise to complete extensions that may be larger than the
     grounded extension.
    4.1 The remaining undecided nodes cannot be attacked by any node that
        is legally IN (otherwise the grounding would have labelled
	      them OUT). This means that within each working layer conditioned by a
        partial solution to the previous layers, some nodes could possibly
	      be labelled IN resulting in a complete extension larger than the
	      in the extension originally computed.
    4.3 The algorithm tries to label each of these nodes IN in sequence, propagating
        the label forwards and backwards as needed. Of course in a cyclic SCC, the
	      propagation may not succeed, but if it does, this will generate a new
        (larger) extension. This process is applied recursively until no further
        extensions can be generated. This produces all new partial solutions up to
	      the current layer and the process continues until all layers have been
	      processed.
    4.4 The maximal extensions obtained in this way are the preferred extensions.
        If a preferred extension has no undecided nodes, then it is also stable.

*/

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <iostream>
#include <deque>
#include <queue>
#include <set>
#include <vector>
#include <stack>
#include <cmath>
#include <algorithm>
#include <string>
#include <limits>
#include <iomanip>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <gmpxx.h>
#include <cstdint>
#include <cassert>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <boost/functional/hash.hpp>

// #include "grounder.h"
#include "Timer.h"


/////////////////////////////////////////////////////////////////////////////////////////////
uint64_t memory_usage;

// use this when calling STL object if you want
// to keep track of memory usage
template <class T> class MemoryCountingAllocator {
    public:
        // type definitions
        typedef T value_type;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;


        // rebind allocator to type U
        template <class U> struct rebind {
            typedef MemoryCountingAllocator<U> other;
        };

        pointer address(reference value) const {
            return &value;
        }
        const_pointer address(const_reference value) const {
            return &value;
        }

        MemoryCountingAllocator() : base() {}
        MemoryCountingAllocator(const MemoryCountingAllocator &) : base() {}
        template <typename U>
            MemoryCountingAllocator(const MemoryCountingAllocator<U> &) : base() {}
        ~MemoryCountingAllocator() {}

        // return maximum number of elements that can be allocated
        size_type max_size() const throw() {
            return base.max_size();
        }

        pointer allocate(size_type num, const void * p = 0) {
            memory_usage += num * sizeof(T);
            return base.allocate(num,p);
        }

        void construct(pointer p, const T &value) {
            return base.construct(p,value);
        }

        // destroy elements of initialized storage p
        void destroy(pointer p) {
            base.destroy(p);
        }

        // deallocate storage p of deleted elements
        void deallocate(pointer p, size_type num ) {
            memory_usage -= num * sizeof(T);
            base.deallocate(p,num);
        }
        std::allocator<T> base;
};

// for our purposes, we don't want to distinguish between allocators.
template <class T1, class T2>
bool operator==(const MemoryCountingAllocator<T1> &, const T2 &) throw() {
    return true;
}

template <class T1, class T2>
bool operator!=(const MemoryCountingAllocator<T1> &, const T2 &) throw() {
    return false;
}

void initializeMemUsageCounter()  {
    memory_usage = 0;
}

uint64_t getMemUsageInBytes()  {
    return memory_usage;
}


typedef std::vector<uint32_t,MemoryCountingAllocator<uint32_t> >  __uint32_vec;
typedef std::vector<__uint32_vec,MemoryCountingAllocator<__uint32_vec> >  __uint32_vec_vec;
typedef std::vector<mpz_class>  __mpz_vec;


/////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

typedef unsigned char IntArgValue_T;
typedef unsigned int IntArgId_T;
typedef string ExtArgId_T;
typedef map<IntArgId_T,IntArgValue_T> sol_type;
typedef set<sol_type> sol_col_type;

struct
ArgNode_T {
    int layer;
    ExtArgId_T extArgId;
    vector<IntArgId_T> attsIn;
    vector<IntArgId_T> attsOut;
};


struct setHash {
    size_t operator()(unordered_set<IntArgId_T> const& theSet) const {
        int result=0;
        for (unordered_set<IntArgId_T>::const_iterator it=theSet.begin(); it!=theSet.end(); it++) {
            result = result + pow(2.0,*it);
        };
        return (result % 300000);
    }
};

namespace std {
    template<> 
        struct hash<mpz_class> {
            std::size_t operator()(const mpz_class& z) const {
                return z.get_ui();
            }
        };
}

// namespace std {
//  template<> 
//  struct hash<__uint32_uset> {
//    std::size_t operator()(const __uint32_uset & z) const {
//      return boost::hash_range(z.begin(),z.end());
//      // return 0; // z.get_ui();
//      }
//  };
// }

struct argHash {
    size_t operator()(IntArgId_T const& argId) const {
        return argId;
    }
};


struct
nodeTuple

{
    int index;
    int lowindex;
    bool isRemoved;
};

struct
sccPair
{
    IntArgId_T node;
    IntArgId_T parent;
};

struct
solNode {
    sol_type * solValue;
    solNode * parent;
    set<solNode *> children;
};

// Global variables:

const long int max_ext_length = 1000000; // The maximum number of characters in an extension
const IntArgId_T maxIntArgId = numeric_limits<unsigned int>::max(); // The maximum number of arguments (16 bits) 0 -> RESERVED
const IntArgId_T nullIntArgId = 0; // The invalid internal argument Id used for special purposes
const double inv_phi=(sqrt(5)-1)/2;
const uint32_t bytes_per_limb=mp_bits_per_limb/8;


map<string, string> problem;
unordered_map<ExtArgId_T, IntArgId_T> intArgId;
unordered_map<IntArgId_T, ArgNode_T> argList;
vector<string> supportedFormats;
vector<string> supportedProbs;
IntArgId_T totArgs, totEdges, totSCCs;
bool decisionProblem = false;
int tolerance = std::numeric_limits<long int>::epsilon ();
int min_int = std::numeric_limits<int>::min();
int prefCtr = 0;
int stepCtr = 0;
int findExtsCtr = 0;
bool quickMode = false;
bool debug = false;
bool deep = false;
bool verbOutput = false;
bool quietMode=false, perfMode=true, noSets=false, stepCount=false, checkSols=false, useConflicts=false;
bool trace = false;
bool skeptical, stableSemantics, stableSafe, globalSuccess, groundedSemantics, triathlon;
IntArgId_T intArgToFind;
string executableName, solutionName, masterSol, secondSol, tgfFilename;
size_t savedFromConflicts, totConflicts, numRecCalls, intMaxSols=0, maxSols=0;
Timer updTime, insTime, cmpTime, stratTimer, groundingTimer, solutionTimer, compositionTimer, globalTimer, selectTimer, poss_in_timer;
stringstream bugstr;
mpz_class multip;
size_t word_length, totElems=0, shift_value;
long long int totExts=0;

bool extComp (vector<uint32_t,MemoryCountingAllocator<unsigned int> > & veca, vector<uint32_t,MemoryCountingAllocator<unsigned int> > & vecb) {
    return lexicographical_compare(veca.begin(), veca.end(),
            vecb.begin(), vecb.end());
}


struct fibonacci_hash {
    size_t operator()(mpz_class hash) const {
        mpz_class result;
        mpz_mul(result.get_mpz_t(),hash.get_mpz_t(),multip.get_mpz_t());
        // cout << result.get_mpz_t() << endl;
        mpz_class andNum, oneNum, remainder;
        andNum=pow(2,64);
        oneNum=pow(2,0);
        mpz_sub(andNum.get_mpz_t(),andNum.get_mpz_t(),oneNum.get_mpz_t());
        mpz_mod(remainder.get_mpz_t(),result.get_mpz_t(),andNum.get_mpz_t());
        mpz_fdiv_q_2exp(result.get_mpz_t(),remainder.get_mpz_t(),shift_value); 
        return result.get_ui();
    }
};

size_t f_hash(mpz_class hash) {
    mpz_class result;
    mpz_mul(result.get_mpz_t(),hash.get_mpz_t(),multip.get_mpz_t());
    cout << result.get_mpz_t() << endl;
    mpz_class andNum, oneNum, remainder;
    andNum=pow(2,64);
    oneNum=pow(2,0);
    mpz_sub(andNum.get_mpz_t(),andNum.get_mpz_t(),oneNum.get_mpz_t());
    cout << "64 '1' bits: " << andNum.get_str(2) << endl;
    mpz_mod(remainder.get_mpz_t(),result.get_mpz_t(),andNum.get_mpz_t());
    cout << "Significant bits: " << remainder.get_str(2) << endl;
    cout << "Shifting " << shift_value << " bits:" << endl;
    mpz_fdiv_q_2exp(result.get_mpz_t(),remainder.get_mpz_t(),shift_value);
    cout << "Result: " << result.get_str(2) << endl;
    return result.get_ui();
};

size_t f_int_hash(size_t hash) {
    size_t multip=pow(2,64)*inv_phi;
    return (hash*multip) >> 61;
};

template <typename Iter>
    Iter
next_iter(Iter iter)
{
    return ++iter;
}

template <typename Iter, typename Cont>
    bool
is_last(Iter iter, const Cont& cont)
{
    return (iter != cont.end()) && (next_iter(iter) == cont.end());
}

IntArgId_T
createIntArgId(ExtArgId_T nodeId) {
    return argList.size()+1;
}

bool
extArgIdExists (ExtArgId_T nodeId) {
    return intArgId.find(nodeId)!=intArgId.end();
}

IntArgId_T
getIntArgId (ExtArgId_T nodeId) {
    unordered_map<ExtArgId_T,IntArgId_T>::iterator idx = intArgId.find(nodeId);
    if (idx != intArgId.end()) {
        return idx->second;
    } else {
        return nullIntArgId;
    }
}

void
display(string message) {
    cout << message;
};

void
display_set (const unordered_set<ExtArgId_T> * extension) {
    cout << "[";
    for (unordered_set<ExtArgId_T>::const_iterator it=extension->begin();
            it!=extension->end(); it++) {
        cout << (*it);
        if (next(it,1)!=extension->end()) {
            cout << ",";
        };
    };
    cout << "]";
};

string
getExtArgId(IntArgId_T intArgId) {
    return argList[intArgId].extArgId;
};

void
wait () {
    cout << "Press ENTER to continue..." << flush;
    cin.ignore (std::numeric_limits<std::streamsize>::max (), '\n');
}

void
wait (string message) {
    cout << message << endl;
    cout << "Press ENTER to continue..." << flush;
    cin.ignore (std::numeric_limits<std::streamsize>::max (), '\n');
}


IntArgId_T
getNumOfArgs(ifstream * infile) {
    bool endOfNodes = false;
    IntArgId_T curIntArgId = 0;
    char * pch;
    char pch_str[256];
    string inputStr, nodeID;
    while (!infile->eof() && !endOfNodes) {
        getline(*infile, inputStr);
        strcpy(pch_str, inputStr.c_str());
        pch = strtok(pch_str, " ,:");
        if (pch != NULL) {
            if (string(pch) != "#") {
                if ((curIntArgId + 1) < maxIntArgId) {
                    curIntArgId++;
                };
                pch = strtok(NULL, " ,:");
                if (pch != NULL) {
                    // Warning: ignoring remaining of line lineNum
                };
            } else {
                endOfNodes = true;
            };
        };
    };
    // Return to the beginning of file
    infile->clear();
    infile->seekg(0, ios::beg);
    return curIntArgId;
}

size_t mem_cost_uint32_vec(__uint32_vec & theVec) {
    // 24bytes (3 words) + 4bytes (node) per node:
    // cout << endl << "Vector:" << endl;
    // cout << theVec.size() << ":" << theVec.capacity() << endl;
    // cout << theVec.size() << ":" << theVec.capacity() << endl;
    // cout << "---------------------------------------" << endl;
    return 24 + (4 * theVec.size());
}

/// size_t mem_cost_uint32_set(set<uint32_t> & theSet) {
///   // 24bytes (3 pointers) + 8bytes (node) per node:
///   return 28 * theSet.size();
/// }
/// 
/// size_t mem_cost_uint32_uset(__uint32_uset & theSet) {
///   // 24bytes (3 pointers) + 8bytes (node) per node:
///   return 32 * theSet.size();
/// }

size_t mem_cost_mpz_object (mpz_class * extSol) {
    // cout << extSol->get_mpz_t()->_mp_alloc << " allocated limbs." << endl;
    // cout << (mp_bits_per_limb/8) << " bytes per limb." << endl;
    return extSol->get_mpz_t()->_mp_alloc * (mp_bits_per_limb/8);
    // Number of allocated limbs to this object * the number of bytes per limb
}

bool
readFile() {
    if (!tgfFilename.empty()) {
        char pch_str[256];
        char * pch;
        ifstream infile;
        bool endOfNodes, error;
        string inputStr, nodeID, sourceID, targetID;
        int lineNum;
        IntArgId_T curIntArgId;
        vector<IntArgId_T>::iterator it2;
        unordered_map<IntArgId_T, ExtArgId_T>::iterator intSourceIdPtr,
            intTargetIdPtr;
        map<IntArgId_T, ExtArgId_T>::iterator extArgIdPtr;
        infile.open(tgfFilename.c_str(), ifstream::in);
        if (infile) {
            totArgs = getNumOfArgs(&infile);
            if (totArgs <= maxIntArgId) {
                intArgId.reserve(totArgs);
                // Read Nodes
                lineNum = 0;
                error = false;
                endOfNodes = false;
                nodeID = "";
                while (!infile.eof() && !error && !endOfNodes) {
                    getline(infile, inputStr);
                    lineNum++;
                    strcpy(pch_str, inputStr.c_str());
                    pch = strtok(pch_str, " ,:");
                    if (pch != NULL) {
                        if (string(pch) != "#") {
                            nodeID = string(pch);
                            if (!extArgIdExists(nodeID)) {
                                // New node ID. Good to go.
                                if (argList.size() < maxIntArgId) {
                                    curIntArgId = intArgId.size();
                                } else {
                                    cout << "Maximum number of arguments exceeded! Aborting." << endl;
                                    exit(0);
                                };
                                intArgId[nodeID]=curIntArgId;
                            }; // else: Warning: node nodeID specified more than once! Ignoring.
                            pch = strtok(NULL, " ,:");
                            if (pch != NULL) {
                                // Warning: ignoring remaining of line lineNum
                            };
                        }
                        else {
                            endOfNodes = true;
                        };
                    };
                };
            } else {
                cout << "Maximum number of arguments of this solver is " << maxIntArgId << "." << endl;
                cout << "Capacity exceeded. Aborting!" << endl;
                error = true;
            };
        } else {
            cout << "Cannot open file \""
                << tgfFilename
                << "\"." << endl;
            error = true;
        };
        infile.close();
        return (!error);
    } else {
        return true;
    }
}

template <typename T>
bool read_uint32_extension(string extStr,T & extension) {
    char pch_str[extStr.size()+1];
    char * pch;
    string nodeID;
    // if (extStr.size() > max_ext_length) {
    //   cout << "Houston we have a problem!" << endl;
    //   cout << "Reserved " << extStr.size()+1 << " characters" << endl;
    //   cout << "This extension is " << extStr.size() << " chars long!" <<endl;
    //   exit(0);
    // };
    strcpy(pch_str, extStr.c_str());
    pch = strtok(pch_str, " ,:[]");
    uint32_t thisId;
    if (pch != NULL) {
        nodeID = string(pch);
        if (intArgId.find(nodeID)==intArgId.end()) {
            intArgId[nodeID]=intArgId.size();
        };
        thisId=getIntArgId(nodeID);
        extension.insert(thisId);
        pch = strtok(NULL, " ,:[]");
        while (pch != NULL) {
            nodeID = string(pch);
            if (intArgId.find(nodeID)==intArgId.end()) {
                intArgId[nodeID]=intArgId.size();
            };
            thisId=getIntArgId(nodeID);
            extension.insert(thisId);
            pch = strtok(NULL, " ,:[]");
        };
        return true;
    };
    return true;
}

int parse_extension(string extStr) {
    char pch_str[extStr.size()+1];
    char * pch;
    string nodeID;
    strcpy(pch_str, extStr.c_str());
    pch = strtok(pch_str, " ,:[]");
    int elemCtr=0;
    if (pch != NULL) {
        nodeID = string(pch);
        if (intArgId.find(nodeID)==intArgId.end()) {
            intArgId[nodeID]=intArgId.size();
        };
        elemCtr=1;
        pch = strtok(NULL, " ,:[]");
        while (pch != NULL) {
            nodeID = string(pch);
            elemCtr++;
            if (intArgId.find(nodeID)==intArgId.end()) {
                intArgId[nodeID]=intArgId.size();
            };
            pch = strtok(NULL, " ,:[]");
        };
    };
    return elemCtr;
}

template <typename T>
bool read_uint32_vec_extension(string extStr,T & extension) {
    char pch_str[extStr.size()+1];
    char * pch;
    string nodeID;
    // if (extStr.size() > max_ext_length) {
    //   cout << "Houston we have a problem!" << endl;
    //   cout << "Reserved " << extStr.size()+1 << " characters" << endl;
    //   cout << "This extension is " << extStr.size() << " chars long!" <<endl;
    //   exit(0);
    // };
    strcpy(pch_str, extStr.c_str());
    pch = strtok(pch_str, " ,:[]");
    uint32_t thisId;
    if (pch != NULL) {
        nodeID = string(pch);
        if (intArgId.find(nodeID)==intArgId.end()) {
            intArgId[nodeID]=intArgId.size();
        };
        thisId=getIntArgId(nodeID);
        extension.push_back(thisId);
        pch = strtok(NULL, " ,:[]");
        while (pch != NULL) {
            nodeID = string(pch);
            if (intArgId.find(nodeID)==intArgId.end()) {
                intArgId[nodeID]=intArgId.size();
            };
            thisId=getIntArgId(nodeID);
            extension.push_back(thisId);
            pch = strtok(NULL, " ,:[]");
        };
        sort (extension.begin(), extension.end());
        return true;
    };
    return true;
}


bool
read_mpz_extension(string extStr, mpz_class *  intExt) {
    char pch_str[extStr.size()+1];
    char * pch;
    string nodeID;
    // if (extStr.size() > max_ext_length) {
    //   cout << "Houston we have a problem!" << endl;
    //   cout << "Reserved " << extStr.size()+1 << " characters" << endl;
    //   cout << "This extension is " << extStr.size() << " chars long!" <<endl;
    //   exit(0);
    // };
    strcpy(pch_str, extStr.c_str());
    pch = strtok(pch_str, " ,:[]");
    IntArgId_T thisBit;
    if (pch != NULL) {
        nodeID = string(pch);
        if (intArgId.find(nodeID)==intArgId.end()) {
            intArgId[nodeID]=intArgId.size();
        };
        thisBit=getIntArgId(nodeID);
        mpz_setbit(intExt->get_mpz_t(),thisBit);
        pch = strtok(NULL, " ,:[]");
        // mpz_t integ, bit;
        // mpz_init2(integ,100);
        // mpz_init2(bit,100);			 
        while (pch != NULL) {
            nodeID = string(pch);
            if (intArgId.find(nodeID)==intArgId.end()) {
                intArgId[nodeID]=intArgId.size();
            };
            thisBit=getIntArgId(nodeID); // 0 was reserved for nullIntArgId
            // cout << "thisBit: " << thisBit << endl;
            mpz_setbit(intExt->get_mpz_t(),thisBit);
            // if (thisBit > 999998) {
            // 	cout << "Limbs allocated: ";
            // 	cout << intExt->get_mpz_t()->_mp_alloc << endl;
            // 	cout << "Bytes allocated: ";
            // 	cout << intExt->get_mpz_t()->_mp_alloc * (mp_bits_per_limb/8) << " bytes" << endl;
            // };
            pch = strtok(NULL, " ,:[]");
        };
        // std::cout << intExt->get_mpz_t() << "-->" << intExt->get_str(2) << std::endl; //base 2 representation
        return true;
    };
    return true;
}

bool
match_sols(unordered_set<ExtArgId_T> * sol_one,
        unordered_set<ExtArgId_T> * sol_two) {
    return ((*sol_one) == (*sol_two));
}

string trim(const string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

long long int
count_solutions(string solutionName,double & avgExtSize) {
    // Attempt to find and check solutions
    string inputStr, solStr;
    unsigned int extCtr=0;
    bool error=false;
    ifstream infile;
    totElems=0;
    infile.open(solutionName.c_str(), ifstream::in);
    if (infile) {
        error = false;
        size_t solBeg, solEnd, firstBracket;
        bool first = true;
        bool end=false;
        while (infile && !error && !end) {
            getline(infile, inputStr, ']');
            if ((inputStr.size() > 0) && !infile.eof() && !infile.fail()) {
                inputStr+="]";
                firstBracket = inputStr.find("[");
                if (first) {
                    first=false;
                    solBeg=inputStr.find("[",firstBracket+1); 
                    if (solBeg==string::npos) {
                        solBeg=firstBracket;
                        //cout << "Single extension found!" << endl;
                    } else {
                        //cout << "Multiple extensions found!" << endl;
                    };
                } else {
                    solBeg=inputStr.find("[");
                };
                solEnd=inputStr.find("]");
                solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
                //cout << solStr<< "-> ";
                int n=parse_extension(solStr);
                totElems+=n;
                //cout << n << " elements" << endl;
                extCtr++;
            } else {
                end=true;
            };
        };
        infile.close();
        avgExtSize=(totElems*1.0)/extCtr;
    } else {
        error = true;
    };
    if (!error) {
        return extCtr;
    } else {
        return -1;
    };
}

template <typename T>
bool read_set_mpz_solutions(T * allExtExts, string solutionName, size_t & totBytes) {
    // Attempt to find and check solutions
    string inputStr, solStr;
    unsigned int extCtr=0;
    bool error=false;
    mpz_class * extSol;
    ifstream infile;
    totBytes=0;
    infile.open(solutionName.c_str(), ifstream::in);
    //  int oldPerc=0, newPerc=0;
    if (infile) {
        error = false;
        size_t solBeg, solEnd, firstBracket;
        bool first = true;
        bool end=false;
        while (infile && !error && !end) {
            getline(infile, inputStr, ']');
            if (inputStr.size() > 0) {
                inputStr+="]";
                firstBracket = inputStr.find("[");
                if (first) {
                    first=false;
                    solBeg=inputStr.find("[",firstBracket+1);
                } else {
                    solBeg=inputStr.find("[");
                };
                solEnd=inputStr.find("]");
                solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
                extSol = new mpz_class;
                size_t theseBytes=0;
                if (read_mpz_extension(solStr,extSol)) {
                    // cout << "Extension successfully read." << endl;
                    extCtr++;
                    theseBytes=mem_cost_mpz_object(extSol);
                    // cout << "This extension needed " << theseBytes << " bytes." << endl;
                    // newPerc=((extCtr*100)/totExts);
                    // if (newPerc!=oldPerc) {
                    //   oldPerc=newPerc;
                    //   printf("\r%d",newPerc); cout << "%";
                    //   fflush(stdout);
                    // };
                    // std::cout << "Extension " << extCtr << ": " << extSol->get_mpz_t() << "-->" << extSol->get_str(2) << std::endl; //base 2 representation
                    allExtExts->insert(*extSol);
                    totBytes=totBytes+ (3*8) + theseBytes;
                } else {
                    error = true;
                };
            } else {
                end=true;
            };
        };
        infile.close();
    } else {
        error = true;
    };
    return (!error);
}

/*bool
  read_set_uint32_solutions(__uint32_set_set * allExtExts, string solutionName, size_t & totBytes) {
// Attempt to find and check solutions
string inputStr, solStr;
unsigned int extCtr=0;
bool error=false;
set<uint32_t> extSol;
ifstream infile;
totBytes=0;
infile.open(solutionName.c_str(), ifstream::in);
if (infile) {
error = false;
size_t solBeg, solEnd, firstBracket;
bool first = true;
bool end=false;
while (infile && !error && !end) {
getline(infile, inputStr, ']');
if (inputStr.size() > 0) {
inputStr+="]";
firstBracket = inputStr.find("[");
if (first) {
first=false;
solBeg=inputStr.find("[",firstBracket+1);
} else {
solBeg=inputStr.find("[");
};
solEnd=inputStr.find("]");
solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
extSol.clear();
if (read_uint32_extension(solStr,extSol)) {
totBytes=totBytes+mem_cost_uint32_set(extSol);
extCtr++;
if (verbOutput) { printf("\r%d",extCtr); cout << " extensions"; };
// std::cout << "Extension " << extCtr << ": " << extSol->get_mpz_t() << "-->" << extSol->get_str(2) << std::endl; //base 2 representation
allExtExts->insert(extSol);
} else {
error = true;
};
} else {
end=true;
};
};
if (verbOutput) { cout << endl; };
infile.close();
} else {
error = true;
};
return (!error);
}

bool
read_uset_uint32_solutions(__uint32_uset_uset * allExtExts, string solutionName,size_t & estTotBytes) {
// Attempt to find and check solutions
string inputStr, solStr;
unsigned int extCtr=0;
bool error=false;
__uint32_uset extSol;
ifstream infile;
infile.open(solutionName.c_str(), ifstream::in);
//  int oldPerc=0, newPerc=0;
estTotBytes=0;
if (infile) {
error = false;
size_t solBeg, solEnd, firstBracket;
bool first = true;
bool end=false;
while (infile && !error && !end) {
getline(infile, inputStr, ']');
if (inputStr.size() > 0) {
inputStr+="]";
firstBracket = inputStr.find("[");
if (first) {
    first=false;
    solBeg=inputStr.find("[",firstBracket+1);
} else {
    solBeg=inputStr.find("[");
};
solEnd=inputStr.find("]");
solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
extSol.clear();
if (read_uint32_extension(solStr,extSol)) {
    estTotBytes=estTotBytes+mem_cost_uint32_uset(extSol);
    extCtr++;
    printf("\r%d",extCtr); cout << " extensions   ";

    // newPerc=((extCtr*100)/totExts);
    // if (newPerc!=oldPerc) {
    //   oldPerc=newPerc;
    //   printf("\r%d",newPerc); cout << "%";
    //   fflush(stdout);
    // };
    // std::cout << "Extension " << extCtr << ": " << extSol->get_mpz_t() << "-->" << extSol->get_str(2) << std::endl; //base 2 representation
    allExtExts->insert(extSol);
} else {
    error = true;
};
} else {
    end=true;
};
};
cout << endl;
infile.close();
} else {
    error = true;
};
return (!error);
}
*/
bool
read_vec_uint32_solutions(__uint32_vec_vec * allExtExts, string solutionName,size_t & totBytes) {
    // Attempt to find and check solutions
    string inputStr, solStr;
    unsigned int extCtr=0;
    bool error=false;
    __uint32_vec extSol;
    ifstream infile;
    infile.open(solutionName.c_str(), ifstream::in);
    //  int oldPerc=0, newPerc=0;
    totBytes=0;
    if (infile) {
        error = false;
        size_t solBeg, solEnd, firstBracket;
        bool first = true;
        bool end=false;
        while (infile && !error && !end) {
            getline(infile, inputStr, ']');
            if (inputStr.size() > 0) {
                inputStr+="]";
                firstBracket = inputStr.find("[");
                if (first) {
                    first=false;
                    solBeg=inputStr.find("[",firstBracket+1);
                } else {
                    solBeg=inputStr.find("[");
                };
                solEnd=inputStr.find("]");
                solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
                extSol.clear();
                if (read_uint32_vec_extension(solStr,extSol)) {
                    extSol.shrink_to_fit();
                    totBytes+=mem_cost_uint32_vec(extSol);
                    // cout << "Extension successfully read." << endl;
                    extCtr++;
                    if (verbOutput) { printf("\r%d",extCtr); };

                    // newPerc=((extCtr*100)/totExts);
                    // if (newPerc!=oldPerc) {
                    //   oldPerc=newPerc;
                    //   printf("\r%d",newPerc); cout << "%";
                    //   fflush(stdout);
                    // };
                    // std::cout << "Extension " << extCtr << ": " << extSol->get_mpz_t() << "-->" << extSol->get_str(2) << std::endl; //base 2 representation
                    allExtExts->push_back(extSol);
                } else {
                    error = true;
                };
            } else {
                end=true;
            };
        };
        totBytes+=24;
        if (verbOutput) { cout << " extensions" << endl; };
        Timer sortTimer;
        sortTimer.start();
        if (verbOutput) { cout << "Sorting extensions... "; };
        sort(allExtExts->begin(),allExtExts->end(), extComp);
        if (verbOutput) {
            cout << "Extensions sorted in " << sortTimer.milli_elapsed() << "ms." << endl;
        };
        infile.close();
    } else {
        error = true;
    };
    return (!error);
}

bool
read_vec_mpz_solutions(__mpz_vec * allExtExts, string solutionName, uint64_t & totBytes) {
    // Attempt to find and check solutions
    string inputStr, solStr;
    unsigned int extCtr=0;
    bool error=false;
    mpz_class * extSol;
    ifstream infile;
    infile.open(solutionName.c_str(), ifstream::in);
    //  int oldPerc=0, newPerc=0;
    // cout << "I'm here!" << endl;
    if (infile) {
        error = false;
        size_t solBeg, solEnd, firstBracket;
        bool first = true;
        bool end=false;
        while (infile && !error && !end) {
            getline(infile, inputStr, ']');
            if ((inputStr.size() > 0) && !infile.eof() && !infile.fail()) {
                inputStr+="]";
                firstBracket = inputStr.find("[");
                if (first) {
                    first=false;
                    solBeg=inputStr.find("[",firstBracket+1);
                    // cout << solBeg << endl;
                    if (solBeg==string::npos) {
                        solBeg=firstBracket;
                        //cout << "Single extension found!" << endl;
                    } else {
                        //cout << "Multiple extensions found!" << endl;
                    };
                } else {
                    solBeg=inputStr.find("[");
                };
                solEnd=inputStr.find("]");
                solStr=inputStr.substr(solBeg,solEnd-solBeg+1);
                extSol=new mpz_class;
                if (read_mpz_extension(solStr,extSol)) {
                    size_t numOfBytes = bytes_per_limb * extSol->get_mpz_t()->_mp_alloc;
                    // size_t numOfSmallBytes = bytes_per_limb * extSol->get_mpz_t()->_mp_size;
                    // size_t size = (mpz_sizeinbase (extSol->get_mpz_t(), 2) + CHAR_BIT-1) / CHAR_BIT;
                    // cout << "Most significant bit is " << mpz_sizeinbase(extSol->get_mpz_t(), 2) << "::" << ceil(mpz_sizeinbase(extSol->get_mpz_t(), 2)/8) << endl;
                    // cout << "This extension occupies " << numOfBytes << " bytes" << endl;
                    // cout << "Extension successfully read." << endl;
                    totBytes+=numOfBytes;
                    extCtr++;
                    ////==>> printf("\r%d",extCtr); 

                    // newPerc=((extCtr*100)/totExts);
                    // if (newPerc!=oldPerc) {
                    //   oldPerc=newPerc;
                    //   printf("\r%d",newPerc); cout << "%";
                    //   fflush(stdout);
                    // };
                    // std::cout << "Extension " << extCtr << ": " << extSol->get_mpz_t() << "-->" << extSol->get_str(2) << std::endl; //base 2 representation
                    allExtExts->push_back(*extSol);
                } else {
                    cout << "Invalid solution file!" << endl;
                    error = true;
                };
            } else {
                end=true;
            };
        };
        // cout << " extensions" << endl;
        Timer sortTimer;
        sortTimer.start();
        if (verbOutput) { cout << "Sorting extensions... "; };
        sort(allExtExts->begin(),allExtExts->end());
        if (verbOutput) {
            cout << "Extensions sorted in " << sortTimer.milli_elapsed()
                << "ms." << endl; }
        infile.close();
    } else {
        error = true;
    };
    // cout << "Total bytes: " << totBytes << endl;
    return (!error);
}

template <typename T>
bool check_mpz_solutions(T * allMasterExts, T * allSecondExts,size_t & totBytes) {
    // multip=pow(2,64)*inv_phi;
    // mpz_class val;
    // for (int n=0; n<17; n++) {
    //   val=n;
    //   cout << "hash(" << n << ")=" << f_hash(val) << endl;
    // };
    // exit(0);
    bool sound = false;
    bool complete = false;
    /// DELETE: __mpz_set * allMasterExts = new __mpz_set;
    cout << "Reading master solutions..." << endl;
    if (read_set_mpz_solutions(allMasterExts,masterSol,totBytes)) {
        cout << "Reading secondary solutions..." << endl;
        if (read_set_mpz_solutions(allSecondExts,secondSol,totBytes)) {
            typename T::iterator mastExtIt=allMasterExts->begin();
            bool thisFound=true;
            while (thisFound && (mastExtIt!=allMasterExts->end())) {
                thisFound=allSecondExts->find(*mastExtIt)!=allSecondExts->end();
                if (!thisFound) {
                    if (verbOutput) {
                        cout << endl << "ERROR: Missing extension "; cout<< *mastExtIt;
                        cout << " in secondary solutions!" << endl;
                    };
                };
                if (thisFound) {
                    mastExtIt++;
                };
            };
            if (thisFound) {
                complete = true;
            };
            typename T::iterator secExtIt=allSecondExts->begin();
            while (thisFound && (secExtIt!=allSecondExts->end())) {
                thisFound = allMasterExts->find(*secExtIt)!=allMasterExts->end();
                if (!thisFound) {
                    if (verbOutput) {
                        cout << endl << "ERROR: Computed extension "; cout << *secExtIt;
                        cout << " not found in master solutions!";
                        cout << endl;
                    };
                };
                secExtIt++;
            };
            if (thisFound) {
                sound = true;
            };
        } else {
            cout << "ERROR: Could not read secondary solution file \"" << secondSol << "\"" << endl;
        };
    } else {
        cout << "ERROR: Could not read master solution file \"" << masterSol << "\"" << endl;
    };
    if (verbOutput) {
        cout << endl;
        if (sound && complete) {
            display("OK"); cout << endl;
        } else {
            display("ERROR"); cout << endl;
            // wait();
        };
    };
    return (sound && complete);
}

void print_one_vec (__uint32_vec & aVec) {
    for (auto elem : aVec) {
        cout << elem << " ";
    };
};

void print_vec (__uint32_vec_vec * allMasterExts) {
    for (auto avec : *allMasterExts) {
        print_one_vec(avec);
        cout << endl;
    };
    cout << endl;
};

bool check_vec_uint32_solutions(__uint32_vec_vec * allMasterExts, __uint32_vec_vec * allSecondExts,size_t & totBytes) {
    // multip=pow(2,64)*inv_phi;
    // mpz_class val;
    // for (int n=0; n<17; n++) {
    //   val=n;
    //   cout << "hash(" << n << ")=" << f_hash(val) << endl;
    // };
    // exit(0);
    bool result = false;
    /// DELETE: __mpz_set * allMasterExts = new __mpz_set;
    if (verbOutput) {
        cout << "Reading master solutions... " << endl;
    };
    Timer readTimer;
    readTimer.restart();
    if (read_vec_uint32_solutions(allMasterExts,masterSol,totBytes)) {
        if (verbOutput) {
            cout << allMasterExts->size() << " extensions read in " << readTimer.milli_elapsed() << "ms -> " << totBytes << " bytes" << endl;
            cout << "Reading secondary solutions... " << endl;
        };
        initializeMemUsageCounter();
        assert(getMemUsageInBytes() == 0);
        totBytes=0;
        if (read_vec_uint32_solutions(allSecondExts,secondSol,totBytes)) {
            // print_vec(allSecondExts);
            if (allMasterExts->size()==allSecondExts->size()) {
                if (verbOutput) {
                    cout << "Extensions have the same sizes: " << allMasterExts->size() << endl;
                };
                __uint32_vec_vec::iterator mastExtIt=allMasterExts->begin();
                __uint32_vec_vec::iterator secoExtIt=allSecondExts->begin();
                bool end=mastExtIt==allMasterExts->end();
                while (!end && *mastExtIt==*secoExtIt) {
                    // print_one_vec(*mastExtIt); cout << endl; // 
                    // print_one_vec(*secoExtIt); cout << endl; // 
                    // cout << "----------------------------------------------------------------" << endl;
                    mastExtIt++;
                    secoExtIt++;
                    end=(mastExtIt==allMasterExts->end());
                };
                result=end;
            } else {
                result = false;
            };
        } else {
            cout << "ERROR: Could not read secondary solution file \"" << secondSol << "\"" << endl;
        };
    } else {
        cout << "ERROR: Could not read master solution file \"" << masterSol << "\"" << endl;
    };
    if (verbOutput) {
        if (result) {
            display("OK"); cout << endl;
        } else {
            display("ERROR"); cout << endl;
            // wait();
        };
    };
    return (result);
}

bool
check_vec_mpz_solutions(__mpz_vec * allMasterExts, __mpz_vec * allSecondExts,size_t & totBytes) {
    bool result = false;
    if (verbOutput) { cout << "Reading master solutions..." << endl; };
    initializeMemUsageCounter();
    assert(getMemUsageInBytes() == 0);
    if (read_vec_mpz_solutions(allMasterExts,masterSol,totBytes)) {
        if (read_vec_mpz_solutions(allSecondExts,secondSol,totBytes)) {
            __mpz_vec::iterator masterExtIt=allMasterExts->begin();
            __mpz_vec::iterator secondExtIt=allSecondExts->begin();
            /*std::cout << "Looping through master:" <<std::endl;
            for (mpz_class member : *allMasterExts) {
                std::cout << member.get_mpz_t() << ", ";
            }
            std::cout << "Done." << std::endl;
            std::cout << "Looping through second:" <<std::endl;
            for (mpz_class member : *allSecondExts) {
                std::cout << member.get_mpz_t() << ", ";
            }
            std::cout << "Done." << std::endl;*/
            int correct = 0;
            while (masterExtIt != allMasterExts->end() && secondExtIt != allSecondExts->end())
            {
                if (*masterExtIt == *secondExtIt) {
                    masterExtIt++, secondExtIt++;
                    correct++;
                    continue;
                }
                if (*masterExtIt < *secondExtIt) {
                    masterExtIt++;
                    continue;
                }
                if (*masterExtIt > *secondExtIt) {
                    secondExtIt++;
                    continue;
                }
            }
/*            while (!end && *mastExtIt==*secoExtIt) {
                mastExtIt++;
                secoExtIt++;
                end=(mastExtIt==allMasterExts->end());
                i++;
            };*/
            int total = allMasterExts->size();
            int secondTotal = allSecondExts->size();

            result = (total == secondTotal && total == correct);
            if (result) {
                cout << "OK" << endl;
            } else {
                cout << "WRONG" << endl;
            };

            std::cout << total << " total" << std::endl;
            std::cout << correct << " correct" << std::endl;
            std::cout << secondTotal - correct << " wrong" << std::endl;
            //result=end;
            //result=(*allMasterExts==*allSecondExts);
        } else {
            cout << "ERROR: Could not read secondary solution file \"" << secondSol << "\"" << endl;
        };
    } else {
        cout << "ERROR: Could not read master solution file \"" << masterSol << "\"" << endl;
    };
    if (verbOutput) {
        if (result) {
            display("OK"); cout << endl;
        } else {
            display("ERROR"); cout << endl;
        };
    };
    return (result);
}

void
display_usage() {
    cout << "Usage:   " << executableName << " master_solution secondary_solution"
        << endl << "Limits: Max number of args=" << maxIntArgId << endl;
};


bool
isTGF (string filename) {
    if (filename.length()>4) {
        string fileExt = filename.substr(filename.length()-3);
        transform (fileExt.begin(), fileExt.end(), fileExt.begin(),
                ::toupper);
        return fileExt=="TGF";
    } else {
        return false;
    }
}


bool
parsOK(char ** argv[], int * argc) {
    bool parError = false;
    string parameter;
    executableName = (*argv)[0];
    if ((*argc) == 1) {
        cout << "compare-exts v0.2" << endl
            << "Copyright Odinaldo Rodrigues, 2018" << endl;
        display_usage();
        return false;
    } else {
        if ((*argc)<=5) {
            parameter = (*argv)[1];
            transform(parameter.begin(), parameter.end(), parameter.begin(),
                    ::toupper);

            int idx = 1;
            while ((idx < (*argc)) && (!parError)) {
                parameter = (*argv)[idx];
                transform (parameter.begin(), parameter.end(), parameter.begin(),
                        ::toupper);
                // Detected filename parameter
                // Check for File parameter. If it exists, return it on variable filename.",
                // Otherwise, makes "parError" true.
                if (parameter.substr(0,1)=="-") {
                    if (parameter.substr(0, 2)=="-V") {
                        verbOutput = true;
                    } else {
                        if (parameter.substr(0, 2)=="-Q") {
                            quietMode = true;
                        } else {
                            if (parameter.substr(0, 2)=="-P") {
                                perfMode=true;
                            } else {
                                if (parameter.substr(0,2)=="-N") {
                                    noSets=true;
                                } else {
                                    parError=true;
                                };
                            };
                        };
                    };
                } else {
                    if (masterSol.empty()) {
                        masterSol=(*argv)[idx];
                    } else {
                        secondSol=(*argv)[idx];
                    }
                };	  
                idx++;
            };
        } else {
            parError = true;
        };
        if (!parError) {
            if (masterSol.empty()) {
                cout << "Master solution file not specified!" << endl;
                parError = true;
            };
            if (secondSol.empty()) {
                cout << "Secondary solution file not specified!" << endl;
                parError = true;
            };
        };
        if (parError) {
            display_usage();
        };
    };
    return(!parError);
}

void
set_stacksize(int mbs) {
    const rlim_t kStackSize = mbs * 1024 * 1024;   // min stack size = 16 MB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }
}

int
main(int argc, char *argv[]) {
    // mpz_class mpz_num=pow(2,127);
    // cout << "Number of bits per limb: " << mp_bits_per_limb << endl;
    // cout << "Number of limbs: " << mpz_num.get_mpz_t()->_mp_size << endl;
    // cout << "Number of allocated limbs: " << mpz_num.get_mpz_t()->_mp_alloc << endl;
    // exit(0);
    // unsigned int uint_32_test;
    // cout << "unsigned int: " << sizeof(uint_32_test) << endl;
    // Increase stack size to 128MB
    set_stacksize(256);
    // Process arguments
    string parameter;
    Timer TGFreadTimer, solReadTimer;
    double avgExtSize=0;
    if (parsOK(&argv, &argc)) {
        bool result;
        size_t totBytes=0, totTime=0; 
        Timer solTimer;
        solTimer.restart();
        totExts=count_solutions(masterSol,avgExtSize);
        if (totExts < 0) {
            cout << "Error reading input file. Aborting." << endl;
            exit(1);
        };
        //
        solTimer.restart();
        __mpz_vec * mpz_vec_allMasterExts=new __mpz_vec;
        __mpz_vec * mpz_vec_allSecondExts=new __mpz_vec;
        mpz_vec_allMasterExts->reserve(totExts);
        mpz_vec_allSecondExts->reserve(totExts);
        if (!perfMode) {
            if (verbOutput) {
                cout << "===================================================" << endl;
                cout << "Solution parsed in " << solTimer.milli_elapsed() << "ms." << endl;
                cout << "Solution has " << totExts << " extensions." << endl;
                cout << "Average extension size is ";
                printf("%4.2f",avgExtSize); cout << " elements." << endl;
                cout << "Number of distinct elements is " << intArgId.size() << endl;
                cout << "Total number of elements is " << totElems << endl;
            };

            if (quietMode) {
                cout << masterSol << "," << totExts << "," << intArgId.size() << "," << totElems;
            };

            // VECTORS OF MPZ INTEGERS
            if (!quietMode) {
                cout << "===================================================" << endl;
                cout << "Using ordered vectors of mpz_class:" << endl;
                cout << "Allocating space for " << totExts << " extension(s)." << endl;
            };
            result=false;
            totBytes=0; totTime=0;
            // totBytes: Estimated size allocated for one set of extensions
            // totElems: total number of elements in one set of extensions
            result=check_vec_mpz_solutions(mpz_vec_allMasterExts,mpz_vec_allSecondExts,totBytes);
            totTime=solTimer.milli_elapsed();
            if (quietMode) {
                cout << "," << totBytes << "," << totTime << ",";
                (result ? cout << "OK" : cout << "ERROR"); 
            } else {
                cout << "Total memory used in solutions: " << totBytes << " bytes" << endl;
                cout << "Average memory usage per node: " << totBytes*1.0/totElems << " bytes" << endl;
                cout << "Time to compare solutions: ";
                (totTime > 10000 ? cout << totTime/1000 << "s." : cout << totTime << "ms."); cout << endl;
                (result ? cout << "Solutions match." : cout << "ERROR"); cout << endl;
            };
            cout << endl;
        } else {
            result=false;
            totBytes=0; totTime=0;
            // totBytes: Estimated size allocated for one set of extensions
            // totElems: total number of elements in one set of extensions
            result=check_vec_mpz_solutions(mpz_vec_allMasterExts,mpz_vec_allSecondExts,totBytes);
/*            if (result) {
                cout << "OK" << endl;
            } else {
                cout << "WRONG" << endl;
            };*/
        };
        delete mpz_vec_allMasterExts;
        delete mpz_vec_allSecondExts;
        if (result) {
            exit(EXIT_SUCCESS);
        } else {
            exit(EXIT_FAILURE);
        };
    };
}


