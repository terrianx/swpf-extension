#!/bin/bash
# Usage: sh perf.sh <NAME_OF_PROGRAM>

echo  "1. Performance of unoptimized code"
time ./${1}_swpf_not > /dev/null
echo "\n"
echo "2. Performance of optimized code"
time ./${1}_swpf > /dev/null
echo "\n"
