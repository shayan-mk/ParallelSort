#include <algorithm> // std::sort
#include <parlay/sequence.h>

namespace sequential_mergesort {

    const int BASE_CASE_SIZE = 1000;

    parlay::sequence<long> merge(parlay::sequence<long> a, parlay::sequence<long> b) {
        parlay::sequence<long> c(a.size() + b.size());
        int ai = 0, ci = 0, bi = 0;
        while(ai < a.size() && bi < b.size()){
            if(a[ai] < b[bi]) {
                c[ci] = a[ai]; ai++; ci++;
            } else {
                c[ci] = b[bi]; bi++; ci++;
            }
        }
        while(ai < a.size()) {
            c[ci] = a[ai]; ai++; ci++;
        }
        while(bi < b.size()) {
            c[ci] = b[bi]; bi++; ci++;
        }
        return c;
    }

    parlay::sequence<long> mergesort_helper(parlay::sequence<long> &array, int start, int end){
        int n = end-start;
        int m = start + n/2;
        // Part 1 base case
        if(n <= BASE_CASE_SIZE) {
            auto result = array.subseq(start, end);
            std::sort(result.begin(), result.end());
            return result;
        }
        else {
            return merge(mergesort_helper(array, start, m), mergesort_helper(array, m, end));
        }
    }

    void mergesort(parlay::sequence<long> &array) {
        array = mergesort_helper(array, 0, array.size());
        // std::cout << "first 10 elements: " << parlay::to_chars(array.head(10)) << std::endl;

    }
}

