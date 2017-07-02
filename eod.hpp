
#ifndef _EOD_H
#define _EOD_H 1

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <math.h>
#include <float.h>
#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <cstdio>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/math/distributions/binomial.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/unordered_map.hpp>
//#include <ql/quantlib.hpp>

#include "assoc.hpp"
#include "dozer.hpp"
#include <Rmath.h>
#include <R_ext/Applic.h>

enum MoveEnum { OPEN_VAL, CLOSE_VAL, LOW_VAL, HIGH_VAL };

//#define TRAIL 49
//#define TRAIL_DIVISER 7.0

#define KEEP 150
#define NUM_QUANTS 7

namespace bd = boost::date_time;
using namespace std;
//using namespace QuantLib;

void impliedVolSetup();

/*double getImpliedVolatility(std::string type, double value, double underlying,
    double strike, double dividendYield, double riskFreeRate,
    double maturityYrs, double volguess, QuantLib::Date &tradeDate, QuantLib::Date &exDate);
*/
class Symbol;
class EOD;

class QuantileDatum {
  public:

  double sum;
  double min;
  double max;
  double num;
  double sumlog;
  double x;

  QuantileDatum() {
    sum = 0;
    min = 0;
    max = 0;
    num = 0;
    sumlog = 0;
    x = 0;
  }
};

class FactorSelect {
  private:

  vector<double> *data;
  vector<double> quantileCutoffs;
  double min;

  double mx;
  double degf;
  double finalFit;

  public:
  double sig;

  vector<double> *getData() { return data; }

  FactorSelect() {
    data = new vector<double>;
  }

  void add(double d) {
    data->push_back(d);
  }


  /**
   * fit t distribution
   */
  void fitT();

  void constructQuantiles();

  int getQuantile(double d);

};

class Factor {
  private:
  
  FactorSelect select;
  vector<double> *data;
  int halfSteps;
  FactorSelect *selections;
 
  public:
  double minSig;

  Factor() {
    data = new vector<double>;
  }

  vector<double> *getData(int offset) {
    return select.getData();
  }

  void add(double d) {
    //select.add(d);
    data->push_back(d);
  }

  /**
   * fit t distribution
   */
  void fitT();

  void constructQuantiles();

  int getQuantile(size_t offset, double d);

};


class SymbolDate {
public:
  string exchange;
  string symbol;
  boost::gregorian::date *tradeDate;
  double open;
  double high;
  double low;
  double close;
  double volume;
  double openinterest;
  double targ;

  //int openF;
  int highF;
  int lowF;
  int closeF;
  int volF;
  int targF;

  //string openFS;
  string highFS;
  string lowFS;
  string closeFS;
  string volFS;
  string targFS;

  double prevOpen;
  double prevClose;
  double prevLow;
  double prevHigh;
  double prevVol;

  Symbol *symbolPtr;

  double mnVol;
  double sdVol;
  double minVol;
  double maxVol;
  double numVolMeas;

  bool holdVal;
  void dump();

  string getSymbolName() { return symbol+exchange; }

  void setHold(bool h) {
    holdVal = h;
  }

  bool assignFactors(int offset, MoveEnum moveEnum);
/* {
    char buf[100];
    openF = symbolPtr->openFactor.getQuantile(open-prevOpen);
    snprintf(buf, sizeof(buf), "%d", openF);
    openFS = buf;
    closeF = symbolPtr->closeFactor.getQuantile(close-prevClose);
    snprintf(buf, sizeof(buf), "%d", closeF);
    closeFS = buf;

    highF = symbolPtr->highFactor.getQuantile(high-prevHigh);
    snprintf(buf, sizeof(buf), "%d", highF);
    highFS = buf;
    lowF = symbolPtr->lowFactor.getQuantile(low-prevLow);
    snprintf(buf, sizeof(buf), "%d", lowF);
    lowFS = buf;

    if (moveEnum == OPEN_VAL) {
      targ = open;
      targF = openF;
      targFS = openFS;
    } else if (moveEnum == CLOSE_VAL) {
      targ = close;
      targF = closeF;
      targFS = closeFS;
    } else if (moveEnum == LOW_VAL) {
      targ = low;
      targF = lowF;
      targFS = lowFS;
    } else if (moveEnum == HIGH_VAL) {
      targ = high;
      targF = highF;
      targFS = highFS
    }
  }
*/
  void setSymbol(Symbol *s) {
    symbolPtr = s;
  }
  SymbolDate(const string &e, std::vector<std::string> &d) {
    prevClose = -999;
    exchange = e;
    symbol = d[0];
    tradeDate = new boost::gregorian::date(atoi(d[1].substr(0,4).c_str()),atoi(d[1].substr(4,2).c_str()),atoi(d[1].substr(6,2).c_str()));
    open = atof(d[2].c_str());
    high = atof(d[3].c_str());
    low = atof(d[4].c_str());
    close = atof(d[5].c_str());
    volume = atof(d[6].c_str());
    if (d.size() > 7)
      openinterest = atof(d[7].c_str());
    else
      openinterest = 0;

    mnVol = 0.0;
    sdVol = 0.0;
    minVol = DBL_MAX;
    maxVol = DBL_MIN;
    numVolMeas = 0.0;
  }
  void setPrevOpen(double p) {
    prevOpen = p;
  }
  void setPrevClose(double p) {
    prevClose = p;
  }
  void setPrevLow(double p) {
    prevLow = p;
  }
  void setPrevHigh(double p) {
    prevHigh = p;
  }
  void setPrevVol(double p) {
    prevVol = p;
  }

  void aggregateVolatility(double v) {
    ++numVolMeas;
    mnVol += v;
    if (minVol > v)
      minVol = v;
    if (maxVol < v)
      maxVol = v;
  }

  void aggregateStdDev(double v) {
    sdVol += pow(v-getMnVol(),2.0);
  }

  double getMnVol() {
    if (numVolMeas == 0)
      return 0;

    return mnVol/numVolMeas;
  }
  double getSdVol() {
    if (numVolMeas == 0)
      return 0;
    return sqrt(sdVol/numVolMeas);
    //return sqrt((sdVol-(mnVol*mnVol)/numVolMeas)/(numVolMeas-1));
    //return sdVol/pow(getMnVol(),2.0);
  }

  double getMinVol() {
    if (minVol == DBL_MAX) {
      return 0;
    }
    return minVol;
  }

  double getMaxVol() {
    if (maxVol == DBL_MIN) {
      return 0;
    }

    return maxVol;
  }

/*
  virtual ~SymbolDate() {
    if (tradeDate != NULL)
      delete tradeDate;
  }
*/
};

class SymbolDateSrtr {
  public:
  bool operator() (const SymbolDate *a, const SymbolDate *b) {
    return *(a->tradeDate) < *(b->tradeDate);
  }
};

template <class UIntType> class MyGen {
  boost::random::mt19937 gen;
  uint32_t *data;
  uint32_t selection;

  public:
  typedef UIntType result_type;

  MyGen() {
    FILE *fptr = fopen("random.dat", "r");
    data = new uint32_t[7000000];
    size_t fr = fread(data, sizeof(uint32_t), 7000000, fptr);
    if (fr != 7000000) {
      cout << "bad read: " << fr << endl;
    } else {
      cout << "good read: " << fr << endl;
    }
    selection = 0;
    fclose(fptr); 
  }
  void seed() { gen.seed(); }
  void seed(UIntType u) { gen.seed(u); }
  template<typename SeeqSeq> void seed(SeeqSeq &s) { gen.seed(s); }
  template<typename It> void seed(It &a, It b) { gen.seed(a,b); }
  result_type operator()() {
    //return gen();

    result_type r = data[selection%7000000];
    ++selection;
    return r;
  }
  template<typename Iter> void generate(Iter a, Iter b) {
    return gen.generate(a,b);
  }

  void discard(boost::uintmax_t u) { gen.discard(u); }

  static result_type min() { return boost::random::mt19937::min(); }
  static result_type max() { return boost::random::mt19937::max(); }
};

class Symbol {
  public:

  vector<SymbolDate *> symbolDates;
  vector<SymbolDate *> symbolDatesNorm;

  boost::gregorian::date *firstDate;
  boost::gregorian::date *lastDate;
  vector<SymbolDate *> symbolDatesFull;
  double mn;
  double avg;
  double openStddev;
  double closeStddev;
  double lowStddev;
  double highStddev;
  double volStddev;

  string exchange;
  string name;
  double fundamentals[8];

  vector<double> dailyDiffs;
  //Factor openFactor;
  Factor closeFactor;
  Factor lowFactor;
  Factor highFactor;
  Factor volFactor;

  int minTarg;
  int maxTarg;
  vector<double> normProbs;
  vector<double> cutOffs;

  //boost::random::mt19937 gen;
  //MyGen<uint32_t> gen;
  EOD *eodPtr;

  double avgSpread;

  /**
   * A two dimensional vector of all cutoff stats
   * I'm starting with a set of ten sets, but I'll have to 
   * adjust this as I go.
   * I suppose these could go from .90-.99 or something
   */
  vector< vector<double> *> adCutoffs;
  vector< vector<double> *> adCutoffsNeg;

  double minCutoff;
  double maxCutoff;
  int numCutoffs;

  private:
  bool invalid;
  int invalidWhy;
  public:
  Symbol() {
    numCutoffs = 50;
    minCutoff = .97;
    maxCutoff = .999;
    invalid = false;

    for (size_t i=0;i<symbolDates.size();++i) {
      symbolDates[i] = NULL;
    }

    firstDate = NULL;
    lastDate = NULL;

    for (size_t i=0;i<8;++i) {
      fundamentals[i] = 0.0;
    }
  }

  Symbol(string n, string e, EOD *eodPtrParam) {
    eodPtr = eodPtrParam;
    numCutoffs = 10;
    minCutoff = .97;
    maxCutoff = .999;
    invalid = false;

    name = n;
    exchange = e;

    firstDate = NULL;
    lastDate = NULL;

    for (size_t i=0;i<symbolDates.size();++i) {
      symbolDates[i] = NULL;
    }

    for (size_t i=0;i<8;++i) {
      fundamentals[i] = 0.0;
    }
  }

  void setInvalid(bool iv, int w) {
    invalid = iv;
    invalidWhy = w;
  }
  void dumpInvalid() {
    cout << invalid << ", " << invalidWhy << endl;
  }
  bool getInvalid() { return invalid; }
  void setExchange(string e) {
    exchange = e;
  }

/*
  virtual ~Symbol() {
    for (size_t i=0;i<symbolDates.size();++i) {
      delete symbolDates[i];
    }

    for (size_t i=0;i<optionSymbols.size();++i) {
      delete optionSymbols[i];
    }
  }
*/
  void iu(itemset &itms, map<string, int> &itmap, map<int, string> &itumap, const string &k, SymbolDate *s, int dateOffset, void_allocator alloc_inst);

  void insert(itemset &itms, size_t i,
      map<string, int> & itmap, map<int, string> & itumap, void_allocator alloc_inst);

  vector<double> *getDailyDiffs() {
    return &dailyDiffs;
  }

  double getMean() { return mn; }
  double getOpenStdDev() { return openStddev; }
  double getCloseStdDev() { return closeStddev; }
  double getLowStdDev() { return lowStddev; }
  double getHighStdDev() { return highStddev; }

  void addSymbolDate(SymbolDate *s) {
    symbolDatesFull.push_back(s);
  }
  boost::gregorian::date *getFirstDate() { return firstDate; }
  boost::gregorian::date *getLastDate() { return lastDate; }

  void dumpData() {
    if (symbolDates.size() < 1 || symbolDates[0] == NULL)
      return;

    //dump a portion of the data
    std::fstream datastr;

    string t = "data/"+symbolDates[0]->exchange+":"+symbolDates[0]->symbol+".csv";
    datastr.open(t.c_str(), std::fstream::out);

    for (size_t i=0;i<dailyDiffs.size();++i)
      datastr << dailyDiffs[i] << endl;

    datastr.close();
  }
/*
  void dump() {

    cout << "mx: " << mx << endl;
    cout << "sig: " << sig << endl;
    cout << "degf: " << degf << endl;
    cout << "final fit: " << finalFit << endl;

    for (size_t i=0;i<dailyDiffs.size();++i) {
      cout << dailyDiffs[i] << ": ";

      double x = dailyDiffs[i];

      double xp = ((x-mx)/sig);
      double yp = dt(xp,degf,1);

      double r = log(sig)-yp;

      cout << r << endl;
    }
  }
*/

  bool isInvalid() {
    return invalid;
  }

  bool processCloseFactor() {
    double prev = -1000;
    mn = 0.0;
    for (size_t i=0;i<symbolDatesNorm.size();++i) {
      if (prev == -1000) {
        prev = symbolDatesNorm[i]->close;
        symbolDatesNorm[i]->setPrevClose(prev);
      } else {
        double d = symbolDatesNorm[i]->close - prev;
        mn += d;
        dailyDiffs.push_back(d);
        closeFactor.add(d);
        symbolDatesNorm[i]->setPrevClose(prev);
        prev = symbolDatesNorm[i]->close;
      }
    }
    mn /= dailyDiffs.size();
 
    prev = -1000;
    for (size_t i=0;i<symbolDates.size();++i) {
      if (prev == -1000) {
        prev = symbolDates[i]->close;
        symbolDates[i]->setPrevClose(prev);
      } else {
        symbolDates[i]->setPrevClose(prev);
        prev = symbolDates[i]->close;
      }
    }

    closeStddev = 0.0;
    for (size_t i=0;i<dailyDiffs.size();++i)
      closeStddev += pow(dailyDiffs[i]-mn, 2.0);

    closeStddev = sqrt(closeStddev/dailyDiffs.size());

    if (closeStddev > .01) {
      //closeFactor.process(.1);
      closeFactor.fitT();
      if (closeFactor.minSig < .001)
        return false;

      closeFactor.constructQuantiles();
    } else {
      return false;
    }

    return true;
  }
/*
  bool processOpenFactor() {
    double prev = -1000;
    for (size_t i=0;i<symbolDates.size();++i) {
      if (prev == -1000) {
        prev = symbolDates[i]->open;
        symbolDates[i]->setPrevOpen(prev);
      } else {
        symbolDates[i]->setPrevOpen(prev);
        prev = symbolDates[i]->open;
      }
    }


    mn = 0.0;
    avg = 0.0;
    avgSpread = 0.0;
    prev = -1000;
    for (size_t i=0;i<symbolDatesNorm.size();++i) {
      avgSpread += symbolDatesNorm[i]->high - symbolDatesNorm[i]->low;
      avg += (symbolDatesNorm[i]->high + symbolDatesNorm[i]->low)/2.0;
      if (prev == -1000) {
        prev = symbolDatesNorm[i]->open;
        symbolDatesNorm[i]->setPrevOpen(prev);
      } else {
        double d = symbolDatesNorm[i]->open - prev;
        mn += d;
        dailyDiffs.push_back(d);
        openFactor.add(d);
        symbolDatesNorm[i]->setPrevOpen(prev);
        prev = symbolDatesNorm[i]->open;
      }
    }
    avg /= symbolDatesNorm.size();
    avgSpread /= symbolDatesNorm.size();
    avgSpread /= avg;
    mn /= dailyDiffs.size();

    openStddev = 0.0;
    for (size_t i=0;i<dailyDiffs.size();++i)
      openStddev += pow(dailyDiffs[i]-mn, 2.0);

    openStddev = sqrt(openStddev/dailyDiffs.size());

    if (openStddev > .01) {
      //openFactor.process(.1);
      openFactor.fitT();
      //cout << "open factor min sig: " << openFactor.minSig << endl;
      if (openFactor.minSig < .001) {
        return false;
      }
      openFactor.constructQuantiles();
    } else {
      return false;
    }

    return true;
  }
*/
  bool processLowFactor() {
    double prev = -1000;

    for (size_t i=0;i<symbolDates.size();++i) {
      if (prev == -1000) {
        prev = symbolDates[i]->low;
        symbolDates[i]->setPrevLow(prev);
      } else {
        symbolDates[i]->setPrevLow(prev);
        prev = symbolDates[i]->low;
      }
    }

    mn = 0.0;
    prev = -1000;
    for (size_t i=0;i<symbolDatesNorm.size();++i) {
      if (prev == -1000) {
        prev = symbolDatesNorm[i]->low;
        symbolDatesNorm[i]->setPrevLow(prev);
      } else {
        double d = symbolDatesNorm[i]->low - prev;
        mn += d;
        dailyDiffs.push_back(d);
        lowFactor.add(d);
        symbolDatesNorm[i]->setPrevLow(prev);
        prev = symbolDatesNorm[i]->low;
      }
    }
    mn /= dailyDiffs.size();

    lowStddev = 0.0;
    for (size_t i=0;i<dailyDiffs.size();++i)
      lowStddev += pow(dailyDiffs[i]-mn, 2.0);

    lowStddev = sqrt(lowStddev/dailyDiffs.size());

    if (lowStddev > .01) {
      //lowFactor.process(.1);
      lowFactor.fitT();
      if (lowFactor.minSig < .001)
        return false;

      lowFactor.constructQuantiles();
    } else {
      return false;
    }

    return true;
  }

  bool processHighFactor() {
    double prev = -1000;
    for (size_t i=0;i<symbolDates.size();++i) {
      if (prev == -1000) {
        prev = symbolDates[i]->high;
        symbolDates[i]->setPrevHigh(prev);
      } else {
        symbolDates[i]->setPrevHigh(prev);
        prev = symbolDates[i]->high;
      }
    }


    mn = 0.0;
    prev = -1000;
    for (size_t i=0;i<symbolDatesNorm.size();++i) {
      if (prev == -1000) {
        prev = symbolDatesNorm[i]->high;
        symbolDatesNorm[i]->setPrevHigh(prev);
      } else {
        double d = symbolDatesNorm[i]->high - prev;
        mn += d;
        dailyDiffs.push_back(d);
        highFactor.add(d);
        symbolDatesNorm[i]->setPrevHigh(prev);
        prev = symbolDatesNorm[i]->high;
      }
    }
    mn /= dailyDiffs.size();

    highStddev = 0.0;
    for (size_t i=0;i<dailyDiffs.size();++i)
      highStddev += pow(dailyDiffs[i]-mn, 2.0);

    highStddev = sqrt(highStddev/dailyDiffs.size());

    if (highStddev > .01) {
      //highFactor.process(.1);
      highFactor.fitT();
      if (highFactor.minSig < .001)
        return false;


      highFactor.constructQuantiles();
    } else {
      return false;
    }

    return true;
  }

  bool processVolFactor() {
    double prev = -1000;

    for (size_t i=0;i<symbolDates.size();++i) {
      if (prev == -1000) {
        prev = symbolDates[i]->volume;
        symbolDates[i]->setPrevVol(prev);
      } else {
        symbolDates[i]->setPrevVol(prev);
        prev = symbolDates[i]->volume;
      }
    }

    mn = 0.0;
    prev = -1000;
    for (size_t i=0;i<symbolDatesNorm.size();++i) {
      if (prev == -1000) {
        prev = symbolDatesNorm[i]->volume;
        symbolDatesNorm[i]->setPrevVol(prev);
      } else {
        double d = symbolDatesNorm[i]->volume - prev;
        mn += d;
        dailyDiffs.push_back(d);
        volFactor.add(d);
        symbolDatesNorm[i]->setPrevVol(prev);
        prev = symbolDatesNorm[i]->volume;
      }
    }
    mn /= dailyDiffs.size();

    volStddev = 0.0;
    for (size_t i=0;i<dailyDiffs.size();++i)
      volStddev += pow(dailyDiffs[i]-mn, 2.0);

    volStddev = sqrt(volStddev/dailyDiffs.size());

    if (volStddev > .01) {
      //lowFactor.process(.1);
      volFactor.fitT();
      if (volFactor.minSig < .001)
        return false;

      volFactor.constructQuantiles();
    } else {
      return false;
    }

    return true;
  }

  /*
   * go through all symbols and determine first and last date
   * construct data and fit t-distributions for later quantile
   * calculation
   */
  bool process1() {
    if (symbolDatesFull.size() < KEEP*2+1) {
      //std::cout << "process1 false symbol size is less than " << KEEP+1 << ":" << symbolDatesFull.size() << endl;
      setInvalid(true, 11);
      return false;
    } else {
      //sort the symbolDates
      SymbolDateSrtr srtr;
      std::sort(symbolDatesFull.begin(),symbolDatesFull.end(), srtr);
      //keep just the final KEEP elements for processing and the preceding
      //KEEP for normalization
/*
      for (size_t i=symbolDatesFull.size()-KEEP*2;i < symbolDatesFull.size()-KEEP;++i) {
        symbolDatesNorm.push_back(symbolDatesFull[i]);
      }
*/
      //for (size_t i=symbolDatesFull.size()-KEEP;i < symbolDatesFull.size();++i) {
      for (size_t i=KEEP;i>0;--i) {
        size_t k = symbolDatesFull.size() - i;
        SymbolDate *s = symbolDatesFull[k];
        symbolDates.push_back(s);
        bool hold = false;
        if (i < 2) {
          hold = true;
        } else if (i % 2) {
          hold = true;
        }
        s->setHold(hold);
        if (!hold) {
          symbolDatesNorm.push_back(s);
        }
      }

      //lastDate = symbolDates[KEEP]->tradeDate;
      lastDate = symbolDates[symbolDates.size()-1]->tradeDate;
    }

    if (lastDate == NULL) {
      //cout << name << "last date is null: " << symbolDates.size() << endl;
      return false;
    }

    return true;
  }
  bool process2(MoveEnum moveEnum);
  void process3(string filename);

  void initADCutoffArray();
  void initADCutoffArray(double cutOff, std::vector<double> &adCs,
    std::vector<double> &probs);

  //double calcADCutoff(boost::random::discrete_distribution<> &dist, boost::random::mt19937 &gen, std::vector<double> &normProbs, size_t testSampleSize, size_t sampleSize, double cutOff);
  //double calcADCutoff(boost::random::discrete_distribution<> &dist, MyGen<uint32_t> &gen, std::vector<double> &normProbs, size_t testSampleSize, size_t sampleSize, double cutOff);
  void calcADCutoff(boost::random::discrete_distribution<> &dist,   MyGen<uint32_t> &gen, std::vector<double> &normProbs, size_t testSampleSize, size_t sampleSize, std::vector<double> &cutOffs, std::vector<double> &retADCutoffs, std::vector<double> &retADCutoffNeg);

  static void performInterpInfl(std::vector<double> &adCs, double infl);



  void populateImpliedVolatility();

  SymbolDate *getSymbolDate(int i) {
    if (i < 0) {
      i = KEEP+i;
    }
    if (i < 0 || i > KEEP) {
      return NULL;
    }
/*
    if (i < KEEP) {
      return symbolDatesNorm[i];
    } else {
*/
      return symbolDates[i];
 //   } 
  }
};

class EOD {
  public:
  map<string, Symbol *> symbols;

  boost::gregorian::date *firstDate;
  boost::gregorian::date *lastDate;
  size_t sz;
  MyGen<uint32_t> gen;

  EOD() {
    firstDate = NULL;
    lastDate = NULL;
    sz = 0;
  }

  bool getHold(int h) {
    if (h < 2) {
      return true;
    } else {
      return h % 2;
    }
  }

  void dumpInvalid() {
    map<string, Symbol *>::iterator st;
    int numInvalid = 0;
    int num = 0;
    for (st=symbols.begin();st!=symbols.end();st++) {
      Symbol *sblPtr = (*st).second;
      if (sblPtr->getInvalid()) {
        sblPtr->dumpInvalid();
        ++numInvalid;
      } else {
        ++num;
      }
    }
    cout << "total, invalid " << num << ", " << numInvalid << endl;
  }
};

#endif /* eod.hpp*/

