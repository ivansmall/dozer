#include "pebble.hpp"
#include "scoop.hpp"

  void Pebble::tumble(int level) {
    Pebble *curPP = scoop->pebbles[scoop->recurStack[level+1]];
    scoop->pebbles[scoop->recurStack[level+1]] = this;
    scoop->pebbles[offst] = curPP;
    curPP->offst = offst;
    offst = scoop->recurStack[level+1];

    ++(scoop->recurStack[level+1]);

    if (payload->holdBack) {
      ++(scoop->holdBackStack[level+1]);
    }

  }

  void Pebble::tumbler(Pebble *cur, int level, int minSup) {
    while (cur != NULL) {
      Scoop *scoop = cur->scoop;
/*
      int lvlm1 = scoop->recurStack[level-1];
      if (lvlm1 < minSup) {
        cur = cur->next;
        continue;
      }
*/

      int lvl = scoop->recurStack[level];

      Pebble *curPP = (scoop->pebbles)[lvl];
      scoop->pebbles[lvl] = cur;
      scoop->pebbles[cur->offst] = curPP;
      curPP->offst = cur->offst;
      cur->offst = lvl;

      //increment the stack
      Payload *tmpp = cur->payload;
      if (tmpp->holdBack) {
        ++(scoop->holdBackStack[level]);
      }
/*
      if (tmpp->holdBackScore) {
        ++(scoop->holdBackScoreStack[level]);
      }

      if (tmpp->holdScore) {
        ++(scoop->holdScoreStack[level]);
      }
*/
      if (tmpp->firstTarget == 0) {
        ++(scoop->noTargetStack[level]);
      }

      ++(scoop->recurStack[level]);


      cur = cur->next;
    }
  }

  bool Pebble::included(int level) {
    if (offst < scoop->recurStack[level]) {
      return true;
    } else {
      return false;
    }
  }

  void Pebble::aggregateTuples(int level, PebbleAccum &pebbleAccum) {
    //I don't add in the first one.  I assume this is an output pebble
    Pebble *curPebble = this;
    while(curPebble != NULL) {
      //is this one included in the itemset
      if (curPebble->included(level) && curPebble->pebblePayload->genOutput) {
        pebbleAccum.addTuple(curPebble->pebblePayload->tuple);
      }
      curPebble = curPebble->prev;
    }
  }

  void Pebble::addAccum(PebbleAccum pebbleAccum, int slice) {
    pebbleOutputPayload->accum[slice].addAccum(pebbleAccum);
  }

/*
  void Pebble::tumblerOld(Pebble *cur, int level) {
    while (cur != NULL) {
      Pebble *p = cur->prev;
      Pebble *n = cur->next;
      cur->prev = cur->scoop->pebbles[cur->scoop->recurStack[level]].prev;
      cur->next = cur->scoop->pebbles[cur->scoop->recurStack[level]].next;
      cur->scoop->pebbles[cur->scoop->recurStack[level]].prev = p;
      cur->scoop->pebbles[cur->scoop->recurStack[level]].next = n;
      if (cur->prev != NULL) cur->prev->next = cur;
      if (p != NULL) p->next = &(cur->scoop->pebbles[cur->scoop->recurStack[level]]);
      if (n != NULL) n->prev = &(cur->scoop->pebbles[cur->scoop->recurStack[level]]);
      if (cur->next != NULL) cur->next->prev = cur;

      //swap them
      Payload *tmpp = cur->payload;
      cur->payload = cur->scoop->pebbles[cur->scoop->recurStack[level]].payload;
      cur->scoop->pebbles[cur->scoop->recurStack[level]].payload = tmpp;
      if (cur->prev == NULL) {
        cur->payload->firstPebble = cur;
      }
      if (cur->scoop->pebbles[cur->scoop->recurStack[level]].prev == NULL) {
        cur->scoop->pebbles[cur->scoop->recurStack[level]].payload->firstPebble = &cur->scoop->pebbles[cur->scoop->recurStack[level]];
      }

      //increment the stack
      if (tmpp->holdBack) {
        ++(cur->scoop->holdBackStack[level]);
      }
      if (tmpp->holdBackScore) {
        ++(cur->scoop->holdBackScoreStack[level]);
      }

      if (tmpp->holdScore) {
        ++(cur->scoop->holdScoreStack[level]);
      }
      if (tmpp->firstTarget == 0) {
        ++(cur->scoop->noTargetStack[level]);
      }

      ++(cur->scoop->recurStack[level]);

      cur = n;
    }
  }
*/

/*
  void Pebble::dumpPayload() {
    std::cout << "dumping payload\n";
    std::cout << payload->conf << "\t" << payload->sup << "\t" << payload->index << "\t" << payload->targetId << "\t";
    int i=0;
    for (;i<((int) payload->assocStack.size())-1;++i) {
      std::cout << payload->assocStack[i] <<  ":";
    }
    if (payload->assocStack.size() != 0)
      std::cout << payload->assocStack[i] << "\n";
    std::cout << std::flush;
  }
*/
