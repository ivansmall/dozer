#include "scoop.hpp"



  bool Scoop::traverse(int level, int minSup, float minConf, int holdMinSup) {

    if (Scoop::recurStackSize <= level || recurStack[level]-holdBackStack[level] < minSup || holdBackStack[level] < holdMinSup)
      return false;
    //push back assoc id
    dozer->pushAssoc(dozerPosition);
    dozer->clearRecur(dozerPosition, level+1);
    if (this != dozer->scoops[dozerPosition])
      std::cout << "scoop malformed\n";

    //if (level == 0) {
      for (int i=0;i<recurStack[level];++i) {
        Pebble **p = pebbles+i;
        Pebble::tumbler(*p, level+1, minSup);
      }
/*
    } else {
      for (int i=0;i<recurStack[level];++i) {
        Pebble **p = pebbles+i;
        (*p)->payload->tag = true;
      }
      //this goes through all of the scoops above min support at level and then
      //traverses and builds level+1
      dozer->tumbleTaggedPayloads(dozerPosition+1, level, minSup);

      //clear the payload tag.  I don't need this anymore.
      for (int i=0;i<recurStack[level];++i) {
        Pebble **p = pebbles+i;
        (*p)->payload->tag = false;
      }
    }
*/

    //check targets
    bool found;
    if (level > 0)
      found = dozer->checkItrstAssoc(dozerPosition, level, minSup, minConf);

    dozer->traverse(dozerPosition+1, level+1, minSup, minConf, holdMinSup);

    dozer->clearRecur(dozerPosition, level+1);

    //clear the payloads
    for (int i=0;i<recurStack[level];++i) {
      Pebble **p = pebbles+i;
      (*p)->payload->tag = true;
    }


    //pop assoc id
    dozer->popAssocId();
    return true;
  }

  void Scoop::tumbleTaggedPayloads(int level, int minSup) {
    for (int i=0;i<recurStack[level];++i) {
      if (pebbles[i]->payload->tag) {
        pebbles[i]->tumble(level);
      }
    }
  }
  //these will have to be constructed very differently once scale becomes
  //signficant.
  Scoop::Scoop(int pitemId, std::string pitemName, int numsets, Dozer *pDozer,
      int pDozerPosition, int offSt, TargetItem *t) {
    itemId = pitemId;
    itemName = pitemName;
    numberPebbles = numsets;
    dozer = pDozer;
    dozerPosition = pDozerPosition;
    targetItem = t;
    pebbles = new Pebble *[numsets];
    for (int i=0;i<numsets;++i) {
      pebbles[i] = new Pebble();
      pebbles[i]->offst = i;
    }
/*
    recurStack.resize(recurStackSize);
    holdBackStack.resize(recurStackSize);
    holdScoreStack.resize(recurStackSize);
    noTargetStack.resize(recurStackSize);
    holdBackScoreStack.resize(recurStackSize);
*/
    evalSize = 0;
    genProb = 0;
    currentPebble = 0;
    outputOffset = offSt;
    //is this really necessary?
    for (int i=0;i<recurStackSize+1;++i) {
      recurStack[i] = 0;
      holdBackStack[i] = 0;
      holdScoreStack[i] = 0;
      noTargetStack[i] = 0;
      holdBackScoreStack[i] = 0;
    }

    //currentPebble = 0;
    //recurStack.push_back(numsets);
    //recurStack.push_back(0);
  }
//bool holdBack, bool holdScore, bool noTarget, bool holdBackScore
  Pebble *Scoop::getNextPebble(bool hb) {
    //select next element from stack, initialize
    //++currentPebble; 
    ++recurStack[0];
    if (recurStack[0] > numberPebbles) {
      std::cout << "error with next pebble, past end\n";
    }
    if (hb) {
      ++holdBackStack[0];
    }
/*
    if (holdBackScore) {
      ++holdBackScoreStack[0];
    }
    if (holdScore) {
      ++holdScoreStack[0];
    }
    if (noTarget) {
      ++noTargetStack[0];
    }
*/
    return *(pebbles+(recurStack[0]-1));//currentPebble-1;
  }

