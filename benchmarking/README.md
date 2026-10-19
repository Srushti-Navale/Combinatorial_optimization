# Benchmarking

This directory contains the recorded benchmark data, performance visualizations, and analysis for the sequential and OpenMP TSP implementations.

## Directory Structure

* `results/` — CSV files containing sequential measurements, OpenMP measurements, and calculated thread-scaling metrics.
* `graphs/` — generated execution-time, scaling, and practical-comparison figures.
* `scripts/plot_results.py` — regenerates the graphs from the saved CSV files.
* `analysis.md` — methodology, interpretation of results, performance bottlenecks, and limitations.

## Regenerate the Graphs

From the repository root:

```bash
python -m pip install pandas matplotlib
python benchmarking/scripts/plot_results.py
```

The script uses the recorded CSV files. It does not execute the TSP solver or rerun the benchmarks.

## Interpretation Notes

* OpenMP thread scaling uses the 4-thread OpenMP run as its baseline.
* The sequential and parallel implementations differ in the number of starting points and distance-calculation strategy.
* The parallel timing excludes distance-matrix construction.
* Therefore, the sequential-versus-parallel chart is a practical comparison, not a controlled measurement of OpenMP speedup.

Refer to `analysis.md` for the full methodology and discussion.
