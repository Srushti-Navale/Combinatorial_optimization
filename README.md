# Combinatorial_optimization
Identify a suitable real-world combinatorial optimization problem and develop a computational solution capable of handling significantly larger problem instances than a basic sequential implementation.

# Parallel & Sequential Traveling Salesperson Problem (TSP) Solvers

A high-performance C implementation and benchmarking suite for the **Traveling Salesperson Problem (TSP)** evaluated on the **Kaggle Traveling Santa** dataset (`cities.csv`).

This repository provides both a modular **sequential baseline** and a **multi-threaded parallel solver** utilizing **OpenMP shared-memory parallelism**.

---

## Repository Architecture

```
Combinatorial_optimization/
├── parallel/
│   └── tsp_omp.c          # Multi-start OpenMP parallel solver with precomputed distance matrix
│
├── sequential/
│   ├── data_loader.c      # CSV parsing utilities
│   ├── data_loader.h      # City coordinate structure and prototypes
│   ├── nearest_neighbor.c # Single-start greedy constructive heuristic
│   ├── nearest_neighbor.h # Nearest Neighbor header
│   ├── subset.c           # Dataset subset bound validation
│   ├── subset.h           # Subset prototypes
│   ├── tsp_seq.c          # Sequential baseline driver
│   ├── tsp_utils.c        # Distance computation and tour validation
│   ├── tsp_utils.h        # TSP utilities header
│   ├── two_opt.c          # On-the-fly 2-Opt local search
│   └── two_opt.h          # 2-Opt header
│
├── tests/
│   ├── test_loader.c      # Dataset loading unit tests
│   ├── test_nn.c          # Nearest Neighbor verification tests
│   ├── test_subset.c      # Problem size boundary tests
│   ├── test_two_opt.c     # 2-Opt swap unit tests
│   └── test_utils.c       # Distance and Hamiltonian cycle validation tests
│
├── Makefile               # Modular build configuration
├── .gitignore             # Build artifact and binary exclusions
└── README.md              # Project documentation
```
Algorithmic Details & Methodology

Compares two approaches for solving Euclidean TSP instances.

1. Sequential Baseline (sequential/)
Construction: Uses the classic greedy Nearest Neighbor (NN) heuristic starting from city index 0. At each step, the nearest unvisited city based on Euclidean distance is selected.
Local Optimization: Uses 2-Opt local search to improve the initial tour. It searches for pairs of edges (A → B) and (C → D) and reverses the intermediate tour section when the new edges produce a shorter route.
Memory & Computation Model: Euclidean distances are calculated on demand using:
d = √((x₂ - x₁)² + (y₂ - y₁)²)
Starting Point: The current sequential implementation starts from city index 0.

2. Parallel OpenMP Solver (parallel/)
Distance Matrix Precomputation: All pairwise Euclidean distances are precomputed into an N × N distance matrix. This replaces repeated distance calculations with O(1) distance lookups during optimization.
Parallel Distance Computation: The distance matrix is computed in parallel using OpenMP:
#pragma omp parallel for
Multi-Start Search: Multiple starting cities are evaluated independently. Each starting city generates a Nearest Neighbor tour followed by 2-Opt optimization.
Dynamic Work Balancing: Work is dynamically distributed across OpenMP threads using:
#pragma omp for schedule(dynamic, 1)

This allows threads that finish earlier to take the next available starting city.

Thread-Safe Best Solution: When a thread discovers a better tour, the global best solution is updated inside an OpenMP critical section:
#pragma omp critical

This prevents race conditions while updating the global best tour.

Prerequisites & Setup

The project requires:

GCC
OpenMP
Make
Linux/Ubuntu

Install the required packages on Ubuntu:

sudo apt update
sudo apt install build-essential gcc libomp-dev make -y

Make sure the cities.csv dataset is available in the project root directory:
```
Combinatorial_optimization/
├── cities.csv
├── parallel/
├── sequential/
├── tests/
├── Makefile
└── README.md
```
Compilation & Execution
1. Sequential Solver (tsp_seq)

The sequential solver requires only the number of cities N as a command-line argument.

Compilation

From the project root:
```
gcc -O3 -Isequential sequential/*.c -o tsp_seq -lm
```
Execution

Run the solver by specifying the number of cities:
```
./tsp_seq 500
```
For example, the above command processes the first 500 cities from cities.csv.

Sequential Solver Parameters
Parameter	Description
NUM_CITIES	Number of cities to process

The current sequential implementation:

Accepts only NUM_CITIES.
Uses cities.csv as the dataset.
Starts the Nearest Neighbor tour from city index 0.
Does not use a NUM_STARTS parameter.
Does not accept the CSV path as a command-line argument.
Output

The sequential solver generates:

best_tour.csv
2. Parallel OpenMP Solver (tsp_omp)

The parallel solver uses OpenMP and supports multiple starting points.

Compilation

From the project root:
```
gcc -O3 -march=native -fopenmp parallel/tsp_omp.c -o tsp_omp -lm
```
Execution Syntax
```
./tsp_omp [NUM_CITIES] [NUM_STARTS] [CSV_PATH]
```
Parameters
Parameter	Default	Description
NUM_CITIES	500	Number of cities to evaluate
NUM_STARTS	64	Number of starting cities used for multi-start search
CSV_PATH	cities.csv	Path to the dataset


Example
```
./tsp_omp 500 64 cities.csv
```
OpenMP Thread Scaling

The number of OpenMP threads can be controlled using the OMP_NUM_THREADS environment variable.

1 Thread
```
export OMP_NUM_THREADS=1
./tsp_omp 500 64 cities.csv
```
2 Threads
```
export OMP_NUM_THREADS=2
./tsp_omp 500 64 cities.csv
```
4 Threads
```
export OMP_NUM_THREADS=4
./tsp_omp 500 64 cities.csv
```
8 Threads
```
export OMP_NUM_THREADS=8
./tsp_omp 500 64 cities.csv
```
These configurations can be used to measure the strong scaling of the parallel implementation.

