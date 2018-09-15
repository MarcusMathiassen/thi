#include "utility.h"

#include <stdarg.h>  // va_list, va_start, va_end
#include <stdio.h>   // printf, vprintf
#include <stdlib.h>  // malloc
#include <assert.h>  // assert
#include <string.h>  // memcpy
#include "stretchy_buffer.h"  // sb_free
#include <sys/time.h> // gettimeofday, timeval

//------------------------------------------------------------------------------
//                               Printing Functions
//------------------------------------------------------------------------------

#define RESET "\033[0m"
#define GRAY "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"

void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(GRAY);
    vprintf(fmt, args);
    printf(RESET "\n");
    va_end(args);
}

void warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(YELLOW);
    vprintf(fmt, args);
    printf(RESET "\n");
    va_end(args);
}

void success(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(GREEN);
    vprintf(fmt, args);
    printf(RESET "\n");
    va_end(args);
}

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf(RED);
    vprintf(fmt, args);
    printf(RESET "\n");
    va_end(args);
    exit(1);
}

//------------------------------------------------------------------------------
//                               File Utility Functions
//------------------------------------------------------------------------------

char* get_file_path_from_dir(const char* dir, const char* filename)
{
    char strbuf[1000];
    u64 d_len = strlen(dir);
    u64 f_len = strlen(filename);
    u64 len = d_len + f_len;
    assert(len < 1000);
    memcpy(strbuf, dir, d_len); // copy dir into strbuf
    memcpy(strbuf+d_len, filename, f_len); // append filename
    char* str = malloc(len);
    if (!str) error("malloc failed");
    memcpy(str, strbuf, len); 
    str[len] = 0;
    return str;
}

char* get_file_ext(const char* filename)
{
    u64 len = strlen(filename);
    u64 i = 0;
    while (filename[len-(++i)] != '.') continue;
    ++len; // skip the '.'
    char* str = malloc(i);
    if (!str) error("malloc failed");
    memcpy(str, filename+len-i, i);
    str[i] = 0;
    return str;
}

char* get_file_dir(const char* filename)
{
    u64 len = strlen(filename);
#ifdef _WIN32
    while (filename[--len] != '\\') continue;
#else
    while (filename[--len] != '/') continue;
#endif
    ++len; // we preserve the '/'
    char* str = malloc(len);
    if (!str) error("malloc failed");
    memcpy(str, filename, len);
    str[len] = 0;
    return str;
}
char* get_file_name(const char* filename)
{
    u64 len = strlen(filename);
    u64 i = 0;
#ifdef _WIN32
    while (filename[len-(++i)] != '\\') continue;
#else
    while (filename[len-(++i)] != '/') continue;
#endif
    ++len; // skip the '/'
    char* str = malloc(i);
    if (!str) error("malloc failed");
    memcpy(str, filename+len-i, i);
    str[len] = 0;
    return str;

}
void write_to_file(const char* filename, const char* buffer) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Error opening file!\n");
        exit(1);
    }
    fputs(buffer, f);
    fclose(f);
}

char* get_file_content(const char* filename) {
    char* buffer = NULL;
    int string_size, read_size;
    FILE* handler = fopen(filename, "r");

    if (handler) {
        // Seek the last byte of the file
        fseek(handler, 0, SEEK_END);
        // Offset from the first to the last byte, or in other words, filesize
        string_size = ftell(handler);
        // go back to the start of the file
        rewind(handler);

        // Allocate a string that can hold it all
        buffer = (char*)malloc(sizeof(char) * (string_size + 1));

        // Read it all in one operation
        read_size = fread(buffer, sizeof(char), string_size, handler);

        // fread doesn't set it so put a \0 in the last position
        // and buffer is now officially a string
        buffer[string_size] = '\0';

        if (string_size != read_size) {
            // Something went wrong, throw away the memory and set
            // the buffer to NULL
            free(buffer);
            buffer = NULL;
        }

        // Always remember to close the file.
        fclose(handler);
    }

    return buffer;
}

//------------------------------------------------------------------------------
//                               General Purpose
//------------------------------------------------------------------------------
void* xmalloc(u64 bytes) {
    void* alloc = malloc(bytes);
    if (!alloc) error("alloc failed");
    return alloc;
}
void* xcalloc(u64 size, u64 bytes) {
    void* alloc = calloc(size, bytes);
    if (!alloc) error("calloc failed");
    return alloc;
}
void* xrealloc(void* ptr, u64 bytes) {
    void* alloc = realloc(ptr, bytes);
    if (!alloc) error("realloc failed");
    return alloc;
}
//------------------------------------------------------------------------------
//                               Timing Utility Functions
//------------------------------------------------------------------------------

typedef struct StackNode StackNode;
struct StackNode
{ 
    Timer data; 
    StackNode* next; 
}; 
  
StackNode* newNode(Timer data) 
{ 
    StackNode* stackNode = (StackNode*) malloc(sizeof(StackNode)); 
    stackNode->data = data; 
    stackNode->next = NULL; 
    return stackNode; 
} 
  
void push_stack(StackNode** root, Timer data) 
{ 
    StackNode* stackNode = newNode(data); 
    stackNode->next = *root; 
    *root = stackNode;
} 
  
Timer pop_stack(StackNode** root) 
{ 
    StackNode* temp = *root; 
    *root = (*root)->next; 
    Timer popped = temp->data; 
    free(temp); 
    return popped; 
} 
  
Timer peek(StackNode* root) 
{ 
    return root->data; 
} 

u64 get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    u64 ret = tv.tv_usec;
    ret /= 1000;
    ret += (tv.tv_sec * 1000);
    return ret;
}

static StackNode* timer_stack = NULL;
static Timer* timers = NULL;

Timer* get_timers() {
    return timers;
}
void push_timer(const char* desc) {
    Timer tm;
    tm.ms = get_time();
    tm.desc = desc;
    push_stack(&timer_stack, tm);
}
void pop_timer() {
    Timer tm = pop_stack(&timer_stack);
    tm.ms = get_time() - tm.ms;
    sb_push(timers, tm);
}

//------------------------------------------------------------------------------
//                               Tests
//------------------------------------------------------------------------------
void utility_tests()
{
    // get_file_dir
    assert(strcmp(get_file_dir("./b/m.thi"), "./b/") == 0);
    assert(strcmp(get_file_dir("./b/m/m.thi"), "./b/m/") == 0);

    // get_file_ext
    assert(strcmp(get_file_ext("./b/m.thi"), "thi") == 0);
    assert(strcmp(get_file_ext("./b/m.thigegerg/o.hrifj"), "hrifj") == 0);

    // get_file_name
    assert(strcmp(get_file_name("./b/m.thi"), "m.thi") == 0);    
    assert(strcmp(get_file_name("./b/m/hergergerg.thi"), "hergergerg.thi") == 0);    

    // get_file_path_from_dir
    assert(strcmp(get_file_path_from_dir("./b/", "test.thi"), "./b/test.thi") == 0);
    assert(strcmp(get_file_path_from_dir("./b/b/", "test.thi"), "./b/b/test.thi") == 0);
}