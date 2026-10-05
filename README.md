# Combinatorial_optimization
Identify a suitable real-world combinatorial optimization problem and develop a computational solution capable of handling significantly larger problem instances than a basic sequential implementation.

# Parallel Traveling Salesperson Problem (TSP) Solver using OpenMP

A high-performance parallel implementation of the **Traveling Salesperson Problem (TSP)** in C using **OpenMP** shared-memory multi-threading.

The solver evaluates the **Kaggle Traveling Santa** dataset using a **Multi-Start Nearest Neighbor** constructive heuristic followed by **2-Opt local search** for tour optimization.

---

## 🚀 Algorithm Architecture

The solver consists of four major stages:

### 1. Euclidean Distance Matrix Precomputation

An `N × N` distance matrix is computed in parallel using:

```c
#pragma omp parallel for
```

This allows constant-time `O(1)` distance lookups during the optimization phase.

### 2. Multi-Start Nearest Neighbor

The solver constructs multiple tours using different starting cities.

Each starting city independently generates a route using the **Nearest Neighbor heuristic**, reducing the chance of getting stuck in a poor local solution.

### 3. 2-Opt Local Search

Each generated tour is improved using the **2-Opt optimization technique**.

2-Opt:

* Selects two edges from the tour.
* Removes the selected edges.
* Reverses the intermediate sub-path.
* Keeps the change if it reduces the total tour length.
* Continues until no further improvement is found or the maximum number of passes is reached.

### 4. Dynamic Work Distribution & Thread Safety

Starting cities are distributed dynamically among OpenMP threads using:

```c
#pragma omp for schedule(dynamic, 1)
```

When a thread finds a better tour, the global best solution is updated inside an OpenMP critical section:

```c
#pragma omp critical
```

This prevents race conditions while updating the shared best solution.

---

## 📁 Project Structure

```text
├── cities.csv          # Kaggle Traveling Santa coordinate dataset
├── data_loader.c       # CSV parsing utilities
├── data_loader.h       # City structure and function prototypes
├── nearest_neighbor.c  # Sequential Nearest Neighbor heuristic
├── nearest_neighbor.h  # Nearest Neighbor header
├── two_opt.c           # Sequential 2-Opt local search
├── two_opt.h           # 2-Opt header
├── tsp_utils.c         # Distance, tour length, and validation utilities
├── tsp_utils.h         # Utility function declarations
├── tsp_seq.c           # Sequential single-core baseline
├── tsp_omp.c           # Parallel OpenMP TSP solver
├── best_tour_omp.csv   # Generated best Hamiltonian cycle
└── Makefile            # Build configuration
```

---

## 🛠️ Prerequisites

The project requires:

* GCC compiler
* OpenMP
* Make (optional, if using the Makefile)
* Linux/Ubuntu environment

### Install Dependencies on Ubuntu

```bash
sudo apt update
sudo apt install build-essential gcc libomp-dev -y
```

Place the `cities.csv` dataset in the project directory.

---

## 🔨 Compilation

Compile the OpenMP solver using GCC with optimization:

```bash
gcc -O3 -march=native -fopenmp tsp_omp.c -o tsp_omp -lm
```

### Compiler Options

| Option          | Description                                  |
| --------------- | -------------------------------------------- |
| `-O3`           | Enables high-level compiler optimizations    |
| `-march=native` | Optimizes the executable for the current CPU |
| `-fopenmp`      | Enables OpenMP support                       |
| `-lm`           | Links the math library                       |

---

## ▶️ Execution

### Syntax

```bash
./tsp_omp [NUM_CITIES] [NUM_STARTS] [CSV_PATH]
```

### Arguments

| Argument     |      Default | Description                                                 |
| ------------ | -----------: | ----------------------------------------------------------- |
| `NUM_CITIES` |        `500` | Number of cities to process from the dataset                |
| `NUM_STARTS` |         `64` | Number of starting cities used for multi-start optimization |
| `CSV_PATH`   | `cities.csv` | Path to the CSV dataset                                     |

### Example

```bash
./tsp_omp 500 64 cities.csv
```

---

## ⚡ Thread Scaling & Benchmarking

The number of OpenMP threads can be controlled using the `OMP_NUM_THREADS` environment variable.

### 1 Thread

```bash
export OMP_NUM_THREADS=1
./tsp_omp 500 64
```

### 2 Threads

```bash
export OMP_NUM_THREADS=2
./tsp_omp 500 64
```

### 4 Threads

```bash
export OMP_NUM_THREADS=4
./tsp_omp 500 64
```

### 8 Threads

```bash
export OMP_NUM_THREADS=8
./tsp_omp 500 64
```

The execution times from different thread counts can be compared to evaluate the **parallel speedup** and **scalability** of the implementation.

---

## 📊 Performance Evaluation

The solver can be benchmarked using different numbers of OpenMP threads.

Important metrics include:

* **Execution Time**
* **Speedup**
* **Parallel Efficiency**
* **Best Tour Length**
* **Number of Threads**

### Speedup

Speedup can be calculated as:

```text
Speedup = Sequential Execution Time / Parallel Execution Time
```

### Parallel Efficiency

```text
Efficiency = Speedup / Number of Threads
```

---

## 📤 Output

After execution, the program generates:

```text
best_tour_omp.csv
```

The file contains the best Hamiltonian cycle found by the solver in the following format:

```csv
Step,CityId
0,123
1,45
2,78
...
```

The final city should return to the starting city, forming a **closed Hamiltonian cycle**.

---

## 🔍 Output Verification

### View the First Few Steps

```bash
head -n 6 best_tour_omp.csv
```

### View the End of the Tour

```bash
tail -n 2 best_tour_omp.csv
```

The final entry should correspond to the starting city, confirming that the route forms a closed tour.

---

## 🧠 Algorithms Used

| Algorithm          | Purpose                                     |
| ------------------ | ------------------------------------------- |
| Nearest Neighbor   | Constructs initial TSP tours                |
| Multi-Start Search | Explores multiple starting points           |
| 2-Opt              | Improves and optimizes generated tours      |
| OpenMP             | Parallelizes computation across CPU threads |
| Euclidean Distance | Calculates distance between cities          |

---

## ⏱️ Complexity Overview

For `N` cities:

### Distance Matrix

```text
Time:  O(N²)
Space: O(N²)
```

### Nearest Neighbor

```text
Time:  O(N²)
Space: O(N)
```

### 2-Opt

Worst-case complexity depends on the number of improvement passes and can approach:

```text
Time: O(N³)
```

### Parallelization

The multi-start tour construction and optimization are independent across starting cities, making them suitable for **shared-memory parallel execution using OpenMP**.

---

## 🔐 Thread Safety

The solver uses OpenMP synchronization to safely update the global best solution.

The critical section ensures that only one thread modifies the shared best tour at a time:

```c
#pragma omp critical
{
    // Update global best solution
}
```

This avoids race conditions between worker threads.

---

## 🎯 Project Goals

The main objectives of this project are to:

* Implement a practical TSP solver in C.
* Compare sequential and parallel execution.
* Apply OpenMP shared-memory parallelism.
* Improve solution quality using 2-Opt local search.
* Evaluate performance using different thread counts.
* Study parallel speedup and scalability.

---

## 📌 Notes

This implementation uses heuristic optimization rather than an exact TSP algorithm. Therefore, the generated tour is **not guaranteed to be globally optimal**.

The quality of the solution depends on:

* Number of starting cities.
* Nearest Neighbor construction.
* 2-Opt optimization.
* Number of optimization passes.
* Dataset characteristics.

---

## 👩‍💻 Technologies Used

* **C**
* **OpenMP**
* **GCC**
* **Linux/Ubuntu**
* **Kaggle Traveling Santa Dataset**

---

## 📄 License

This project is intended for academic and educational purposes.

