#include <math.h>
#include <stdlib.h>

#include "tsp_utils.h"

double euclidean_distance(const City *a, const City *b)
{
    double dx = a->x - b->x;
    double dy = a->y - b->y;

    return sqrt(dx * dx + dy * dy);
}

double calculate_tour_length(const City *cities, const int *tour, int n)
{
    double total = 0.0;

    for (int i = 0; i < n - 1; i++) {
        total += euclidean_distance(
            &cities[tour[i]],
            &cities[tour[i + 1]]
        );
    }

    /* Return from the last city to the starting city */
    total += euclidean_distance(
        &cities[tour[n - 1]],
        &cities[tour[0]]
    );

    return total;
}

int validate_tour(const int *tour, int n)
{
    if (tour == NULL || n <= 0) {
        return 0;
    }

    int *visited = calloc(n, sizeof(int));

    if (visited == NULL) {
        return 0;
    }

    for (int i = 0; i < n; i++) {

        if (tour[i] < 0 || tour[i] >= n) {
            free(visited);
            return 0;
        }

        if (visited[tour[i]]) {
            free(visited);
            return 0;
        }

        visited[tour[i]] = 1;
    }

    free(visited);

    return 1;
}