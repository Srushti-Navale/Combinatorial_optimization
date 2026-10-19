# Benchmarking and Performance Analysis

## 1. Objective

This analysis evaluates the execution time of the sequential and OpenMP implementations of the Traveling Salesperson Problem (TSP). It examines how runtime changes with input size, how the OpenMP implementation responds to different thread counts, and the trade-offs between execution time and tour quality.

## 2. Experimental Setup

* **Dataset:** Traveling Santa 2018 – Prime Paths, using subsets of 500, 1,000, 2,000, 5,000 and 8,000 cities.
* **Processor:** Intel Core i5-1334U.
* **Memory:** Approximately 16 GB RAM.
* **Environment:** Ubuntu under WSL2.
* **Compiler:** GCC 13.3.0.
* **OpenMP thread counts:** 4, 8 and 12.
* **Parallel search:** 24 starting points, with dynamic scheduling.

The full dataset contains 197,769 cities. The benchmark subset was capped at 8,000 cities for practical memory and runtime reasons. The parallel implementation precomputes an \(N \times N\) distance matrix, so its memory requirement grows quadratically with the number of cities.

## 3. Sequential Performance

The sequential implementation uses Nearest Neighbor to construct an initial tour and 2-Opt to improve it. Its execution time increases substantially as the number of cities grows.

At 500 cities, the recorded execution time was approximately 0.079 seconds. At 8,000 cities, it reached approximately 316.42 seconds.

This growth highlights the computational cost of local-search optimization on larger inputs. The sequential implementation uses one starting point, so its result also depends on the quality of that initial tour.

See `graphs/sequential_execution_time.png` and `results/sequential_results.csv`.

## 4. OpenMP Performance

The OpenMP implementation evaluates 24 starting points using multiple threads. The recorded runtime generally decreases as the input grows and more threads are used, although scaling is not uniform at every input size.

For 8,000 cities:

| Threads |  Execution time |
| ------: | --------------: |
|       4 | 26.3883 seconds |
|       8 | 19.6496 seconds |
|      12 | 17.2276 seconds |

Increasing the thread count from 4 to 12 reduced the measured search time by approximately 34.7% for this input.

At 500 cities, 12 threads were slower than 4 threads. For smaller workloads, thread-management overhead and other parallelization costs can outweigh the benefits of additional threads.

See `graphs/openmp_execution_time.png` and `results/openmp_results.csv`.

## 5. Thread Scaling

Thread scaling is measured relative to the 4-thread OpenMP run:

$$
S(P)=\frac{T_4}{T_P}
$$

$$
E(P)=\frac{S(P)}{P/4}
$$

Here, \(T_4\) is the runtime with 4 threads and \(T_P\) is the runtime with \(P\) threads. Efficiency is expressed as a percentage in the CSV file.

For 8,000 cities, the 12-thread run achieved approximately 1.532× speedup relative to the 4-thread run, with approximately 51.1% efficiency under this baseline definition.

These are **OpenMP thread-scaling measurements**, not conventional sequential-to-parallel speedup measurements. The 4-thread run is the baseline, so the results show the benefit of adding threads within the parallel implementation.

See `graphs/openmp_scaling_speedup.png` and `results/scaling_results.csv`.

## 6. Practical Sequential vs OpenMP Comparison

At 8,000 cities, the sequential implementation recorded 316.42 seconds for one start. The OpenMP implementation recorded 17.23 seconds for 24 starts with 12 threads.

This is a useful practical comparison, but it is **not a like-for-like speedup measurement**. The two implementations differ in search breadth and distance calculation strategy:

* The sequential version evaluates one start and computes distances on demand.
* The OpenMP version evaluates 24 starts and uses a precomputed distance matrix.
* The parallel timing excludes distance-matrix construction, while the sequential timing covers its documented search and tour-length calculation scope.

A 24-start sequential run was not measured because even one sequential start at 8,000 cities took more than five minutes. Therefore, the results should not be presented as a controlled comparison of the effect of OpenMP alone.

See `graphs/practical_comparison.png`.

## 7. Tour Quality

The parallel implementation found a shorter tour than the sequential implementation at each tested size. At 8,000 cities, the sequential final tour length was 254,469.54, compared with 253,147.21 for the OpenMP best tour — approximately 0.52% shorter.

This difference should be interpreted in the context of the search strategy. Evaluating 24 starting points increases the opportunity to find a better tour. The quality improvement cannot be attributed to parallel execution alone.

## 8. Performance Bottlenecks and Amdahl's Law

Amdahl's Law explains why increasing the number of threads does not produce unlimited speedup. Serial work, synchronization, memory access, scheduling overhead and other non-parallel portions constrain the gains available from additional threads.

The measured results demonstrate diminishing returns: for 8,000 cities, going from 4 to 8 threads saves approximately 6.74 seconds, while going from 8 to 12 threads saves approximately 2.42 seconds.

The small-input results also show that additional threads can increase execution time when parallel overhead is large relative to the useful work.

## 9. Limitations

1. Only the tested input sizes and thread counts are represented.
2. The sequential and parallel implementations do not perform an identical search workload.
3. The parallel runtime excludes matrix construction, which must be considered when interpreting the results.
4. The results reflect one hardware and software environment and may differ on other systems.
5. The 24-start sequential runtime was not measured and must not be presented as an experimental result.

## 10. Reproducing the Graphs

The CSV files in `results/` contain the recorded measurements. Run `scripts/plot_results.py` to regenerate the figures. This script only reads the CSV files and does not rerun the TSP benchmarks.

Install the required Python packages with `python -m pip install pandas matplotlib` if they are not already available.
