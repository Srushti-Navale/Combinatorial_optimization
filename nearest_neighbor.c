#include <stdlib.h>

#include "nearest_neighbor.h"
#include "tsp_utils.h"

int nearest_neighbor(const City *cities, int n, int *tour)
{
    if (cities == NULL || tour == NULL || n <= 0) {
        return 0;
    }

    int *visited = calloc(n, sizeof(int));

    if (visited == NULL) {
        return 0;
    }

    /* Start from city index 0 */
    tour[0] = 0;
    visited[0] = 1;

    for (int i = 1; i < n; i++) {

        int current = tour[i - 1];
        int nearest = -1;
        double best_distance = 0.0;

        for (int j = 0; j < n; j++) {

            if (visited[j]) {
                continue;
            }

            double d = euclidean_distance(
                &cities[current],
                &cities[j]
            );

            if (nearest == -1 || d < best_distance) {
                nearest = j;
                best_distance = d;
            }
        }

        if (nearest == -1) {
            free(visited);
            return 0;
        }

        tour[i] = nearest;
        visited[nearest] = 1;
    }

    free(visited);

    return 1;
}