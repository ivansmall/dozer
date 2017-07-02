#ifndef _ASSOC_H
#define _ASSOC_H 1

#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

using namespace std;
using namespace boost;

class ntuple;
class assoc;
class itemset;
class targetitem;

typedef boost::interprocess::managed_shared_memory::segment_manager                       segment_manager_t;
typedef boost::interprocess::allocator<int, segment_manager_t>                            int_allocator;
typedef boost::interprocess::allocator<double, segment_manager_t>                         double_allocator;

typedef boost::interprocess::allocator<char, segment_manager_t>                           char_allocator;
typedef boost::interprocess::vector<int, int_allocator>                                   int_vector;
typedef boost::interprocess::vector<double, double_allocator>                             double_vector;

typedef boost::interprocess::basic_string<char, std::char_traits<char>, char_allocator>   char_string;
typedef boost::interprocess::allocator<char_string, segment_manager_t>                    string_allocator;
typedef boost::interprocess::allocator<void, segment_manager_t>                           void_allocator;

typedef boost::interprocess::allocator< itemset, boost::interprocess::managed_mapped_file::segment_manager>             vec_value_type_allocator;
typedef boost::interprocess::allocator< ntuple, boost::interprocess::managed_mapped_file::segment_manager>             ntuple_value_type_allocator;
typedef boost::interprocess::allocator< targetitem, boost::interprocess::managed_mapped_file::segment_manager>             tvec_value_type_allocator;



typedef boost::interprocess::vector< itemset, vec_value_type_allocator>  itemset_vec_type;
typedef boost::interprocess::vector< ntuple, ntuple_value_type_allocator>  ntuple_vec_type;
typedef boost::interprocess::vector< char_string, string_allocator>  item_map_type;
typedef boost::interprocess::vector< targetitem, tvec_value_type_allocator>  targetitem_vec_type;




class ntuple {
  public:

  int_vector tuple;

  ntuple(const void_allocator &void_alloc)
      : tuple(void_alloc) {}

  ntuple& operator=(const ntuple& i) {
    this->tuple = i.tuple;
    return *this;
  }
};

class assoc {
  int id;
  char_string name;
  int_vector itemIds;
  int support;
  float confidence;
  public:
  assoc(int pid, const char *pname, const void_allocator &void_alloc,
      const int psup, const float pconf)
      : id(pid), name(pname, void_alloc), itemIds(void_alloc), support(psup),
      confidence(pconf)
   {}
};

class ItemData {
  public:
//    ItemData() {}
    ItemData(int d, int lc, int dc, int p, int l, int c, int s) : dateOffset(d), labCount(lc), drugCount(dc), payDelay(p), lengthOfStay(l), charlsonIndex(c), supLos(s) {} 
    int dateOffset;
    int labCount;
    int drugCount;
    int payDelay;
    int lengthOfStay;
    int charlsonIndex;
    int supLos;

  ItemData& operator=(const ItemData& i) {
    this->dateOffset = i.dateOffset;
    this->labCount = i.labCount;
    this->drugCount = i.drugCount;
    this->payDelay = i.payDelay;
    this->lengthOfStay = i.lengthOfStay;
    this->charlsonIndex = i.charlsonIndex;
    this->supLos = i.supLos;

    return *this;
  }
};


class itemset {
  public:
  int_vector itemIds;
  double_vector ntuples;
  double_vector outVals;
  double_vector prevOutVals;
  int_vector outValsF;
  double_vector openVals;
  bool genOutput;
  bool includeInDistCalc;
  int dateOffset;
  
  itemset(const void_allocator &void_alloc, bool go,
         bool idc, int dateOffset)
      : itemIds(void_alloc), ntuples(void_alloc), outVals(void_alloc),
        prevOutVals(void_alloc), outValsF(void_alloc), openVals(void_alloc) {
    genOutput = go;
    includeInDistCalc = idc;
    this->dateOffset = dateOffset;
  }

  itemset& operator=(const itemset& i) {
    this->itemIds = i.itemIds;
    this->ntuples = i.ntuples;
    this->outVals = i.outVals;
    this->prevOutVals = i.prevOutVals;
    this->outValsF = i.outValsF;
    this->genOutput = i.genOutput;
    this->includeInDistCalc = i.includeInDistCalc;
    this->dateOffset = i.dateOffset;
    this->openVals = i.openVals;
    return *this;
  }
};

class targetitem {
  public:
  double_vector normProbs;
  char_string name;
  int_vector cutoffSizes;
  double_vector cutoffs;
  double_vector cutoffsNeg;

  double testOutput;
  double targetVal;

  targetitem(const void_allocator &void_alloc)
      : normProbs(void_alloc), name(void_alloc), cutoffSizes(void_alloc),
        cutoffs(void_alloc), cutoffsNeg(void_alloc) {}

  targetitem& operator=(const targetitem& i) {
    this->normProbs = i.normProbs;
    this->name = i.name;
    this->cutoffSizes = i.cutoffSizes;
    this->cutoffs = i.cutoffs;
    this->cutoffsNeg = i.cutoffsNeg;

    this->testOutput = i.testOutput;
    return *this;
  }


};


/*
  int_vector itemTimeOffsets;

  int_vector labCount;
  int_vector drugCount;
  int_vector payDelay;
  int_vector lengthOfStay;
  int_vector charlsonIndex;
  int_vector supLos;

  itemset(const void_allocator &void_alloc)
      : itemIds(void_alloc), itemTimeOffsets(void_alloc),
        labCount(void_alloc), drugCount(void_alloc),
        payDelay(void_alloc), lengthOfStay(void_alloc),
        charlsonIndex(void_alloc), supLos(void_alloc)
   {}
  itemset& operator=(const itemset& i) {
    this->itemIds = i.itemIds;
    this->itemTimeOffsets = i.itemTimeOffsets;
    this->labCount = i.labCount;
    this->drugCount = i.drugCount;
    this->payDelay = i.payDelay;
    this->lengthOfStay = i.lengthOfStay;
    this->charlsonIndex = i.charlsonIndex;
    this->supLos = i.supLos;

    return *this;

  }

};
*/


//typedef boost::interprocess::allocator< char_string, boost::interprocess::managed_mapped_file::segment_manager>   map_value_type_allocator;

//typedef boost::interprocess::map< char_string, char_string, std::less<char_string>, string_allocator>  item_adjust_type;

#endif /* assoc.hpp*/
