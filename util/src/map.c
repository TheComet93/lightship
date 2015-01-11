#include "util/map.h"
#include "util/memory.h"

struct map_t*
map_create(void)
{
    struct map_t* map = (struct map_t*)MALLOC(sizeof(struct map_t));
    map_init_map(map);
    return map;
}

void
map_init_map(struct map_t* map)
{
    ordered_vector_init_vector(&map->vector, sizeof(struct map_key_value_t));
}

void
map_destroy(struct map_t* map)
{
    map_clear(map);
    FREE(map);
}
#include <stdio.h>
struct map_key_value_t*
map_find_lower_bound(struct map_t* map, intptr_t hash)
{
    /* algorithm taken from GNU GCC stdlibc++'s lower_bound function, line 2121 in stl_algo.h */
    /* https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a02014.html */
    intptr_t half;
    struct map_key_value_t* middle;
    struct map_key_value_t* data = (struct map_key_value_t*)map->vector.data;
    intptr_t len = map->vector.count;

    if(!len)
        return NULL;

    while(len > 0)
    {
        half = len >> 1;
        middle = data + half;
        if(middle->hash < hash)
        {
            data = middle;
            ++data;
            len = len - half - 1;
        }
        else
            len = half;
    }
    return data;
}

void*
map_find(struct map_t* map, intptr_t hash)
{
    struct map_key_value_t* data = map_find_lower_bound(map, hash);
    if(!data || data->hash != hash)
        return NULL;
    return data->value;
}

void
map_insert(struct map_t* map, intptr_t hash, void* value)
{
    struct map_key_value_t* data = map_find_lower_bound(map, hash);
    if(data && data->hash == hash)
        return;

    if(map->vector.count == 0)
        data = (struct map_key_value_t*)ordered_vector_push_emplace(&map->vector);
    else
        data = ordered_vector_insert_emplace(&map->vector,
                          data - (struct map_key_value_t*)map->vector.data);
    
    data->hash = hash;
    data->value = value;
}

void*
map_erase(struct map_t* map, intptr_t hash)
{
    void* value;
    struct map_key_value_t* data = map_find_lower_bound(map, hash);
    if(!data || data->hash != hash)
        return NULL;
    
    value = data->value;
    ordered_vector_erase_element(&map->vector, (DATA_POINTER_TYPE*)data);
    return value;
}

void
map_clear(struct map_t* map)
{
    ordered_vector_clear_free(&map->vector);
}

intptr_t
map_count(struct map_t* map)
{
    return map->vector.count;
}

#ifdef _DEBUG
void
map_print(struct map_t* map)
{
    int i = 0;
    {
        ORDERED_VECTOR_FOR_EACH(&map->vector, struct map_key_value_t, item)
        {
            printf("hash: %ld, value (ptr): %ld\n", item->hash, item->value);
            i++;
        }
        printf("items in map: %d\n", i);
    }
}
#endif