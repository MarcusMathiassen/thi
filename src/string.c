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
//                               string.c
//------------------------------------------------------------------------------

#include "string.h"
#include "constants.h"

#include "typedefs.h"
#include "utility.h" // xmalloc, xrealloc
  // assert
#include <stdarg.h>  // va_list, va_start, va_end
#include <stdio.h>   // vsnprintf
#include <stdlib.h>  // free
#include <string.h>  // memcpy

string* string_create(char* str) {
    xassert(str);
    string* s = xmalloc(sizeof(string));
    s->len = 0;
    s->cap = STRING_STARTING_ALLOC;
    s->c_str = xmalloc(s->cap);
    string_append(s, str);
    return s;
}
string* string_create_f(char* fmt, ...) {
    xassert(fmt);
    va_list args;
    va_start(args, fmt);
    s64 n = 1 + vsnprintf(0, 0, fmt, args);
    va_end(args);
    char* str = xmalloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    string* s = string_create(str);
    free(str);
    return s;
}
char* string_data(string* this) {
    xassert(this);
    xassert(this->c_str);
    return this->c_str;
}

void string_append(string* this, char* str) {
    xassert(this);
    if (!str) return;
    s64 str_len = xstrlen(str);
    if (str_len == 0) return;
    xassert(this->len <= this->cap);
    while (this->len + str_len >= this->cap) {
        this->cap *= 2;
        this->c_str = xrealloc(this->c_str, this->cap * sizeof(*this->c_str));
    }
    memcpy(this->c_str + this->len, str, str_len);
    this->len += str_len;
    this->c_str[this->len] = 0;
}

void string_append_f(string* this, char* fmt, ...) {
    xassert(this);
    xassert(fmt);
    va_list args;
    va_start(args, fmt);
    s64 n = 1 + vsnprintf(0, 0, fmt, args);
    va_end(args);
    char* str = xmalloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    string_append(this, str);
    free(str);
}
void string_destroy(string* this) {
    xassert(this);
    free(this->c_str);
    free(this);
}

//------------------------------------------------------------------------------
//                               Tests
//------------------------------------------------------------------------------

void string_tests(void) {
    // string test
    string* s = string_create("Hello");
    xassert(s->len == 5);
    xassert(strcmp(string_data(s), "Hello") == 0);
    string_append(s, ", Marcus Mathiasssen.");
    xassert(s->len == 26);
    xassert(strcmp(string_data(s), "Hello, Marcus Mathiasssen.") == 0);
    string_append(s, " It's nice to see you again. How are you?");
    xassert(s->len == 67);
    xassert(strcmp(string_data(s),
                  "Hello, Marcus Mathiasssen. It's nice to see you "
                  "again. How are you?") == 0);
}
