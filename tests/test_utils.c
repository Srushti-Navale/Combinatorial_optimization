#include <stdio.h>
#include "tsp_utils.h"

int main(void)
{
    City cities[4] = {
        {0, 0.0, 0.0},
        {1, 3.0, 0.0},
        {2, 3.0, 4.0},
        {3, 0.0, 4.0}
    };

    int tour[] = {0, 1, 2, 3};

    printf("Tour valid: %s\n",
           validate_tour(tour, 4) ? "YES" : "NO");

    printf("Tour length: %.2f\n",
           calculate_tour_length(cities, tour, 4));

    return 0;
}