#ifndef DATA_LOADER_H
#define DATA_LOADER_H

typedef struct {
    long long city_id;
    double x;
    double y;
} City;

int load_cities(const char *filename, City **cities);

void free_cities(City *cities);

#endif