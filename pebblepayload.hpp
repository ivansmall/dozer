
#ifndef _PEBBLEPAYLOAD_H
#define _PEBBLEPAYLOAD_H 1

#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include "pebble.hpp"
#include "scoop.hpp"
#include "targetassoc.hpp"
#include "assoc.hpp"

using namespace std;
class Pebble;

class PebblePayload {

  public:
  int itemId;
  vector<double> tuple;
  bool unknownOut;
  double out, prevOut, open;
  bool genOutput;
  bool includeInDistCalc;
  int dateOffset;


  public:
  PebblePayload() {}
  PebblePayload(int id, double_vector &t, int offset, size_t numTuple,
      bool u, double o, double p, bool otp, double open, int dateOffset) {
    for (size_t i=offset;i<offset+numTuple;++i) {
      tuple.push_back(t[i]);
    }
    itemId = id;
    unknownOut = u;
    out = o;
    this->open = open;
    prevOut = p;
    genOutput = otp;
    includeInDistCalc = !otp;
    this->dateOffset = dateOffset;
  }
  const vector<double> &getTuple() {
    return tuple;
  }
};


class PebbleAccum {
  public:

  vector<long double> tupleExpansion;
  int numTuples;
  int numAssoc;
  public:

  PebbleAccum() {
   numTuples = 0; 
   numAssoc = 0;
  }

  void addTuple(const vector<double> &tuple);

  void addAccum(const PebbleAccum &accum);

  void dumpFittingData(fstream &of);

  void setTupleSize(int tupleSize);
};

class PebbleOutputPayload {
public:
  vector<PebbleAccum> accum;
public:
  double output;
  bool unknownOutput;
  double prevOutput;
  double open;
  string name;
  int dateOffset;

  public:
  PebbleOutputPayload(int slices, double o, bool u, double p, int tupleSize, double open, int dateOffset);

  void addPebble(Pebble p, int slice);

  void dumpFittingData(fstream &of, double testOutput, string name, bool withName);
};

#endif
