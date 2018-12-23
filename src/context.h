#ifndef CONTEXT_H
#define CONTEXT_H

#include "stack.h"
#include "typedefs.h"
#include "value.h" // Value

typedef struct Context Context;

// The context holds information about the current state of the codegen phase.
struct Context
{
    Value* current_function;

    Stack stack;

    u64 stack_index;

    // Label information
    u64 total_label_counter;
    u64 current_label_counter;

    const char* temp_label0;
    const char* temp_label1;

    const char* label_continue_to;
    const char* label_break_to;
};

void ctx_tests(void);
void ctx_init(Context* ctx);
void ctx_push_label(Context* ctx);
void ctx_pop_label(Context* ctx);
void ctx_set_continue_label(Context* ctx, const char* label);
void ctx_set_break_label(Context* ctx, const char* label);
const char* ctx_get_unique_label(Context* ctx);
#endif
