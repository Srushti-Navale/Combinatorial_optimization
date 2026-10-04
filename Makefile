CC = gcc
MPICC = mpicc
CFLAGS = -O3 -Wall
LIBS = -lm

SEQ_SOURCES = tsp_seq.c data_loader.c subset.c tsp_utils.c nearest_neighbor.c two_opt.c

all: tsp_seq tsp_mpi
	
tsp_seq: $(SEQ_SOURCES)
	$(CC) $(CFLAGS) $(SEQ_SOURCES) -o tsp_seq $(LIBS)

tsp_mpi: tsp_mpi.c
	$(MPICC) $(CFLAGS) tsp_mpi.c -o tsp_mpi $(LIBS)

clean:
	rm -f tsp_seq tsp_mpi best_tour.csv