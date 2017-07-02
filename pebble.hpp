
#ifndef _PEBBLE_H
#define _PEBBLE_H 1

#include <iostream>

#include <vector>
#include <string>
#include "scoop.hpp"
#include "pebblepayload.hpp"

class Scoop;
class Payload;
class PebblePayload;
class PebbleOutputPayload;

class Pebble {

  //These are pointers into other scoops of handfuls of pebbles.
  //the bottom three bits index the pebbles within the handful.
  //The rest form a global pointer, that when added to the dozer base
  //pointer, give a location for the handful.
public:
  Pebble *prev;
  Pebble *next;
  Payload *payload;
  PebblePayload *pebblePayload;
  PebbleOutputPayload *pebbleOutputPayload;

  Scoop *scoop;
  int offst;

  Pebble() {
    prev = NULL;
    next = NULL;
    scoop = NULL;
    payload = NULL;
    offst = -1;
  }

  bool included(int level);
  void tumble(int level);
  static void tumbler(Pebble *cur, int level, int minSup);

  void dumpPayload();

  void aggregateTuples(int level, PebbleAccum &pebbleAccum);
  void addAccum(PebbleAccum pebbleAccum, int slice);

/*
  Scoop *getScoopPtr(int lnk) {
     return (Scoop *) lnk&x1FFFFFFF;
  }

  int getHandfulOffset(int lnk) {
    return lnk & 7;
  }
*/
};

#endif /* pebble.hpp */
