#include <stdio.h>
#include "data_loader.h"

int main(void)
{
    City *cities = NULL;

    int n = load_cities("cities.csv", &cities);

    if (n < 0) {
        printf("Failed to load cities.\n");
        return 1;
    }

    printf("Loaded %d cities.\n\n", n);

    printf("First 5 cities:\n");

    for (int i = 0; i < 5 && i < n; i++) {
        printf("ID: %I64d, X: %.6f, Y: %.6f\n",
               cities[i].city_id,
               cities[i].x,
               cities[i].y);
    }

    printf("\nLast city:\n");

    printf("ID: %I64d, X: %.6f, Y: %.6f\n",
           cities[n - 1].city_id,
           cities[n - 1].x,
           cities[n - 1].y);

    free_cities(cities);

    return 0;
}