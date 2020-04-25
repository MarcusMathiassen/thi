// Copyright (c) 2020 Marcus Mathiassen

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>

#define RESET     "\033[0m"
#define GRAY      "\033[30m"
#define RED       "\033[31m"
#define GREEN     "\033[32m"
#define YELLOW    "\033[33m"
#define RGB_GRAY  "\033[38;2;110;110;110;m"
#define RGB_WHITE "\033[38;2;255;255;255;m"

#define foreach(i, c)         for (s64 (i) = 0;   (i) < (c); ++(i))
#define foreach_reverse(i, c) for (s64 (i) = c-1; (i) >= 0;  --(i))

#define internal        static
#define global_variable static
#define local_persists  static

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;

#ifndef bool
#define bool u8
#endif

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define CONST_ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define BYTES(n) (n)
#define KILOBYTES(n) (BYTES(n) * 1024ULL)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ULL)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ULL)
#define TERABYTES(n) (GIGABYTES(n) * 1024ULL)

#define TO_NANOSECONDS(n)  (n)
#define TO_MICROSECONDS(n) ((u64)TO_NANOSECONDS(n) / 1000ULL)
#define TO_MILLISECONDS(n) ((u64)TO_MICROSECONDS(n) / 1000ULL)
#define TO_SECONDS(n)      ((u64)TO_MILLISECONDS(n) / 1000ULL)
#define TO_MINUTES(n)      ((u64)TO_SECONDS(n) / 60ULL)
#define TO_HOURS(n)        ((u64)TO_MINUTES(n) / 60ULL)

#define NANOSECONDS(n)  (n)
#define MICROSECONDS(n) ((u64)NANOSECONDS(n) * 1000ULL)
#define MILLISECONDS(n) ((u64)MICROSECONDS(n) * 1000ULL)
#define SECONDS(n)      ((u64)MILLISECONDS(n) * 1000ULL)
#define MINUTES(n)      ((u64)SECONDS(n) * 60ULL)
#define HOURS(n)        ((u64)MINUTES(n) * 60ULL)

#define DEFAULT_GIGABYTE_SUFFIX "gb"
#define DEFAULT_MEGABYTE_SUFFIX "mb"
#define DEFAULT_KILOBYTE_SUFFIX "kb"
#define DEFAULT_BYTE_SUFFIX     "b"

#define DEFAULT_HOURS_SUFFIX        "h"
#define DEFAULT_MINUTES_SUFFIX      "m"
#define DEFAULT_SECONDS_SUFFIX      "s"
#define DEFAULT_MILLISECONDS_SUFFIX "ms"
#define DEFAULT_MICROSECONDS_SUFFIX "us"
#define DEFAULT_NANOSECONDS_SUFFIX  "ns"

#if DEV
    #define ASSERT_KIND_IN_RANGE(ENUM, kind) tassert(0 <= kind && kind < _##ENUM##_count_, "kind = %d", kind)
    #define ERROR_UNHANDLED_KIND(str) default: error("[%s:%s:%d] Unhandled case '%s'", give_unique_color((char*)__FILE__), give_unique_color((char*)__func__), __LINE__, give_unique_color(str));
    #define UNREACHABLE error("[%s:%s:%d] %s", give_unique_color((char*)__FILE__), give_unique_color((char*)__func__), __LINE__, give_unique_color("UNREACHABLE"));
    #define UNFINISHED error("[%s:%s:%d] %s", give_unique_color((char*)__FILE__), give_unique_color((char*)__func__), __LINE__, give_unique_color("UNFINISHED"));
#else
    #define ASSERT_KIND_IN_RANGE(ENUM, kind)
    #define ERROR_UNHANDLED_KIND(str)
    #define UNREACHABLE
    #define UNFINISHED
#endif

#define FALLTHROUGH

#include "utility.c"
#include "list.c"
#include "string.c"
#include "lex.c"
#include "parse.c"
#include "codegen.c"

internal void assemble(char* asm_file, char* exec_name)
{
    string* comp_call = make_string_f("nasm -f macho64 %s.s -o %s.o", asm_file, exec_name);
    debug("Assembling with options '%s'", ucolor(string_data(comp_call)));
    system(string_data(comp_call));
}

internal void linking_stage(list_t* links, char* exec_name)
{
    string* link_call = make_string_f("ld -macosx_version_min 10.15 -no_pie -o %s %s.o -e _main", exec_name, exec_name);
    debug("Linking with options '%s'", ucolor(string_data(link_call)));
    system(string_data(link_call));
    // Cleanup object files
}

int main(int argc, char** argv)
{
    char* source_file = argv[1];
    
    char* ext = get_file_extension(source_file);
    char* dir = get_file_directory(source_file);
    char* name = get_file_name(source_file);
    char* exec_name = remove_file_extension(name);

    {
        char* source = get_file_content(source_file);
        token_t* tokens = lex(source);
        ast_t* ast = parse(tokens);
        char* code = to_x64(ast);

        if (code)
        {
            char* name_with_ext_removed = remove_file_extension(name);
            write_to_file(strf("%s.s", name_with_ext_removed), code);
            
            assemble(name_with_ext_removed, exec_name);
            linking_stage(make_list(), exec_name);
        }
    }
    return 0;
}
