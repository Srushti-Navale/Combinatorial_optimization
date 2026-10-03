# Combinatorial_optimization
Identify a suitable real-world combinatorial optimization problem and develop a computational solution capable of handling significantly larger problem instances than a basic sequential implementation.

#1. Environment Setup  
Prerequisites  
Install the OpenMPI development libraries and standard build toolchain:
```
sudo apt update && sudo apt install -y build-essential openmpi-bin libopenmpi-dev
```

Dataset Placement  
Ensure cities.csv is placed in the project root directory alongside the source files:
```
Combinatorial_optimization/
├── cities.csv        # Kaggle dataset coordinates (CityId,X,Y)
├── tsp_mpi.c         # Parallel MPI solver implementation
├── Makefile          # Build configuration
└── README.md
```
2. Compilation
compile directly via mpicc:
```
mpicc -O3 -Wall tsp_mpi.c -o tsp_mpi -lm
```
3. Execution Syntax
Launch the parallel solver using mpirun:
```
mpirun --allow-run-as-root --oversubscribe -np <NUM_PROCESSES> ./tsp_mpi <NUM_CITIES>
```
Parameter Reference  
-np <NUM_PROCESSES>: Specifies the total number of MPI worker processes ($P \in \{1, 2, 4, 8\}$).  
<NUM_CITIES>: The number of cities to extract from cities.csv (e.g., 500, 1000, 2000).  
--allow-run-as-root: Required if running inside a containerized root environment.  
--oversubscribe: Allows process counts exceeding physical CPU core availability.  

4. Benchmark Execution Commands
Run the following test configurations to collect scaling metrics for Person C's performance analysis:
```
# 1 Process (Baseline)
mpirun --allow-run-as-root --oversubscribe -np 1 ./tsp_mpi 1000

# 2 Processes
mpirun --allow-run-as-root --oversubscribe -np 2 ./tsp_mpi 1000

# 4 Processes
mpirun --allow-run-as-root --oversubscribe -np 4 ./tsp_mpi 1000

# 8 Processes
mpirun --allow-run-as-root --oversubscribe -np 8 ./tsp_mpi 1000
```
5. Output Deliverables
Terminal Scorecard:Live iteration log reporting which process ranks found improved local routes.
Execution scorecard summarizing total wall-clock time, lowest tour cost, and the winning process rank.
Optimal Path File (best_tour.csv):Formatted two-column CSV (Step,CityId) tracing the complete, verified Hamiltonian cycle from origin city back to origin.
