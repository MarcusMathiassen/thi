#include "type.h"

#include <assert.h>   // assert
#include <stdlib.h>   // xmalloc
#include <string.h>   // strcmp
#include "ast.h"      // AST*
#include "globals.h"  // TYPE_LIST_STARTING_ALLOC
#include "string.h"   // strf, append_string, string
#include "utility.h"  // error

//------------------------------------------------------------------------------
//                               type.c
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                               Public
//------------------------------------------------------------------------------

char* type_kind_to_str(Type_Kind kind) {
    switch (kind) {
        case TYPE_INT: return "TYPE_INT";
        case TYPE_FLOAT: return "TYPE_FLOAT";
        case TYPE_STRING: return "TYPE_STRING";
        case TYPE_POINTER: return "TYPE_POINTER";
        case TYPE_ARRAY: return "TYPE_ARRAY";
        case TYPE_ENUM: return "TYPE_ENUM";
        case TYPE_STRUCT: return "TYPE_STRUCT";
        case TYPE_FUNCTION: return "TYPE_FUNCTION";
        default: warning("not implemented type_kind_to_str kind %d", kind);
    }
    return "";
}

s64 get_size_of_underlying_type(Type* type) {
    switch (type->kind) {
        case TYPE_POINTER: return get_size_of_type(type->Pointer.pointee);
        case TYPE_ARRAY: return get_size_of_type(type->Array.type);
        default: return get_size_of_type(type);
    }
    return 0;
}
s64 get_size_of_type(Type* type) {
    switch (type->kind) {
        case TYPE_INT: return type->Int.bytes;
        case TYPE_FLOAT: return type->Float.bytes;
        case TYPE_STRING: return type->String.len;
        case TYPE_POINTER: return 8;
        case TYPE_ARRAY: return get_size_of_type(type->Array.type) * type->Array.size;
        case TYPE_STRUCT: {
            s64 accum_size = 0;
            if (type->Struct.members) {
                LIST_FOREACH(type->Struct.members) {
                    AST* mem = (AST*)it->data;
                    if (mem->kind != AST_FUNCTION) accum_size += get_size_of_type(mem->Variable_Decl.type);
                }
            }
            return accum_size;
        }
        case TYPE_ENUM: {
            s64 accum_size = 0;
            if (type->Enum.members) {
                LIST_FOREACH(type->Enum.members) { accum_size += 8; }
            }
            return accum_size;
        }
        case TYPE_FUNCTION: {
            s64 accum_size = 0;
            LIST_FOREACH(type->Function.args) {
                AST* arg = (AST*)it->data;
                accum_size += get_size_of_type(arg->Variable_Decl.type);
            }
            return accum_size;
        }
        default: warning("get_size_of_type kind %s not implemented.", type_kind_to_str(type->kind));
    }
    return 0;
}

s64 get_offset_in_struct_to_field(Type* type, char* name) {
    assert(type);
    assert(type->kind == TYPE_STRUCT);
    s64 accum_size = 0;
    LIST_FOREACH(type->Struct.members) {
        AST* mem = (AST*)it->data;
        if (strcmp(name, mem->Variable_Decl.name) == 0) {
            return accum_size;
        }
        accum_size += get_size_of_type(mem->Variable_Decl.type);
    }
    error("cant find field: %s", name);
    return -1;
}

s64 type_function_get_arg_count(Type* type) {
    assert(type);
    assert(type->kind == TYPE_FUNCTION);
    return type->Function.args->count;
}

s64 type_array_get_count(Type* type) {
    assert(type);
    assert(type->kind == TYPE_ARRAY);
    return type->Array.size;
}

char* type_to_str(Type* type) {
    if (!type) return "\"\"";
    switch (type->kind) {
        case TYPE_ARRAY: return strf("%s[%d]", type_to_str(type->Array.type), type->Array.size);
        case TYPE_INT: return strf(type->Int.is_unsigned ? "u%d" : "s%d", type->Int.bytes * 8);
        case TYPE_POINTER: return strf("%s*", type_to_str(type->Pointer.pointee));
        case TYPE_FLOAT: return strf("f%d", type->Float.bytes * 8);
        case TYPE_STRING: return strf("\"\", %d", type->String.len);
        case TYPE_STRUCT: {
            string str = make_string_f("type %s {", type->Struct.name);
            if (type->Struct.members) {
                s64 count = type->Struct.members->count;
                s64 index = 0;
                LIST_FOREACH(type->Struct.members) {
                    AST* mem = (AST*)it->data;
                    append_string_f(&str, "%s", ast_to_str(mem));
                    if (index != count - 1) {
                        append_string(&str, " ");
                    }
                    index += 1;
                }
            }
            append_string(&str, "}");
            return str.c_str;
        };

        case TYPE_ENUM: {
            char* s = strf("%s\n", type->Enum.name);
            LIST_FOREACH(type->Enum.members) {
                AST* mem = (AST*)it->data;
                // info("%s %s", mem->Constant_Decl.name, ast_to_str(mem->Constant_Decl.value));
                s = strf("%s\t%s", s, strf("%s %s\n", mem->Constant_Decl.name, ast_to_str(mem->Constant_Decl.value)));
                // append_string_f(&str, "%s %s\n", mem->Constant_Decl.name, ast_to_str(mem->Constant_Decl.value));
            }
            return s;
        };

        case TYPE_FUNCTION: {
            string str       = make_string(type->Function.name);
            s64    arg_count = type->Function.args->count;
            s64    arg_index = 0;
            append_string(&str, "(");
            LIST_FOREACH(type->Function.args) {
                append_string(&str, ast_to_str(it->data));
                if (arg_index != arg_count - 1) {
                    append_string(&str, ", ");
                }
                arg_index += 1;
            }
            append_string(&str, ")");
            if (type->Function.ret_type) append_string_f(&str, " -> %s", type_to_str(type->Function.ret_type));
            return str.c_str;
        }
        default: warning("type_to_str not implemented kind %d", type_kind_to_str(type->kind));
    }
    return NULL;
}

//------------------------------------------------------------------------------
//                               Type Maker Functions
//------------------------------------------------------------------------------

Type* make_type(Type_Kind kind) {
    Type* t = xmalloc(sizeof(Type));
    t->kind = kind;
    return t;
}

Type* make_type_array(Type* type, s32 size) {
    assert(type);
    assert(size > 0);
    Type* t       = make_type(TYPE_ARRAY);
    t->Array.type = type;
    t->Array.size = size;
    return t;
}

Type* make_type_int(s8 bytes, bool is_unsigned) {
    assert(bytes > 0 && bytes < 9);
    assert(is_unsigned == 1 || is_unsigned == 0);
    Type* t            = make_type(TYPE_INT);
    t->Int.bytes       = bytes;
    t->Int.is_unsigned = is_unsigned;
    return t;
}

Type* make_type_float(s8 bytes) {
    assert(bytes > 0 && bytes < 9);
    Type* t        = make_type(TYPE_FLOAT);
    t->Float.bytes = bytes;
    return t;
}

Type* make_type_string(s64 len) {
    assert(len);
    Type* t       = make_type(TYPE_STRING);
    t->String.len = len;
    return t;
}

Type* make_type_pointer(Type* pointee) {
    Type* t            = make_type(TYPE_POINTER);
    t->Pointer.pointee = pointee;
    return t;
}

Type* make_type_enum(char* name, List* members) {
    assert(name);
    Type* t         = make_type(TYPE_ENUM);
    t->Enum.name    = name;
    t->Enum.members = members;
    return t;
}

Type* make_type_struct(char* name, List* members) {
    assert(name);
    Type* t           = make_type(TYPE_STRUCT);
    t->Struct.name    = name;
    t->Struct.members = members;
    return t;
}

Type* make_type_function(char* name, List* args, Type* ret_type) {
    assert(name);
    Type* t              = make_type(TYPE_FUNCTION);
    t->Function.name     = name;
    t->Function.args     = args;
    t->Function.ret_type = ret_type;
    return t;
}