
#include "payload.hpp"

void Payload::init() {
  index = -1;
  holdTargetId=-1;
  selTargetId=-1;
  firstTarget = 0;
  err = -1;
  sumerr = 0.0;
  sum = 0.0;
  maxConf = 999999.0;
  sumCnt = 0;
  holdBack = false;
  for (int i=0; i<numMeas; ++i) {
    conf[i] = 0.0f;
    sup[i] = 0;
    targetId[i] = 0;
    totConf[i] = 0.0f;
    cnt[i] = 0;
  }
  tag = false;
  outputPebbles = true;
}
Payload::Payload() {
  init();
}

Payload::Payload(int i, int sz, int f, int od, bool hb) {
  init();
  itemId = i;
  itemsetSize = sz;
  firstTarget = f;
  id = od;
  holdBack = hb;
}

/*
void Payload::initConfMeas() {
  Payload::confMeas = new int[numMeas];
  for (int i=0; i<numMeas; ++i) {
    confMeas[i] = i*3+3;
  }
}
*/
