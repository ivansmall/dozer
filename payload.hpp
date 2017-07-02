
#ifndef _PAYLOAD_H
#define _PAYLOAD_H 1

#include <iostream>

#include <vector>
#include <string>
#include "pebble.hpp"
#include "scoop.hpp"
#include "targetassoc.hpp"

/**
 * probably a virtual class connected to pebbles.  I will use this for the
 * validation and the evaluation.  Will probably store many scores when doing
 * evaluation.  I will in turn use these to construct models for the association
 * selection process.
 */
class Payload {
public:
    bool tag;
    bool outputPebbles;

    int index;
    bool holdBack;
    bool holdScore;
    bool holdBackScore;
    int firstTarget;
    int itemsetSize;
    int id;
    Pebble *firstPebble;
    int itemId;

  private:
    static const int numMeas=50;
    static int *confMeas;
    std::vector<int> assocStack;
    float conf[numMeas];
    float totConf[numMeas];
    int cnt[numMeas];
    int sup[numMeas];
    int targetId[numMeas];
    int holdTargetId;
    double err;
    int selTargetId;
    std::string Id;
    double sum;
    double sumerr;
    double maxConf;
    double sumCnt;

    std::vector<TargetAssoc *> targetAssocs;


public:
  Payload();
  Payload(int i, int sz, int f, int od, bool hb);
  void init();
  static void initConfMeas() {
    confMeas = new int[numMeas];
    for (int i=0; i<numMeas; ++i) {
      confMeas[i] = i*3+3;
    }
  }
  void addTargetAssoc(TargetAssoc *trgAssoc) {
    targetAssocs.push_back(trgAssoc);
  }

};
/*
class PayloadSrtr {
public:
  PayloadSrtr() {}
  bool operator() (Payload *p1, Payload *p2) {
    return (p1->index < p2->index);
  }
};
*/

#endif 
