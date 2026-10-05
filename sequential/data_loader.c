#include <stdio.h>
#include <stdlib.h>
#include "data_loader.h"

int load_cities(const char *filename, City **cities)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error opening cities.csv");
        return -1;
    }

    char line[256];

    /* Skip the CSV header */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    int capacity = 1024;
    int count = 0;

    *cities = malloc(capacity * sizeof(City));

    if (*cities == NULL) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {

        City city;

        if (sscanf(line, "%lld,%lf,%lf",
                   &city.city_id,
                   &city.x,
                   &city.y) != 3) {
            continue;
        }

        if (count >= capacity) {
            capacity *= 2;

            City *temp = realloc(*cities,
                                  capacity * sizeof(City));

            if (temp == NULL) {
                free(*cities);
                fclose(file);
                return -1;
            }

            *cities = temp;
        }

        (*cities)[count] = city;
        count++;
    }

    fclose(file);

    return count;
}

void free_cities(City *cities)
{
    free(cities);
}