
#include <algorithm>
#include <parlay/parallel.h>
#include <parlay/sequence.h>

namespace parallel_mergesort {

    // Keep this equal to BASE_CASE_SIZE in sequential_mergesort.h.
    const int SEQUENTIAL_BASE_CASE_SIZE = 1000;

    // Stop creating parallel tasks below this size. The sequential fallback
    // still uses mergesort with the 1000-element std::sort base case.
    const int PARALLEL_BASE_CASE_SIZE = 16 * 1024;

    parlay::sequence<long> sort_base_case(const parlay::sequence<long> &array,
                                          int start, int end) {
        auto result = array.subseq(start, end);
        std::sort(result.begin(), result.end());
        return result;
    }

    parlay::sequence<long> sequential_merge(const parlay::sequence<long> &a,
                                            const parlay::sequence<long> &b) {
        parlay::sequence<long> result(a.size() + b.size());
        int ai = 0, bi = 0, ci = 0;
        while(ai < a.size() && bi < b.size()) {
            if(a[ai] < b[bi]) result[ci++] = a[ai++];
            else result[ci++] = b[bi++];
        }
        while(ai < a.size()) result[ci++] = a[ai++];
        while(bi < b.size()) result[ci++] = b[bi++];
        return result;
    }

    // The same sequential merge, but writing into an existing output array.
    // This version is used when no new temporary sequence should be allocated.
    void sequential_merge_into(const long *a, int a_size,
                               const long *b, int b_size, long *output) {
        int ai = 0, bi = 0, ci = 0;
        while(ai < a_size && bi < b_size) {
            if(a[ai] < b[bi]) output[ci++] = a[ai++];
            else output[ci++] = b[bi++];
        }
        while(ai < a_size) output[ci++] = a[ai++];
        while(bi < b_size) output[ci++] = b[bi++];
    }

    // Same sequential algorithm and base case as the optimized baseline.
    // Parallel algorithms call this below the task-scheduling cutoff.
    parlay::sequence<long> sequential_mergesort(
            const parlay::sequence<long> &array, int start, int end) {
        int n = end - start;
        if(n <= SEQUENTIAL_BASE_CASE_SIZE)
            return sort_base_case(array, start, end);

        int middle = start + n/2;
        return sequential_merge(
            sequential_mergesort(array, start, middle),
            sequential_mergesort(array, middle, end));
    }

    // Part 2: run the recursive sorts in parallel and merge sequentially.
    parlay::sequence<long> mergesort_helper(const parlay::sequence<long> &array,
                                            int start, int end) {
        int n = end - start;
        if(n <= PARALLEL_BASE_CASE_SIZE)
            return sequential_mergesort(array, start, end);

        int middle = start + n/2;
        parlay::sequence<long> left;
        parlay::sequence<long> right;

        parlay::parallel_do(
            [&] { left = mergesort_helper(array, start, middle); },
            [&] { right = mergesort_helper(array, middle, end); });

        return sequential_merge(left, right);
    }

    void mergesort(parlay::sequence<long> &array) {
        array = mergesort_helper(array, 0, array.size());
    }

    // Merge two sorted ranges in parallel. The larger range is split at its
    // median and lower_bound finds the corresponding split in the other range.
    void parallel_merge(const long *a, int a_size,
                        const long *b, int b_size, long *output) {
        if(a_size < b_size) {
            parallel_merge(b, b_size, a, a_size, output);
            return;
        }

        int total_size = a_size + b_size;
        if(total_size == 0) return;
        if(total_size <= PARALLEL_BASE_CASE_SIZE) {
            sequential_merge_into(a, a_size, b, b_size, output);
            return;
        }

        int a_middle = a_size/2;
        long pivot = a[a_middle];
        const long *b_middle_iterator = std::lower_bound(b, b + b_size, pivot);
        int b_middle = b_middle_iterator - b;
        int output_middle = a_middle + b_middle;

        parlay::parallel_do(
            [=] { parallel_merge(a, a_middle, b, b_middle, output); },
            [=] {
                parallel_merge(a + a_middle, a_size - a_middle,
                               b + b_middle, b_size - b_middle,
                               output + output_middle);
            });
    }

    // Part 3: run both the recursive sorts and merge in parallel.
    parlay::sequence<long> parallel_merge_sort_helper(
            const parlay::sequence<long> &array, int start, int end) {
        int n = end - start;
        if(n <= PARALLEL_BASE_CASE_SIZE)
            return sequential_mergesort(array, start, end);

        int middle = start + n/2;
        parlay::sequence<long> left;
        parlay::sequence<long> right;

        parlay::parallel_do(
            [&] { left = parallel_merge_sort_helper(array, start, middle); },
            [&] { right = parallel_merge_sort_helper(array, middle, end); });

        parlay::sequence<long> result(n);
        parallel_merge(left.data(), left.size(), right.data(), right.size(),
                       result.data());
        return result;
    }

    void parallel_merge_sort(parlay::sequence<long> &array) {
        array = parallel_merge_sort_helper(array, 0, array.size());
    }

    // Sequential fallback for the cache-efficient version. It performs the
    // same mergesort work without creating Parlay tasks.
    void cache_efficient_sequential_sort(long *source, long *destination,
                                         int start, int end) {
        int n = end - start;
        if(n <= SEQUENTIAL_BASE_CASE_SIZE) {
            std::copy(source + start, source + end, destination + start);
            std::sort(destination + start, destination + end);
            return;
        }

        int middle = start + n/2;
        cache_efficient_sequential_sort(destination, source, start, middle);
        cache_efficient_sequential_sort(destination, source, middle, end);
        sequential_merge_into(source + start, middle - start,
                              source + middle, end - middle,
                              destination + start);
    }

    // Part 4: alternate source and destination arrays at each recursion level
    // so that all recursive calls reuse the same temporary array.
    void cache_efficient_sort_helper(long *source, long *destination,
                                     int start, int end) {
        int n = end - start;
        if(n <= PARALLEL_BASE_CASE_SIZE) {
            cache_efficient_sequential_sort(source, destination, start, end);
            return;
        }

        int middle = start + n/2;
        parlay::parallel_do(
            [=] { cache_efficient_sort_helper(destination, source,
                                               start, middle); },
            [=] { cache_efficient_sort_helper(destination, source,
                                               middle, end); });

        parallel_merge(source + start, middle - start, source + middle,
                       end - middle, destination + start);
    }

    void cache_efficient_sort(parlay::sequence<long> &array) {
        if(array.size() <= SEQUENTIAL_BASE_CASE_SIZE) {
            std::sort(array.begin(), array.end());
            return;
        }

        parlay::sequence<long> temporary = array;
        cache_efficient_sort_helper(temporary.data(), array.data(),
                                    0, array.size());
    }
}
