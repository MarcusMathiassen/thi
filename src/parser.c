#include "parser.h"
#include <assert.h>    // assert
#include <ctype.h>     // atoll
#include <stdarg.h>    // va_list, va_start, va_end
#include <stdio.h>     // printf, vprintf
#include <stdlib.h>    // xmalloc
#include <string.h>    // strcmp
#include "globals.h"   // add_symbol
#include "lexer.h"     // Token, Token_Kind, generate_tokens_from_source, token_array_get_info_of
#include "typedefs.h"  // s32 , s64, etc.
#include "typespec.h"  // Typespec, make_typspec_*,
#include "utility.h"   // info, error, warning, success, strf, get_file_content

//------------------------------------------------------------------------------
//                              parser.c
//------------------------------------------------------------------------------

#define DEBUG_START info("%s: %s", __func__, token_to_str(pctx->curr_tok));

#define BIN_OP_COUNT 41
struct {
    Token_Kind kind;
    s32        p;
} binop_precedence[BIN_OP_COUNT] = {
    {TOKEN_OPEN_BRACKET, 100},  // []
    {TOKEN_DOT, 100},           // .
    {TOKEN_OPEN_PAREN, 100},    // ()
    {TOKEN_RIGHT_ARROW, 100},   // ->
    {TOKEN_BANG, 85},           // !
    {TOKEN_TILDE, 85},          // ~
    {THI_SYNTAX_POINTER, 85},   // *
    {THI_SYNTAX_ADDRESS, 85},   // &
    {TOKEN_ASTERISK, 80},       // *
    {TOKEN_FWSLASH, 80},        // /
    {TOKEN_PERCENT, 80},        // %
    {TOKEN_MINUS, 70},          // -
    {TOKEN_PLUS, 70},           // +
    {TOKEN_LT_LT, 65},          // <<
    {TOKEN_GT_GT, 65},          // >>
    {TOKEN_LT, 60},             // <
    {TOKEN_GT, 60},             // >
    {TOKEN_GT_EQ, 60},          // >=
    {TOKEN_LT_EQ, 60},          // <=
    {TOKEN_EQ_EQ, 50},          // ==
    {TOKEN_BANG_EQ, 50},        // !=
    {TOKEN_AND_AND, 48},        // &&
    {TOKEN_PIPE_PIPE, 47},      // ||
    {TOKEN_AT, 40},             // @
    {TOKEN_AND, 40},            // &
    {TOKEN_HAT, 30},            // ^
    {TOKEN_PIPE, 20},           // |
    {TOKEN_QUESTION_MARK, 15},  // ?
    {TOKEN_COLON, 15},          // :
    {TOKEN_EQ, 10},             // =
    {TOKEN_PLUS_EQ, 10},        // +=
    {TOKEN_MINUS_EQ, 10},       // -=
    {TOKEN_ASTERISK_EQ, 10},    // *=
    {TOKEN_FWSLASH_EQ, 10},     // /=
    {TOKEN_PERCENT_EQ, 10},     // %=
    {TOKEN_AND_EQ, 10},         // &=
    {TOKEN_HAT_EQ, 10},         // ^=
    {TOKEN_PIPE_EQ, 10},        // |=
    {TOKEN_LT_LT_EQ, 10},       // <<=
    {TOKEN_GT_GT_EQ, 10},       // >>=
};

//------------------------------------------------------------------------------
//                               Parser
//------------------------------------------------------------------------------

typedef struct {
    Token_Array token_array;
    List*       ast;
    s64         token_index;
    Token       curr_tok;
    Token       prev_tok;
    Token_Kind  top_tok_kind;
    char*       source_file;
    char*       source;

    bool o_current_function_should_return_a_value;
    bool l_current_function_should_return_a_value;

    Expr* last_if_statement; // used for dangling else
} Parser_Context;

Parser_Context make_parser_context() {
    Parser_Context pctx;
    pctx.token_index                              = 0;
    pctx.top_tok_kind                             = TOKEN_UNKNOWN;
    pctx.curr_tok.kind                            = TOKEN_UNKNOWN;
    pctx.o_current_function_should_return_a_value = false;
    pctx.l_current_function_should_return_a_value = false;
    return pctx;
}

//------------------------------------------------------------------------------
//              Each construct of the language gets its own function
//------------------------------------------------------------------------------

void recursively_fill_ast(List* ast, Parser_Context* pctx);

Expr* parse_top_level(Parser_Context* pctx);
Expr* parse_statement(Parser_Context* pctx);
Expr* parse_primary(Parser_Context* pctx);
Expr* parse_identifier(Parser_Context* pctx);
Expr* parse_variable_decl(Parser_Context* pctx);
Expr* parse_constant_decl(Parser_Context* pctx);
Expr* parse_function_def_or_call(Parser_Context* pctx);
Expr* parse_function_call(Parser_Context* pctx);
Expr* parse_block(Parser_Context* pctx);
Expr* parse_return(Parser_Context* pctx);
Expr* parse_note(Parser_Context* pctx);
Expr* parse_expression(Parser_Context* pctx);
Expr* parse_unary(Parser_Context* pctx);
Expr* parse_binary(Parser_Context* pctx, s32 expr_prec, Expr* lhs);
Expr* parse_integer(Parser_Context* pctx);
Expr* parse_float(Parser_Context* pctx);
Expr* parse_parens(Parser_Context* pctx);
Expr* parse_defer(Parser_Context* pctx);
Expr* parse_load(Parser_Context* pctx);
Expr* parse_extern(Parser_Context* pctx);
Expr* parse_link(Parser_Context* pctx);
Expr* parse_if(Parser_Context* pctx);
Expr* parse_dangling_else(Parser_Context* pctx);
Expr* parse_for(Parser_Context* pctx);
Expr* parse_while(Parser_Context* pctx);
Expr* parse_break(Parser_Context* pctx);
Expr* parse_continue(Parser_Context* pctx);
Expr* parse_string(Parser_Context* pctx);
Expr* parse_sizeof(Parser_Context* pctx);
Expr* parse_def(Parser_Context* pctx);
Expr* parse_macro(Parser_Context* pctx);
Expr* parse_type(Parser_Context* pctx);
Expr* parse_cast(Parser_Context* pctx);

//------------------------------------------------------------------------------
//                               Helpers
//------------------------------------------------------------------------------

Typespec* parse_type_signature(Parser_Context* pctx, char* struct_name);
Typespec* parse_function_signature(Parser_Context* pctx, char* func_name);

s64       get_integer(Parser_Context* pctx);
f64       get_float(Parser_Context* pctx);
Typespec* get_type(Parser_Context* pctx);

Token      next_tok(Parser_Context* pctx);
bool       tok_is_on_same_line(Parser_Context* pctx);
bool       tok_is(Parser_Context* pctx, Token_Kind kind);
int        get_tok_precedence(Parser_Context* pctx);
void       eat(Parser_Context* pctx);
void       eat_kind(Parser_Context* pctx, Token_Kind kind);
Token_Kind next_tok_kind(Parser_Context* pctx);
bool       next_tok_is_on_same_line(Parser_Context* pctx);

void syntax_error(Parser_Context* pctx, char* fmt, ...);

//------------------------------------------------------------------------------
//                               Public
//------------------------------------------------------------------------------

void give_type_to_all_nodes(Expr* expr) {
    switch (expr->kind) {
        case EXPR_MACRO: give_type_to_all_nodes(expr->Macro.expr); break;
        case EXPR_NOTE: give_type_to_all_nodes(expr->Note.expr); break;
        case EXPR_UNARY: give_type_to_all_nodes(expr->Unary.operand); break;
        case EXPR_BINARY: {
            give_type_to_all_nodes(expr->Binary.lhs);
            give_type_to_all_nodes(expr->Binary.rhs);
        } break;
        case EXPR_GROUPING: give_type_to_all_nodes(expr->Grouping.expr); break;
        case EXPR_BLOCK: {
            List* stmts = expr->Block.stmts;
            LIST_FOREACH(stmts) {
                Expr* stmt = (Expr*)it->data;
                give_type_to_all_nodes(stmt);
            }
        } break;
        case EXPR_FUNCTION: {
            give_type_to_all_nodes(expr->Function.body);
        } break;
    }

    expr->type = get_inferred_type_of_expr(expr);
    if (expr->type) warning("%s | %s", expr_to_str(expr), typespec_to_str(expr->type));
}

// List* generate_ast_from_tokens(Token_Array tokens) {

//     List* ast = make_list();

//     Parser_Context pctx = make_parser_context();
//     pctx.token_array    = token_array;
//     pctx.ast            = ast;
//     pctx.source_file    = source_file;
//     pctx.source         = source;

//     info("Generating ast from tokens..");
//     eat(&pctx);
//     while (!tok_is(&pctx, TOKEN_EOF)) {
//         Expr* stmt = parse_top_level(&pctx);
//         if (stmt) {
//             list_append(ast, stmt);
//         }
//     }

//     list_append(ast_list, ast)
// }

// AST* parse_load(Parse_Context* pctx) {
//     eat_kind(pctx, TOKEN_LOAD);
//     char* file_to_load = pctx->curr_tok.value;
//     eat_kind(pctx, TOKEN_STRING);
//     make_ast_load(file_to_load);
// }

void parse(List* ast, char* source_file) {
    // We need to set some state
    char* last_file = get_source_file();
    char* last_dir  = get_current_dir();
    char* dir       = get_file_directory(source_file);

    set_source_file(source_file);
    set_current_dir(dir);

    list_append(file_list, source_file);

    push_timer(source_file);

    char*       source      = get_file_content(source_file);
    Token_Array token_array = generate_tokens_from_source(source);
    print_tokens(token_array);

    Parser_Context pctx = make_parser_context();
    pctx.token_array    = token_array;
    pctx.ast            = ast;
    pctx.source_file    = source_file;
    pctx.source         = source;

    recursively_fill_ast(ast, &pctx);

    info("Transforming AST");
    // Transform function call that look like definitions to definitions.
    LIST_FOREACH(ast) {
        Expr* expr      = (Expr*)it->data;
        Expr* next_expr = NULL;

        if (it->next) {
            next_expr = (Expr*)it->next->data;
            // warning("curr %s\nnext %s", expr_to_str(expr), expr_to_str(next_expr));

            if (expr->kind == EXPR_CALL && next_expr->kind == EXPR_BLOCK) {
                char*     func_name = expr->Call.callee;
                List*     args      = expr->Call.args;
                Typespec* type      = make_typespec_function(func_name, args, NULL);
                add_symbol(func_name, type);

                list_append(ast, make_expr_function(type, it->next->data));
                list_remove(ast, it->next);
                list_remove(ast, it);
            }

            if (expr->kind == EXPR_IDENT && next_expr->kind == EXPR_BLOCK) {
                char* ident              = expr->Ident.name;
                bool  has_variable_decls = false;
                bool  has_func_defs      = false;

                LIST_FOREACH(next_expr->Block.stmts) {
                    Expr* e = (Expr*)it->data;
                    if (e->kind == EXPR_VARIABLE_DECL) {
                        has_func_defs = true;
                        break;
                    }
                    if (e->kind == EXPR_VARIABLE_DECL) {
                        has_variable_decls = true;
                        break;
                    }
                }

                Typespec* type     = NULL;
                Expr*     new_expr = NULL;
                if (has_variable_decls || has_func_defs) {
                    // warning("ITS A STRUCT!");
                    type     = make_typespec_struct(ident, NULL);
                    new_expr = make_expr_struct(type);
                } else if (!has_variable_decls && !has_func_defs) {
                    // warning("ITS AN ENUM! %s %s", ident, expr_to_str(next_expr));
                    type     = make_typespec_enum(ident, next_expr->Block.stmts);
                    new_expr = make_expr_enum(type);
                }
                add_symbol(ident, type);

                list_append(ast, new_expr);
                list_remove(ast, it->next);
                list_remove(ast, it);
            }
        }
    }

    if (detailed_print) {
        string json      = make_string("{\"AST\": [");
        s64    ast_count = ast->count;
        s64    counter   = 0;
        LIST_FOREACH(ast) {
            append_string(&json, expr_to_json(it->data));
            if (counter != ast_count - 1) append_string(&json, ",");
            counter += 1;
        }
        append_string(&json, "]}");
        warning(json.c_str);

        print_symbol_map();
        print_ast(ast);
    }

    pop_timer();

    set_source_file(last_file);
    set_current_dir(last_dir);
}

//------------------------------------------------------------------------------
//                               Private
//------------------------------------------------------------------------------

void recursively_fill_ast(List* ast, Parser_Context* pctx) {
    info("Generating ast from tokens..");
    eat(pctx);
    while (!tok_is(pctx, TOKEN_EOF)) {
        Expr* stmt = parse_top_level(pctx);
        if (stmt) {
            list_append(ast, stmt);
        }
    }
}

Expr* parse_top_level(Parser_Context* pctx) {
    DEBUG_START;
    pctx->top_tok_kind = pctx->curr_tok.kind;
    switch (pctx->curr_tok.kind) {
        case TOKEN_NEWLINE: eat(pctx); break;
        case TOKEN_BLOCK_START: return parse_block(pctx);
        case TOKEN_IDENTIFIER: return parse_identifier(pctx);
        case TOKEN_TYPE: return parse_type(pctx);
        case TOKEN_DEF: return parse_def(pctx);
        case TOKEN_EXTERN: return parse_extern(pctx);
        case TOKEN_LOAD: return parse_load(pctx);
        case TOKEN_LINK: return parse_link(pctx);
        default:
            warning("Invalid toplevel token '%s'. Skipping it.", token_to_str(pctx->curr_tok));
            eat(pctx);
            break;
    }
    return NULL;
}

Expr* parse_statement(Parser_Context* pctx) {
    DEBUG_START;
    switch (pctx->curr_tok.kind) {
        case TOKEN_BLOCK_START: return parse_block(pctx);
        case TOKEN_DEF: return parse_def(pctx);
        case TOKEN_TYPE: return parse_type(pctx);
        case THI_SYNTAX_POINTER: return parse_unary(pctx);
        case TOKEN_IDENTIFIER: return parse_expression(pctx);
        case TOKEN_RETURN: return parse_return(pctx);
        case TOKEN_BREAK: return parse_break(pctx);
        case TOKEN_CONTINUE: return parse_continue(pctx);
        case TOKEN_IF: return parse_if(pctx);
        case TOKEN_ELSE: return parse_dangling_else(pctx);
        case TOKEN_DEFER: return parse_defer(pctx);
        case TOKEN_FOR: return parse_for(pctx);
        case TOKEN_WHILE: return parse_while(pctx);
        case TOKEN_EXTERN: return parse_extern(pctx);
        case TOKEN_LOAD: return parse_load(pctx);
        case TOKEN_LINK: return parse_link(pctx);
        case TOKEN_NEWLINE: break;
        default:
            warning("Invalid statement token '%s'. Skipping it.", token_to_str(pctx->curr_tok));
            eat(pctx);
            break;
    }
    eat_kind(pctx, TOKEN_NEWLINE);
    return NULL;
}

Expr* parse_primary(Parser_Context* pctx) {
    DEBUG_START;
    switch (pctx->curr_tok.kind) {
        case TOKEN_TRUE: eat(pctx); return make_expr_int(1);
        case TOKEN_FALSE: eat(pctx); return make_expr_int(0);
        case TOKEN_SIZEOF: return parse_sizeof(pctx);
        case TOKEN_CAST: return parse_cast(pctx);
        case TOKEN_IDENTIFIER: return parse_identifier(pctx);
        case TOKEN_DOLLAR_SIGN: return parse_note(pctx);
        case TOKEN_FLOAT: return parse_float(pctx);
        case TOKEN_CHAR:  // FALLTHROUGH
        case TOKEN_HEX:   // FALLTHROUGH
        case TOKEN_INTEGER: return parse_integer(pctx);
        case TOKEN_STRING: return parse_string(pctx);
        case TOKEN_OPEN_PAREN: return parse_parens(pctx);
        default:
            warning("Invalid primary token '%s'. Skipping it.", token_to_str(pctx->curr_tok));
            eat(pctx);
            break;
    }
    return NULL;
}

Expr* parse_identifier(Parser_Context* pctx) {
    DEBUG_START;

    switch (next_tok_kind(pctx)) {
        case TOKEN_IDENTIFIER:  // FALLTHROUGH
        case TOKEN_COLON_EQ: return parse_variable_decl(pctx);
        case TOKEN_COLON_COLON: return parse_constant_decl(pctx);
        // case TOKEN_OPEN_PAREN: return parse_function_call(pctx);
        case TOKEN_OPEN_PAREN: return parse_function_def_or_call(pctx);
    }
    char* ident = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    return make_expr_ident(ident);
}

Expr* parse_function_def_or_call(Parser_Context* pctx) {
    char* ident = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);

    List* args = make_list();

    bool has_multiple_arguments = false;
    eat_kind(pctx, TOKEN_OPEN_PAREN);
    while (!tok_is(pctx, TOKEN_CLOSE_PAREN)) {
        if (has_multiple_arguments) {
            eat_kind(pctx, TOKEN_COMMA);
        }
        Expr* arg              = parse_expression(pctx);
        has_multiple_arguments = true;
        list_append(args, arg);
    }
    eat_kind(pctx, TOKEN_CLOSE_PAREN);

    Typespec* ret_type = NULL;
    if (tok_is(pctx, TOKEN_IDENTIFIER) && tok_is_on_same_line(pctx)) {
        ret_type = get_type(pctx);
    }

    Expr* expr = NULL;
    if (ret_type) {
        Typespec* type = make_typespec_function(ident, args, ret_type);
        Expr*     body = parse_block(pctx);
        add_symbol(ident, type);
        list_append(pctx->ast, make_expr_function(type, body));
    } else {
        expr = make_expr_call(ident, args);
    }

    return expr;
}

Expr* parse_load(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_LOAD);
    string file                = make_string(strf("%s%s", get_current_dir(), pctx->curr_tok.value));
    bool   file_already_parsed = false;
    LIST_FOREACH(get_file_list()) {
        char* f = (char*)it->data;
        if (strcmp(f, file.c_str) == 0) {
            file_already_parsed = true;
        }
    }
    if (!file_already_parsed) parse(pctx->ast, file.c_str);
    eat_kind(pctx, TOKEN_STRING);
    return NULL;
}

Expr* parse_link(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_LINK);
    char* lib_name = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_STRING);
    add_link(lib_name);
    return NULL;
}

Expr* parse_extern(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_EXTERN);
    char* func_name = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    Typespec* func_t = parse_function_signature(pctx, func_name);
    add_foreign_function(func_name, func_t);
    add_symbol(func_name, func_t);
    return NULL;
}

Expr* parse_continue(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_CONTINUE);
    return make_expr_continue();
}

Expr* parse_break(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_BREAK);
    return make_expr_break();
}

Expr* parse_while(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_WHILE);
    Expr* cond       = parse_expression(pctx);
    Expr* then_block = parse_block(pctx);
    return make_expr_while(cond, then_block);
}

Expr* parse_for(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_FOR);
    Expr* init = parse_statement(pctx);
    eat_kind(pctx, TOKEN_COMMA);
    Expr* cond = parse_expression(pctx);
    eat_kind(pctx, TOKEN_COMMA);
    Expr* step       = parse_expression(pctx);
    Expr* then_block = parse_block(pctx);
    return make_expr_for(init, cond, step, then_block);
}

Expr* parse_defer(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_DEFER);
    Expr* block = parse_block(pctx);
    return make_expr_defer(block);
}

Expr* parse_dangling_else(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_ELSE);
    Expr* else_block = parse_block(pctx);
    pctx->last_if_statement->If.else_block = else_block;
    return NULL;
}

Expr* parse_if(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_IF);
    Expr* cond       = parse_expression(pctx);
    Expr* then_block = parse_block(pctx);
    Expr* else_block = NULL;
    if (tok_is(pctx, TOKEN_ELSE)) {
        eat_kind(pctx, TOKEN_ELSE);
        else_block = parse_block(pctx);
    }

    Expr* expr = make_expr_if(cond, then_block, else_block);

    pctx->last_if_statement = expr;

    return expr;
}

Expr* parse_sizeof(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_SIZEOF);
    eat_kind(pctx, TOKEN_OPEN_PAREN);
    Typespec* t = get_type(pctx);
    eat_kind(pctx, TOKEN_CLOSE_PAREN);
    s64 size = get_size_of_typespec(t);
    return make_expr_int(size);
}

Expr* parse_macro(Parser_Context* pctx) {
    char* identifier = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    eat_kind(pctx, TOKEN_COLON_COLON);
    Expr* expr = parse_expression(pctx);
    add_macro_def(identifier, expr);
    return NULL;
}

Expr* parse_def(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_DEF);
    char* identifier = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    Typespec* type = parse_function_signature(pctx, identifier);
    Expr*     body = parse_block(pctx);
    Expr*     func = make_expr_function(type, body);
    add_symbol(identifier, type);
    list_append(pctx->ast, func);
    return NULL;
}

Expr* parse_type(Parser_Context* pctx) {
    eat_kind(pctx, TOKEN_TYPE);
    char* identifier = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    Typespec* type = parse_type_signature(pctx, identifier);
    add_symbol(identifier, type);
    return NULL;
}

Expr* parse_string(Parser_Context* pctx) {
    DEBUG_START;
    char* value = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_STRING);
    return make_expr_string(value);
}

Expr* parse_block(Parser_Context* pctx) {
    DEBUG_START;

    // Is it a single statement?
    if (!tok_is(pctx, TOKEN_NEWLINE)) {
        return parse_statement(pctx);
    }

    List* stmts = make_list();

    // else it's a block
    eat_kind(pctx, TOKEN_NEWLINE);
    eat_kind(pctx, TOKEN_BLOCK_START);
    while (!tok_is(pctx, TOKEN_BLOCK_END)) {
        Expr* stmt = parse_statement(pctx);
        if (stmt) {
            list_append(stmts, stmt);
        }
    }
    eat_kind(pctx, TOKEN_BLOCK_END);
    return make_expr_block(stmts);
}

Expr* parse_return(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_RETURN);
    Expr* expr = NULL;
    if (tok_is_on_same_line(pctx)) {
        expr = parse_expression(pctx);
    }
    return make_expr_return(expr);
}

Expr* parse_function_call(Parser_Context* pctx) {
    DEBUG_START;

    char* ident = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);

    eat_kind(pctx, TOKEN_OPEN_PAREN);

    List* args = make_list();

    bool has_multiple_arguments = false;
    while (!tok_is(pctx, TOKEN_CLOSE_PAREN)) {
        if (has_multiple_arguments) eat_kind(pctx, TOKEN_COMMA);
        Expr* arg = parse_expression(pctx);
        list_append(args, arg);
        has_multiple_arguments = true;
    }
    eat_kind(pctx, TOKEN_CLOSE_PAREN);

    return make_expr_call(ident, args);
}

Expr* parse_constant_decl(Parser_Context* pctx) {
    DEBUG_START;
    char* name = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);
    eat_kind(pctx, TOKEN_COLON_COLON);
    Expr* value = parse_expression(pctx);
    return make_expr_constant_decl(name, value);
}

Expr* parse_variable_decl(Parser_Context* pctx) {
    DEBUG_START;

    char*     variable_name  = pctx->curr_tok.value;
    Typespec* variable_type  = NULL;
    Expr*     variable_value = NULL;

    eat_kind(pctx, TOKEN_IDENTIFIER);

    if (tok_is(pctx, TOKEN_COLON_EQ)) {
        // We need to infer the type based on the assignment expr
        eat_kind(pctx, TOKEN_COLON_EQ);
        variable_value = parse_expression(pctx);
        variable_type  = get_inferred_type_of_expr(variable_value);
    }

    if (tok_is_on_same_line(pctx) && tok_is(pctx, TOKEN_IDENTIFIER)) {
        variable_type = get_type(pctx);
        if (tok_is(pctx, TOKEN_EQ)) {
            eat_kind(pctx, TOKEN_EQ);
            variable_value = parse_expression(pctx);
        }
    }

    return make_expr_variable_decl(variable_name, variable_type, variable_value);
}

Expr* parse_binary(Parser_Context* pctx, s32 expr_prec, Expr* lhs) {
    DEBUG_START;

    Expr* expr = NULL;

    // If this is a binop, find its precedence.
    while (1) {
        s32 tok_prec = get_tok_precedence(pctx);

        // If this is a binop that binds at least as tightly as the current
        // binop, consume it, otherwise we are done.
        if (tok_prec < expr_prec) {
            expr = lhs;
            break;
        }

        // Okay, we know this is a binop.
        Token_Kind binary_op_token = pctx->curr_tok.kind;
        eat(pctx);  // eat binop

        // Parser the unary expression after the binary operator.
        Expr* rhs = parse_unary(pctx);
        if (!rhs) {
            expr = NULL;
            break;
        }
        // If BinOp binds less tightly with rhs than the operator after rhs, let
        // the pending operator take rhs as its lhs.
        s32 next_prec = get_tok_precedence(pctx);
        if (tok_prec < next_prec) {
            rhs = parse_binary(pctx, tok_prec + 1, rhs);

            if (!rhs) {
                expr = NULL;
                break;
            }
        }

        // Merge LHS/rhs.
        lhs = make_expr_binary(binary_op_token, lhs, rhs);
    }

    return expr;
}

Typespec* active_type = NULL;
Expr*     read_field_access_expr(Parser_Context* pctx, Expr* expr) {
    eat_kind(pctx, TOKEN_DOT);
    char* field_name = pctx->curr_tok.value;
    eat_kind(pctx, TOKEN_IDENTIFIER);

    Typespec* type = get_inferred_type_of_expr(expr);
    active_type    = type;
    s64 offset     = get_offset_in_struct_to_field(type, field_name);

    expr = make_expr_unary(THI_SYNTAX_ADDRESS, expr);
    expr = make_expr_binary(TOKEN_PLUS, expr, make_expr_int(offset));

    return make_expr_unary(THI_SYNTAX_POINTER, expr);
}

Expr* read_subscript_expr(Parser_Context* pctx, Expr* expr) {
    eat_kind(pctx, TOKEN_OPEN_BRACKET);
    Expr* sub = parse_expression(pctx);
    if (!sub) syntax_error(pctx, "empty subscript");
    eat_kind(pctx, TOKEN_CLOSE_BRACKET);
    return make_expr_subscript(expr, sub);
}

Expr* parse_postfix_tail(Parser_Context* pctx, Expr* primary_expr) {
    DEBUG_START;
    if (!primary_expr) return NULL;
    while (1) {
        if (tok_is(pctx, TOKEN_DOT)) {
            primary_expr = read_field_access_expr(pctx, primary_expr);
            continue;
        }
        if (tok_is(pctx, TOKEN_OPEN_BRACKET)) {
            primary_expr = read_subscript_expr(pctx, primary_expr);
            continue;
        }
        return primary_expr;
    }
}

Expr* parse_postfix(Parser_Context* pctx) {
    DEBUG_START;
    Expr* primary_expr = parse_primary(pctx);
    return parse_postfix_tail(pctx, primary_expr);
}

Expr* parse_cast(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_CAST);
    eat_kind(pctx, TOKEN_OPEN_PAREN);
    Typespec* type = get_type(pctx);
    eat_kind(pctx, TOKEN_COMMA);
    Expr* expr = parse_expression(pctx);
    eat_kind(pctx, TOKEN_CLOSE_PAREN);
    return make_expr_cast(expr, type);
}

Expr* parse_unary(Parser_Context* pctx) {
    DEBUG_START;
    Expr* unary_expr = NULL;
    if (tok_is(pctx, TOKEN_BANG) || tok_is(pctx, THI_SYNTAX_POINTER) || tok_is(pctx, TOKEN_MINUS) ||
        tok_is(pctx, TOKEN_PLUS) || tok_is(pctx, THI_SYNTAX_ADDRESS) || tok_is(pctx, TOKEN_TILDE)) {
        Token_Kind op = pctx->curr_tok.kind;
        eat(pctx);

        Expr* operand = parse_unary(pctx);
        if (operand) {
            unary_expr = make_expr_unary(op, operand);
        }
    }

    // If the current token is not an operator, it must be a primary expression.
    return unary_expr ? unary_expr : parse_postfix(pctx);
}

Expr* parse_note(Parser_Context* pctx) {
    DEBUG_START;
    eat_kind(pctx, TOKEN_DOLLAR_SIGN);
    Expr* expr = NULL;
    switch (pctx->curr_tok.kind) {
        case TOKEN_HEX: expr = parse_integer(pctx); break;
        case TOKEN_INTEGER: expr = parse_integer(pctx); break;
        default: error("unknown token when expecting a note");
    }
    return make_expr_note(expr);
}

Expr* parse_expression(Parser_Context* pctx) {
    DEBUG_START;
    Expr* lhs  = parse_unary(pctx);
    Expr* expr = NULL;
    if (lhs) {
        expr = parse_binary(pctx, 0, lhs);
    }
    return expr;
}

Expr* parse_float(Parser_Context* pctx) {
    DEBUG_START;
    Expr* res = make_expr_float(get_float(pctx));
    return res;
}
Expr* parse_integer(Parser_Context* pctx) {
    DEBUG_START;
    Expr* res = make_expr_int(get_integer(pctx));
    return res;
}

Expr* parse_parens(Parser_Context* pctx) {
    DEBUG_START;

    eat_kind(pctx, TOKEN_OPEN_PAREN);
    Expr* expr = parse_expression(pctx);
    eat_kind(pctx, TOKEN_CLOSE_PAREN);

    return make_expr_grouping(expr);
}

//------------------------------------------------------------------------------
//                               Helpers
//------------------------------------------------------------------------------

Typespec* parse_enum_signature(Parser_Context* pctx, char* name) {
    DEBUG_START;

    eat_kind(pctx, TOKEN_ENUM);
    eat_kind(pctx, TOKEN_BLOCK_START);
    while (!tok_is(pctx, TOKEN_BLOCK_END)) {
        info("  enum member: %s", pctx->curr_tok.value);
        eat_kind(pctx, TOKEN_IDENTIFIER);
    }
    eat_kind(pctx, TOKEN_BLOCK_END);

    return make_typespec_enum(name, NULL);
}

Typespec* parse_type_signature(Parser_Context* pctx, char* struct_name) {
    DEBUG_START;

    eat_kind(pctx, TOKEN_BLOCK_START);

    List* members = make_list();

    while (!tok_is(pctx, TOKEN_BLOCK_END)) {
        Expr* expr = NULL;
        switch (pctx->curr_tok.kind) {
            case TOKEN_DEF: expr = parse_def(pctx); break;
            case TOKEN_IDENTIFIER: expr = parse_variable_decl(pctx); break;
            default:
                syntax_error(pctx,
                             "Only variable declaration and function definition are valid statements inside a type.");
        }
        if (expr) list_append(members, expr);
    }
    eat_kind(pctx, TOKEN_BLOCK_END);

    return make_typespec_struct(struct_name, members);
}

Typespec* parse_function_signature(Parser_Context* pctx, char* func_name) {
    DEBUG_START;

    eat_kind(pctx, TOKEN_OPEN_PAREN);

    List* args = make_list();

    bool has_multiple_arguments = false;
    while (!tok_is(pctx, TOKEN_CLOSE_PAREN)) {
        if (has_multiple_arguments) {
            eat_kind(pctx, TOKEN_COMMA);
        }

        Expr* expr = NULL;

        // foreign's dont have named parameters
        if (pctx->top_tok_kind == TOKEN_EXTERN) {
            expr = make_expr_variable_decl(NULL, get_type(pctx), NULL);
        } else {
            if (tok_is(pctx, TOKEN_IDENTIFIER)) {
                expr = parse_variable_decl(pctx);
            } else
                syntax_error(pctx, "Only variable declaration valid inside a functions parameters.");
        }
        has_multiple_arguments = true;
        list_append(args, expr);
    }
    eat_kind(pctx, TOKEN_CLOSE_PAREN);

    Typespec* ret_type = NULL;
    if (tok_is_on_same_line(pctx)) {
        ret_type = get_type(pctx);
    }

    return make_typespec_function(func_name, args, ret_type);
}

Typespec* get_type(Parser_Context* pctx) {
    DEBUG_START;

    char* type_name = pctx->curr_tok.value;

    eat_kind(pctx, TOKEN_IDENTIFIER);
    Typespec* type = NULL;

    if (is_builtin_type(type_name))
        type = get_builtin_type(type_name);
    else
        type = get_symbol(type_name);

    // If the type is not yet known, make a stub for it
    if (!type) {
        type = make_typespec_struct(type_name, NULL);
    }

    switch (pctx->curr_tok.kind) {
        case THI_SYNTAX_POINTER: {
            while (tok_is(pctx, THI_SYNTAX_POINTER)) {
                eat_kind(pctx, THI_SYNTAX_POINTER);
                type = make_typespec_pointer(type);
            }
        } break;
        case TOKEN_OPEN_BRACKET: {
            eat_kind(pctx, TOKEN_OPEN_BRACKET);
            s64 size = 0;
            if (tok_is(pctx, TOKEN_INTEGER) || tok_is(pctx, TOKEN_HEX)) {
                size = get_integer(pctx);
            }
            eat_kind(pctx, TOKEN_CLOSE_BRACKET);
            type = make_typespec_array(type, size);
        } break;
    }

    return type;
}

s64 get_integer(Parser_Context* pctx) {
    DEBUG_START;

    s64 value = 0;
    switch (pctx->curr_tok.kind) {
        case TOKEN_CHAR: {
            u8 c = pctx->curr_tok.value[0];
            if (c == '\\') {
                u8 c = pctx->curr_tok.value[1];
                switch (c) {
                    case 'a': value = 7; break;
                    case 'n': value = 10; break;
                    case 't': value = 9; break;
                    case '\\': value = 92; break;
                    case '\'': value = 27; break;
                    case '"': value = 22; break;
                }
            } else
                value = c;
            break;
        }
        case TOKEN_INTEGER: value = atoll(pctx->curr_tok.value); break;
        case TOKEN_HEX: value = strtoll(pctx->curr_tok.value, NULL, 0); break;
        default: error("not an integer.");
    }
    eat(pctx);

    return value;
}

f64 get_float(Parser_Context* pctx) {
    DEBUG_START;

    f64 value = atof(pctx->curr_tok.value);
    eat_kind(pctx, TOKEN_FLOAT);

    return value;
}

int get_tok_precedence(Parser_Context* pctx) {
    DEBUG_START;
    for (int i = 0; i < BIN_OP_COUNT; ++i)
        if (binop_precedence[i].kind == pctx->curr_tok.kind) return binop_precedence[i].p;
    return -1;
}

Token next_tok(Parser_Context* pctx) {
    if (pctx->token_array.count < pctx->token_index + 1) {
        error("No next token. We're all out.");
    }
    return pctx->token_array.data[pctx->token_index];
}

Token_Kind next_tok_kind(Parser_Context* pctx) {
    if (pctx->token_array.count < pctx->token_index + 1) {
        error("No next token. We're all out.");
    }
    Token_Kind kind = pctx->token_array.data[pctx->token_index].kind;
    return kind;
}

bool tok_is_on_same_line(Parser_Context* pctx) {
    s64        l1 = pctx->curr_tok.line_pos;
    s64        l2 = pctx->prev_tok.line_pos;
    return l1 == l2;
}
bool next_tok_is_on_same_line(Parser_Context* pctx) {
    Token t2 = next_tok(pctx);
    s64        l1 = pctx->curr_tok.line_pos;
    s64        l2 = t2.line_pos;
    return l1 == l2;
}
bool tok_is(Parser_Context* pctx, Token_Kind kind) { return pctx->curr_tok.kind == kind; }

void eat(Parser_Context* pctx) {
    pctx->prev_tok = pctx->curr_tok;
    pctx->curr_tok = pctx->token_array.data[pctx->token_index];
    pctx->token_index += 1;
}

void eat_kind(Parser_Context* pctx, Token_Kind kind) {
    Token_Kind tk = pctx->curr_tok.kind;
    if (tk != kind) {
        syntax_error(pctx, "Expected '%s' got '%s'", token_kind_to_str(kind), token_kind_to_str(tk));
    }
    eat(pctx);
}

void syntax_error(Parser_Context* pctx, char* fmt, ...) {
    assert(fmt);

    char* source_file = pctx->source_file;
    s64 line_pos = pctx->curr_tok.line_pos;
    s64 col_pos  = pctx->curr_tok.col_pos;

    va_list args;
    va_start(args, fmt);
    printf("%s%s %lld:%lld Syntax Error: ", RED, source_file, line_pos, col_pos);
    vprintf(fmt, args);
    puts(RESET);
    va_end(args);
    exit(1);
}
