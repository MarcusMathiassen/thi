#include "ast.h"
#include "globals.h"
#include "lexer.h"           // token_kind_to_str,
#include "stretchy_buffer.h" // sb_push
#include "string.h"          // strf, append_string, string
#include "utility.h"         // info, success, error, warning
#include <assert.h>          // assert
#include <stdio.h>           // sprintf,
#include <stdlib.h>          // malloc,

//------------------------------------------------------------------------------
//                               ast.c
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                               Public
//------------------------------------------------------------------------------

char* expr_kind_to_str(Expr_Kind kind)
{
    switch (kind) {
    case EXPR_ASM: return "EXPR_ASM";
    case EXPR_MACRO: return "EXPR_MACRO";
    case EXPR_NOTE: return "EXPR_NOTE";
    case EXPR_INT: return "EXPR_INT";
    case EXPR_FLOAT: return "EXPR_FLOAT";
    case EXPR_STRING: return "EXPR_STRING";
    case EXPR_IDENT: return "EXPR_IDENT";
    case EXPR_CALL: return "EXPR_CALL";
    case EXPR_UNARY: return "EXPR_UNARY";
    case EXPR_BINARY: return "EXPR_BINARY";
    case EXPR_VARIABLE_DECL: return "EXPR_VARIABLE_DECL";
    case EXPR_STRUCT: return "EXPR_STRUCT";
    case EXPR_BLOCK: return "EXPR_BLOCK";
    case EXPR_GROUPING: return "EXPR_GROUPING";
    }
    return "print not implemented";
}

char* expr_to_str(Expr* expr)
{
    char* result = NULL;
    switch (expr->kind) {
    case EXPR_ASM: {
        result = strf("%s", expr->Asm.str);
    } break;
    case EXPR_MACRO: {
        result = strf("%s :: %s", expr->Macro.name, expr_to_str(expr->Macro.expr));
    } break;
    case EXPR_NOTE: {
        result = strf("$%s", expr_to_str(expr->Note.expr));
    } break;
    case EXPR_INT: {
        result = strf("%llu", expr->Int.val);
    } break;
    case EXPR_STRING: {
        result = strf("\"%s\"", expr->String.val);
    } break;
    case EXPR_IDENT: {
        result = strf("%s", expr->Ident.name);
    } break;
    case EXPR_UNARY: {
        result = strf("%s%s", token_kind_to_str(expr->Unary.op), expr_to_str(expr->Unary.operand));
    } break;
    case EXPR_BINARY: {
        result = strf("%s %s %s", expr_to_str(expr->Binary.lhs), token_kind_to_str(expr->Binary.op),
                      expr_to_str(expr->Binary.rhs));
    } break;
    case EXPR_VARIABLE_DECL: {
        result = strf(expr->Variable_Decl.value ? "%s: %s = %s" : "%s: %s", expr->Variable_Decl.name,
                      typespec_to_str(expr->Variable_Decl.type),
                      expr->Variable_Decl.value ? expr_to_str(expr->Variable_Decl.value) : "");
    } break;
    case EXPR_BLOCK: {
        string str = make_string("");
        for (int i = 0; i < sb_count(expr->Block.stmts); ++i) {
            append_string_f(&str, "\t%s\n", expr_to_str(expr->Block.stmts[i]));
        }
        result = str.c_str;
    } break;
    case EXPR_STRUCT: {
        result = strf("%s", typespec_to_str(expr->Struct.type));
    } break;
    case EXPR_GROUPING: result = strf("(%s)", expr_to_str(expr->Grouping.expr)); break;
    case EXPR_CALL: {
        string str = make_string_f("%s", expr->Call.callee);
        result = str.c_str;
    } break;
    }
    assert(result);
    return result;
}

char* expr_to_str_debug(Expr* expr)
{
    char* result = NULL;
    switch (expr->kind) {
    case EXPR_ASM: {
        result = strf("%s", expr->Asm.str);
    } break;
    case EXPR_STRUCT: {
        result = strf("%s", typespec_to_str(expr->Struct.type));
    } break;
    case EXPR_MACRO: {
        result = strf("%s :: %s", expr->Macro.name, expr_to_str(expr->Macro.expr));
    } break;
    case EXPR_NOTE: {
        result = strf("$%s", expr_to_str(expr->Note.expr));
    } break;
    case EXPR_INT: {
        result = strf("%lld", expr->Int.val);
    } break;
    case EXPR_STRING: {
        result = strf("\"%s\"", expr->String.val);
    } break;
    case EXPR_IDENT: {
        result = strf("%s", expr->Ident.name);
    } break;
    case EXPR_UNARY: {
        result = strf("%s%s", token_kind_to_str(expr->Unary.op), expr_to_str(expr->Unary.operand));
    } break;
    case EXPR_BINARY: {
        result = strf("%s %s %s", expr_to_str(expr->Binary.lhs), token_kind_to_str(expr->Binary.op),
                      expr_to_str(expr->Binary.rhs));
    } break;
    case EXPR_VARIABLE_DECL: {
        result = strf(expr->Variable_Decl.value ? "%s: %s = %s" : "%s: %s", expr->Variable_Decl.name,
                      typespec_to_str(expr->Variable_Decl.type),
                      expr->Variable_Decl.value ? expr_to_str(expr->Variable_Decl.value) : "");
    } break;
    case EXPR_BLOCK: {
        string str = make_string("");
        for (int i = 0; i < sb_count(expr->Block.stmts); ++i) {
            append_string_f(&str, "\t%s\n", expr_to_str(expr->Block.stmts[i]));
        }
        result = str.c_str;
    } break;
    case EXPR_GROUPING: result = strf("(%s)", expr_to_str(expr->Grouping.expr)); break;
    case EXPR_CALL: {
        string str = make_string_f("%s", expr->Call.callee);
        result = str.c_str;
    } break;
    }
    assert(result);
    return wrap_with_colored_parens(result);
}

char* expr_to_json(Expr* expr)
{
    char* result = NULL;
    switch (expr->kind) {
    case EXPR_ASM: {
        result = strf("{\"%s\": {\"asm\": \"%s\"}}", expr_kind_to_str(expr->kind), expr->Asm.str);
    } break;
    case EXPR_MACRO: {
        warning("MACRO: %s", expr->Macro.name);
        result = strf("{\"%s\": {\"name\": \"%s\", \"expr\": \"%s\"}}", expr_kind_to_str(expr->kind), expr->Macro.name,
                      expr_to_json(expr));
    } break;
    case EXPR_NOTE: {
        result = strf("{\"%s\": {\"note\":\"%s\"}}", expr_kind_to_str(expr->kind), expr_to_json(expr->Note.expr));
    } break;
    case EXPR_INT: {
        result = strf("{\"%s\": {\"value\": %lld}}", expr_kind_to_str(expr->kind), expr->Int.val);
    } break;
    case EXPR_STRING: {
        result = strf("{\"%s\": {\"value\": \"%s\"}}", expr_kind_to_str(expr->kind), expr->String.val);
    } break;
    case EXPR_STRUCT: {
        result = strf("{\"%s\": {\"type\": \"%s\"}}", expr_kind_to_str(expr->kind), typespec_to_str(expr->Struct.type));
    } break;
    case EXPR_IDENT: {
        result = strf("{\"%s\": {\"ident\": \"%s\"}}", expr_kind_to_str(expr->kind), expr->Ident.name);
    } break;
    case EXPR_UNARY: {
        result = strf("{\"%s\": {\"op\": \"%s\", \"expr\": \"%s\"}}", expr_kind_to_str(expr->kind),
                      token_kind_to_str(expr->Unary.op), expr_to_json(expr->Unary.operand));
    } break;
    case EXPR_BINARY: {
        result =
            strf("{\"%s\": {\"op\": \"%s\", \"lhs\": %s, \"rhs\": %s}}", expr_kind_to_str(expr->kind),
                 token_kind_to_str(expr->Binary.op), expr_to_json(expr->Binary.lhs), expr_to_json(expr->Binary.rhs));
    } break;
    case EXPR_VARIABLE_DECL: {
        result = strf("{\"%s\": {\"name\": \"%s\", \"type\": \"%s\", \"value\": \"%s\"}}", expr_kind_to_str(expr->kind),
                      expr->Variable_Decl.name, typespec_to_str(expr->Variable_Decl.type),
                      expr->Variable_Decl.value ? expr_to_json(expr->Variable_Decl.value) : "");
    } break;
    case EXPR_BLOCK: {
        i32 block_count = sb_count(expr->Block.stmts);
        string str = make_string("{\"EXPR_BLOCK\": [");
        for (int i = 0; i < block_count; ++i) {
            append_string(&str, expr_to_json(expr->Block.stmts[i]));
            if (i != block_count - 1) append_string(&str, ", ");
        }
        append_string(&str, "]}");
        result = str.c_str;
    } break;
    case EXPR_GROUPING: {
        result = strf("{\"%s\": {\"expr\": %s}}", expr_kind_to_str(expr->kind), expr_to_json(expr->Grouping.expr));
    } break;
    case EXPR_CALL: {
        i32 arg_count = sb_count(expr->Call.args);
        string str = make_string(strf("{\"%s\": {\"callee\": \"%s\", \"args\": [", expr_kind_to_str(expr->kind), expr->Call.callee));
        for (int i = 0; i < arg_count; ++i) {
            append_string(&str, expr_to_json(expr->Call.args[i]));
            if (i != arg_count - 1) {
                append_string(&str, ", ");
            }
        }
        append_string(&str, "]}}");
        result = str.c_str;
    } break;
    }
    assert(result);
    return result;
}


Typespec* get_inferred_type_of_expr(Expr* expr)
{
    switch(expr->kind)
    {
    case EXPR_MACRO: return get_inferred_type_of_expr(expr->Macro.expr);
    case EXPR_NOTE: return get_inferred_type_of_expr(expr->Note.expr);
    case EXPR_INT: return make_typespec_int(DEFAULT_INTEGER_BIT_SIZE, 0);
    case EXPR_FLOAT: error("get_inferred_type_of_expr EXPR_FLOAT not implemented");
    case EXPR_STRING: error("get_inferred_type_of_expr EXPR_STRING not implemented");
    case EXPR_IDENT: return get_symbol(expr->Ident.name);
    case EXPR_CALL: return get_symbol(expr->Call.callee);
    case EXPR_UNARY: return get_inferred_type_of_expr(expr->Unary.operand);
    case EXPR_BINARY: return get_inferred_type_of_expr(expr->Binary.rhs);
    case EXPR_VARIABLE_DECL: return expr->Variable_Decl.type;
    case EXPR_STRUCT: return expr->Struct.type;
    case EXPR_GROUPING: return get_inferred_type_of_expr(expr->Grouping.expr);
    // default: error("%s has no type", expr_kind_to_str(expr->kind));
    }
    return NULL;
}

void print_ast(List* ast)
{
    info("Printing AST..");
    LIST_FOREACH(ast) { info("%s", expr_to_str_debug((Expr*)it->data)); }
}

char* ast_to_json(List* ast)
{
    info("Printing AST as JSON..");
    string json = make_string("");
    LIST_FOREACH(ast)
    {
        Expr* expr = (Expr*)it->data;
        append_string_f(&json, "%s", expr_to_json(expr));
    }
    return json.c_str;
}

//------------------------------------------------------------------------------
//                               Expr Maker Functions
//------------------------------------------------------------------------------

Expr* make_expr(Expr_Kind kind)
{
    Expr* e = xmalloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

Expr* make_expr_asm(char* str)
{
    assert(str);
    Expr* e = make_expr(EXPR_ASM);
    e->Asm.str = str;
    return e;
}

Expr* make_expr_macro(char* name, Expr* expr)
{
    assert(name);
    assert(expr);
    Expr* e = make_expr(EXPR_MACRO);
    e->Macro.name = name;
    e->Macro.expr = expr;
    return e;
}

Expr* make_expr_note(Expr* expr)
{
    assert(expr);
    Expr* e = make_expr(EXPR_NOTE);
    e->Note.expr = expr;
    return e;
}

Expr* make_expr_int(i64 value)
{
    Expr* e = make_expr(EXPR_INT);
    e->Int.val = value;
    return e;
}

Expr* make_expr_float(f64 value)
{
    Expr* e = make_expr(EXPR_FLOAT);
    e->Float.val = value;
    return e;
}

Expr* make_expr_string(char* value)
{
    Expr* e = make_expr(EXPR_STRING);
    e->String.val = value;
    return e;
}

Expr* make_expr_ident(char* ident)
{
    assert(ident);
    Expr* e = make_expr(EXPR_IDENT);
    e->Ident.name = ident;
    return e;
}

Expr* make_expr_struct(Typespec* struct_t)
{
    assert(struct_t);
    Expr* e = make_expr(EXPR_STRUCT);
    e->Struct.type = struct_t;
    return e;
}

Expr* make_expr_binary(Token_Kind op, Expr* lhs, Expr* rhs)
{
    assert(op != TOKEN_UNKNOWN);
    assert(lhs);
    assert(rhs);
    Expr* e = make_expr(EXPR_BINARY);
    e->Binary.op = op;
    e->Binary.lhs = lhs;
    e->Binary.rhs = rhs;
    return e;
}

Expr* make_expr_unary(Token_Kind op, Expr* operand)
{
    assert(op != TOKEN_UNKNOWN);
    assert(operand);
    Expr* e = make_expr(EXPR_UNARY);
    e->Unary.op = op;
    e->Unary.operand = operand;
    return e;
}

Expr* make_expr_block(Expr** stmts)
{
    Expr* e = make_expr(EXPR_BLOCK);
    e->Block.stmts = stmts;
    return e;
}

Expr* make_expr_call(char* callee, Expr** args)
{
    assert(callee);
    assert(args);
    Expr* e = make_expr(EXPR_CALL);
    e->Call.callee = callee;
    e->Call.args = args;
    return e;
}

Expr* make_expr_grouping(Expr* expr)
{
    assert(expr);
    Expr* e = make_expr(EXPR_GROUPING);
    e->Grouping.expr = expr;
    return e;
}

Expr* make_expr_variable_decl(char* name, Typespec* type, Expr* value)
{
    assert(name);
    assert(type);
    // Value can be NULL if the decl doesnt have an initial value.
    Expr* e = make_expr(EXPR_VARIABLE_DECL);
    e->Variable_Decl.name = name;
    e->Variable_Decl.type = type;
    e->Variable_Decl.value = value;
    return e;
}
