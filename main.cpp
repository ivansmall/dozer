#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <math.h>
#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <cstdio>


#include "assoc.hpp"
#include "dozer.hpp"

using namespace std;

namespace ip = boost::interprocess;
namespace po = boost::program_options;

int generateSample(string inputFilename, string filename) {
   const int FileSize          = 65536*10;

   //Named allocate capable shared memory allocator
   ip::managed_mapped_file mfile(ip::create_only, filename.c_str(), FileSize);

   //An allocator convertible to any allocator<T, segment_manager_t> type
   void_allocator alloc_inst (mfile.get_segment_manager());
   string_allocator allocstr_inst (mfile.get_segment_manager());


   item_map_type *itemMap = mfile.construct<item_map_type>
         ("ItemKeyMap")(allocstr_inst);

   item_map_type *memberMap = mfile.construct<item_map_type>
         ("MemberKeyMap")(allocstr_inst);

   itemset_vec_type *itemsets = mfile.construct<itemset_vec_type>
         ("ItemSetVector")(allocstr_inst);

   int_vector *ids = mfile.construct<int_vector>
          ("ItemSetIds")(allocstr_inst);

   //read in a file 
   std::fstream inputStream;
   inputStream.open(inputFilename.c_str(), std::fstream::in);
   std::map<string,int> keymap;




   //vector<eval> evaluationResults;
   //vector<eval> holdBackResults;
   string delimiters = " ,";
   size_t current;
   size_t next = -1;
   std::string s;
   int keyId = 0;
   std::string buffer;
   int itemsetId = 0;
   while(getline(inputStream, buffer, '\n')) {
     itemset itms(alloc_inst, true, true, 1);
     do {
       current = next + 1;
       next = buffer.find_first_of( delimiters, current );
       s = buffer.substr( current, next - current );
       if (keymap.count(s) == 0) {
         keymap[s] = keyId;
         ++keyId; char_string c(s.c_str(), allocstr_inst);
         itemMap->push_back(c);
       } else {
         keyId = keymap[s];
       }
       itms.itemIds.push_back(keyId);
     }
     while (next != string::npos);

     itemsets->push_back(itms);
     ids->push_back(itemsetId);

     ++itemsetId;
   }

   //Flush cached data from memory-mapped file to disk
   mfile.flush();
   ip::managed_mapped_file::shrink_to_fit(filename.c_str());
}

  static std::vector<std::string> split(string buffer, string delimiters) {
    size_t current;
    size_t next = -1;
    std::vector<std::string> flds;
    do {
       current = next + 1;
       next = buffer.find_first_of( delimiters, current );
       std::string s = buffer.substr( current, next - current );
       flds.push_back(s);
     }
     while (next != string::npos);

     return flds;
  }


class SampAssoc {
  public:

    //probabilities of the general association case
    //these do not modify the outcome
    static std::vector<std::string> outcomeNoms;
    static std::vector<double> normProbs;

    //association specific outcome probabilities
    std::vector<double> *probs;

    //nominal values in this association
    std::vector<std::string> *values;

    //the distribution of outcome values for this outcome changing 
    //association
    boost::random::discrete_distribution<> *modOutcomeDist;


    //outcome samples that do influence outcome
    static std::vector<SampAssoc *> outcomeSamples;
    static std::vector<double> outcomeProbs;

    //probability of each itemsets presence
    //outcome samples that do not influence outcome
    static std::vector<SampAssoc *> samples;
    static std::vector<double> assocProbs;

    //general random generater
    static boost::random::mt19937 *gen;
    //distribution of itemsets that do not modify outcome
    static boost::random::discrete_distribution<> *dist;
    //distribution of itemsets that do modify outcome
    static boost::random::discrete_distribution<> *outcomeDist;


    //sample assoc with outcome prob attached
    SampAssoc(const std::vector<double>& modProbs, const std::vector<std::string>& vals) {
      if (modProbs.size() > normProbs.size()) {
        std::cerr << "incorrect sample association construction" << std::endl;
      }

      probs = new std::vector<double>(normProbs.size());
      for (int i=0;i<normProbs.size();++i) {
        if (i > modProbs.size()) {
          (*probs)[i] = normProbs[i];
        } else {
          (*probs)[i] = normProbs[i]*modProbs[i];
        }
      }
      modOutcomeDist = new boost::random::discrete_distribution<>(probs->begin(), probs->end());

      values = new std::vector<std::string>(vals);
    }

    SampAssoc(const std::vector<std::string>& vals) {
      values = new std::vector<std::string>(vals);
      probs = NULL;
    }

    ~SampAssoc() {
      if (probs != NULL)
        delete probs;
      if (values != NULL)
        delete values;
    }

    void dump() {
      if (probs != NULL) {
        for (int i=0;i<probs->size();++i) {
          std::cout << (*probs)[i] << ", ";
        }
      }
      std::cout << std::endl;
      for (int i=0;i<values->size();++i) {
        std::cout << (*values)[i] << ", ";
      }
      std::cout << std::endl;
   
    }

    static void addOutcomeMods(double prob, const std::vector<double>& oprobMods, std::vector<std::string>& vals) {

      outcomeProbs.push_back(prob);
      outcomeSamples.push_back(new SampAssoc(oprobMods, vals));
    }

    static void add(double prob, std::vector<std::string>& vals) {
      assocProbs.push_back(prob);
      samples.push_back(new SampAssoc(vals));
    }

    static std::set<std::string> getNextAssoc() {
      std::set<std::string> assoc;
      std::vector<std::string> items;

      //select an outcome generting itemset
      int k = (*outcomeDist)(*gen);
      SampAssoc *smp = outcomeSamples[k];

      assoc.insert(smp->values->begin(), smp->values->end());

      //select the outcome element
      k = (*smp->modOutcomeDist)(*gen);
      assoc.insert(outcomeNoms[k]);
std::cout << " outcome " << k << ", " << outcomeNoms[k] << std::endl;

      //select 3 general elements
      for (int i=0;i<3;++i) {
        k = (*dist)(*gen);
        smp = samples[k];
        assoc.insert(smp->values->begin(), smp->values->end());
      }


      return assoc;
    }

    /*
     * construct all of the random distributions
     */
    static void construct() {
      SampAssoc::gen = new boost::random::mt19937();

      //top level dist
      dist = new boost::random::discrete_distribution<>(assocProbs.begin(), assocProbs.end());

      outcomeDist = new boost::random::discrete_distribution<>(outcomeProbs.begin(), outcomeProbs.end());

    }
};

std::vector<double> SampAssoc::normProbs;
std::vector<std::string> SampAssoc::outcomeNoms;
std::vector<double> SampAssoc::outcomeProbs;

std::vector<SampAssoc *> SampAssoc::outcomeSamples;
std::vector<SampAssoc *> SampAssoc::samples;
std::vector<double> SampAssoc::assocProbs;

boost::random::mt19937 *SampAssoc::gen;
boost::random::discrete_distribution<> *SampAssoc::dist;
boost::random::discrete_distribution<> *SampAssoc::outcomeDist;


inline double convertToDouble(std::string const& s) {
  std::istringstream i(s);
  double x;
  if (!(i >> x))
    std::cerr << "bad conversion to double" << s << std::endl;

  return x;
}


int generateProbSample(string inputFilename, string filename) {
   const int FileSize          = 65536*1000;

   //Named allocate capable shared memory allocator
   ip::managed_mapped_file mfile(ip::create_only, filename.c_str(), FileSize);

   //An allocator convertible to any allocator<T, segment_manager_t> type
   void_allocator alloc_inst (mfile.get_segment_manager());
   string_allocator allocstr_inst (mfile.get_segment_manager());


   item_map_type *itemMap = mfile.construct<item_map_type>
         ("ItemKeyMap")(allocstr_inst);

   item_map_type *memberMap = mfile.construct<item_map_type>
         ("MemberKeyMap")(allocstr_inst);

   itemset_vec_type *itemsets = mfile.construct<itemset_vec_type>
         ("ItemSetVector")(allocstr_inst);

   int_vector *ids = mfile.construct<int_vector>
          ("ItemSetIds")(allocstr_inst);

   //read in a file item probabilities
   std::fstream inputStream;
   inputStream.open(inputFilename.c_str(), std::fstream::in);
   std::map<string,double> outcomeProbs;

   string delimiters = " ,";

   //update both the outcome nominal value and the normal probability
   std::string buffer;
   while(getline(inputStream, buffer, '\n')) {
     std::vector<std::string> flds = split(buffer, delimiters);
     if (flds[0] == "*")
       break;
     SampAssoc::normProbs.push_back(convertToDouble(flds[1]));
     SampAssoc::outcomeNoms.push_back(flds[0]);
   }

   //first line assembly, second line outcome adjustements
   while(getline(inputStream, buffer, '\n')) {
     std::vector<std::string> flds = split(buffer, delimiters);
     if (flds.size() == 0)
       break;
     if (flds[0] == "*")
       break;

     getline(inputStream, buffer, '\n');
     std::vector<std::string> oprobsStr = split(buffer, delimiters);
     std::vector<double> oprobs(oprobsStr.size());
     for (int i=0;i<oprobsStr.size();++i)
       oprobs[i] = convertToDouble(oprobsStr[i]);

     std::vector<std::string> nomFlds(flds.begin()+1, flds.end());
     SampAssoc::addOutcomeMods(convertToDouble(flds[0]), oprobs, nomFlds);
   }

   //just assembly to complete the associations
   while(getline(inputStream, buffer, '\n')) {
     std::vector<std::string> flds = split(buffer, delimiters);
     if (flds.size() < 2)
       break;

     std::vector<std::string> nomFlds(flds.begin()+1, flds.end());
     SampAssoc::add(convertToDouble(flds[0]), nomFlds);
   }

   //dump all of the sample associations probs loaded
   std::cout << "outcome samples" << std::endl;
   for (int i=0;i<SampAssoc::outcomeSamples.size();++i) {
     std::cout << "  " << std::endl;
     SampAssoc::outcomeSamples[i]->dump();
   }
   std::cout << "samples" << std::endl;
   for (int i=0;i<SampAssoc::samples.size();++i) {
     std::cout << "  " << std::endl;
     SampAssoc::samples[i]->dump();
   }
   std::cout << "assoc probs" << std::endl;
   for (int i=0;i<SampAssoc::assocProbs.size();++i) {
     std::cout <<SampAssoc::assocProbs[i] << std::endl;
   }

   //construct all of the probabilities
   SampAssoc::construct();

   std::map<string,int> keymap;
   int itemsetId = 0;
   int currentKeyId = -1;
   for (int i=0;i<5000;++i) {
     itemset itms(alloc_inst,true,true, 1);

     std::set<string> st = SampAssoc::getNextAssoc();
     for (std::set<string>::iterator it=st.begin(); it!=st.end(); ++it) {
       int keyId;
       if (keymap.count(*it) == 0) {
         ++currentKeyId;
         keyId = currentKeyId;
         keymap[*it] = keyId;
         char_string c(it->c_str(), allocstr_inst);
         itemMap->push_back(c);
//std::cout << "placing " << *it << " " << keyId << " " << c << std::endl;
       } else {
         keyId = keymap[*it];
       }
       itms.itemIds.push_back(keyId);
     }
     itemsets->push_back(itms);
     ids->push_back(itemsetId);

     ++itemsetId;
   }

    int itemMapSize = itemMap->size();
    for (int i=0;i<itemMapSize;++i) {
      std::string sk((*itemMap)[i].c_str());
//      std::cout << "mapping " << i << " to " << sk << " " << keymap[sk] << "\n";
    }


   //Flush cached data from memory-mapped file to disk
   mfile.flush();
   ip::managed_mapped_file::shrink_to_fit(filename.c_str());
}

int dump2(string filename) {
//Map preexisting file again in memory
   ip::managed_mapped_file mfile(ip::open_only, filename.c_str());
   int next_file_size = mfile.get_size();

   item_map_type *itemMap = mfile.find<item_map_type>("ItemKeyMap").first;

   itemset_vec_type *myvec = mfile.find<itemset_vec_type>("ItemSetVector").first;
   itemset_vec_type *eval = mfile.find<itemset_vec_type>("ItemSetsEval").first;

   //Check vector is still there
   if(!myvec)
     printf("unable to locate itemset vector");
/*
   for (int i=0;i<myvec->size();++i) {
     itemset &it = (*myvec)[i];
     for (int j=0;j<it.itemIds.size();++j) {
       cout << it.itemIds[j] << ",";
     }
     cout << "\n";
   }

   cout << "evaluation data\n";
   for (int i=0;i<eval->size();++i) {
     itemset &it = (*eval)[i];
     for (int j=0;j<it.itemIds.size();++j) {
       cout << it.itemIds[j] << ",";
     }
     cout << "\n";
   }
*/
   if(!itemMap)
     printf("unable to locate assoc map");
   //for (int i=0;i<itemMap

   //char_string test("test");

   //assoc &a = (*mymap)[test];
}

int dumpItemsets(string filename) {
//Map preexisting file again in memory
   ip::managed_mapped_file mfile(ip::open_only, filename.c_str());
   int next_file_size = mfile.get_size();

   item_map_type *itemMap = mfile.find<item_map_type>("ItemKeyMap").first;

   item_map_type *memberMap = mfile.find<item_map_type>("MemberKeyMap").first;

   itemset_vec_type *myvec = mfile.find<itemset_vec_type>("ItemSetVector").first;
   targetitem_vec_type *targetitems = mfile.find<targetitem_vec_type>
          ("TargetItemVector").first;
   int_vector *ids = mfile.find<int_vector>("ItemSetIds").first;

   //Check vector is still there
   if(!myvec)
     printf("unable to locate itemset vector");

   std::map<int, std::string> impItems;
   for (int i=0;i<itemMap->size();++i) {
     std::string sk((*itemMap)[i].c_str());
     //std::cout << "mapping " << i << " to " << sk << "\n";
     std::pair<int, std::string> ip(i, sk);

     impItems.insert(ip);
   }

   cout << "dumping target items" << endl;
   for (size_t i=0;i<targetitems->size();++i) {
     targetitem &t=(*targetitems)[i];

     cout << "name: ";
     cout << t.name << endl;

     cout << "norm probs: " << t.normProbs.size() << endl;
     for (size_t j=0;j<t.normProbs.size();++j) {
       cout << t.normProbs[j] << ", ";
     }
     cout << endl;

     cout << "cutoffSizes: " << t.cutoffSizes.size() << endl;

     for (size_t j=0;j<t.cutoffSizes.size();++j) {
       cout << t.cutoffSizes[j] << ", ";
     }
     cout << endl;

     cout << "cutoffs: " << t.cutoffs.size() << endl;
     for (size_t j=0;j<150;++j) {
       cout << t.cutoffs[j] << ", ";
     }
     cout << endl;
     for (size_t j=0;j<t.cutoffs.size();j+=t.cutoffs.size()/t.cutoffSizes.size()) {
       cout << t.cutoffs[j+15] << ", ";
     }
     cout << endl;
   }

   cout << "dumping itemsets: " << myvec->size() << endl;
   for (int i=0;i<myvec->size();++i) {
     itemset &it = (*myvec)[i];
     //cout << "member id: " << (*memberMap)[i].c_str() << "\n";
     cout << "\n*******" << it.itemIds.size() << endl;
     for (int j=0;j<it.itemIds.size();++j) {
       //cout << it.itemIds[j] << ":" << impItems[it.itemIds[j] ] << ",";
       cout << impItems[it.itemIds[j] ] << "\n";
     }
   }

}
int dump3(string filename) {
//Map preexisting file again in memory
   ip::managed_mapped_file mfile(ip::open_only, filename.c_str());
   int next_file_size = mfile.get_size();

   item_map_type *itemMap = mfile.find<item_map_type>("ItemKeyMap").first;

   itemset_vec_type *myvec = mfile.find<itemset_vec_type>("ItemSetVector").first;
   itemset_vec_type *eval = mfile.find<itemset_vec_type>("ItemSetsEval").first;

   targetitem_vec_type *targetitems = mfile.find<targetitem_vec_type>
          ("TargetItemVector").first;

   cout << "Target Items" << endl;
   for (int i=0;i<targetitems->size();++i) {

     cout << "name: " << (*targetitems)[i].name << endl;
     cout << "norm probs: " << endl;
     //for (int j=0;j<
     if (i > 100)
       break;
   }

   //Check vector is still there
   if(!myvec)
     printf("unable to locate itemset vector");

   std::map<int, std::string> impItems;
   for (int i=0;i<itemMap->size();++i) {
     std::string sk((*itemMap)[i].c_str());
//     std::cout << "mapping " << i << " to " << sk << "\n";
     std::pair<int, std::string> ip(i, sk);

     impItems.insert(ip);
   }


   int numTuples = -1;
   for (int i=0;i<myvec->size();++i) {
     itemset &it = (*myvec)[i];
     for (int j=0;j<it.itemIds.size();++j) {
       //cout << it.itemIds[j] << ":" << impItems[it.itemIds[j] ] << ",";
       if (numTuples == -1) {
         numTuples = it.ntuples.size()/it.itemIds.size();
         cout << "num tuples: " << numTuples << endl;
       }
       cout << it.itemIds[j] << "," << impItems[it.itemIds[j] ];
       std::string sk(impItems[it.itemIds[j] ].c_str());
       if (sk.find("out") == 0) {
         int f = sk.find("_");
         if (f == string::npos) {
           cerr << "out symbol doesn't match format " << sk << endl;
         }
         string symbol = sk.substr(3, f-3);
         int k = atoi(sk.substr(f+1).c_str());
         cout << " " << symbol << "***" << k;
         cout << " " << it.outVals[k] << ", " << it.prevOutVals[k];
         cout << ", " << it.outValsF[k] << " " ;
       }


       cout << "(";
       for (int k=0;k<numTuples;++k) {
         if (k != 0)
           cout << ",";
         cout << it.ntuples[j*numTuples+k];
       }
       cout << ")" << endl;
     }
     cout << "\n*******\n";
   }

}
int dump4(string filename) {
//Map preexisting file again in memory
   ip::managed_mapped_file mfile(ip::open_only, filename.c_str());
   int next_file_size = mfile.get_size();

   item_map_type *itemMap = mfile.find<item_map_type>("ItemKeyMap").first;

   itemset_vec_type *myvec = mfile.find<itemset_vec_type>("ItemSetVector").first;
   targetitem_vec_type *targetitems = mfile.find<targetitem_vec_type>
          ("TargetItemVector").first;

   cout << "Target Items" << endl;
   for (int i=0;i<targetitems->size();++i) {
     cout << "name: " << (*targetitems)[i].name << endl;
     cout << "  avg spread: " << (*targetitems)[i].targetVal << endl;
     cout << "  test output: " << (*targetitems)[i].testOutput << endl;
   }

   //Check vector is still there
   if(!myvec)
     printf("unable to locate itemset vector");

   std::map<int, std::string> impItems;
   for (int i=0;i<itemMap->size();++i) {
     std::string sk((*itemMap)[i].c_str());
//     std::cout << "mapping " << i << " to " << sk << "\n";
     std::pair<int, std::string> ip(i, sk);

     impItems.insert(ip);
   }

   cout << "vec size " << myvec->size() << endl;

   int numTuples = -1;
   for (int i=0;i<myvec->size();++i) {
     itemset &it = (*myvec)[i];
     int numOutVals = 0;
     for (int j=0;j<it.itemIds.size();++j) {
       //cout << it.itemIds[j] << ":" << impItems[it.itemIds[j] ] << ",";
       if (numTuples == -1) {
         cout << "item ids size: " << it.itemIds.size() << endl;
         cout << "ntuples size: " << it.ntuples.size() << endl;
         cout << "out vals size: " << it.outVals.size() << endl;
         cout << "prev out vals size: " << it.prevOutVals.size() << endl;
         cout << "out valsf size: " << it.outValsF.size() << endl;
         numTuples = it.ntuples.size()/it.itemIds.size();
         cout << "num tuples: " << numTuples << endl;
       }
       cout << it.itemIds[j] << "," << impItems[it.itemIds[j] ];
       std::string sk(impItems[it.itemIds[j] ].c_str());
       if (sk.find("out") == 0) {
         int f = sk.find("_");
         if (f == string::npos) {
           cerr << "out symbol doesn't match format " << sk << endl;
         }
         string symbol = sk.substr(3, f-3);
         int k = atoi(sk.substr(f+1).c_str());
         cout << " " << symbol << "***" << k;
         cout << " " << it.outVals[numOutVals] << ", " << it.prevOutVals[numOutVals];
         cout << ", " << it.outValsF[numOutVals] << " " ;
         ++numOutVals;
       }


       cout << "(";
       for (int k=0;k<numTuples;++k) {
         if (k != 0)
           cout << ",";
         cout << it.ntuples[j*numTuples+k];
       }
       cout << ")" << endl;
     }
     cout << "\n*******\n";
   }

}


/*
struct myclass {
  bool operator() (int i,int j) { return (i<j);}
} myobject;
*/

void generateAssociations(vector<string> *inputFilenames, float minSup, float minConf, vector<string> *targets, bool dumpe, double binomCutoff) {

   ip::managed_mapped_file mfile(ip::open_only, (*inputFilenames)[0].c_str());

   item_map_type *itemMap = mfile.find<item_map_type>("ItemKeyMap").first;

   itemset_vec_type *itemsets = mfile.find<itemset_vec_type>("ItemSetVector").first;
   //itemset_vec_type *outItemsets = mfile.find<itemset_vec_type>("OutItemSetVector").first;


   int_vector *ids = mfile.find<int_vector>("ItemSetIds").first;


   targetitem_vec_type *targetitems = mfile.find<targetitem_vec_type>
          ("TargetItemVector").first;


   //...
   //for each symbol include:
   //name
   //normal probs vector
   //vector of ad cutoffs vector(size of itemsets) times number of symbols 

   vector<int> *cnts = new vector<int>();
   cnts->resize(itemMap->size());
   Dozer::cntItemsets(itemsets, cnts);
   //Dozer::cntItemsets(outItemsets, cnts);

/*
   for (int i=0;i<cnts.size();++i) {
     cout << i << "  " << cnts[i] << "\n";
   }
*/
   std::vector<TargetItem *> *ts = new std::vector<TargetItem *>();

   int targetitemsSize = targetitems->size();
   for (int i=0;i<targetitemsSize;++i) {
     targetitem &t = (*targetitems)[i];
//what happens if I restrict these?
     TargetItem *ti = new TargetItem(t);
     ts->push_back(ti);
   }

   Dozer *dzr = new Dozer(itemMap, cnts, ts, binomCutoff);

   dzr->loadItemsets(itemsets, ids);

   if (dumpe) {
     dzr->dumpItemScoops();
   } else {
     //probably need an output file here too
     string outdir = "out"+(*inputFilenames)[0];
     dzr->generateAssociations(minSup, minConf, outdir);
   }
}

int main(int ac, char **av) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("minsupport,s", po::value<float>(), "minimum support")
    ("minconfidence,c", po::value<float>(), "minimum confidence")
    ("target,t", po::value< vector<string> >(), "multiple target values, assume independent")
    ("generate,g", po::value<string>(), "generate sample into filename")
    ("probgen,p", po::value<string>(), "generate probability driven sample into filename")
    ("dump,d", po::value<string>(), "dump file")
    ("dumpe,e",po::value<string>(), "dump eval data")
    ("dumpf,f",po::value<string>(), "dump interesting association data")
    ("inputfile,i", po::value< vector<string> >(), "multiple input files, assume independent")
    ("adCutoff,a", po::value<double>(), "Anderson Darling Cutoff")
    ("adCutoffMax,m", po::value<double>(), "Anderson Darling Cutoff Max")
    ("binomCutoff,b", po::value<double>(), "Binomial Cutoff")
  ;

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);    

  if (vm.count("help")) {
    cout << desc << "\n";
    return 0;
  }

  vector<string> inputFilenames;
  if (vm.count("inputfile")) {
    cout << vm.count("inputfile") << "\n";
    inputFilenames = vm["inputfile"].as< vector<string> >();
    cout << "Input file values set to:\n";
    for (int i=0;i<inputFilenames.size();++i) {
      cout << inputFilenames[i] << "\n";
    }
  }

  if (vm.count("generate")) {
    string filename;

    if (!vm.count("inputfile")) {
      cout << "Input file not set, cannot generate sample set.\n";
      exit(1);
    }

    filename = vm["generate"].as<string>();
    cout << "generate sample filename: " << filename << "\n";
    return generateSample(inputFilenames[0], filename);
  }

  if (vm.count("probgen")) {
    string filename;

    if (!vm.count("inputfile")) {
      cout << "Input file not set, cannot generate sample set.\n";
      exit(1);
    }

    filename = vm["probgen"].as<string>();
    cout << "generate sample filename: " << filename << "\n";
    return generateProbSample(inputFilenames[0], filename);
  }


  if (vm.count("dump")) {
    string filename;

    filename = vm["dump"].as<string>();
    cout << "dump filename: " << filename << "\n";
    return dump4(filename);
  }

  bool dumpf=false;
  if (vm.count("dumpf")) {
    dumpf=true;
    string filename;

    filename = vm["dumpf"].as<string>();
    cout << "dump assoc folds filename: " << filename << "\n";
    return dumpItemsets(filename);
  }


  float minSup;
  if (vm.count("minsupport")) {
    minSup = vm["minsupport"].as<float>();
    cout << "Minimum support was set to " << vm["minsupport"].as<float>() << ".\n";
  } else {
    cout << "Minimum support was not set.\n";
    exit(1);
  }

//I really need to remove this
  float minConf;
  if (vm.count("minconfidence")) {
    cout << "Minimum confidence was set to " << vm["minconfidence"].as<float>() << ".\n";
    minConf = vm["minconfidence"].as<float>();
  } else {
    cout << "Minimum confidence was not set.\n";
    exit(1);
  }

  vector<string> targets;
  bool itrstAssoc=true;
  if (vm.count("target")) {
    itrstAssoc=false;

    cout << vm.count("target") << "\n";
    targets = vm["target"].as< vector<string> >();
    cout << "Target values set to:\n";
    for (int i=0;i<targets.size();++i) {
      cout << targets[i] << "\n";
    }
  }

  if (!vm.count("inputfile")) {
    cout << "Input file not set.\n";
    exit(1);
  }

  bool dumpe = false;
  if (vm.count("dumpe")) {
    dumpe = true;
  }

  double adCutoff = .97;
  if (vm.count("adCutoff")) {
    adCutoff = vm["adCutoff"].as<double>();
    cout << "Anderson Darling cutoff was set to " << vm["adCutoff"].as<double>() << ".\n";
  } else {
    cout << "Anderson Darling cutoff was defaulted to." << adCutoff << endl;
  }

  double adCutoffMax = .98;
  if (vm.count("adCutoffMax")) {
    adCutoffMax = vm["adCutoffMax"].as<double>();
    cout << "Anderson Darling cutoff max was set to " << vm["adCutoffMax"].as<double>() << ".\n";
  } else {
    cout << "Anderson Darling cutoff was defaulted to." << adCutoff << endl;
  }

  double binomCutoff = .5;
  if (vm.count("binomCutoff")) {
    binomCutoff = vm["binomCutoff"].as<double>();
    cout << "Binomial cutoff was set to " << vm["binomCutoff"].as<double>() << ".\n";
  } else {
    cout << "Binomial cutoff was defaulted to." << binomCutoff << endl;
  }

  generateAssociations(&inputFilenames, minSup, minConf, &targets, dumpe, binomCutoff);


}
