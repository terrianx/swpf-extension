#!/bin/bash
# Usage: sh run.sh <NAME_OF_PROGRAM>

PATH2LIB="../build/swprefetchpass/swprefetchpass.so"
PASS=swprefetchpass

# Check for correct num args
if [ "$#" -ne 1 ]; then
    echo "Usage: sh run.sh <NAME_OF_PROGRAM>"
    exit 2
fi

# Delete outputs from previous runs. Update this when you want to retain some files.
rm -f *_swpf *.bc *_output *_not_output *.ll

# Convert source code to bitcode (IR).
clang -emit-llvm -c ${1}.c -Xclang -disable-O0-optnone -o ${1}.bc
# Run pass on bitcode
opt -load-pass-plugin="${PATH2LIB}" -passes="${PASS}" ${1}.bc -o ${1}.swpf.bc > /dev/null

# Generate binary executable before swpf
clang ${1}.bc -o ${1}_swpf_not

# Generate binary executable after swpf
clang ${1}.swpf.bc -o ${1}_swpf

# Creates viz pdf
sh viz.sh ${1}

# Cleanup: Remove intermediate files
rm -f *.ll *.bc
# sh clean.sh
