#ifndef SUBSET_H
#define SUBSET_H

#include "data_loader.h"

/*
 * Returns the number of cities that can be used.
 * If requested_n is larger than the dataset size,
 * the full dataset size is returned.
 */
int get_subset_size(int requested_n, int total_cities);

#endif