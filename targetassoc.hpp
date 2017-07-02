
#ifndef _TARGETASSOC_H
#define _TARGETASSOC_H 1

#include <iostream>

#include <vector>
#include <string>
#include "pebble.hpp"
#include "scoop.hpp"

class Pebble;

/**
 *
 * Holds a found association and stats associated with it
 *
 */
class TargetAssoc {
  public:

  std::vector<std::string> itemNames;
//  double binom;
//  double andDarl;
  double support;
  double holdSupport;
  int offset;
  std::vector<int> assoc;
  double outputDistSeparation;

  //hashed item values
  int hashedItemCollection;

  std::vector<double> *probDist;
  double probSupport;
//temporary
//std::vector<Pebble *> members;

  TargetAssoc(double s, double h, std::vector<int> hldAssoc) {
//    binom = b;
//    andDarl = a;
    support = s;
    holdSupport = h;
    assoc = hldAssoc;
    probDist = NULL;
    probSupport = 0.0;
  }
  virtual ~TargetAssoc() {
    if (probDist != NULL)
      delete probDist;
  }
  void setOutputDistSeparation(double d) {
    outputDistSeparation = d;
  }
  void setProbDist(std::vector<double> *d) {
    probDist = d;
  }

  void setProbSupport(double p) {
    probSupport = p;
  }

  void setOffset(int t) {
    offset = t;
  }

  void addItemName(std::string name) {
    itemNames.push_back(name);

  }
};

class SparseTargetAssoc {
  public:

  double support;
  double holdSupport;

  SparseTargetAssoc(double s, double h) {
    support = s;
    holdSupport = h;
  }

  SparseTargetAssoc(TargetAssoc *t) {
    support = t->support;
    holdSupport = t->holdSupport;
  }
};
#endif 
