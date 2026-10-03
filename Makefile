CC = gcc
MPICC = mpicc
CFLAGS = -O3 -Wall
LIBS = -lm

all: tsp_mpi

tsp_mpi: tsp_mpi.c
	$(MPICC) $(CFLAGS) tsp_mpi.c -o tsp_mpi $(LIBS)

clean:
	rm -f tsp_mpi best_tour.csv
