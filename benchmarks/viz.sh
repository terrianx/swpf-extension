#!/bin/bash
# Usage: sh viz.sh <NAME_OF_PROGRAM> [TYPE]
# TYPE should be one of: cfg, cfg-only, dom, dom-only, postdom, postdom-only.
# Default type is cfg.

BENCH=$1

# Default to cfg
VIZ_TYPE=${2:-cfg}

OUTPUT_DIR=$(realpath ./dot)  # will put .pdf file here
TMP_DIR=$OUTPUT_DIR/tmp       # will put .dot files here
BITCODE_DIR=$(pwd)            # run this script from the same dir as bitcode

mkdir -p $OUTPUT_DIR
mkdir -p $TMP_DIR
cd $TMP_DIR

BITCODE=$BITCODE_DIR/$BENCH.swpf.bc

# Generate .dot files in tmp dir
opt $PROF_FLAGS -passes="dot-$VIZ_TYPE" $BITCODE > /dev/null

# Combine .dot files into PDF
if [ $VIZ_TYPE = "cfg" ]; then
  DOT_FILES=$(ls .*.dot)
else
  DOT_FILES=$(ls *.dot)
fi
cat $DOT_FILES | dot -Tpdf > $OUTPUT_DIR/$BENCH.$VIZ_TYPE.pdf
echo "Created $BENCH.$VIZ_TYPE.pdf"

rm -rf $TMP_DIR
