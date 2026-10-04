#ifndef NEAREST_NEIGHBOR_H
#define NEAREST_NEIGHBOR_H

#include "data_loader.h"

/* Build a TSP tour using the Nearest Neighbor heuristic */
int nearest_neighbor(const City *cities, int n, int *tour);

#endif