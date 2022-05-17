#include "compiler.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char** defined_funcs;
    int* func_params;
    int defined_funcs_vals;
    int defined_funcs_size;
    
    char** defined_vars;
    char** var_types;
    int defined_vars_vals;
    int defined_vars_size;
    char* var_init;

    uint64_t local_vars;

    char* code;
    char* filename;

    char* section_bss;
    char* section_data;

    uint64_t defined_strings;

    void* parent_ctx;
    char is_child_ctx;

    uint64_t comparison;
    uint64_t header; // How many bytes (divided by 8) to offset variable accesses by
} lqdCompilerContext;

lqdCompilerContext* create_context(char* code, char* filename, char is_child, uint64_t context_header) {
    lqdCompilerContext* ctx = malloc(sizeof(lqdCompilerContext));
    ctx -> code = code;
    ctx -> filename = filename;
    ctx -> defined_funcs = malloc(4 * sizeof(char*));
    ctx -> func_params = malloc(4 * sizeof(int));
    ctx -> defined_funcs_size = 4;
    ctx -> defined_funcs_vals = 0;

    ctx -> defined_vars = malloc(4 * sizeof(char*));
    ctx -> var_types = malloc(4 * sizeof(char*));
    ctx -> var_init = malloc(4 * sizeof(char));
    ctx -> defined_vars_size = 4;
    ctx -> defined_vars_vals = 0;

    ctx -> local_vars = 0;

    ctx -> section_bss = malloc(1);
    ctx -> section_bss[0] = 0;
    ctx -> section_data = malloc(1);
    ctx -> section_data[0] = 0;
    ctx -> defined_strings = 0;

    ctx -> is_child_ctx = is_child;

    ctx -> comparison = 0;
    ctx -> header = context_header;
    return ctx;
}

void delete_context(lqdCompilerContext* ctx) {
    for (int i = 0; i < ctx -> defined_funcs_vals; i++)
        free(ctx -> defined_funcs[i]);
    for (int i = 0; i < ctx -> defined_vars_vals; i++) {
        free(ctx -> defined_vars[i]);
        free(ctx -> var_types[i]);
    }
    free(ctx -> defined_funcs);
    free(ctx -> section_bss);
    free(ctx -> section_data);
    free(ctx -> func_params);
    free(ctx -> var_init);
    free(ctx -> var_types);
    free(ctx -> defined_vars);
    free(ctx);
}

void lqdCompilerError(lqdCompilerContext* ctx, uint64_t line, int idx_start, int idx_end, char* message) {
    fprintf(stderr, "╭────────────────────[ Compiler Error ]────→\n");
    fprintf(stderr, "│ %s\n", message);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ ");
    int idx = 0;
    int i = 0;
    for (i = 0; i < idx_start; i++)
        if (ctx -> code[i] == '\n') idx = i;
    i = line == 1 ? 0 : 1;
    while (ctx -> code[idx + i] && ctx -> code[idx + i] != '\n') {
        fprintf(stderr, "%c", ctx -> code[idx + i]);
        i++;
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "│ ");
    for (i = line == 1 ? -1 : 0; i < (int)(idx_start - idx); i++)
        fprintf(stderr, " ");
    fprintf(stderr, "^");
    for (i = 0; i < idx_end - idx_start; i++)
        fprintf(stderr, "~");
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ Line: %ld, File: %s\n", line, ctx -> filename);
    fprintf(stderr, "╯\n");
    exit(1);
}

void define_func(lqdCompilerContext* ctx, char* name, int params) {
    if (ctx -> defined_funcs_vals == ctx -> defined_funcs_size) {
        ctx -> defined_funcs_size *= 2;
        ctx -> defined_funcs = realloc(ctx -> defined_funcs, ctx -> defined_funcs_size * sizeof(char*));
        ctx -> func_params = realloc(ctx -> func_params, ctx -> defined_funcs_size * sizeof(int));
    }
    ctx -> defined_funcs[ctx -> defined_funcs_vals] = malloc(strlen(name) + 1);
    ctx -> func_params[ctx -> defined_funcs_vals] = params;
    strcpy(ctx -> defined_funcs[ctx -> defined_funcs_vals], name);
    ctx -> defined_funcs_vals++;
}

void define_var(lqdCompilerContext* ctx, char* name, char* type, char initialized) {
    if (ctx -> defined_vars_vals == ctx -> defined_vars_size) {
        ctx -> defined_vars_size *= 2;
        ctx -> defined_vars = realloc(ctx -> defined_vars, ctx -> defined_vars_size * sizeof(char*));
        ctx -> var_types = realloc(ctx -> var_types, ctx -> defined_vars_size * sizeof(char*));
        ctx -> var_init = realloc(ctx -> var_init, ctx -> defined_vars_size);
    }
    ctx -> defined_vars[ctx -> defined_vars_vals] = malloc(strlen(name) + 1);
    ctx -> var_types[ctx -> defined_vars_vals] = malloc(strlen(type) + 1);
    ctx -> var_init[ctx -> defined_vars_vals] = initialized;
    strcpy(ctx -> defined_vars[ctx -> defined_vars_vals], name);
    strcpy(ctx -> var_types[ctx -> defined_vars_vals], type);
    ctx -> defined_vars_vals++;
    ctx -> local_vars++;
}

void strconcat(char** dst, char* src) {
    *dst = realloc(*dst, strlen(*dst) + strlen(src) + 2);
    strcat(*dst, src);
}

char** get_glob_section_data(lqdCompilerContext* ctx) {
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx)
        current_ctx = current_ctx -> parent_ctx;
    return &current_ctx -> section_data;
}
char** get_glob_section_bss(lqdCompilerContext* ctx) {
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx)
        current_ctx = current_ctx -> parent_ctx;
    return &current_ctx -> section_bss;
}

char* linux_x86_64_compile_statements(lqdStatementsNode* statements, lqdCompilerContext* ctx);
char* linux_x86_64_compile_stmnt(lqdASTNode statement, lqdCompilerContext* ctx, uint64_t context_header) {
    lqdStatementsNode* stmnts = lqdStatementsNode_new(1);
    lqdStatementsNode_push(stmnts, statement);
    char* code;
    if (statement.type == NT_Statements) {
        lqdCompilerContext* child_ctx = create_context(ctx -> code, ctx -> filename, 1, context_header);
        child_ctx -> parent_ctx = ctx;
        code = linux_x86_64_compile_statements(stmnts, child_ctx);
        delete_context(child_ctx);
    } else {
        code = linux_x86_64_compile_statements(stmnts, ctx);
    }
    free(stmnts -> statements);
    free(stmnts);
    return code;
}

char* linux_x86_64_NoStmnt() {
    return ""; // Only for lines like `while true;`
};
char* linux_x86_64_Number(lqdNumberNode* node, lqdCompilerContext* ctx) {
    char* num = malloc(1);
    num[0] = 0;
    strconcat(&num, "    push ");
    strconcat(&num, node -> tok.value);
    strconcat(&num, "\n");
    return num;
};
char* linux_x86_64_Char(lqdCharNode* node, lqdCompilerContext* ctx) {
    char* chr = malloc(1);
    chr[0] = 0;
    strconcat(&chr, "    push ");
    strconcat(&chr, node -> tok.value);
    strconcat(&chr, "\n");
    return chr;
};
char* linux_x86_64_String(lqdStringNode* node, lqdCompilerContext* ctx) {
    char* str_name = malloc(1);
    str_name[0] = 0;
    strconcat(&str_name, "str");
    char* tmp = malloc(8);
    lqdCompilerContext* top_lvl_ctx = ctx;
    while (top_lvl_ctx -> is_child_ctx)top_lvl_ctx = top_lvl_ctx -> parent_ctx;
    sprintf(tmp, "%li", top_lvl_ctx -> defined_strings++);
    strconcat(&str_name, tmp);
    free(tmp);
    char** section_data = get_glob_section_data(ctx);
    strconcat(section_data, "    ");
    strconcat(section_data, str_name);
    strconcat(section_data, ": db \"");
    strconcat(section_data, node -> tok.value);
    strconcat(section_data, "\"\n");
    char* ret_string = malloc(1);
    ret_string[0] = 0;
    strconcat(&ret_string, "    push ");
    strconcat(&ret_string, str_name);
    strconcat(&ret_string, "\n");
    free(str_name);
    return ret_string;
};
char* linux_x86_64_VarAccess(lqdVarAccessNode* node, lqdCompilerContext* ctx) {
    int var_idx = 0;
    int var_idx_real = 0;
    char is_bottom = 1;
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx) {
        for (uint64_t i = 0; i < current_ctx -> defined_vars_vals; i++)
            if (!strcmp(current_ctx -> defined_vars[i], node -> path -> tokens[0].value)) {
                var_idx = 1;
                var_idx_real += (int) i;
            }
        if (!var_idx) {
            var_idx_real -= ctx -> header;
            if (ctx -> header && is_bottom) { // I actually have no fucking clue why
                var_idx_real--; // this is needed, but without it
                is_bottom = 0; // clyqd doesn't work.
            }
            current_ctx = current_ctx -> parent_ctx;
        } else {
            break;
        }
    }
    if (!var_idx)
        lqdCompilerError(ctx, node -> path -> tokens[0].line, node -> path -> tokens[0].idx_start, node -> path -> tokens[node -> path -> values-1].idx_end, "Undefined variable!");
    char* var_access = malloc(1);
    var_access[0] = 0;
    strconcat(&var_access, "    mov rax, [");
    strconcat(&var_access, node -> path -> tokens[0].value);
    strconcat(&var_access, "]\n    push rax\n");
    return var_access;
};
char* linux_x86_64_BinOp(lqdBinOpNode* node, lqdCompilerContext* ctx) {
    char* binop = malloc(1);
    binop[0] = 0;
    char* tmp = linux_x86_64_compile_stmnt(node -> left, ctx, 0);
    strconcat(&binop, tmp);
    free(tmp);
    tmp = linux_x86_64_compile_stmnt(node -> right, ctx, 0);
    strconcat(&binop, tmp);
    free(tmp);
    strconcat(&binop, "    pop rax\n");
    strconcat(&binop, "    pop rbx\n");
    switch (node -> op.type) {
        case TT_ADD:
            strconcat(&binop, "    add rbx, rax\n");
            break;
        case TT_SUB:
            strconcat(&binop, "    sub rbx, rax\n");
            break;
        case TT_MUL:
            strconcat(&binop, "    xchg rax, rbx\n");
            strconcat(&binop, "    xor rdx, rdx\n");
            strconcat(&binop, "    mul rbx\n");
            strconcat(&binop, "    xchg rax, rbx\n");
            break;
        case TT_DIV:
            strconcat(&binop, "    xchg rax, rbx\n");
            strconcat(&binop, "    xor rdx, rdx\n");
            strconcat(&binop, "    div rbx\n");
            strconcat(&binop, "    xchg rax, rbx\n");
            break;
        default:
            strconcat(&binop, "    cmp rbx, rax\n");
            switch (node -> op.type) {
                case TT_EQEQ:
                    strconcat(&binop, "    je .cmp_success");
                    break;
                case TT_LT:
                    strconcat(&binop, "    jl .cmp_success");
                    break;
                case TT_GT:
                    strconcat(&binop, "    jg .cmp_success");
                    break;
                case TT_LTE:
                    strconcat(&binop, "    jle .cmp_success");
                    break;
                case TT_GTE:
                    strconcat(&binop, "    jge .cmp_success");
                    break;
                case TT_NE:
                    strconcat(&binop, "    jne .cmp_success");
                    break;
                default:
                    break;
            }
            char* tmp = malloc(8);
            sprintf(tmp, "%li", ctx -> comparison++);
            strconcat(&binop, tmp);
            strconcat(&binop, "\n    jmp .cmp_fail");
            strconcat(&binop, tmp);
            strconcat(&binop, "\n.cmp_success");
            strconcat(&binop, tmp);
            strconcat(&binop, ":\n    mov rbx, 1\n");
            strconcat(&binop, "    jmp .cmp_end");
            strconcat(&binop, tmp);
            strconcat(&binop, "\n.cmp_fail");
            strconcat(&binop, tmp);
            strconcat(&binop, ":\n    mov rbx, 0\n");
            strconcat(&binop, ".cmp_end");
            strconcat(&binop, tmp);
            strconcat(&binop, ":\n");
            free(tmp);
            break;
    }
    strconcat(&binop, "    push rbx\n");
    return binop;
};
char* linux_x86_64_Unary(lqdUnaryOpNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_VarDecl(lqdVarDeclNode* node, lqdCompilerContext* ctx) {
    define_var(ctx, node -> name.value, node -> type.type.value, node -> initialized);
    char* var_body = malloc(1);
    var_body[0] = 0;
    if (node -> initialized) {
        char* tmp = linux_x86_64_compile_stmnt(node -> initializer, ctx, 0);
        strconcat(&var_body, tmp);
        free(tmp);
    } else {
        strconcat(&var_body, "    add rsp, 8\n");
    }
    strconcat(get_glob_section_bss(ctx), "    ");
    strconcat(get_glob_section_bss(ctx), node -> name.value);
    strconcat(get_glob_section_bss(ctx), ": resq 1\n"); // Reserve 8 bytes, TODO: replace to match variable type
    strconcat(&var_body, "    pop rax\n");
    strconcat(&var_body, "    mov [");
    strconcat(&var_body, node -> name.value);
    strconcat(&var_body, "], rax\n");
    return var_body;
};
char* linux_x86_64_FuncDecl(lqdFuncDeclNode* node, lqdCompilerContext* ctx) {
    define_func(ctx, node -> name.value, node -> params -> values);
    char* func_body = malloc(1);
    func_body[0] = 0;
    if (node -> is_extern) {
        strconcat(&func_body, "extern ");
        strconcat(&func_body, node -> name.value);
        strconcat(&func_body, "\n");
        return func_body;
    }
    if (ctx -> is_child_ctx) {
        strconcat(&func_body, "    jmp .");
        strconcat(&func_body, node -> name.value);
        strconcat(&func_body, "_end\n.");
    }
    strconcat(&func_body, node -> name.value);
    strconcat(&func_body, ":\n");
    strconcat(&func_body, "    push rbp\n");
    strconcat(&func_body, "    mov rbp, rsp\n");
    char* tmp = linux_x86_64_compile_stmnt(node -> statement, ctx, 2);
    strconcat(&func_body, tmp);
    free(tmp);
    strconcat(&func_body, "    mov rsp, rbp\n");
    strconcat(&func_body, "    pop rbp\n");
    strconcat(&func_body, "    ret\n");
    if (ctx -> is_child_ctx) {
        strconcat(&func_body, ".");
        strconcat(&func_body, node -> name.value);
        strconcat(&func_body, "_end:\n");
    }
    return func_body;
};
char* linux_x86_64_Statements(lqdStatementsNode* node, lqdCompilerContext* ctx) {
    char* statements_body = malloc(1);
    statements_body[0] = 0;
    for (int i = 0; i < node -> values; i++) {
        char* stmnt = linux_x86_64_compile_stmnt(node -> statements[i], ctx, 0);
        strconcat(&statements_body, stmnt);
        free(stmnt);
    }
    return statements_body;
};
char* linux_x86_64_FuncCall(lqdFuncCallNode* node, lqdCompilerContext* ctx) {
    lqdchar found = -1;
    lqdchar is_nested = 0;
    lqdCompilerContext* current_ctx = ctx;
    if (!ctx -> is_child_ctx) {
        for (int i = 0; i < ctx -> defined_funcs_vals; i++)
            if (!strcmp(node -> call_path -> tokens[0].value, ctx -> defined_funcs[i]))
                found = i;
    } else {
        is_nested = 1;
        while (current_ctx -> is_child_ctx) {
            for (int i = 0; i < current_ctx -> defined_funcs_vals; i++)
                if (!strcmp(node -> call_path -> tokens[0].value, current_ctx -> defined_funcs[i]))
                    found = i;
            if (found == -1) {
                current_ctx = (lqdCompilerContext*) current_ctx -> parent_ctx;
                if (!current_ctx -> is_child_ctx) {
                    is_nested = 0;
                    for (int i = 0; i < current_ctx -> defined_funcs_vals; i++)
                        if (!strcmp(node -> call_path -> tokens[0].value, current_ctx -> defined_funcs[i]))
                            found = i;
                }
            } else {
                break;
            }
        }
    }
    if (found == -1)
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[0].idx_end, "Undefined function!");
    if ((int) node -> args -> values != current_ctx -> func_params[(int)found])
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[0].idx_end, "Invalid number of arguments!");
    char* call_body = malloc(1);
    call_body[0] = 0;
    for (int i = node -> args -> values - 1; i >= 0; i--) {
        char* tmp = linux_x86_64_compile_stmnt(node -> args -> statements[i], ctx, 0);
        strconcat(&call_body, tmp);
        free(tmp);
    }
    strconcat(&call_body, "    call ");
    if (is_nested)
        strconcat(&call_body, ".");
    strconcat(&call_body, node -> call_path -> tokens[0].value);
    strconcat(&call_body, "\n");
    return call_body;
};
char* linux_x86_64_If(lqdIfNode* node, lqdCompilerContext* ctx) {
    char* if_body = malloc(1);
    if_body[0] = 0;
    char* tmp = malloc(8);
    tmp[0] = 0;
    sprintf(tmp, "%li", ctx -> comparison++);
    for (int i = 0; i < node -> conditions -> values; i++) {
        strconcat(&if_body, ".if_case_");
        strconcat(&if_body, tmp);
        strconcat(&if_body, "_");
        char* tmp2 = malloc(12);
        tmp2[0] = 0;
        sprintf(tmp2, "%i", i);
        strconcat(&if_body, tmp2);
        strconcat(&if_body, ":\n");
        char* tmp3 = linux_x86_64_compile_stmnt(node -> conditions -> statements[i], ctx, 0);
        strconcat(&if_body, tmp3);
        strconcat(&if_body, "    pop rax\n");
        strconcat(&if_body, "    cmp rax, 1\n");
        strconcat(&if_body, "    jne .if_case_");
        strconcat(&if_body, tmp);
        strconcat(&if_body, "_");
        sprintf(tmp2, "%i", i+1);
        strconcat(&if_body, tmp2);
        strconcat(&if_body, "\n");
        free(tmp3);
        tmp3 = linux_x86_64_compile_stmnt(node -> bodies -> statements[i], ctx, 0);
        strconcat(&if_body, tmp3);
        strconcat(&if_body, "    jmp .if_");
        strconcat(&if_body, tmp);
        strconcat(&if_body, "_end\n");
        free(tmp3);
        free(tmp2);
    }
    strconcat(&if_body, ".if_case_");
    strconcat(&if_body, tmp);
    strconcat(&if_body, "_");
    char* tmp2 = malloc(8);
    tmp2[0] = 0;
    sprintf(tmp2, "%li", node -> conditions -> values);
    strconcat(&if_body, tmp2);
    strconcat(&if_body, ":\n");
    if (node -> has_else_case) {
        char* else_case = linux_x86_64_compile_stmnt(node -> else_case, ctx, 0);
        strconcat(&if_body, else_case);
        free(else_case);
    }
    strconcat(&if_body, ".if_");
    strconcat(&if_body, tmp);
    strconcat(&if_body, "_end:\n");
    free(tmp);
    free(tmp2);
    return if_body;
};
char* linux_x86_64_For(lqdForNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_While(lqdWhileNode* node, lqdCompilerContext* ctx) {
    char* while_body = malloc(1);
    while_body[0] = 0;
    char* tmp = malloc(8);
    tmp[0] = 0;
    sprintf(tmp, "%li", ctx -> comparison++);
    strconcat(&while_body, "    jmp .while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "_end\n");
    strconcat(&while_body, ".while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, ":\n");
    char* tmp2 = linux_x86_64_compile_stmnt(node -> body, ctx, 0);
    strconcat(&while_body, tmp2);
    free(tmp2);
    strconcat(&while_body, ".while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "_end:\n");
    tmp2 = linux_x86_64_compile_stmnt(node -> condition, ctx, 0);
    strconcat(&while_body, tmp2);
    strconcat(&while_body, "    pop rax\n");
    strconcat(&while_body, "    cmp rax, 1\n");
    strconcat(&while_body, "    je .while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "\n");
    free(tmp2);
    free(tmp);
    return while_body;
};
char* linux_x86_64_Struct(lqdStructNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_VarReassign(lqdVarReassignNode* node, lqdCompilerContext* ctx) {
    int var_idx = 0;
    int var_idx_real = 0;
    lqdchar is_bottom = 1;
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx) { /* TODO: Clean up this loop */
        for (uint64_t i = 0; i < current_ctx -> defined_vars_vals; i++)
            if (!strcmp(current_ctx -> defined_vars[i], node -> var -> tokens[0].value)) {
                var_idx = 1;
                var_idx_real += (int) i;
            }
        if (!var_idx) {
            var_idx_real -= ctx -> header;
            if (ctx -> header && is_bottom) {
                var_idx_real--;
                is_bottom = 0;
            }
            current_ctx = current_ctx -> parent_ctx;
        } else {
            break;
        }
    }
    if (!var_idx)
        lqdCompilerError(ctx, node -> var -> tokens[0].line, node -> var -> tokens[0].idx_start, node -> var -> tokens[node -> var -> values-1].idx_end, "Undefined variable!");
    char* var_reassign = malloc(1);
    var_reassign[0] = 0;
    char* tmp2 = linux_x86_64_compile_stmnt(node -> value, ctx, 0);
    strconcat(&var_reassign, tmp2);
    free(tmp2);
    strconcat(&var_reassign, "    pop rax\n");
    strconcat(&var_reassign, "    mov [");
    strconcat(&var_reassign, node -> var -> tokens[0].value);
    strconcat(&var_reassign, "], rax\n");
    return var_reassign;
};
char* linux_x86_64_Return(lqdReturnNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_Continue(lqdContinueNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_Break(lqdBreakNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_Construct(lqdConstructorNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* linux_x86_64_Employ(lqdEmployNode* node, lqdCompilerContext* ctx) {
    return "";
};

char* linux_x86_64_compile_statements(lqdStatementsNode* statements, lqdCompilerContext* ctx) {
    char* section_text = malloc(1);
    section_text[0] = 0;
    for (uint64_t i = 0; i < statements -> values; i++) {
        switch (statements -> statements[i].type) {
            case NT_NoStmnt:
                strconcat(&section_text, linux_x86_64_NoStmnt());
                break;
            case NT_Number: {
                char* num = linux_x86_64_Number((lqdNumberNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, num);
                free(num);
                break;
            }
            case NT_Char: {
                char* chr = linux_x86_64_Char((lqdCharNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, chr);
                free(chr);
                break;
            }
            case NT_String: {
                char* str = linux_x86_64_String((lqdStringNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, str);
                free(str);
                break;
            }
            case NT_VarAccess: {
                char* var_access = linux_x86_64_VarAccess((lqdVarAccessNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, var_access);
                free(var_access);
                break;
            }
            case NT_BinOp: {
                char* op = linux_x86_64_BinOp((lqdBinOpNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, op);
                free(op);
                break;
            }
            case NT_Unary:
                strconcat(&section_text, linux_x86_64_Unary((lqdUnaryOpNode*)statements -> statements[i].node, ctx));
                break;
            case NT_VarDecl: {
                char* var_decl = linux_x86_64_VarDecl((lqdVarDeclNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, var_decl);
                free(var_decl);
                break;
            }
            case NT_FuncDecl: {
                char* decl = linux_x86_64_FuncDecl((lqdFuncDeclNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, decl);
                free(decl);
                break;
            }
            case NT_Statements: {
                char* stmnts = linux_x86_64_Statements((lqdStatementsNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, stmnts);
                free(stmnts);
                break;
            }
            case NT_FuncCall: {
                /* TODO: dynamic calling comvention*/
                char* call = linux_x86_64_FuncCall((lqdFuncCallNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, call);
                free(call);
                break;
            }
            case NT_If: {
                char* if_body = linux_x86_64_If((lqdIfNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, if_body);
                free(if_body);
                break;
            }
            case NT_For:
                strconcat(&section_text, linux_x86_64_For((lqdForNode*)statements -> statements[i].node, ctx));
                break;
            case NT_While: {
                char* while_body = linux_x86_64_While((lqdWhileNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, while_body);
                free(while_body);
                break;
            }
            case NT_Struct:
                strconcat(&section_text, linux_x86_64_Struct((lqdStructNode*)statements -> statements[i].node, ctx));
                break;
            case NT_VarReassign: {
                char* reassign_body = linux_x86_64_VarReassign((lqdVarReassignNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, reassign_body);
                free(reassign_body);
                break;
            }
            case NT_Return:
                strconcat(&section_text, linux_x86_64_Return((lqdReturnNode*)statements -> statements[i].node, ctx));
                break;
            case NT_Continue:
                strconcat(&section_text, linux_x86_64_Continue((lqdContinueNode*)statements -> statements[i].node, ctx));
                break;
            case NT_Break:
                strconcat(&section_text, linux_x86_64_Break((lqdBreakNode*)statements -> statements[i].node, ctx));
                break;
            case NT_Construct:
                strconcat(&section_text, linux_x86_64_Construct((lqdConstructorNode*)statements -> statements[i].node, ctx));
                break;
            case NT_Employ:
                strconcat(&section_text, linux_x86_64_Employ((lqdEmployNode*)statements -> statements[i].node, ctx));
                break;
            default:
                break;
        }
    }
    return section_text;
}

char* linux_x86_64_compile(lqdStatementsNode* statements, char* code, char* filename) {
    char* section_text = malloc(1);
    lqdCompilerContext* ctx = create_context(code, filename, 0, 0);
    section_text[0] = 0;
    
    char* _code = linux_x86_64_compile_statements(statements, ctx);
    strconcat(&section_text, _code);
    free(_code);

    char* final_code = malloc(1);
    final_code[0] = 0;
    char* section_data = malloc(1);
    section_data[0] = 0;
    char* section_bss = malloc(1);
    section_bss[0] = 0;
    strconcat(&section_data, "section .data\n");
    strconcat(&section_data, ctx -> section_data);
    strconcat(&final_code, section_data);
    strconcat(&section_bss, "section .bss\n");
    strconcat(&section_bss, ctx -> section_bss);
    strconcat(&final_code, section_bss);
    strconcat(&final_code, "section .text\n");
    strconcat(&final_code, "global _start\n");
    strconcat(&final_code, "_start:\n");
    strconcat(&final_code, "    call main\n");
    strconcat(&final_code, "    mov rax, 60\n");
    strconcat(&final_code, "    mov rdi, 0\n");
    strconcat(&final_code, "    syscall\n");
    strconcat(&final_code, section_text);

    delete_context(ctx);
    free(section_text);
    free(section_data);
    free(section_bss);

    return final_code;
}