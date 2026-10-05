#include <stdlib.h>

#include "two_opt.h"
#include "tsp_utils.h"

static void reverse_section(int *tour, int start, int end)
{
    while (start < end) {
        int temp = tour[start];
        tour[start] = tour[end];
        tour[end] = temp;

        start++;
        end--;
    }
}

void two_opt(const City *cities, int n, int *tour)
{
    if (cities == NULL || tour == NULL || n < 4) {
        return;
    }

    int improved = 1;

    while (improved) {

        improved = 0;

        for (int i = 1; i < n - 1; i++) {

            for (int j = i + 2; j < n; j++) {

                int a = tour[i - 1];
                int b = tour[i];
                int c = tour[j];

                int d;

                if (j + 1 < n) {
                    d = tour[j + 1];
                } else {
                    d = tour[0];
                }

                /*
                 * Only two edges change during a 2-opt swap.
                 *
                 * Old:
                 * A -> B
                 * C -> D
                 *
                 * New:
                 * A -> C
                 * B -> D
                 */

                double old_distance =
                    euclidean_distance(&cities[a], &cities[b]) +
                    euclidean_distance(&cities[c], &cities[d]);

                double new_distance =
                    euclidean_distance(&cities[a], &cities[c]) +
                    euclidean_distance(&cities[b], &cities[d]);

                if (new_distance < old_distance - 1e-9) {

                    reverse_section(tour, i, j);

                    improved = 1;
                    break;
                }
            }

            if (improved) {
                break;
            }
        }
    }
}