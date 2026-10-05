#ifndef TWO_OPT_H
#define TWO_OPT_H

#include "data_loader.h"

/* Improve an existing TSP tour using the 2-opt heuristic */
void two_opt(const City *cities, int n, int *tour);

#endif