from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"
GRAPHS = ROOT / "graphs"
GRAPHS.mkdir(parents=True, exist_ok=True)

sequential = pd.read_csv(RESULTS / "sequential_results.csv")
openmp = pd.read_csv(RESULTS / "openmp_results.csv")
scaling = pd.read_csv(RESULTS / "scaling_results.csv")

# 1. Sequential execution time
plt.figure()
plt.plot(
    sequential["cities"],
    sequential["execution_time_seconds"],
    marker="o",
)
plt.xlabel("Number of cities")
plt.ylabel("Execution time (seconds)")
plt.title("Sequential TSP Execution Time")
plt.grid(True)
plt.tight_layout()
plt.savefig(GRAPHS / "sequential_execution_time.png", dpi=200)
plt.close()

# 2. OpenMP execution time by thread count
plt.figure()
for threads, group in openmp.groupby("threads"):
    group = group.sort_values("cities")
    plt.plot(
        group["cities"],
        group["execution_time_seconds"],
        marker="o",
        label=f"{threads} threads",
    )
plt.xlabel("Number of cities")
plt.ylabel("Execution time (seconds)")
plt.title("OpenMP TSP Execution Time")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(GRAPHS / "openmp_execution_time.png", dpi=200)
plt.close()

# 3. OpenMP scaling relative to four threads
plt.figure()
for threads in sorted(scaling["threads"].unique()):
    if threads == 4:
        continue
    group = scaling[scaling["threads"] == threads].sort_values("cities")
    plt.plot(
        group["cities"],
        group["speedup_vs_4_threads"],
        marker="o",
        label=f"{threads} threads",
    )

plt.axhline(1, linestyle="--")
plt.xlabel("Number of cities")
plt.ylabel("Speedup relative to 4 threads")
plt.title("OpenMP Thread Scaling")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(GRAPHS / "openmp_scaling_speedup.png", dpi=200)
plt.close()

# 4. Practical comparison: sequential 1 start vs OpenMP 24 starts
parallel_12 = openmp[openmp["threads"] == 12]
comparison = sequential.merge(
    parallel_12[["cities", "execution_time_seconds"]],
    on="cities",
    suffixes=("_sequential", "_openmp_12_threads"),
)

plt.figure()
plt.plot(
    comparison["cities"],
    comparison["execution_time_seconds_sequential"],
    marker="o",
    label="Sequential (1 start)",
)
plt.plot(
    comparison["cities"],
    comparison["execution_time_seconds_openmp_12_threads"],
    marker="o",
    label="OpenMP (24 starts, 12 threads)",
)
plt.xlabel("Number of cities")
plt.ylabel("Reported execution time (seconds)")
plt.title("Practical Sequential vs OpenMP Comparison")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(GRAPHS / "practical_comparison.png", dpi=200)
plt.close()

print(f"Graphs saved to: {GRAPHS}")