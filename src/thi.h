#ifndef THI_H
#define THI_H

#include "ast.h"
#include "list.h"
#include "map.h"
#include "stack.h"
#include "string.h"
#include "type.h"
#include "typedefs.h"

typedef struct {
    f64   ms;
    char* desc;
} Timer;

typedef struct {
    bool detailed_print;
    bool debug_mode;
    bool enable_constant_folding;
    bool optimize;

    s64 lines;
    s64 comments;

    List* ast_int_passes;

    Type_Ref_List unresolved_types;
    AST_Ref_List  calls;
    AST_Ref_List  variables_in_need_of_type_inference;
    AST_Ref_List  constants;
    AST_Ref_List  identifiers;
    AST_Ref_List  structs;
    AST_Ref_List  enums;
    AST_Ref_List  field_access;
    AST_Ref_List  externs;
    AST_Ref_List  subscripts;
    List*         loads;
    List*         links;

    List*  ast;
    Map*   symbol_map;
    Map*   macro_map;
    Stack* timer_stack;
    List*  timer_list;

    string output_name;
    string source_file;
    char*  input_file;
    char*  previous_file;
    string current_directory;
} Thi;

Thi   make_thi();
void  add_load(Thi* thi, char* loaded_file);
void  add_link(Thi* thi, char* library_name);
Type* add_symbol(Thi* thi, char* name, Type* type);
Type* get_symbol(Thi* thi, char* name);
void  set_current_directory(Thi* thi, char* dir_name);
char* get_current_directory(Thi* thi);
List* get_load_list(Thi* thi);
List* get_link_list(Thi* thi);
char* get_output_name(Thi* thi);
void  set_output_name(Thi* thi, char* name);
void  set_source_file(Thi* thi, char* file_name);
char* get_source_file(Thi* thi);
char* get_previous_source_file(Thi* thi);
void  print_symbol_map(Thi* thi);
void  push_timer(Thi* thi, char* desc);
void  pop_timer(Thi* thi);
List* get_timers(Thi* thi);
#endif
