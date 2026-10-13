#include<iostream>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>
#include <algorithm>    // std::sort

#include <sequential_mergesort.h> 
#include <parallel_mergesort.h> 

// **************************************************************
// Driver
// **************************************************************
int main(int argc, char* argv[]) {
  auto usage = "Usage: mergesort <n> <algname>";
  if (argc != 3) std::cout << usage << std::endl;
  else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { std::cout << usage << std::endl; return 1; }
    parlay::random_generator gen;
    std::uniform_int_distribution<long> dis(0, n-1);

    // generate random long values
    auto data = parlay::tabulate(n, [&] (long i) {
      auto r = gen[i];
      return dis(r);});

    std::cout << "first 10 elements: " << parlay::to_chars(data.head(10)) << std::endl;

    parlay::internal::timer t("Time");
    parlay::sequence<long> result;
    for (int i=0; i < 5; i++) {
      result = data;
      t.start();
      if(((std::string)argv[2]) == "seq")
        sequential_mergesort::mergesort(result);
      else if(((std::string)argv[2]) == "par")
        parallel_mergesort::mergesort(result);
      else if(((std::string)argv[2]) == "pmerge")
        parallel_mergesort::parallel_merge_sort(result);
      else if(((std::string)argv[2]) == "cache")
        parallel_mergesort::cache_efficient_sort(result);
      else if(((std::string)argv[2]) == "stdsort") {
        std::sort(result.begin(), result.end());
      }
      else {
        std::cout << "invalid algname: " << ((std::string)argv[2]) << std::endl;
        exit(1);
      }
      t.next("mergesort");
    }

    auto first_ten = result.head(10);

    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
  }
}
