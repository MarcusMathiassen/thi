#include "globals.h"

#include "utility.h" // warning, error

static Type_Map* symbol_map;
static Type_Map* builtin_type_map;

void init_maps()
{
    builtin_type_map = NULL;
    symbol_map = NULL;
    symbol_map = (Map*)xmalloc(sizeof(Map));
    builtin_type_map = (Map*)xmalloc(sizeof(Map));
    map_init(symbol_map);
    map_init(builtin_type_map);
}

void print_symbol_map()
{
    int count = symbol_map->size;
    info("symbol_map count: %d", count);
    for (int i = 0; i < count; ++i)
        print_type(symbol_map->data[i].data);
}

bool is_builtin_type(const char* name)
{
    if (map_get(builtin_type_map, name))
        return true;
    return false;
}
void add_builtin_type(const char* name, Type* type)
{
    if (map_set(builtin_type_map, name, type) == MAP_EXISTS)  {
        warning("type redecl: '%s'", name);
    }
    info("added builtin type: %s", name);
}

Type* get_builtin_type(const char* name)
{
    Type* type = map_get(builtin_type_map, name);
    if (!type) {
        error("no type with name '%s'", name);
    }
    return type;
}

void add_symbol(const char* name, Type* type)
{
    if (map_set(symbol_map, name, type) == MAP_EXISTS)  {
        warning("symbol redecl: '%s'", name);
    }
    info("added symbol: %s", name);
}

Type* get_symbol(const char* name)
{
    Type* type = map_get(symbol_map, name);
    if (!type) {
        error("no symbol with name '%s'", name);
    }
    return type;
}