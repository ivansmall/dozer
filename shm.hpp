
#ifndef _EOD_H
#define _EOD_H 1

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <math.h>
#include <float.h>
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
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/math/distributions/binomial.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/unordered_map.hpp>

#include "assoc.hpp"
#include "dozer.hpp"

template <class UIntType> class MyGen {
  boost::random::mt19937 gen;
  uint32_t *data;
  uint32_t selection;

  public:
  typedef UIntType result_type;

  MyGen() {
    FILE *fptr = fopen("random.dat", "r");
    data = new uint32_t[7000000];
    size_t fr = fread(data, sizeof(uint32_t), 7000000, fptr);
    if (fr != 7000000) {
      cout << "bad read: " << fr << endl;
    } else {
      cout << "good read: " << fr << endl;
    }
    selection = 0;
    fclose(fptr); 
  }
  void seed() { gen.seed(); }
  void seed(UIntType u) { gen.seed(u); }
  template<typename SeeqSeq> void seed(SeeqSeq &s) { gen.seed(s); }
  template<typename It> void seed(It &a, It b) { gen.seed(a,b); }
  result_type operator()() {
    //return gen();

    result_type r = data[selection%7000000];
    ++selection;
    return r;
  }
  template<typename Iter> void generate(Iter a, Iter b) {
    return gen.generate(a,b);
  }

  void discard(boost::uintmax_t u) { gen.discard(u); }

  static result_type min() { return boost::random::mt19937::min(); }
  static result_type max() { return boost::random::mt19937::max(); }
};

#endif /* shm.hpp*/

