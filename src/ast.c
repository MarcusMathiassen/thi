#include "ast.h"

#include <stdlib.h>           // malloc,
#include "stretchy_buffer.h"  // sb_push
#include "utility.h"          // info, success, error, warning
#include "lexer.h"          // token_kind_to_str,

//------------------------------------------------------------------------------
//                               ast.c
//------------------------------------------------------------------------------
static Expr* alloc_expr();

//------------------------------------------------------------------------------
//                               Public
//------------------------------------------------------------------------------

const char* expr_kind_to_str(Expr_Kind kind)
{
    switch (kind)
    {
        case EXPR_NONE: return "EXPR_NONE";
        case EXPR_INT: return "EXPR_INT";
        case EXPR_FLOAT: return "EXPR_FLOAT";
        case EXPR_IDENT: return "EXPR_IDENT";
        case EXPR_CALL: return "EXPR_CALL";
        case EXPR_UNARY: return "EXPR_UNARY";
        case EXPR_BINARY: return "EXPR_BINARY";
        case EXPR_COMPOUND: return "EXPR_COMPOUND";
        case EXPR_RET: return "EXPR_RET";
        case EXPR_VAR_DECL: return "EXPR_VAR_DECL";
        case EXPR_FUNC: return "EXPR_FUNC";
        case EXPR_IF: return "EXPR_IF";
        case EXPR_FOR: return "EXPR_FOR";
        case EXPR_BLOCK: return "EXPR_BLOCK";
        case EXPR_WHILE: return "EXPR_WHILE";
        case EXPR_GROUPING: return "EXPR_GROUPING";
        default: return "print not implemented";
    }
}

void print_expr(Expr* expr)
{
    info(expr_kind_to_str(expr->kind));
    switch (expr->kind)
    {
        case EXPR_NONE: warning("EXPR_NONE not print implemented."); break;

        case EXPR_INT: info("%lld", expr->Int.val); break;
        case EXPR_FLOAT: info("%f", expr->Float.val); break;
        case EXPR_IDENT: info("%s", expr->Ident.name); break;
        case EXPR_CALL: warning("EXPR_CALL not print implemented."); break;
        case EXPR_UNARY: info("%c", expr->Unary.op); break;
        case EXPR_BINARY: info("%s", token_kind_to_str(expr->Binary.op)); break;
        case EXPR_COMPOUND: warning("EXPR_COMPOUND not print implemented."); break;
        case EXPR_RET: print_expr(expr->Ret.expr); break;
        case EXPR_VAR_DECL: warning("EXPR_VAR_DECL not print implemented."); break;
        case EXPR_FUNC: 
            print_type(expr->Func.type);
            print_expr(expr->Func.body);
            break;        case EXPR_IF: warning("EXPR_IF not print implemented."); break;
        case EXPR_FOR: warning("EXPR_FOR not print implemented."); break;
        case EXPR_BLOCK:
            for (int i = 0; i < sb_count(expr->Block.stmts); ++i)
            {
                print_expr(expr->Block.stmts[i]);   
            }
            break;
        case EXPR_WHILE: warning("EXPR_WHILE not print implemented."); break;
        case EXPR_GROUPING: warning("EXPR_GROUPING not print implemented."); break;
        default: warning("print not implemented %s", expr_kind_to_str(expr->kind));
    }
}

void print_ast(AST** ast)
{
    for (int i = 0; i < sb_count(ast); ++i) {
        print_expr(ast[i]);
    }
}

//------------------------------------------------------------------------------
//                               Expr Maker Functions
//------------------------------------------------------------------------------

Expr* make_expr_int(int value) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_INT;
    temp->Int.val = value;
    return temp;
}

Expr* make_expr_float(float value) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_FLOAT;
    temp->Float.val = value;
    return temp;
}

Expr* make_expr_ident(const char* ident) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_IDENT;
    temp->Ident.name = ident;
    return temp;
}

Expr* make_expr_func(Type* func_t, Expr* body) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_FUNC;
    temp->Func.type = func_t;
    temp->Func.body = body;
    return temp;
}

Expr* make_expr_binary(char op, Expr* lhs, Expr* rhs) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_BINARY;
    temp->Binary.op = op;
    temp->Binary.lhs = lhs;
    temp->Binary.rhs = rhs;
    return temp;
}

Expr* make_expr_unary(char op, Expr* operand) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_UNARY;
    temp->Unary.op = op;
    temp->Unary.operand = operand;
    return temp;
}

Expr* make_expr_block(Expr** stmts) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_BLOCK;
    temp->Block.stmts = stmts;
    return temp;
}

Expr* make_expr_grouping(Expr* expr) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_GROUPING;
    temp->Grouping.expr = expr;
    return temp;
}
Expr* make_expr_ret(Expr* expr) {
    Expr* temp = alloc_expr();
    temp->kind = EXPR_RET;
    temp->Ret.expr = expr;
    return temp;
}

//------------------------------------------------------------------------------
//                               Private
//------------------------------------------------------------------------------

static Expr* alloc_expr() {
    Expr* temp = (Expr*)malloc(sizeof(Expr));
    if (!temp) error("Out of memory trying to allocate expression.");
    return temp;
}