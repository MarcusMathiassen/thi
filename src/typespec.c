#include "typespec.h"

#include "string.h"  // strf, append_string, string
#include "utility.h" // error
#include <assert.h>  // assert
#include <stdlib.h>  // xmalloc

//------------------------------------------------------------------------------
//                               typespec.c
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                               Public
//------------------------------------------------------------------------------

char* typespec_kind_to_str(Typespec_Kind kind)
{
    switch (kind) {
    case TYPESPEC_INT: return "TYPESPEC_INT";
    case TYPESPEC_FLOAT: return "TYPESPEC_FLOAT";
    case TYPESPEC_STRING: return "TYPESPEC_STRING";
    case TYPESPEC_POINTER: return "TYPESPEC_POINTER";
    case TYPESPEC_ARRAY: return "TYPESPEC_ARRAY";
    case TYPESPEC_ENUM: return "TYPESPEC_ENUM";
    case TYPESPEC_STRUCT: return "TYPESPEC_STRUCT";
    case TYPESPEC_FUNCTION: return "TYPESPEC_FUNCTION";
    default: error("not implemented typespec_kind_to_str kind %d", kind);
    }
    return "";
}

s64 get_size_of_underlying_typespec(Typespec* type)
{
    switch (type->kind) {
    case TYPESPEC_POINTER: return get_size_of_typespec(type->Pointer.pointee);
    case TYPESPEC_ARRAY: return get_size_of_typespec(type->Array.type);
    default: return get_size_of_typespec(type);
    }
    return 0;
}
s64 get_size_of_typespec(Typespec* type)
{
    switch (type->kind) {
    case TYPESPEC_INT: return type->Int.bytes;
    case TYPESPEC_FLOAT: return type->Float.bytes;
    case TYPESPEC_STRING: return type->String.len;
    case TYPESPEC_POINTER: return 8;
    case TYPESPEC_ARRAY: return get_size_of_typespec(type->Array.type) * type->Array.size;
    case TYPESPEC_STRUCT: {
        s64 accum_size = 0;
        LIST_FOREACH(type->Struct.members)
        {
            Arg* mem = (Arg*)it->data;
            accum_size += get_size_of_typespec(mem->type);
        }
        return accum_size;
    }
    case TYPESPEC_FUNCTION: {
        s64 accum_size = 0;
        LIST_FOREACH(type->Function.args)
        {
            Arg* arg = (Arg*)it->data;
            accum_size += get_size_of_typespec(arg->type);
        }
        return accum_size;
    }
    default: error("get_size_of_typespec kind %s not implemented.", typespec_kind_to_str(type->kind));
    }
    return 0;
}

s64 typespec_function_get_arg_count(Typespec* type)
{
    assert(type);
    assert(type->kind == TYPESPEC_FUNCTION);
    return type->Function.args->count;
}

s64 typespec_array_get_count(Typespec* type)
{
    assert(type);
    assert(type->kind == TYPESPEC_ARRAY);
    return type->Array.size;
}

char* typespec_to_str(Typespec* type)
{
    switch (type->kind) {
    case TYPESPEC_ARRAY: return strf("%s[%d]", typespec_to_str(type->Array.type), type->Array.size);
    case TYPESPEC_INT: return strf(type->Int.is_unsigned ? "u%d" : "s%d", type->Int.bytes * 8);
    case TYPESPEC_POINTER: return strf("%s*", typespec_to_str(type->Pointer.pointee));
    case TYPESPEC_FLOAT: return strf("f%d", type->Float.bytes * 8);
    case TYPESPEC_STRING: return strf("\"\", %d", type->String.len);
    case TYPESPEC_STRUCT: {
        string str = make_string(strf("%s :: {\n", type->Struct.name));
        LIST_FOREACH(type->Struct.members)
        {
            Arg* mem = (Arg*)it->data;
            append_string(&str, strf("%s: %s", mem->name, typespec_to_str(mem->type)));
        }
        append_string(&str, "}\n");
        return str.c_str;
    };
    case TYPESPEC_ENUM: {
        string str = make_string(strf("%s :: enum {", type->Enum.name));
        LIST_FOREACH(type->Enum.members)
        {
            char* mem = (char*)it->data;
            append_string(&str, strf("%s", mem));
        }
        return str.c_str;
    };

    case TYPESPEC_FUNCTION: {
        string str = make_string(strf("%s :: (", type->Function.name));
        strf("func. name: %d", type->Function.name);

        s64 arg_count = type->Function.args->count;
        s64 arg_index = 0;
        LIST_FOREACH(type->Function.args)
        {
            Arg* arg = (Arg*)it->data;
            if (arg->name)
                append_string(&str, strf("%s: %s", arg->name, typespec_to_str(arg->type)));
            else
                append_string(&str, strf("%s", typespec_to_str(arg->type)));

            if (arg_index != arg_count - 1) {
                append_string(&str, ", ");
            }
            arg_index += 1;
        }
        if (type->Function.ret_type) append_string(&str, strf(") -> %s", typespec_to_str(type->Function.ret_type)));
        return str.c_str;
    }
    default: warning("typespec_to_str not implemented kind %d", typespec_kind_to_str(type->kind));
    }
    return NULL;
}

//------------------------------------------------------------------------------
//                               Type Maker Functions
//------------------------------------------------------------------------------

Typespec* make_typespec(Typespec_Kind kind)
{
    Typespec* t = xmalloc(sizeof(Typespec));
    t->kind = kind;
    return t;
}

Typespec* make_typespec_array(Typespec* type, s32 size)
{
    assert(type);
    assert(size > 0);
    Typespec* t = make_typespec(TYPESPEC_ARRAY);
    t->Array.type = type;
    t->Array.size = size;
    return t;
}

Typespec* make_typespec_int(s8 bytes, bool is_unsigned)
{
    warning("%d", bytes);
    assert(bytes > 0 && bytes < 9);
    assert(is_unsigned == 1 || is_unsigned == 0);
    Typespec* t = make_typespec(TYPESPEC_INT);
    t->Int.bytes = bytes;
    t->Int.is_unsigned = is_unsigned;
    return t;
}

Typespec* make_typespec_float(s8 bytes)
{
    assert(bytes > 0 && bytes < 9);
    Typespec* t = make_typespec(TYPESPEC_FLOAT);
    t->Float.bytes = bytes;
    return t;
}

Typespec* make_typespec_string(s64 len)
{
    assert(len);
    Typespec* t = make_typespec(TYPESPEC_STRING);
    t->String.len = len;
    return t;
}

Typespec* make_typespec_pointer(Typespec* pointee)
{
    Typespec* t = make_typespec(TYPESPEC_POINTER);
    t->Pointer.pointee = pointee;
    return t;
}

Typespec* make_typespec_enum(char* name, List* members)
{
    assert(name);
    Typespec* t = make_typespec(TYPESPEC_ENUM);
    t->Enum.name = name;
    t->Enum.members = members;
    return t;
}

Typespec* make_typespec_struct(char* name, List* members)
{
    assert(name);
    Typespec* t = make_typespec(TYPESPEC_STRUCT);
    t->Struct.name = name;
    t->Struct.members = members;
    return t;
}

Typespec* make_typespec_function(char* name, List* args, Typespec* ret_type)
{
    assert(name);
    Typespec* t = make_typespec(TYPESPEC_FUNCTION);
    t->Function.name = name;
    t->Function.args = args;
    t->Function.ret_type = ret_type;
    return t;
}
