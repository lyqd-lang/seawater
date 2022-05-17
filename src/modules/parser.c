#include "parser.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include "lqdint.h"
#include <string.h>

void advance(lqdParserCtx* ctx) {
    ctx -> tok = ctx -> toks -> tokens[ctx -> tok_idx++];
}

lqdToken peek(lqdParserCtx* ctx) {
    return ctx -> toks -> tokens[ctx -> tok_idx];
}

lqdASTNode null_node = {0,NULL};

void lqdSyntaxError(lqdParserCtx* ctx, char* message) {
    fprintf(stderr, "╭────────────────────[ Syntax Error ]────→\n");
    fprintf(stderr, "│ %s\n", message);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ ");
    int idx = 0;
    int i = 0;
    for (i = 0; i < ctx -> tok.idx_start; i++)
        if (ctx -> code[i] == '\n') idx = i;
    i = ctx -> tok.line == 1 ? 0 : 1;
    while (ctx -> code[idx + i] && ctx -> code[idx + i] != '\n') {
        fprintf(stderr, "%c", ctx -> code[idx + i]);
        i++;
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "│ ");
    for (i = ctx -> tok.line == 1 ? -1 : 0; i < (int)(ctx -> tok.idx_start - idx); i++)
        fprintf(stderr, " ");
    fprintf(stderr, "^");
    for (i = 0; i < ctx -> tok.idx_end - ctx -> tok.idx_start; i++)
        fprintf(stderr, "~");
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ Line: %ld, File: %s\n", ctx -> tok.line, ctx -> filename);
    fprintf(stderr, "╯\n");
    exit(1);
}

lqdASTNode expr(lqdParserCtx* ctx);
lqdASTNode term(lqdParserCtx* ctx);
lqdASTNode factor(lqdParserCtx* ctx);
lqdASTNode statement(lqdParserCtx* ctx);
lqdType type(lqdParserCtx* ctx);


lqdType type(lqdParserCtx* ctx) {
    lqdType _type = {lqdToken_new(0,NULL,0,0,0)};
    if (ctx -> tok.type != TT_IDEN) {
        lqdSyntaxError(ctx, "Expected a type!");
        return _type;
    }
    _type.type = ctx -> tok;
    advance(ctx);
    return _type;
}

lqdASTNode statement(lqdParserCtx* ctx) {
    if (ctx -> tok.type == TT_IDEN && (!strcmp(ctx -> tok.value, "fn") || !strcmp(ctx -> tok.value, "extern"))) {
        char is_extern = !strcmp(ctx -> tok.value, "extern");
        advance(ctx);
        if (ctx -> tok.type != TT_IDEN)
            lqdSyntaxError(ctx, "Expected identifier!");
        lqdToken func_name = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_LPAREN)
            lqdSyntaxError(ctx, "Expected '('!");
        advance(ctx);
        lqdParameterArray* params = lqdParameterArray_new(1);
        if (ctx -> tok.type != TT_RPAREN) {
            do {
                if (ctx -> tok.type == TT_COMMA)advance(ctx);
                lqdType param_type = type(ctx);
                if (ctx -> tok.type != TT_IDEN)
                    lqdSyntaxError(ctx, "Expected identifier!");
                lqdToken param_name = ctx -> tok;
                advance(ctx);
                lqdParameter param = {param_type, param_name};
                lqdParameterArray_push(params, param);
                if (ctx -> tok.type != TT_COMMA && ctx -> tok.type != TT_RPAREN)
                    lqdSyntaxError(ctx, "Expected ',' or ')'!");
            } while (ctx -> tok.type == TT_COMMA);
        }
        if (ctx -> tok.type != TT_RPAREN)
            lqdSyntaxError(ctx, "Expected ')'!");
        advance(ctx);
        char is_void = 1;
        lqdType ret_type;
        if (ctx -> tok.type == TT_SUB) {
            advance(ctx);
            if (ctx -> tok.type != TT_GT)
                lqdSyntaxError(ctx, "Expected '>' (to finish the '->')!");
            advance(ctx);
            ret_type = type(ctx);
            is_void = 0;
        }
        lqdASTNode stmnt = statement(ctx);
        lqdFuncDeclNode* func_decl = malloc(sizeof(lqdFuncDeclNode));
        if (!is_void)
            func_decl -> ret_type = ret_type;
        func_decl -> name = func_name;
        func_decl -> params = params;
        func_decl -> is_void = is_void;
        func_decl -> statement = stmnt;
        func_decl -> is_extern = is_extern;
        lqdASTNode node = {NT_FuncDecl, func_decl};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "employ")) {
        advance(ctx);
        lqdTokenArray* lib_path = lqdTokenArray_new(1);
        do {
            if (ctx -> tok.type == TT_PERIOD)advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected library path!");
            lqdTokenArray_push(lib_path, ctx -> tok);
            advance(ctx);
        } while(ctx -> tok.type == TT_PERIOD);
        lqdToken alias;
        char is_aliased = 0;
        if (ctx -> tok.type == TT_SUB) {
            advance(ctx);
            if (ctx -> tok.type != TT_GT)
                lqdSyntaxError(ctx, "Expected '>' (to finish the '->')!");
            advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected identifier for library alias!");
            alias = ctx -> tok;
            advance(ctx);
            is_aliased = 1;
        }
        if (ctx -> tok.type != TT_SEMICOLON)
            lqdSyntaxError(ctx, "Expected ';'!");
        advance(ctx);
        lqdEmployNode* employment = malloc(sizeof(lqdEmployNode));
        employment -> lib_path = lib_path;
        employment -> is_aliased = is_aliased;
        if (is_aliased)
            employment -> alias = alias;
        lqdASTNode node = {NT_Employ, employment};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "if")) {
        char is_else = 0;
        lqdStatementsNode* conditions = lqdStatementsNode_new(1);
        lqdStatementsNode* bodies = lqdStatementsNode_new(1); 
        lqdASTNode else_case;
        char has_else_case = 0;
        do {
            is_else = 0;
            if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "else")) {
                advance(ctx);
                is_else = 1;
                has_else_case = 1;
            }
            if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "if")) {
                advance(ctx);
                lqdStatementsNode_push(conditions, expr(ctx));
                lqdStatementsNode_push(bodies, statement(ctx));
            } else if (is_else) {
                else_case = statement(ctx);
            }
        } while (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "else"));
        lqdIfNode* if_node = malloc(sizeof(lqdIfNode));
        if_node -> conditions = conditions;
        if_node -> bodies = bodies;
        if_node -> else_case = else_case;
        if_node -> has_else_case = has_else_case;
        lqdASTNode node = {NT_If, if_node};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "return")) {
        advance(ctx);
        if (ctx -> tok.type == TT_SEMICOLON) {
            advance(ctx);
            lqdReturnNode* ret = malloc(sizeof(lqdReturnNode));
            ret -> returns = 0;
            lqdASTNode node = {NT_Return, ret};
            return node;
        } else {
            lqdASTNode return_val = expr(ctx);
            if (ctx -> tok.type != TT_SEMICOLON)
                lqdSyntaxError(ctx, "Expected ';'");
            advance(ctx);
            lqdReturnNode* ret = malloc(sizeof(lqdReturnNode));
            ret -> returns = 1;
            ret -> return_value = return_val;
            lqdASTNode node = {NT_Return, ret};
            return node;
        }
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "continue")) {
        lqdContinueNode* cont = malloc(sizeof(lqdContinueNode));
        cont -> tok = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_SEMICOLON)
            lqdSyntaxError(ctx, "Expected ';'");
        advance(ctx);
        lqdASTNode node = {NT_Continue, cont};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "break")) {
        lqdBreakNode* brake = malloc(sizeof(lqdBreakNode));
        brake -> tok = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_SEMICOLON)
            lqdSyntaxError(ctx, "Expected ';'");
        advance(ctx);
        lqdASTNode node = {NT_Break, brake};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "for")) {
        advance(ctx);
        if (ctx -> tok.type != TT_IDEN)
            lqdSyntaxError(ctx, "Expected identifier!");
        lqdToken iterator = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_IDEN || strcmp(ctx -> tok.value, "in"))
            lqdSyntaxError(ctx, "Expected 'in'");
        advance(ctx);
        lqdTokenArray* arr_path = lqdTokenArray_new(1);
        do {
            if (ctx -> tok.type == TT_PERIOD)advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected identifier!");
            lqdTokenArray_push(arr_path, ctx -> tok);
            advance(ctx);
        } while (ctx -> tok.type == TT_PERIOD);
        lqdASTNode body = statement(ctx);
        lqdForNode* loop = malloc(sizeof(lqdForNode));
        loop -> iterator = iterator;
        loop -> arr = arr_path;
        loop -> body = body;
        lqdASTNode node = {NT_For, loop};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "while")) {
        advance(ctx);
        lqdWhileNode* loop = malloc(sizeof(lqdWhileNode));
        loop -> condition = expr(ctx);
        loop -> body = statement(ctx);
        lqdASTNode node = {NT_While, loop};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "struct")) {
        advance(ctx);
        if (ctx -> tok.type != TT_IDEN)
            lqdSyntaxError(ctx, "Expected identifier!");
        lqdToken name = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_LBRACE)
            lqdSyntaxError(ctx, "Expected '{'!");
        advance(ctx);
        lqdStatementsNode* statements = lqdStatementsNode_new(1);
        while (ctx -> tok.type != TT_RBRACE) {
            lqdStatementsNode_push(statements, statement(ctx));
            if (ctx -> tok.type == TT_EOF)
                lqdSyntaxError(ctx, "Reached EOF in code block!");
        }
        advance(ctx);
        lqdStructNode* structure = malloc(sizeof(lqdStructNode));
        structure -> name = name;
        structure -> body = statements;
        lqdASTNode node = {NT_Struct, structure};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && (peek(ctx).type == TT_PERIOD || peek(ctx).type == TT_LPAREN)) {
        lqdTokenArray* call_path = lqdTokenArray_new(1);
        do {
            if (ctx -> tok.type == TT_PERIOD)advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected identifier!");
            lqdTokenArray_push(call_path, ctx -> tok);
            advance(ctx);
        } while (ctx -> tok.type == TT_PERIOD);
        if (ctx -> tok.type == TT_EQ) {
            advance(ctx);
            lqdASTNode value = expr(ctx);
            if (ctx -> tok.type != TT_SEMICOLON)
                lqdSyntaxError(ctx, "Expected ';'");
            advance(ctx);
            lqdVarReassignNode* reassign = malloc(sizeof(lqdVarReassignNode));
            reassign -> var = call_path;
            reassign -> value = value;
            lqdASTNode node = {NT_VarReassign, reassign};
            return node;
        }
        if (ctx -> tok.type != TT_LPAREN)
            lqdSyntaxError(ctx, "Expected '('");
        advance(ctx);
        lqdStatementsNode* args = lqdStatementsNode_new(1);
        if (ctx -> tok.type != TT_RPAREN) do {
            if (ctx -> tok.type == TT_COMMA)advance(ctx);
            lqdStatementsNode_push(args, expr(ctx));
        } while (ctx -> tok.type == TT_COMMA);
        if (ctx -> tok.type != TT_RPAREN)
            lqdSyntaxError(ctx, "Expected ')'");
        advance(ctx);
        if (ctx -> tok.type != TT_SEMICOLON)
            lqdSyntaxError(ctx, "Expected ';'");
        advance(ctx);
        lqdFuncCallNode* func_call = malloc(sizeof(lqdFuncCallNode));
        func_call -> call_path = call_path;
        func_call -> args = args;
        lqdASTNode node = {NT_FuncCall, func_call};
        return node;
    } else if (ctx -> tok.type == TT_IDEN) {
        if (peek(ctx).type == TT_EQ) {
            lqdTokenArray* var_path = lqdTokenArray_new(1);
            lqdTokenArray_push(var_path, ctx -> tok);
            advance(ctx);
            advance(ctx);
            lqdASTNode new_value = expr(ctx);
            if (ctx -> tok.type != TT_SEMICOLON)
                lqdSyntaxError(ctx, "Expected ';'");
            lqdVarReassignNode* reassign = malloc(sizeof(lqdVarReassignNode));
            reassign -> var = var_path;
            reassign -> value = new_value;
            lqdASTNode node = {NT_VarReassign, reassign};
            return node;
        }
        lqdType var_type = type(ctx);
        if (ctx -> tok.type != TT_IDEN)
            lqdSyntaxError(ctx, "Expected identifier!");
        lqdToken var_name = ctx -> tok;
        advance(ctx);
        if (ctx -> tok.type != TT_SEMICOLON && ctx -> tok.type != TT_EQ)
            lqdSyntaxError(ctx, "Expected '=' or ';'!");
        if (ctx -> tok.type == TT_SEMICOLON) {
            advance(ctx);
            lqdVarDeclNode* var_decl = malloc(sizeof(lqdVarDeclNode));
            var_decl -> type = var_type;
            var_decl -> name = var_name;
            var_decl -> initialized = 0;
            lqdASTNode node = {NT_VarDecl, var_decl};
            return node;
        } else {
            advance(ctx);
            lqdASTNode value = expr(ctx);
            if (ctx -> tok.type != TT_SEMICOLON)
                lqdSyntaxError(ctx, "Expected ';'!");
            advance(ctx);
            lqdVarDeclNode* var_decl = malloc(sizeof(lqdVarDeclNode));
            var_decl -> type = var_type;
            var_decl -> name = var_name;
            var_decl -> initialized = 1;
            var_decl -> initializer = value;
            lqdASTNode node = {NT_VarDecl, var_decl};
            return node;
        }
    } else if (ctx -> tok.type == TT_SEMICOLON) {
        advance(ctx);
        lqdASTNode node = {NT_NoStmnt, NULL};
        return node;
    } else if (ctx -> tok.type == TT_LBRACE) {
        advance(ctx);
        lqdStatementsNode* statements = lqdStatementsNode_new(1);
        while (ctx -> tok.type != TT_RBRACE) {
            lqdStatementsNode_push(statements, statement(ctx));
            if (ctx -> tok.type == TT_EOF)
                lqdSyntaxError(ctx, "Reached EOF in code block!");
        }
        advance(ctx);
        lqdASTNode node = {NT_Statements, statements};
        return node;
    }
    lqdSyntaxError(ctx, "Expected statement!");
    return null_node;
}

lqdASTNode factor(lqdParserCtx* ctx) {
    if (ctx -> tok.type == TT_INT || ctx -> tok.type == TT_FLOAT) {
        lqdNumberNode* num = malloc(sizeof(lqdNumberNode));
        num -> tok = ctx -> tok;
        advance(ctx);
        lqdASTNode node = {NT_Number, num};
        return node;
    } else if (ctx -> tok.type == TT_SUB) { // Currently the only unary op
        lqdToken op = ctx -> tok;
        advance(ctx);
        lqdASTNode value = factor(ctx);
        lqdUnaryOpNode* unary = malloc(sizeof(lqdUnaryOpNode));
        unary -> op = op;
        unary -> value = value;
        lqdASTNode node = {NT_Unary, unary};
        return node;
    } else if (ctx -> tok.type == TT_LPAREN) {
        advance(ctx);
        lqdASTNode node = expr(ctx);
       
        if (ctx -> tok.type != TT_RPAREN) {
            lqdSyntaxError(ctx, "Expected ')'!");
        }
        advance(ctx);
        return node;
    } else if (ctx -> tok.type == TT_IDEN && !strcmp(ctx -> tok.value, "new")) {
        advance(ctx);
        lqdTokenArray* struct_path = lqdTokenArray_new(1);
        do {
            if (ctx -> tok.type == TT_PERIOD)advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected identifier!");
            lqdTokenArray_push(struct_path, ctx -> tok);
            advance(ctx);
        } while (ctx -> tok.type == TT_PERIOD);
        if (ctx -> tok.type != TT_LPAREN)
            lqdSyntaxError(ctx, "Expected '('");
        advance(ctx);
        lqdStatementsNode* args = lqdStatementsNode_new(1);
        if (ctx -> tok.type == TT_RPAREN) {
            advance(ctx);
            lqdConstructorNode* new = malloc(sizeof(lqdConstructorNode));
            new -> struct_path = struct_path;
            new -> args = args;
            lqdASTNode node = {NT_Construct, new};
            return node;
        }
        do {
            if (ctx -> tok.type == TT_COMMA)advance(ctx);
            lqdStatementsNode_push(args, expr(ctx));
        } while (ctx -> tok.type == TT_COMMA);
        if (ctx -> tok.type != TT_RPAREN)
            lqdSyntaxError(ctx, "Expected ')'");
        advance(ctx);
        lqdConstructorNode* new = malloc(sizeof(lqdConstructorNode));
        new -> struct_path = struct_path;
        new -> args = args;
        lqdASTNode node = {NT_Construct, new};
        return node;
    } else if (ctx -> tok.type == TT_IDEN && (peek(ctx).type == TT_PERIOD || peek(ctx).type == TT_LPAREN)) {
        lqdTokenArray* call_path = lqdTokenArray_new(1);
        do {
            if (ctx -> tok.type == TT_PERIOD)advance(ctx);
            if (ctx -> tok.type != TT_IDEN)
                lqdSyntaxError(ctx, "Expected identifier!");
            lqdTokenArray_push(call_path, ctx -> tok);
            advance(ctx);
        } while (ctx -> tok.type == TT_PERIOD);
        if (ctx -> tok.type != TT_LPAREN) {
            lqdVarAccessNode* var = malloc(sizeof(lqdVarAccessNode));
            var -> path = call_path;
            lqdASTNode slice;
            char has_slice = 0;
            if (ctx -> tok.type == TT_LBRACKET) {
                has_slice = 1;
                advance(ctx);
                slice = expr(ctx);
                if (ctx -> tok.type != TT_RBRACKET)
                    lqdSyntaxError(ctx, "Expected ']'");
                advance(ctx);
            }
            var -> slice = slice;
            var -> has_slice = has_slice;
            lqdASTNode node = {NT_VarAccess, var};
            return node;
        }
        advance(ctx);
        lqdStatementsNode* args = lqdStatementsNode_new(1);
        if (ctx -> tok.type != TT_RPAREN) do {
            if (ctx -> tok.type == TT_COMMA)advance(ctx);
            lqdStatementsNode_push(args, expr(ctx));
        } while (ctx -> tok.type == TT_COMMA);
        if (ctx -> tok.type != TT_RPAREN)
            lqdSyntaxError(ctx, "Expected ')'");
        advance(ctx);
        lqdFuncCallNode* func_call = malloc(sizeof(lqdFuncCallNode));
        func_call -> call_path = call_path;
        func_call -> args = args;
        lqdASTNode node = {NT_FuncCall, func_call};
        return node;
    } else if (ctx -> tok.type == TT_IDEN) {
        lqdTokenArray* path = lqdTokenArray_new(1);
        lqdTokenArray_push(path, ctx -> tok);
        advance(ctx);
        lqdASTNode slice;
        char has_slice = 0;
        if (ctx -> tok.type == TT_LBRACKET) {
            has_slice = 1;
            advance(ctx);
            slice = expr(ctx);
            if (ctx -> tok.type != TT_RBRACKET)
                lqdSyntaxError(ctx, "Expected ']'");
            advance(ctx);
        }
        lqdVarAccessNode* var = malloc(sizeof(lqdVarAccessNode));
        var -> path = path;
        var -> slice = slice;
        var -> has_slice = has_slice;
        lqdASTNode node = {NT_VarAccess, var};
        return node;
    } else if (ctx -> tok.type == TT_STR) {
        lqdStringNode* var = malloc(sizeof(lqdStringNode));
        var -> tok = ctx -> tok;
        advance(ctx);
        lqdASTNode node = {NT_String, var};
        return node;
    } else if (ctx -> tok.type == TT_CHAR) {
        lqdCharNode* var = malloc(sizeof(lqdCharNode));
        var -> tok = ctx -> tok;
        advance(ctx);
        lqdASTNode node = {NT_Char, var};
        return node;
    }
    lqdSyntaxError(ctx, "Expected int, float, '-', '(', or identifier!");
    return null_node;
}

lqdASTNode term(lqdParserCtx* ctx) {
    lqdASTNode left = factor(ctx);
    while (ctx -> tok.type == TT_MUL || ctx -> tok.type == TT_DIV) {
        lqdToken op = ctx -> tok;
        advance(ctx);
        lqdASTNode right = factor(ctx);
       
        lqdBinOpNode* binop = malloc(sizeof(lqdBinOpNode));
        binop -> left = left;
        binop -> op = op;
        binop -> right = right;
        left.type = NT_BinOp;
        left.node = binop;
    }
    return left;
}

lqdASTNode arith_expr(lqdParserCtx* ctx) {
    lqdASTNode left = term(ctx);
   
    while (ctx -> tok.type == TT_ADD || ctx -> tok.type == TT_SUB) {
        lqdToken op = ctx -> tok;
        advance(ctx);
        lqdASTNode right = term(ctx);
       
        lqdBinOpNode* binop = malloc(sizeof(lqdBinOpNode));
        binop -> left = left;
        binop -> op = op;
        binop -> right = right;
        left.type = NT_BinOp;
        left.node = binop;
    }
    return left;
}

lqdASTNode comp_expr(lqdParserCtx* ctx) {
    lqdASTNode left = arith_expr(ctx);
    while (ctx -> tok.type == TT_GT || ctx -> tok.type == TT_LT || ctx -> tok.type == TT_LTE || ctx -> tok.type == TT_GTE || ctx -> tok.type == TT_EQEQ || ctx -> tok.type == TT_NE) {
        lqdToken op = ctx -> tok;
        advance(ctx);
        lqdASTNode right = arith_expr(ctx);
        lqdBinOpNode* binop = malloc(sizeof(lqdBinOpNode));
        binop -> left = left;
        binop -> op = op;
        binop -> right = right;
        left.type = NT_BinOp;
        left.node = binop;
    }
    return left;
}

lqdASTNode expr(lqdParserCtx* ctx) {
    return comp_expr(ctx);
}

lqdStatementsNode* parse(lqdTokenArray* tokens, char* code, char* filename) {
    lqdParserCtx* ctx = malloc(sizeof(lqdParserCtx));
    ctx -> toks = tokens;
    ctx -> tok_idx = 0;
    ctx -> filename = filename;
    ctx -> code = code;
    advance(ctx);
    lqdStatementsNode* statements = lqdStatementsNode_new(1);
    while (ctx -> tok.type != TT_EOF)
        lqdStatementsNode_push(statements, statement(ctx));
    free(ctx);
    return statements;
}