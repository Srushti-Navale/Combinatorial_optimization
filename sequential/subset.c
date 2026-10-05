#include "subset.h"

int get_subset_size(int requested_n, int total_cities)
{
    if (requested_n <= 0) {
        return 0;
    }

    if (requested_n > total_cities) {
        return total_cities;
    }

    return requested_n;
}