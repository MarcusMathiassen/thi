#ifndef TYPESPEC_H
#define TYPESPEC_H

#include "typedefs.h"

//------------------------------------------------------------------------------
//                               typespec.h
//------------------------------------------------------------------------------

typedef struct Typespec Typespec;
typedef struct Arg Arg;
typedef enum Typespec_Kind Typespec_Kind;

u64 get_size_of_typespec(Typespec* Typespec);

void print_type(Typespec* Typespec);

Typespec* make_typespec_int(i8 bits, bool is_unsigned);
Typespec* make_typespec_float(i8 bits);
Typespec* make_typespec_pointer(Typespec* pointee);
Typespec* make_typespec_struct(const char* name, Arg* members);
Typespec* make_typespec_func(const char* name, Arg* args, Typespec* ret_type);

enum Typespec_Kind
{
    TYPESPEC_INT,
    TYPESPEC_FLOAT,
    TYPESPEC_POINTER,

    TYPESPEC_ENUM,
    TYPESPEC_STRUCT,
    TYPESPEC_FUNC,
};

struct Arg
{
    const char* name;
    Typespec* type;
};

struct Typespec
{
    Typespec_Kind kind;
    union
    {
        struct { i8 bits; i8 is_unsigned;                           } Int;
        struct { i8 bits;                                           } Float;
        struct { Typespec* pointee;                                 } Pointer;
        struct { const char* name; Arg* members;                    } Struct;
        struct { const char* name; Arg* args; Typespec* ret_type;   } Func;
    };
};

#endif