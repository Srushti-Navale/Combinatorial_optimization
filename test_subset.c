#include <stdio.h>
#include "subset.h"

int main(void)
{
    int total = 197769;

    printf("500 -> %d\n", get_subset_size(500, total));
    printf("1000 -> %d\n", get_subset_size(1000, total));
    printf("2000 -> %d\n", get_subset_size(2000, total));
    printf("200000 -> %d\n", get_subset_size(200000, total));
    printf("-10 -> %d\n", get_subset_size(-10, total));

    return 0;
}