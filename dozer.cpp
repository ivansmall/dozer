#include "dozer.hpp"
#include "shm.hpp"
#include "gperftools/profiler.h"
#include <iostream>
#include <iomanip>
  /**
   *
   * @param itemMap is a vector of item name strings.
   *  The index of each is used as the numeric id in the itemsets.
   *  This is a disked backed list that the function will
   *  quickly transform into an in memory equivalent.
   * @param cnt is a vector of population counts for each of the keys.  There
   *  is a static utility function named cntItemsets.  That can be used to 
   *  iterated through all itemsets.
   * @param targets is a vector of item string names that are targetted
   */
  Dozer::Dozer(item_map_type *pItemMap, std::vector<int> *cnt, std::vector<TargetItem *> *ts, double binomC) {
    test=0.0;
    numberItemsets = 0;
    numberHoldBackItemsets = 0;
    numberHoldScoreItemsets = 0;

    currentRecursion = 0;
    evalItemId = 0;
    evalItemIdIndex = 0;
    
    generateItrst = false;
    totSup = 0;
    binomCutoff = binomC;
    //adCutoff = adC; 
    //adCutoffMax = adCMax;

    totalFound = 0;
    totalDuplicatesFound = 0;
    totalChecked = 0;
    totalADChecked = 0;

    targetItems = ts;
    itemMap = pItemMap;
    std::map<string, TargetItem *> targetItemMap;
    //construct a map of symbol to a TargetItem passed in
    //this gives me a map of symbols to norms and cutoffs
    for (int i=0;i<ts->size();++i) {
      targetItemMap[(*ts)[i]->getSymbolName()] = (*ts)[i];
    }

    int itemMapSize = itemMap->size();
    //std::vector<std::string> *targets;
    //targets = new std::vector<std::string>();

    HOLD_SCORES = false;
    //setup back and forth mapping for all items 
    //the itemMap maintains an integer to name mapping
    for (int i=0;i<itemMapSize;++i) {
      std::string sk((*itemMap)[i].c_str());
      //anything containing out is assumed to follow a specific format
      //and is an outcome target value
      //outsymbolName_value
      //if this is not an outcome variable then just default it to this
      int outputF = 99999;
      if (sk.find("out") == 0) {
        int f = sk.find("_");
        if (f == string::npos) {
          cerr << "out symbol doesn't match format " << sk << endl;  
        }
        string symbol = sk.substr(3, f-3);
        outputF = atoi(sk.substr(f+1).c_str());
        if (targetItemMap.count(symbol) == 0) {
          cout << "unknown target" << endl;
        }
        if (targets.count(symbol) == 0) {
          std::vector<int> *v = new std::vector<int>();
          v->push_back(i);
          std::pair<std::string, std::vector<int> *> p(symbol, v); 
          targets.insert(p);
        } else {
          targets[symbol]->push_back(i);
        }
/*
        TargetItem ti;
        std::pair<std::string, TargetItem> oc(sk, ti);
        targetItems.insert(oc);
*/
        std::pair<int, std::string> symbolPair(i, symbol);
        symbolItems.insert(symbolPair);
      }

      //std::cout << "mapping " << i << " to " << sk << "\n";
      std::pair<std::string, int> sp(sk, i);
      std::pair<int, std::string> ip(i, sk);
      std::pair<int, int> op(i, outputF);

      smpItems.insert(sp);
      impItems.insert(ip);
      outItems.insert(op);
    }

    //maintain a list of all targets
    //these will be moved to the end of the itemsets
    int targetsSize = targets.size();
    boost::unordered_map<std::string, std::vector<int> *>::iterator it;
    for (it = targets.begin(); it != targets.end(); ++it) {
      for (size_t j=0;j<(*it).second->size();++j) {
        iTargets.insert((*(*it).second)[j]);
      }
    }
    //the itemset sorter.  They are sorted in ascending prevelance
    //the targets are moved to the end
    srtr = new Dzrsrtr(&iTargets, cnt);
    psrtr = new PebbleSrtr(&iTargets, cnt);
    int cntSize = cnt->size();
    scoopsSize = cnt->size();
    scoops = new Scoop *[scoopsSize];
    ordering = new std::vector<int>(cntSize);
    ordrmp =  new std::vector<int>(cntSize);
    for (int i=0;i<cntSize; ++i) {
      (*ordering)[i] = i;
    }

    //sort the scoop ordering
    std::sort (ordering->begin(), ordering->end(), *srtr);

    for (int i=0;i<cntSize; ++i) {
      (*ordrmp)[(*ordering)[i]] = i;
    }

    //construct the scoops, these are later loaded by the loadItemsets function
    firstTargetPosition = 0;
    for (int i=0;i<cntSize;++i) {
      //this list has been sorted by asscending prevelance
      //this provides the transformation
      int ti = (*ordering)[i];
      TargetItem *targI = NULL;
      if (iTargets.count(ti)) {
        targI = targetItemMap[symbolItems[ti]];

        if (firstTargetPosition == 0)
          firstTargetPosition = i;//scoopsSize;//scoops.size();
      }
//cout << "scoop:" << (*ordering)[i] << ", " << impItems[ti] << ", " << (*cnt)[ti] << ", " << scoops.size() << ", " << outItems[ti] << ", " << targI << ", " << symbolItems[ti] << endl;

      Scoop *s = new Scoop((*ordering)[i], impItems[ti], (*cnt)[ti], this, i, outItems[ti], targI);

      scoops[i] = s;//.push_back(s);
      //std::cout << "i " << i << ", " << s->itemId << ", " << s->numberPebbles << "\n";
    }
    std::cout << "first target position " << firstTargetPosition << "\n";
    std::cout << "total positions " << scoopsSize << endl;
  }

  void Dozer::loadItemsetsSection(itemset &it, int_vector *ids, int i, std::vector<int> &holdHalf, int numSlices) {

      int sz = it.itemIds.size();

      std::vector<int> v(sz);
      std::vector<PebblePayload *> vpp(sz);

      //take itemset out of disk mapped image and place it in local vector
      int numTuple = it.ntuples.size()/it.itemIds.size();
      int outVals = 0;
      int numOutVals = -1;
      for (int j=0;j<it.itemIds.size();++j) {
        std::string m((*itemMap)[it.itemIds[j] ].c_str());

        double vppOut = 99998;
        bool vppUnknownOut = 99998;
        double vppPrevOut = 99998;
        double vppOpen = 99998;
        if (m.find("out") == 0) {
          ++numOutVals;
          if (it.outVals[numOutVals] == 99999) {
            //out val without value
            vppUnknownOut = true;
            vppPrevOut = it.prevOutVals[numOutVals];
          } else {
            //out val
            vppUnknownOut = false;
            vppOut = it.outVals[numOutVals];
            vppPrevOut = it.prevOutVals[numOutVals];
            vppOpen = it.openVals[numOutVals];
          }
        }
        vpp[j] = new PebblePayload(it.itemIds[j], it.ntuples, j*numTuple, numTuple, vppUnknownOut, vppOut, vppPrevOut, it.genOutput, vppOpen, it.dateOffset);
      }

      std::sort(vpp.begin(), vpp.end(), *psrtr);

      //construct a payload for this itemset
      //assign an id for this payload, this could contain memberid information
      //or some other useful identifier
      int od = -1;
      if (ids != NULL) {
        od = (*ids)[i];
      }
      bool holdBack = false;
      if (holdHalf[i] == 1) {
        holdBack = true;
        ++numberHoldBackItemsets;
      }

      Payload *p = new Payload(i, sz, 0, od, holdBack);
      //add this to the global list of payloads
//I don't think this is used anymore
      payloads.push_back(p);

      Pebble *prev=NULL;
      Pebble *cur=NULL;
      ++numberItemsets;

      for (int j=0;j<sz;++j) {
        Scoop *scp = scoops[(*ordrmp)[vpp[j]->itemId]];
        bool target = false;
        if ((*ordrmp)[vpp[j]->itemId] >= firstTargetPosition) {
          if (p->firstTarget == 0) {
            p->firstTarget = (*ordrmp)[vpp[j]->itemId];
          }
          target = true;
        }
        //holdBack, holdScore, noTarget, holdBackScore
        cur = scp->getNextPebble(holdBack);

        if (j == 0)
          p->firstPebble = cur;

        cur->pebblePayload = vpp[j];
        if (target) {
          //I need to place the number of slices as well as the actual output 
          //in here
          cur->pebbleOutputPayload = new PebbleOutputPayload(numSlices,
            vpp[j]->out, vpp[j]->unknownOut, vpp[j]->prevOut, numTuple, 
            vpp[j]->open, vpp[j]->dateOffset);
          scp->targetItem->addOutputPayload(cur->pebbleOutputPayload);
        }
        cur->payload = p;
        cur->scoop = scp;
        if (prev != NULL)
          prev->next = cur;
        cur->prev = prev;
        cur->next = NULL;
        prev = cur;
      }
   }
  /**
   * load in a portion of the itemsets.  I assume these to reside
   * in multiple load files with a global id/name mapping.  A
   * static method goes through and adds up all of the items to generate
   * the count levels.
   * @param itemsets is a disk backed vector of 
   */
  void Dozer::loadItemsets(itemset_vec_type *itemsets, int_vector *ids) {
    int itemsetsSize = itemsets->size();

    //pin this to the same seed
    //boost::random::mt19937 gen, genScore;
    MyGen<uint32_t> gen, genScore;
    int zeroItemIds = 0;

    boost::random::uniform_int_distribution<> dist(0,1);
    double probs[] = {1,5};
    boost::random::discrete_distribution<> distScore(probs);

    int numSlices = (*targetItems)[0]->getNumSlices();

    cout << "itemsetsSize: " << itemsetsSize << endl;
    cout << "numSlices: " << numSlices << endl;

    std::vector<int> holdHalf(itemsetsSize);
    for (int i=0;i<itemsetsSize;++i) {
      holdHalf[i] = i % 2;
    }
    for (int i=0;i<itemsetsSize;++i) {
      int j = ((double) gen())/(gen.max()-gen.min())*itemsetsSize;
      int h = holdHalf[i];
      holdHalf[i] = holdHalf[j];
      holdHalf[j] = h;
    }

    for (int i=0;i<itemsetsSize;++i) {
      itemset &it = (*itemsets)[i];

      int sz = it.itemIds.size();
      if (sz == 0) {
        ++zeroItemIds;
        continue;
      }

      loadItemsetsSection(it, ids, i, holdHalf, numSlices);
    }

/*
    for (int i=0;i<outItemsets->size();++i) {
      itemset &it = (*outItemsets)[i];

      int sz = it.itemIds.size();
      if (sz == 0) {
        ++zeroItemIds;
        continue;
      }

      loadItemsetsSection(it, ids, i, holdHalf, numSlices);
    }
*/

    cout << "numberHoldBackItemsets: " << numberHoldBackItemsets << endl;
    cout << "number of zero item ids" << zeroItemIds << endl;


  }

  void Dozer::dump() {
    //dump out completed scoops and pebbles
    std::cout << "dumping structure\n";
    for (int i=0;i<scoopsSize; ++i) {
      Scoop *scp = scoops[i];
      //scp->dump();
    }
  }

  void Dozer::dumpNames() {
    //dump out completed scoops and pebbles
    std::cout << "dumping structure\n";
    for (int i=0;i<scoopsSize; ++i) {
      std::cout << i << " : " << scoops[i]->itemName << "\n";
    }
  }


  void Dozer::check() {
    //dump out completed scoops and pebbles
    std::cout << "dumping structure\n";
    for (int i=0;i<scoopsSize; ++i) {
      Scoop *scp = scoops[i];
      //scp->check();
    }
  }


  /**
   * go through and add itemset counts onto the cnt vector.
   * I assume this vector is the correct size.
   */
  void Dozer::cntItemsets(itemset_vec_type *itemsets, std::vector<int> *cnts) {
   int itemsetsSize = itemsets->size();
   for (int i=0;i<itemsetsSize;++i) {
     //if (i % 20 != 0)
       //continue;
     itemset &it = (*itemsets)[i];
     int itSize = it.itemIds.size();
     for (int j=0;j<itSize;++j) {
       ++((*cnts)[it.itemIds[j]]);
     }
   }
  }

  void Dozer::dumpTargetRollup() {
    vector<double> targetSums;
    targetSums.resize((*targetItems)[0]->numAssoc.size());
    for (int i=0;i<targetItems->size();++i) {
      for (int j=0;j<targetSums.size();++j) {
        targetSums[j] += (*targetItems)[i]->numAssoc[j];
      }
    }

    cout << "target sums: ";
    for (int i=0;i<targetSums.size();++i) {
      cout << targetSums[i] << ", ";
    }
    cout << endl;
  }

  void Dozer::traverse(int offset, int level, int minSup, float minConf, int holdMinSup) {
     int scoopsSize = scoopsSize;

     //traverse using the sewn pebbles for beginning levels
     for (int i=offset;i<firstTargetPosition; ++i) {
       if (level == 0) {
         if ((i % 500) == 0) {
           //if (i == 30000) ProfilerStart("/tmp/gprof.out");
           //if (i == 31000) ProfilerStop();

           std::cout << "offset = " << i << ", " << scoopsSize <<
              ", " << totalFound <<
              ", " << totalDuplicatesFound <<
              ", " << totalChecked <<
              ", " << totalADChecked <<
              ", " << totalFound << "\n";
           dumpTargetRollup();
         }

/*
         if (i == 790000) {
           cout << "exiting" << endl;
           std::cout << "offset = " << i << ", " << scoopsSize <<
                ", " << totalFound <<
                ", " << totalDuplicatesFound <<
                ", " << totalChecked <<
                ", " << totalADChecked <<
                ", " << totalFound << "\n";

           dumpTargetRollup();
           exit(0);
         }
*/
       }
       Scoop *scp = scoops[i];

/*
       //check that stacks are all cleared
       if (level != 0) {
         for (int i=offset;i<firstTargetPosition; ++i) {
           if (scoops[i]->recurStack[level+1] != 0) {
             std::cout << "scoop level not cleared for "<< i << " lev " << (level+1) << "\n";
           }
         }
       }
*/
//... don't continue traversing, if the first one is equal to minsup???
      if (i == 8019 && level == 0)
{
cout << "arrived at 8019\n";
}
      scp->traverse(level, minSup, minConf, holdMinSup);
    }

  }

  void Dozer::updateGenProb() {
/*
    float tot=0.0f;
    for (int i=firstTargetPosition;i<scoops.size();++i) {
      tot += scoops[i]->recurStack[0];
    }
*/
    std::cout << "number itemsets: " << numberItemsets << "\n";
    int scoopsSize = scoopsSize;
    for (int i=0;i<evalItemIdIndex;++i) {
      scoops[i]->setGenProb(((float) scoops[i]->recurStack[0])/numberItemsets);

      std::cout << "gen prob: " << i << "," << scoops[i]->genProb << "\n";

    }

    for (int i=evalItemIdIndex+1;i<scoopsSize;++i) {
      scoops[i]->setGenProb(((float) scoops[i]->recurStack[0])/(numberItemsets-scoops[evalItemIdIndex]->recurStack[0]));
      std::cout << "target gen prob: " << scoops[i]->genProb << "\n";
    }


  }

  void Dozer::generateAssociations(float pMinSup, float minConf, string outdir) {
    int minSup;
    if (pMinSup > 1) {
      minSup = pMinSup;
    } else {
      minSup = pMinSup*numberItemsets+.5;
    }
    std::cout << "initing ad and binom to cutoff values " << adCutoff << ":" << adCutoffMax << ", " << binomCutoff << std::endl;
    initBinomCutoffArray(binomCutoff);
    //initADCutoffArray(adCutoff, adCutoffs);
    //initADCutoffArray(adCutoffMax, adCutoffMaxs);

    int holdMinSup = binomCutoffs[minSup];
    cout << "min sup and hold min support set to : " << pMinSup << ": " << minSup << ", " << holdMinSup << endl;
/*
    scoopsA = new Scoop *[scoops.size()];
    for (int i=0;i<scoops.size();++i) {
      scoopsA[i] = scoops[i];
    }
*/
    //dumpNames();

    //open out a claims expansion file
    //remove("claimexp.csv");
    //claimexpstr.open("claimexp.csv", std::fstream::out);
    remove("assocdata.csv");
    assocdatastr.open("assocdata.csv", std::fstream::out);

    /*dump();
    scoops[2]->pebbles[0].tumble(1);
    dump();
*/
    std::cout << "number itemsets: " << numberItemsets << "\n";
    //updateGenProb();

    //ProfilerStart("/tmp/gprof.out");
    traverse(0, 0, minSup, minConf, holdMinSup); 
    //ProfilerStop();
    dumpTargetRollup();


//    Targsrtr srtr;

//    sort(targetItems->begin(), targetItems->end(), srtr);

    mkdir(outdir.c_str(), S_IRWXU|S_IRWXG);
    for (int i=0;i<targetItems->size();++i) {
      TargetItem *t = (*targetItems)[i];
      std::fstream symbolOut;
      symbolOut.open((outdir+"/"+t->getSymbolName()).c_str(), std::fstream::out);
/*
      cout << t->getSymbolName() << ": " << t->totalOverZero << " * ";
      for (int j=0;j<t->numAssoc.size();++j) {
        cout << t->numAssoc[j] << ", ";
      }
      cout << endl;
*/
      //I think I'm going to need more than one stream here
      t->dumpOutputPayloads(assocdatastr, true);
      t->dumpOutputPayloads(symbolOut, false);
      symbolOut.close();
    }

    std::cout << " total associations found: " << totalFound  << ": " << totalChecked << ": " << totalDuplicatesFound << ":" << totalADChecked << std::endl;

    removeRedundant();
    //finalAssociations = targetAssociations;
/*
    //dump them out
    for (int i=0;i<finalAssociations.size();++i) {
      TargetAssoc *a = finalAssociations[i];
      //find out if this is redundant
      std::vector<int> assoc = a->assoc;
        for (int j=0;j<assoc.size();++j) {
          std::cout << " " << scoops[assoc[j]]->itemName << ",";
        }
        std::cout << std::endl;
    }
*/

    std::cout << " total associations found: " << totalFound  << ": " << totalChecked << ": " << totalDuplicatesFound << " final after redundant removal: " << finalAssociations.size() << std::endl;

    assocdatastr.close();
  }

  void Dozer::removeRedundant() {

    //take out those explainable by others
    //take regular set and compute probability using sub set
    //associations
    //used binomial proportion test with the hold back set and keep
    //if if this fails.

    for (int i=0;i<targetAssociations.size();++i) {
      TargetAssoc *a = targetAssociations[i];
      //find out if this is redundant
      std::vector<int> assoc = a->assoc;

      bool foundRed = false;
      for (int j=1;j<assoc.size();++j) {
        std::vector<int> b(assoc.begin(), assoc.begin()+j);
        std::vector<int> c(assoc.begin()+j, assoc.end());

        if (!allAssociations.count(b) || !allAssociations.count(c)) {
          std::cout << "unable to find both associations";
          for (std::vector<int>::iterator it = b.begin(); it != b.end(); ++it)
            std::cout << ' ' << *it;
          std::cout << '\n';
          for (std::vector<int>::iterator it = c.begin(); it != c.end(); ++it)
            std::cout << ' ' << *it;
          std::cout << '\n';
          continue;
        }

        SparseTargetAssoc *bt = allAssociations[b];
        SparseTargetAssoc *ct = allAssociations[c];

        int s = (double) bt->support * (double) ct->support
                /(double) numberItemsets*2.0+.5;
        if (Dozer::getBinomConfLowerBound(s, a->holdSupport)) {
          foundRed = true;
          break;
        }
/*
        if (allAssociations.count(b) > 0 && allAssociations.count(c))
          std::cout << "found them both: " << std::endl;
        else
          std::cout << "unable to find either: " << std::endl;
*/
      }
      if (!foundRed) {
        a->offset = finalAssociations.size();
        finalAssociations.push_back(a);
        std::cout << std::endl;
        for (int j=0;j<assoc.size();++j) {
          std::cout << " " << scoops[assoc[j]]->itemName << ",";
        }
        std::cout << std::endl;
      } else {
        a->offset = -1;
      }
    }
  }

  void Dozer::dumpItemScoops() {
    for (int j=0;j<scoopsSize;++j) {
      std::cout << j << ", " << scoops[j]->itemId << "\n";
    }
  }

  void Dozer::pushAssoc(int position) {
    assocStack.push_back(position);
  }
  void Dozer::pushConf(float conf) {
    confStack.push_back(conf);
  }


  void Dozer::popAssocId() {
    assocStack.pop_back();
  }
  void Dozer::popConf() {
    confStack.pop_back();
  }


  bool Dozer::getBinomConfLowerBound(int os, int hf) {
//cout << "get binom conf lower bound " << os << ":" << hf << endl;
    int mx = os;
    int mn = hf;
    if (mx < mn) {
      mn = mx;
      mx = hf;
    }

    if (mx > binomCutoffs.size() && mn > binomCutoffs[binomCutoffs.size()-1])
        return true;

    if (mn >= binomCutoffs[mx]) {
      //std::cout << "binom " << mx << " : " << mn << " " << binomCutoffs[mx] << " " << endl ;
      return true;
    } else
      return false;

/*

    double l = boost::math::binomial_distribution<>::find_lower_bound_on_p(
                   numberItemsets*.5+1, mx, .90);
    if (mn > l )
      std::cout << "binom passed " << l << std::endl;
    else
      std::cout << "binom failed " << l << std::endl;
    if (mn > l)
      return true;
    return false; //spurious
*/
  }

  bool Dozer::checkOutputDistSeparation(int position, int level) {
    double mx = 0.0;
    double n = 0.0;

    //clear all of the TargetItem measurement arrays
    //add in all of 

    for (int i=0;i<targetItems->size();++i) {
      (*targetItems)[i]->clearMeas();
    }

    int maxN=0;
    for (int i=firstTargetPosition;i<scoopsSize;++i) {
      //m is the support at this level
      int m = scoops[i]->recurStack[level];//-scoops[i]->holdScoreStack[level];
      if (m > 0) {
        //cout << scoops[i]->targetItem->getSymbolName() << ", " << scoops[i]->targetItem->getNumMeas() << ", " << m << endl;
        int n = scoops[i]->targetItem->addMeas(scoops[i]->outputOffset, m, scoops[i]->pebbles);
        if (n > maxN)
          maxN = n;
      }
    }
    
    if (maxN < 2)
      return false;

    PebbleAccum pebbleAccum;
    aggregateAssocPebbles(level, pebbleAccum);

    bool fnd = false;
    for (int i=0;i<targetItems->size();++i) {
        //cout << "::" << (*targetItems)[i]->getSymbolName() << ", " << (*targetItems)[i]->getNumMeas() << ", " << i << endl;

      (*targetItems)[i]->normMeas();
      int fndSlice = (*targetItems)[i]->calcAD(level);
      if (fndSlice != -999) {
//this adds to all output items, not just those involved in this association

//        if (pebbleAccum.numAssoc > 0) {
          (*targetItems)[i]->addAccum(pebbleAccum, fndSlice);
          fnd = true;
//        }
//        (*targetItems)[i]->aggregatePebbles(fndSlice, level);
      }
    }

/*
    place rollup for this output target item
      
    for (int i=firstTargetPosition;i<scoopsSize;++i) {
      
      //m is the support at this level
      int m = scoops[i]->recurStack[level]-scoops[i]->holdScoreStack[level];
      if (m > 0) {
        //cout << scoops[i]->targetItem->getSymbolName() << ", " << scoops[i]->targetItem->getNumMeas() << ", " << m << endl;

        int n = scoops[i]->targetItem->addMeas(scoops[i]->outputOffset, m);
        if (n > maxN)
          maxN = n;
      }
    }
*/ 
    return fnd;
  }

  /**
   * aggregate all pebbles present in this association.  Start from the final
     scoop and proceed from all pebbles in that level.  Go through each itemset
     and add in all those pebbles present in the association.
   */
  void Dozer::aggregateAssocPebbles(int level, PebbleAccum &pebbleAccum) {
    Scoop *scp = scoops[assocStack[assocStack.size()-1]];
    for (int i=0;i<scp->recurStack[level];++i) {
      Pebble *pbl = scp->pebbles[i];
/*
if (pbl->scoop->dozerPosition > pbl->scoop->dozer->firstTargetPosition)
cout << "adding in output " << pbl->scoop->dozerPosition << ", " << pbl->scoop->dozer->firstTargetPosition << endl;
*/
      //only aggregate pebbles that are part of the output set

      if (!pbl->pebblePayload->genOutput)
        continue;

      pebbleAccum.addTuple(pbl->pebblePayload->tuple);
      pbl->payload->tag = true;
    }

    for (int i=assocStack.size()-2;i>-1;--i) {
      --level;
      Scoop *scp = scoops[assocStack[i]];
      for (int j=0;j<scp->recurStack[level];++j) {
        Pebble *pbl = scp->pebbles[i];
        if (pbl->payload->tag == true) {// && pbl->payload->outputPebbles == true) {
          Pebble *pbl = scp->pebbles[i];
          pebbleAccum.addTuple(pbl->pebblePayload->tuple);
        }
      }
    }

    for (int i=0;i<scp->recurStack[level];++i) {
      Pebble *pbl = scp->pebbles[i];
      pbl->payload->tag = false;
    }
  }

  void Dozer::printAssociation(int sup, float conf, int targetPos) {
    for (int i=0;i<assocStack.size()-1;++i) {
      //std::cout << (*ordering)[assocStack[i]] << ", ";
      std::cout << assocStack[i] << ", ";
    }
    std::cout << assocStack[assocStack.size()-1] << " :: sup = " << sup << " conf = " << conf << " trg " << targetPos << "\n";
  }

  bool Dozer::checkItrstAssoc(int position, int level, int minSup, float minConf) {
    int os = scoops[position]->recurStack[level]-scoops[position]->holdBackStack[level];

    //hold back stack size
    int hs = scoops[position]->holdBackStack[level];

    ++totalChecked;
    bool lb = getBinomConfLowerBound(os, hs);
    bool fnd = false;
    if (lb) {
      ++totalADChecked;
      //check Anderson Darling stats on output data
      fnd = checkOutputDistSeparation(position, level);
      fnd = true;
    }

    if (fnd)
      ++totalFound;
    return fnd;
  }



  void Dozer::clearRecur(int dozerPosition, int level) {
    if (level == 0) return;
    for (int i=dozerPosition;i<scoopsSize;++i) {
      scoops[i]->recurStack[level] = 0;
      scoops[i]->holdBackStack[level] = 0;
    }
  }

  double Dozer::calcADCutoff(boost::random::discrete_distribution<> &dist, boost::random::mt19937 &gen, std::vector<double> &normProbs, int testSampleSize, int sampleSize, double cutOff) {

  std::vector<double> testSamples(testSampleSize);
  std::vector<double> testProbs(normProbs.size());
/*  std::vector<double> checkSamples(normProbs.size());
  for (int j=0;j<normProbs.size();++j) {
    checkSamples[j] = 0.0;
  }
*/

  for (int i=0;i<testSampleSize;++i) {
    for (int j=0;j<normProbs.size();++j) {
      testProbs[j] = 0.0;
    }

    double ns;
    int s = sampleSize;
    if (s > 100) s = 100;
    for (int j=0;j<s;++j) {
      int k = dist(gen);
      if (k >= normProbs.size())
        std::cout << "greater than sample size\n";
      ++testProbs[k];
//      ++checkSamples[k];
    }

    for (int j=0;j<normProbs.size();++j) {
      testProbs[j] = testProbs[j]/s;
    }

    double mx=0.0;
    double mn=0.0;
    double a;
    /* Anderson Darling */
    for (int j=0;j<normProbs.size();++j) {
      a = fabs(testProbs[j]-normProbs[j])/sqrt(normProbs[j]*(1.0-normProbs[j]));
      if (mx < a)
        mx = a;
    }
/*
    //difference squared
    a = 0.0;
    for (int j=0;j<normProbs.size();++j) {
      a += pow(testProbs[j]-normProbs[j], 2);
    }
*/ 
    testSamples[i] = mx;
  }
/*
  double a=0.0;
  for (int i=0;i<checkSamples.size();++i) {
    a += checkSamples[i];
  }
*/

  //sort the testSamples
  std::sort(testSamples.begin(), testSamples.end());

  //pick out the cutoff
  int c = cutOff*testSampleSize;
  
  if (c < 0 || c >=  testSampleSize) {
    std::cout << "cutoff is out of range " << cutOff << std::endl;
    return -1;
  } else {
    return testSamples[c];
  }
  }

  /**
   * Perform the Monte Carlo statistic calculation for the Anderson
   * Darling cutoff used during association discovery.
   *
   * @param is the cutoff 0-1 for the Anderson Darling statistic.
   *
   */
  void Dozer::initADCutoffArray(double cutOff, std::vector<double> &adCs) {

    double infl=1.0;
    if (cutOff > .9999) {
      infl = cutOff;
      cutOff = .9999;
    }

    int targetsLength = scoopsSize-firstTargetPosition;
    probs.resize(targetsLength);

    double t=0.0;

    //normalize these probabilities
    for (int i=0;i<targetsLength;++i) {
      probs[i] = scoops[i+firstTargetPosition]->numberPebbles;
      t += probs[i];
    }
    for (int i=0;i<targetsLength;++i) {
      probs[i] = probs[i]/t;
    }
    for (int i=0;i<probs.size();++i) {
      std::cout << "norm " << i << " " << probs[i] << std::endl;
    }

    boost::random::mt19937 gen;

    boost::random::discrete_distribution<> dist(probs.begin(), probs.end());

    int stp=1;
    int tot = t;
    //size the ad cutoffs for the total number of possible itemsets
    //containing a target value.  I assume that the itemsets contain only
    //a single target.
    adCs.resize(tot+1);

    int stepM = 2;
    int cliff = 100;
    for (int i=2;i<tot; i+=stp) {
      int c = 10000;//tot*10/i; ???

      if (i % cliff == 0)
        stp*=stepM;

      adCs[i] = calcADCutoff(dist, gen, probs, c, i, cutOff);
//std::cout << i << ", " << adCs[i] << std::endl;
    }
    adCs[tot] = calcADCutoff(dist, gen, probs, 1, tot, cutOff);

    //this assumes the first and last elements are non zero
    for (int i=2;i<adCs.size();++i) {
      double v = adCs[i];
      double prevx,prev,next,nextx;
      if (v != 0.0) {
        prevx = i;
        prev = v;
        next = 0.0;
        continue;
      }
      if (next == 0.0) {
        for (int j=i+1;j<adCs.size();++j) {
          if (adCs[j] != 0.0) {
            nextx = j;
            next = adCs[j];
            break;
          }
        }
      }
      //perform line interpolation 
//std::cout << "approx " << prev << "," << next << "," << prevx << "," << nextx << std::endl;
      double s = (next-prev)/(nextx-prevx);
      adCs[i] = s*(i-prevx)+prev;

//std::cout << i << "\t" << adCs[i] << std::endl;

    }

    for (int i=0;i<adCs.size();++i) {
      adCs[i] = adCs[i]*infl;
    }
  }

  /**
   *
   * @param is the cutoff 0-1 for the Anderson Darling statistic.
   *
   */
  void Dozer::initBinomCutoffArray(double cutOff) {
    binomCutoffs.resize(numberItemsets/2+1);

    for (int i=2;i<binomCutoffs.size();++i) {
      double l = boost::math::binomial_distribution<>::find_lower_bound_on_p(
                   numberItemsets/2, i, (1-cutOff)/2.0, boost::math::binomial_distribution<>::jeffreys_prior_interval);
      binomCutoffs[i] = (int) (l*numberItemsets/2.0+.5);//rint(l*numberItemsets/2.0);
//std::cout << "i cutOff numberItemsets/2 " << i << " " << 100*(1-cutOff) << " " << l*numberItemsets/2 << std::endl;
    }
  }
/*
  void TargetItem::aggregatePebbles(int slice, int level) {
return;
    for (size_t i=0;i<pebbles.size();++i) {
      //traverse pebble
      Pebble *curPebble = pebbles[i];
      curPebble->aggregateTuples(slice, level);
    }
  }
*/

  void TargetItem::addAccum(PebbleAccum &pebbleAccum, int slice) {
    if (slice < 0) {
        for (int i=0;i<pebbles.size();++i) {
          int sc = pebbles[i]->pebbleOutputPayload->accum.size()+slice;
          pebbles[i]->pebbleOutputPayload->accum[sc].addAccum(pebbleAccum);
        }
    } else {
      for (int i=0;i<pebbles.size();++i) {
        pebbles[i]->pebbleOutputPayload->accum[slice].addAccum(pebbleAccum);
      }
    }
/*
    for (int i=0;i<outputPayloads.size();++i) {
if (outputPayloads[i]->accum.size() < slice+1) {
  cout << "accum is wrong size " << outputPayloads[i]->accum.size() << ":" << slice << endl;
}
      outputPayloads[i]->accum[slice].addAccum(pebbleAccum);
    }
*/
  }

  void TargetItem::addOutputPayload(PebbleOutputPayload *op) {
    outputPayloads.push_back(op);
  }
  void TargetItem::dumpOutputPayloads(fstream &of, bool withName) {
    for (int i=0;i<outputPayloads.size();++i) {
      outputPayloads[i]->dumpFittingData(of, testOutput, symbolName, withName);
    }
  }


  //just place the checks here, that's probably easier
  /**
   * @param o is the output number
   * @param m is the level of itemsets with this output
   *
   */
  int TargetItem::addMeas(int o, int m, Pebble **p) {
/*
    if (o > 9999) {
      if (norm > 0)
        cout << "o is > 9999 and norm > 0" << endl;
    }
*/

    //add all the pebbles to a vector for later calculations
    int numOutputVals = 0;
    for (int i=0;i<m;++i) {
      if (p[i]->pebblePayload->genOutput) {
        pebbles.push_back(p[i]);
      }
      if (p[i]->pebblePayload->includeInDistCalc) {
        ++numOutputVals;
      }
    }

    if (o < 9999) {
      measSep[o] += numOutputVals;
      norm += numOutputVals;
    }

    return norm;
  }

  bool PebbleSrtr::operator() (PebblePayload *pi,PebblePayload *pj) {
    long i = (*cnt)[pi->itemId];
    long j = (*cnt)[pj->itemId];

    if (targets->count(pi->itemId))
      i = i+1e7L;
    if (targets->count(pj->itemId))
      j = j+1e7L;

    if (i == j) {
      return pi->itemId < pj->itemId;
    } else {
      return (i<j);
    }
  }

  void Dozer::tumbleTaggedPayloads(int scoopStrt, int level, int minSup) {
    for (int i=scoopStrt;i<scoopsSize;++i) {
      if (scoops[i]->recurStack[level] > minSup) {
        scoops[i]->tumbleTaggedPayloads(level, minSup);
      }
    }
  }
/*
 void Dozer::dumpPayloads(assocdatastr.open("assocdata.csv"), std::fstream::out)   { 
    //pull this from the health mods.  I need
    //to generate a file with
    //symbol, outcome or NA, AD ranges for outcome mods, date offset from beginning,
    //payload info.  day of week possibly ... dunno
    //I might want assoc target info for these.  support level sum/avg ...
    //some other info might be interesting.  Have to see
  }

*/





