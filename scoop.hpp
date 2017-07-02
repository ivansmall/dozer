
#ifndef _SCOOP_H
#define _SCOOP_H 1
#include <cstdio>
#include <iostream>

#include <vector>
#include <string>

#include "pebble.hpp"
#include "dozer.hpp"

class Pebble;
class Dozer;
class TargetItem;

class Scoop {
  //a stack of recurred passes through the scoop of pebbles.
//private:
public:
  static const int recurStackSize=5;
  int recurStack[recurStackSize+1];
  int holdBackStack[recurStackSize+1];
  int holdScoreStack[recurStackSize+1];
  int noTargetStack[recurStackSize+1];
  int holdBackScoreStack[recurStackSize+1];

/*
  std::vector<int> recurStack;
  std::vector<int> holdBackStack;
  std::vector<int> holdScoreStack;
  std::vector<int> noTargetStack;
  std::vector<int> holdBackScoreStack;
*/

  int itemId;
  std::string itemName;
  int outputOffset;
  Dozer *dozer;
  int dozerPosition;
  int evalSize;

  TargetItem *targetItem;
  //the pebbles organized as handfuls of pebbles.  This allows
  //for a scoop pointr to be readily accessible.
  //private handfuls *Handful;
  Pebble **pebbles;
  int numberPebbles;
  int currentPebble;
  float genProb;
public:

  void setGenProb(float pGenProb) {
    genProb = pGenProb;
  }

  bool traverse(int level, int minSup, float minConf, int holdMinSup);
  void dump();
  void check();

  //these will have to be constructed very differently once scale becomes
  //signficant.
  Scoop(int pitemId, std::string itemName, int numsets, Dozer *pDozer, int pDdozerPosition, int output, TargetItem *t);
  //bool holdBack, bool holdScore, bool noTarget, bool holdBackScore
  Pebble *getNextPebble(bool hb);
  void tumbleTaggedPayloads(int level, int minSup);

};

#endif 
