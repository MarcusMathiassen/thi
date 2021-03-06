// Copyright (c) 2019 Marcus Mathiassen

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

//------------------------------------------------------------------------------
//                               map.c
//------------------------------------------------------------------------------

#include "map.h"
#include "utility.h"
#include <string.h> // memset
#include <stdlib.h>

/**
    * when the map should reallocate more space
    * 
    * range [0.0, 1.0]
*/
#define MAP_DEEMED_FULL_PERCENTAGE 0.75
#define MAP_TABLE_GROWTH_RATE 2

typedef struct
{
    s32 id;
    float val;
} Test_Type;

void map_tests(void) {

    Map* map = make_map();

    Test_Type t1;
    t1.id = 0;
    t1.val = 3.43f;

    Test_Type t2;
    t2.id = 1;
    t2.val = 6.41f;

    map_set(map, "t1", &t1);
    map_set(map, "t2", &t2);
    map_set(map, "t3", &t2);
    map_set(map, "t4", &t2);
    map_set(map, "t5", &t2);
    map_set(map, "t6", &t2);
    map_set(map, "t7", &t2);
    map_set(map, "t8", &t2);
    map_set(map, "t9", &t2);
    map_set(map, "t10", &t2);
    map_set(map, "t11", &t2);
    map_set(map, "t12", &t2);
    map_set(map, "t13", &t2);

    xassert(((Test_Type*)map_get(map, "t1"))->val == 3.43f);
    xassert(((Test_Type*)map_get(map, "t2"))->val == 6.41f);
}


// djb2
static u32 hash(char* str) {
    xassert(str);
    u32 hash = 5381;
    s32 c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static inline Map_Element* find_slot_with_key(Map* map, char* key) {
    xassert(map && key);
    u32 index = hash(key);
    Map_Element* probe;
    while ((probe = &map->elements[index++ % map->table_size])->key)
        if (strcmp(key, probe->key) == 0)
            break;
    return probe;
}

static inline Map_Element* find_empty_slot(Map* map, char* key) {
    xassert(map && key);
    u32 index = hash(key);
    Map_Element* probe;
    while ((probe = &map->elements[index++ % map->table_size])->key);
    return probe;
}

Map* make_map_with_initial_size(s64 initial_size) {
    tassert(initial_size > 0, "initial_size = %lld", initial_size);
    Map* map = xmalloc(sizeof(Map));
    map->count = 0;
    map->table_size = initial_size;
    map->elements = xcalloc(map->table_size, sizeof(Map_Element));
    return map;
}

Map* make_map() {
    return make_map_with_initial_size(DEFAULT_MAP_STARTING_TABLE_SIZE);
}

void map_destroy(Map* map) {
    free(map->elements);
    free(map);
    map = NULL;
}

s64 map_count(Map* map) {
    return map->count;
}

inline static void map_increase_table_size_and_rehash(Map* map, s64 new_table_size) {
    Map_Element* nelements = xcalloc(new_table_size, sizeof(Map_Element));
    map_foreach(map) {
        u32 index = hash(it->key);        
        Map_Element* probe;
        while ((probe = &nelements[index++ % new_table_size])->key);
        probe->key = it->key;
        probe->value = it->value;
    }
    free(map->elements);
    map->elements = nelements;
    map->table_size = new_table_size;
}

void* map_set(Map* map, char* key, void* value) {
    xassert(map && key && value);
    if ((float)map->count / map->table_size > MAP_DEEMED_FULL_PERCENTAGE) {
        map_increase_table_size_and_rehash(map, map->table_size * MAP_TABLE_GROWTH_RATE);
    }
    Map_Element* slot = find_empty_slot(map, key);
    slot->key = key;
    slot->value = value;
    ++map->count;
    return value;
}

void* map_get(Map* map, char* key) {
    xassert(map && key);
    Map_Element* slot = find_slot_with_key(map, key);
    return slot->value;
}
