CC = gcc
CFLAGS = -O3 -Wall -march=native
OMPFLAGS = -fopenmp
LIBS = -lm

SEQ_SOURCES = sequential/tsp_seq.c \
              sequential/data_loader.c \
              sequential/subset.c \
              sequential/tsp_utils.c \
              sequential/nearest_neighbor.c \
              sequential/two_opt.c

OMP_SOURCES = parallel/tsp_omp.c

all: tsp_seq tsp_omp

tsp_seq: $(SEQ_SOURCES)
	$(CC) $(CFLAGS) -Isequential $(SEQ_SOURCES) -o tsp_seq $(LIBS)

tsp_omp: $(OMP_SOURCES)
	$(CC) $(CFLAGS) $(OMPFLAGS) $(OMP_SOURCES) -o tsp_omp $(LIBS)

clean:
	rm -f tsp_seq tsp_omp *.o best_tour*.csv
