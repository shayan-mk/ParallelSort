#!/usr/bin/env bash

set -euo pipefail

threads="${1:-32}"
n=20000000
log_file="experiment_${threads}_workers.log"

make
: > "$log_file"

average_time() {
    algorithm="$1"
    workers="$2"
    output="$(PARLAY_NUM_THREADS="$workers" ./mergesort "$n" "$algorithm")"
    {
        echo "algorithm=$algorithm workers=$workers"
        echo "$output"
        echo
    } >> "$log_file"
    awk '/Time: mergesort:/ { total += $3; count++ }
         END { printf "%.6f", total / count }' <<< "$output"
}

ratio() {
    awk -v numerator="$1" -v denominator="$2" \
        'BEGIN { printf "%.3f", numerator / denominator }'
}

seq_time="$(average_time seq 1)"

printf '\nFive-trial mean, n=%d, max workers=%d\n' "$n" "$threads"
printf '\n%-10s %14s %14s %14s %14s\n' \
    algorithm one_worker_s max_worker_s versus_seq self_speedup
printf '%-10s %14s %14s %14s %14s\n' \
    seq "$seq_time" N/A 1.000 N/A

for algorithm in par pmerge cache; do
    one_time="$(average_time "$algorithm" 1)"
    max_time="$(average_time "$algorithm" "$threads")"
    printf '%-10s %14s %14s %14s %14s\n' \
        "$algorithm" "$one_time" "$max_time" \
        "$(ratio "$seq_time" "$one_time")" \
        "$(ratio "$one_time" "$max_time")"
done

printf '\nRaw trial output saved to %s\n' "$log_file"
