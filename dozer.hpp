
#ifndef _DOZER_H
#define _DOZER_H 1

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <math.h>
#include "assoc.hpp"
#include "scoop.hpp"
#include "payload.hpp"
#include "targetassoc.hpp"
#include "pebblepayload.hpp"
#include <cstdio>
#include <boost/math/distributions/binomial.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/unordered_map.hpp>
#include <initializer_list>

//class Pebble;
class Scoop;
class Payload;
class PebbleOutputPayload;
class PebblePayload;
class PebbleAccum;

using namespace std;

class Dzrsrtr {
private:
  std::set<int> *targets;
  std::vector<int> *cnt;

public:

  Dzrsrtr(std::set<int> *ptargets, std::vector<int> *pcnt) {
    targets = ptargets;
    cnt = pcnt;
  }

  bool operator() (int pi,int pj) {
    long i = (*cnt)[pi];
    long j = (*cnt)[pj];

    if (targets->count(pi))
      i = i+1e7L;
    if (targets->count(pj))
      j = j+1e7L;

    if (i == j) {
      return pi<pj;
    } else {
      return (i<j);
    }
  }
};

class PebbleSrtr {
private:
  std::set<int> *targets;
  std::vector<int> *cnt;

public:

  PebbleSrtr(std::set<int> *ptargets, std::vector<int> *pcnt) {
    targets = ptargets;
    cnt = pcnt;
  }

  bool operator() (PebblePayload *pi,PebblePayload *pj);
/* {
    long i = (*cnt)[pi->itemId];
    long j = (*cnt)[pj->itemId];

    if (targets->count(pi->itemId))
      i = i+1e7L;
    if (targets->count(pj->itemId))
      j = j+1e7L;

    if (i == j) {
      return pi->itemId < pj->itemId;
    } else {
      return (i<j);
    }
  }
*/
};


class TargetItem {
private:
  string symbolName;
  vector<double> normProbs;
  vector< vector<double> * > adCutoffs;
  vector< vector<double> * > adCutoffsNeg;

  vector<double> measSep;
  double norm;

  vector<Pebble *> pebbles;
  double testOutput;
  vector<PebbleOutputPayload *> outputPayloads;

public:

  vector<int> numAssoc;
  vector<int> numAssocNeg;

  int totalOverZero;
public:
  void addOutputPayload(PebbleOutputPayload *op);
  void dumpOutputPayloads(fstream &of, bool withName);

  TargetItem(string sn, vector<double> np,
      vector< vector<double> *> ac) {
    symbolName = sn;
    normProbs = np;
    adCutoffs = ac;
    numAssoc.resize(adCutoffs.size());
    numAssocNeg.resize(adCutoffs.size());

    totalOverZero = 0;
  }
  TargetItem(const targetitem &t) {
    symbolName = t.name.c_str();
    normProbs.resize(t.normProbs.size());
    for (size_t i=0;i<t.normProbs.size();++i)
      normProbs[i] = t.normProbs[i];

    adCutoffs.resize(t.cutoffSizes.size());
    adCutoffsNeg.resize(t.cutoffSizes.size());
    for (size_t i=0;i<t.cutoffSizes.size();++i) {
      adCutoffs[i] = new vector<double>(t.cutoffSizes[i]);
      adCutoffsNeg[i] = new vector<double>(t.cutoffSizes[i]);
    }

    int k=0;
    for (size_t i=0;i<t.cutoffSizes.size();++i) {
      for (int j=0;j<t.cutoffSizes[i];++j, ++k) {
        (*adCutoffs[i])[j] = t.cutoffs[k];
        (*adCutoffsNeg[i])[j] = t.cutoffsNeg[k];

      }
    }

    measSep.resize(t.normProbs.size());
    numAssoc.resize(adCutoffs.size());
    numAssocNeg.resize(adCutoffs.size());
    totalOverZero = 0;
    testOutput = t.testOutput;
  }
  
  int getNumSlices() {
    return adCutoffs.size();
  }
  const string & getSymbolName() {
    return symbolName;
  }

  void clearMeas() {
    norm = 0;
    for (size_t i=0;i<measSep.size();++i)
      measSep[i] = 0;
    pebbles.resize(0);
  }

  //just place the checks here, that's probably easier
  /**
   * @param o is the output number
   * @param m is the level of itemsets with this output
   *
   */
  int addMeas(int o, int m, Pebble **p);

  int getNumMeas() {
    return norm;
  }

  void normMeas() {
    for (size_t i=0;i<measSep.size();++i)
      measSep[i] /= norm;
  }

  /**
   * 
   */
  int calcAD(int level) {

    double mxAD=0.0;

    //compare the measured distribution and calculate the max
    //measure compared to the normalized one.  This is compared to each
    //element in the array of cutoffs
/*
    for (size_t i=0;i<measSep.size();++i) {
      double a = fabs(measSep[i]-normProbs[i])/sqrt(normProbs[i]*(1.0-normProbs[i]));
      if (a > mxAD)
        mxAD = a;
    }
*/
    double a = 0.0;
    for (size_t i=0;i<measSep.size();++i) {
      //a = fabs(measSep[i]-normProbs[i])/sqrt(normProbs[i]*(1.0-normProbs[i]));
      a += measSep[i]*i;
    }
    mxAD = a;

    int foundSlice=-999;
    //the norm represents the support
    for (int i=adCutoffs.size()-1;i >= 0;--i) {
      if ((*adCutoffs[i])[norm] < mxAD) {
        ++numAssoc[i];
        if (i > 0)
          ++totalOverZero;
        //build up the association details here

        //expand the pebble tuples for all involved pebbles
        //I should also expand those associated with the itemset
        //if I have any
        foundSlice = i;

//        aggregatePebbles(foundSlice, level);
//cout << "calcAD " << symbolName <<  i << ", " << numAssoc[i] << ", " << (*adCutoffs[i])[norm] << endl;
        return foundSlice;
      }
    }

    for (size_t i=0;i < adCutoffsNeg.size();++i) {
      if ((*adCutoffsNeg[i])[norm] > mxAD) {
        ++numAssocNeg[i];
        if (i > 0)
          ++totalOverZero;

        //build up the association details here

        //expand the pebble tuples for all involved pebbles
        //I should also expand those associated with the itemset
        //if I have any
        foundSlice = -i-1;

//        aggregatePebbles(foundSlice, level);
//cout << "calcAD " << symbolName <<  i << ", " << numAssoc[i] << ", " << (*adCutoffs[i])[norm] << endl;
        return foundSlice;
      }
    }

    return foundSlice;
  }

  /**
   * go through all pebbles and aggregate them together
   */
  void aggregatePebbles(int slice, int level);

  /**
   * go through all output payloads and add this pebbleAccum
   * at the appropriate slice
   */
  void addAccum(PebbleAccum &pebbleAccum, int slice);
};

class Targsrtr {
public:
  bool operator() (TargetItem *a, TargetItem *b) {
    return a->totalOverZero < b->totalOverZero;
  }
};


class Dozer {
  /**
   * the level of current recurion in the associations;
   */

public:

  //std::vector<Scoop *> scoops; //array of scoops, one for each item id
  Scoop **scoops;
  int scoopsSize;
  Scoop **scoopsA;

  std::map<std::string, int> smpItems;
  std::map<int, std::string> impItems;
  std::map<int, int> outItems;
  std::map<int, std::string> symbolItems;

  std::set<int> iTargets;
  Dzrsrtr *srtr;  //need to destroy this, not right now
  PebbleSrtr *psrtr;

  std::vector<int> *ordering;
  std::vector<int> *ordrmp;
  std::vector<int> assocStack;
  std::vector<float> confStack;
  int numberItemsets;
  int numberHoldBackItemsets;
  int numberHoldScoreItemsets;

  int currentRecursion;
  int firstTargetPosition;
  int evalItemId;
  int evalItemIdIndex;
  bool generateItrst;
  int totSup; 
  std::vector<TargetAssoc *> targetAssociations;
  std::vector<TargetAssoc *> finalAssociations;

  //maintain a hash of associations based upon the set of included
  //itemsets.  This is used to eliminate clear duplicates.
  boost::unordered_map<int, TargetAssoc *> hashedTargetAssociations;

  boost::unordered_map<std::vector<int>, SparseTargetAssoc *> allAssociations;

  boost::unordered_map<std::string, std::vector<int> *> targets;
  std::vector<double> adCutoffs;
  std::vector<double> adCutoffMaxs;
  std::vector<int> binomCutoffs;
  std::vector<double> probs;
  std::vector<Payload *> payloads;
  double binomCutoff;
  double adCutoff;
  double adCutoffMax;
  int totalFound;
  int totalDuplicatesFound;
  int totalChecked;
  int totalADChecked;

  boost::unordered_map<int, boost::unordered_map<int, ItemData *> > holdItemsets;
  std::vector<TargetItem *> *targetItems;
  item_map_type *itemMap;

  /**
   * Dozer has been loaded with an evaluation set.
   */
  bool evalLoaded;
  /**
   * hold back a percentage of the item sets for testing 
   */
  float test;
  std::fstream claimexpstr;
  std::fstream assocdatastr;
  std::fstream assocdatafannstr;

  bool HOLD_SCORES; 
public:

/*
  Pebble *getPebblePointer(int lnk) {
    return basePointer+(lnk&x1FFFFFFF)<<3;
  }
  int createLnk(int offset, int handfulPos) {
    return offset>>3+handfulPos;
  }
*/ 

  /**
   * use a lookup table and determine which scoop this pebble
   * belongs to.  
   */
/*
  Pebble *getScoopPointer(int link) {
    return ???;
  }
*/
  /**
   * itemMap is a vector of string names.  The index is a map value into
   * the itemsets
   * cnt is a vector of count levels for each item id.  This vector and
   * the item map should be the same length.
   * targets is a vector of string names indicating the targets for 
   * generating associations that pass confidence and support levels.
   */

  Dozer(item_map_type *itemMap, std::vector<int> *cnt, std::vector<TargetItem *> *targets, double binomC);

  //Dozer(item_map_type *itemMap, std::vector<int> *cnt);

  /**
   * load in a portion of the total itemsets.  I assume these to reside
   * in multiple load files, but that the id/name mapping is global.  A
   * static method goes through and adds up all of the items to generate
   * the count levels.
   */

  void loadItemsetsSection(itemset &it, int_vector *ids, int i, std::vector<int> &holdHalf, int numSlices);

  void loadItemsets(itemset_vec_type *itemsets, int_vector *ids=NULL);

  void dumpMetricStats(std::vector<int> *m, std::fstream *corrdatastr, std::vector<int> *r=NULL);
  void dumpMetricStats( std::fstream &corrdatastr, int_vector *m);
  void dumpCorrData(  std::fstream &corrdatastr);
  void dumpPayloads(  std::fstream &assocdatastr, const char *inclFilename);
  void dumpClaimDrug(Payload *p, std::fstream &exclDatastr);

  void dumpPayloadsStats(  std::fstream &assocdatastr);
  void dumpPayloadsFann(  std::fstream &assocdatastr);

  /**
   * go through and add itemset counts onto the cnt vector.
   * I assume this vector is the correct size.
   */
  static void cntItemsets(itemset_vec_type *itemsets, std::vector<int> *cnts);

  /**
   * iterate through the scoops and build up associations above support
   * I need to calculate random occurence numbers as well, so I can
   * deterimine confidence numbers.  I suppose, because I walk
   * up adding items one at a time, that I should be able to deterimine
   * confidence along the way.
   */
  void dumpTargetRollup();
  void traverse(int offset, int level, int minSup, float minConf, int holdMinSup);
  void tumbleTaggedPayloads(int scoopStrt, int level, int minSup);
  void updateGenProb();
  void generateAssociations(float minSup, float minConf, string outdir);
  void dumpItemScoops();
  void pushAssoc(int position);
  void popAssocId();
  void pushConf(float itemId);
  void popConf();

  bool getBinomConfLowerBound(int os, int hf);

  void printAssociation(int sup, float conf, int targetPos);
  void dump();
  void dumpNames();
  void check();
  bool checkItrstAssoc(int position, int level, int minSup, float minConf);
  void dumpAssocRule(int position, int level, float floatprob, std::string targetname, std::string holdname);
  bool checkTargetsEval(int position, int level, int minSup, float minConf);

  double getErrEstimate(double indProb, double genProb, double conf);

  double getIndProb();
  void setEvalSize();

  void clearRecur(int dozerPosition, int level);

  void printEvaluationData();

  void genEvaluationCalc();

  bool checkOutputDistSeparation(int position, int level, double *adValMx, TargetAssoc *targ);
  bool checkOutputDistSeparation(int position, int level);

  void initADCutoffArray(double cutOff, std::vector<double> &adCs);
  void initBinomCutoffArray(double cutOff);

  double calcADCutoff(boost::random::discrete_distribution<> &dist, boost::random::mt19937 &gen, std::vector<double> &normProbs, int testSampleSize, int sampleSize, double cutOff);

  void removeRedundant();

  void getPayloadsVec(bool addOutput, std::vector<std::vector<double> > &p);

  /**
   * read in n-tuples of item sets.  I do this in two passes so that
   * I can make memory requirement calculations.
   */
  //...constructScoops()
  void aggregateAssocPebbles(int level, PebbleAccum &pebbleAccum);

};

#endif /* dozer.hpp */
