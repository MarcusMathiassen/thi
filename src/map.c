#include "map.h"
#include "utility.h"
#include <assert.h>
#include <string.h>

#define INITIAL_SIZE 256

static u32 hash_it(const char *str);

void map_tests(void) {
  Map map;
  map_init(&map);
}

void map_init(Map *map) {
  assert(map != NULL);
  map->data = NULL;
  map->data = xcalloc(INITIAL_SIZE, sizeof(Map_Element));
  map->table_size = INITIAL_SIZE;
  map->size = 0;
}

// Add a pointer to the hashmap with some key
int map_set(Map *map, const char *key, any_t value) {
  assert(map);
  assert(key);

  u64 hash_val = hash_it(key);
  for (u64 i = 0; i < map->size; ++i) {
    if (map->data[i].key == hash_val) {
      return MAP_EXISTS;
    }
  }

  // Make sure there is space for it
  if (map->size >= map->table_size) {
    map->data = xrealloc(map->data, (map->table_size + INITIAL_SIZE) *
                                        sizeof(Map_Element));
  }

  map->data[map->size].key = hash_val;
  map->data[map->size++].data = value;
  return MAP_SUCCESS;
}

// Get your pointer out of the hashmap with a key
any_t map_get(Map *map, const char *key) {
  assert(map);
  assert(key);
  u64 hash_val = hash_it(key);
  for (u64 i = 0; i < map->size; ++i) {
    if (map->data[i].key == hash_val) {
      return map->data[i].data;
    }
  }

  return NULL;
}

//------------------------------------------------------------------------------
//                               Hash Function
//------------------------------------------------------------------------------

static u32 hash_it(const char *str) {
  assert(str);
  u32 hash = 5381;
  i32 c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}
