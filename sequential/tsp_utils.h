#ifndef TSP_UTILS_H
#define TSP_UTILS_H

#include "data_loader.h"

/* Calculate Euclidean distance between two cities */
double euclidean_distance(const City *a, const City *b);

/* Calculate total length of a closed TSP tour */
double calculate_tour_length(const City *cities, const int *tour, int n);

/* Check whether a tour visits every city exactly once */
int validate_tour(const int *tour, int n);

#endif