#include "pebblepayload.hpp"

#define TUPLE_SIZE 4
  void PebbleOutputPayload::addPebble(Pebble p, int slice) {
    accum[slice].addTuple(p.pebblePayload->tuple);
  }
  //void dumpFittingData(fstream &of, double testOutput, string name);

  void PebbleOutputPayload::dumpFittingData(fstream &of, double testOutput,
      string name, bool withName) {
    //if (!genOutput)
      //return;
//    if (accum[0].numTuples == 0)
//      return;
    if (withName)
      of << name << ",";
    if (testOutput == 99999)
      of << "NA,";
    else
      of << testOutput << ",";

    if (unknownOutput) {
      of << "NA,NA";
    } else {
      of << output << "," << open;
    }
    of << "," << prevOutput << "," << dateOffset;
    for (int i=0;i<accum.size();++i) {
      of << "," << accum[i].numAssoc;
    }  
    for (int i=0;i<accum.size();++i) {
      accum[i].dumpFittingData(of);
    }
    of << endl;
  }

  PebbleOutputPayload::PebbleOutputPayload(int slices, double o, bool u, double p, int tupleSize, double open, int dateOffset) {
    accum.resize(slices*2);
    for (int i=0;i<accum.size();++i) {
      accum[i].setTupleSize(tupleSize);
    }
    output = o;
    unknownOutput = u;
    prevOutput = p;
    this->open = open;
    this->dateOffset = dateOffset;

//cout << "pebble output payload " << slices << ", " << output << ", " << unknownOutput << ", "  << prevOutput << endl;
  }

  void PebbleAccum::addTuple(const vector<double> &tuple) {
    if (numTuples == 0) {
      tupleExpansion.resize(tuple.size()*TUPLE_SIZE);
    }
    ++numTuples;
    for (size_t i=0;i<tuple.size();++i) {
      // sum, min, max, sqrd;
      tupleExpansion[i*TUPLE_SIZE] += tuple[i];
      tupleExpansion[i*TUPLE_SIZE+1] += pow(tuple[i], 2.0);
      if (tuple[i] > tupleExpansion[i*TUPLE_SIZE+2]) {
        tupleExpansion[i*TUPLE_SIZE+2] = tuple[i];
      }
      if (tupleExpansion[i*TUPLE_SIZE+3] == 0 || tuple[i] < tupleExpansion[i*TUPLE_SIZE+3]) {
        tupleExpansion[i*TUPLE_SIZE+3] = tuple[i];
      }
    }
  }

  void PebbleAccum::setTupleSize(int tupleSize) {
    tupleExpansion.resize(tupleSize*TUPLE_SIZE);
  }

  void PebbleAccum::addAccum(const PebbleAccum &accum) {
    if (numTuples == 0) {
      tupleExpansion.resize(accum.tupleExpansion.size());
      numTuples = accum.numTuples;
    } else {
      numTuples += accum.numTuples;
    }
    ++numAssoc;
//cout << "accum size " << accum.tupleExpansion.size() << ":" << tupleExpansion.size() << ":" << numTuples << endl;
    if (tupleExpansion.size() != accum.tupleExpansion.size()) {
      cout << "different sizes" << endl;
      //tupleExpansion.resize(accum.tupleExpansion.size());
      //numTuples = accum.numTuples;
    }

//addAccum
    // sum, sqrd, max, min
    for (size_t i=0;i<tupleExpansion.size()/TUPLE_SIZE;++i) {
      tupleExpansion[i*TUPLE_SIZE] += accum.tupleExpansion[i*TUPLE_SIZE];
      tupleExpansion[i*TUPLE_SIZE+1] += accum.tupleExpansion[i*TUPLE_SIZE+1];
      if (tupleExpansion[i*TUPLE_SIZE+2] < accum.tupleExpansion[i*TUPLE_SIZE+2]) {
        tupleExpansion[i*TUPLE_SIZE+2] = accum.tupleExpansion[i*TUPLE_SIZE+2];
      }
      if (tupleExpansion[i*TUPLE_SIZE+3] > accum.tupleExpansion[i*TUPLE_SIZE+3]) {
        tupleExpansion[i*TUPLE_SIZE+3] = accum.tupleExpansion[i*TUPLE_SIZE+3];
      }
    }
/*
    for (size_t i=0;i<tuple.size();++i) {
      tupleExpansion[i*TUPLE_SIZE] += tuple[i];
      tupleExpansion[i*TUPLE_SIZE+1] += pow(tuple[i], 2.0);
      if (tuple[i] > tupleExpansion[i*TUPLE_SIZE+2]) {
        tupleExpansion[i*TUPLE_SIZE+2] = tuple[i];
      }
      if (tupleExpansion[i*TUPLE_SIZE+3] == 0 || tuple[i] < tupleExpansion[i*TUPLE_SIZE+3]) {
        tupleExpansion[i*TUPLE_SIZE+3] = tuple[i];
      }
    }
*/
  }

  void PebbleAccum::dumpFittingData(fstream &of) {
    for (size_t i=0;i<tupleExpansion.size()/TUPLE_SIZE;++i) {

      double mn = 0.0;
      if (numTuples != 0)
        mn = tupleExpansion[i*TUPLE_SIZE]/numTuples;

      double stdev = 0.0;
      if (numTuples != 0)
        stdev = sqrt(tupleExpansion[i*TUPLE_SIZE+1]/numTuples-pow(mn, 2.0));

      of << "," << numTuples;
      of << "," << mn;
      of << "," << stdev;
      of << "," << tupleExpansion[i*TUPLE_SIZE+2];
      of << "," << tupleExpansion[i*TUPLE_SIZE+3];
    } 
  }

