## Run

To compile the compiler pass:
```
mkdir build && cmake ..
cd build
make
```

To run the pass and generate the prefetched code:
```
cd benchmarks
sh run.sh BENCHMARK_NAME
```
where BENCHMARK_NAME is one of:
- indir_double_small
- indir_double_big
- indir_triple_small
- indir_triple_big
- cor_indir_double
- cor_indir_triple
- fault_call

Note that this command also automatically generates the CFG with the inserted prefetch instructions as a pdf in the dot directory. Furthermore, it automatically creates a BENCHMARK_NAME.diff file containing the diff output comparing the optimized code output with the not optimized code output; this file being empty means that no diffs were found, meaning that the optimization does not affect correctness.

To compare performance stats:
```
cd benchmarks
sh perf.sh BENCHMARK_NAME
```

If there are permission issues at any time, try running:
```
chmod +x
```
