#include "compiler.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char** defined_funcs;
    lqdParameterArray** func_params;
    int defined_funcs_vals;
    int defined_funcs_size;
    char* is_extern_func;
    
    char** defined_vars;
    char** var_types;
    int defined_vars_vals;
    int defined_vars_size;
    char* var_init;

    char** defined_libs;
    void** libraries;
    int libraries_vals;
    int libraries_size;
} lqdNamespace;

typedef struct {
    lqdNamespace* namespace;

    uint64_t local_vars;

    char* code;
    char* filename;

    char* section_bss;
    char* section_data;

    uint64_t defined_arrays;

    void* parent_ctx;
    char is_child_ctx;

    int id;
    int child_contexts;

    char is_lib;
    char* lib_name;

    char* for_end;
    char* while_end;
    char* true_for_end;
    char* true_while_end;

    uint64_t comparison;

    char returns;
} lqdCompilerContext;

lqdCompilerContext* create_context(char* code, char* filename, char is_child, uint64_t id) {
    lqdCompilerContext* ctx = malloc(sizeof(lqdCompilerContext));
    ctx -> code = code;
    ctx -> filename = filename;
    ctx -> namespace = malloc(sizeof(lqdNamespace));
    ctx -> namespace -> defined_funcs = malloc(4 * sizeof(char*));
    ctx -> namespace -> func_params = malloc(4 * sizeof(lqdParameterArray*));
    ctx -> namespace -> defined_funcs_size = 4;
    ctx -> namespace -> defined_funcs_vals = 0;
    ctx -> namespace -> is_extern_func = malloc(4);

    ctx -> namespace -> defined_vars = malloc(4 * sizeof(char*));
    ctx -> namespace -> var_types = malloc(4 * sizeof(char*));
    ctx -> namespace -> var_init = malloc(4 * sizeof(char));
    ctx -> namespace -> defined_vars_size = 4;
    ctx -> namespace -> defined_vars_vals = 0;

    ctx -> namespace -> defined_libs = malloc(4 * sizeof(char*));
    ctx -> namespace -> libraries = malloc(4 * sizeof(lqdNamespace*));
    ctx -> namespace -> libraries_size = 4;
    ctx -> namespace -> libraries_vals = 0;

    ctx -> local_vars = 0;

    ctx -> section_bss = malloc(1);
    ctx -> section_bss[0] = 0;
    ctx -> section_data = malloc(1);
    ctx -> section_data[0] = 0;
    ctx -> defined_arrays = 0;

    ctx -> is_child_ctx = is_child;

    ctx -> is_lib = 0;

    ctx -> comparison = 0;
    ctx -> id = id;
    ctx -> child_contexts = 0;
    return ctx;
}

char is_defined(lqdCompilerContext* ctx, char* name) {
    char exists = 0;
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx) {
        for (uint64_t i = 0; i < current_ctx -> namespace -> defined_vars_vals; i++) {
            if (!strcmp(current_ctx -> namespace -> defined_vars[i], name)) {
                exists = current_ctx -> id;
            }
        }
        if (!exists) {
            current_ctx = current_ctx -> parent_ctx;
        } else {
            break;
        }
    }
    return exists;
}

char is_defined_func(lqdCompilerContext* ctx, lqdTokenArray* path) {
    char exists = -1;
    char is_nested = 1;
    char* root = path -> tokens[0].value;
    int idx = 0;
    lqdCompilerContext* current_ctx = ctx;
    uint64_t vals = path -> values == 1 ? current_ctx -> namespace -> defined_funcs_vals : current_ctx -> namespace -> libraries_vals;
    char** defs = path -> values == 1 ? current_ctx -> namespace -> defined_funcs : current_ctx -> namespace -> defined_libs;
    while (current_ctx -> is_child_ctx) {
        for (uint64_t i = 0; i < vals; i++) {
            if (!strcmp(defs[i], root)) {
                exists = 1;
                idx = i;
            }
        }
        if (exists == -1) {
            current_ctx = current_ctx -> parent_ctx;
            vals = path -> values == 1 ? current_ctx -> namespace -> defined_funcs_vals : current_ctx -> namespace -> libraries_vals;
            defs = path -> values == 1 ? current_ctx -> namespace -> defined_funcs : current_ctx -> namespace -> defined_libs;
            if (!current_ctx -> is_child_ctx) {
                is_nested = 0;
                for (uint64_t i = 0; i < vals; i++) {
                    if (!strcmp(defs[i], root)) {
                        exists = 1;
                        idx = i;
                    }
                }
            }
        } else {
            break;
        }
    }
    char is_extern = current_ctx -> namespace -> is_extern_func[idx] * 2;
    if (path -> values > 1) {
        is_nested = 0;
        lqdNamespace* currentns = current_ctx -> namespace -> libraries[idx];
        char found = 0;
        int i = 1;
        while (i != path -> values - 1) {
            for (uint64_t j = 0; j < currentns -> libraries_vals; j++) {
                if (!strcmp(currentns -> defined_libs[j], path -> tokens[i].value)) {
                    found = 1;
                    currentns = currentns -> libraries[j];
                }
            }
            if (!found)
                return 0;
            i++;
        }
        for (uint64_t j = 0; j < currentns -> defined_funcs_vals; j++) {
            if (!strcmp(currentns -> defined_funcs[j], path -> tokens[i].value)) {
                found = 1;
                is_extern = currentns -> is_extern_func[j] * 2;
            }
        }
        if (!found)
            exists = -3;
    }
    return exists + is_nested + is_extern;
}

void delete_namespace(lqdNamespace* ns) {
    for (int i = 0; i < ns -> defined_funcs_vals; i++)
        free(ns -> defined_funcs[i]);
    for (int i = 0; i < ns -> defined_vars_vals; i++) {
        free(ns -> defined_vars[i]);
        free(ns -> var_types[i]);
    }
    for (int i = 0; i < ns -> libraries_vals; i++) {
        free(ns -> defined_libs[i]);
        delete_namespace(ns -> libraries[i]);
    }
    free(ns -> defined_funcs);
    free(ns -> func_params);
    free(ns -> var_init);
    free(ns -> var_types);
    free(ns -> defined_vars);
    free(ns -> libraries);
    free(ns -> defined_libs);
    free(ns -> is_extern_func);
    free(ns);
}

void delete_context(lqdCompilerContext* ctx) {
    delete_namespace(ctx -> namespace);
    if (ctx -> is_lib)
        free(ctx -> lib_name);
    free(ctx -> section_bss);
    free(ctx -> section_data);
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

void define_lib(lqdCompilerContext* ctx, char* name) {
    if (ctx -> namespace -> libraries_vals == ctx -> namespace -> libraries_size) {
        ctx -> namespace -> libraries_size *= 2;
        ctx -> namespace -> libraries = realloc(ctx -> namespace -> libraries, ctx -> namespace -> libraries_size * sizeof(lqdNamespace*));
        ctx -> namespace -> defined_libs = realloc(ctx -> namespace -> defined_libs, ctx -> namespace -> libraries_size * sizeof(char*));
    }
    ((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals] = (lqdNamespace*) malloc(sizeof(lqdNamespace));
    ctx -> namespace -> defined_libs[ctx -> namespace -> libraries_vals] = malloc(strlen(name) + 1);
    strcpy(ctx -> namespace -> defined_libs[ctx -> namespace -> libraries_vals], name);

    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_funcs = malloc(4 * sizeof(char*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).func_params = malloc(4 * sizeof(lqdParameterArray*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_funcs_size = 4;
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_funcs_vals = 0;
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).is_extern_func = malloc(4);

    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_vars = malloc(4 * sizeof(char*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).var_types = malloc(4 * sizeof(char*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).var_init = malloc(4 * sizeof(char));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_vars_size = 4;
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_vars_vals = 0;

    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).defined_libs = malloc(4 * sizeof(char*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).libraries = malloc(4 * sizeof(lqdNamespace*));
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).libraries_size = 4;
    (*(((lqdNamespace**)ctx -> namespace -> libraries)[ctx -> namespace -> libraries_vals])).libraries_vals = 0;

    ctx -> namespace -> libraries_vals++;
}

void define_func(lqdNamespace* namespace, char* name, lqdParameterArray* params, char is_extern) {
    if (namespace -> defined_funcs_vals == namespace -> defined_funcs_size) {
        namespace -> defined_funcs_size *= 2;
        namespace -> defined_funcs = realloc(namespace -> defined_funcs, namespace -> defined_funcs_size * sizeof(char*));
        namespace -> func_params = realloc(namespace -> func_params, namespace -> defined_funcs_size * sizeof(lqdParameterArray*));
        namespace -> is_extern_func = realloc(namespace -> is_extern_func, namespace -> defined_funcs_size);
    }
    namespace -> defined_funcs[namespace -> defined_funcs_vals] = malloc(strlen(name) + 1);
    namespace -> func_params[namespace -> defined_funcs_vals] = params;
    namespace -> is_extern_func[namespace -> defined_funcs_vals] = is_extern;
    strcpy(namespace -> defined_funcs[namespace -> defined_funcs_vals], name);
    namespace -> defined_funcs_vals++;
}

void define_var(lqdCompilerContext* ctx, char* name, char* type, char initialized) {
    if (ctx -> namespace -> defined_vars_vals == ctx -> namespace -> defined_vars_size) {
        ctx -> namespace -> defined_vars_size *= 2;
        ctx -> namespace -> defined_vars = realloc(ctx -> namespace -> defined_vars, ctx -> namespace -> defined_vars_size * sizeof(char*));
        ctx -> namespace -> var_types = realloc(ctx -> namespace -> var_types, ctx -> namespace -> defined_vars_size * sizeof(char*));
        ctx -> namespace -> var_init = realloc(ctx -> namespace -> var_init, ctx -> namespace -> defined_vars_size);
    }
    ctx -> namespace -> defined_vars[ctx -> namespace -> defined_vars_vals] = malloc(strlen(name) + 1);
    ctx -> namespace -> var_types[ctx -> namespace -> defined_vars_vals] = malloc(strlen(type) + 1);
    ctx -> namespace -> var_init[ctx -> namespace -> defined_vars_vals] = initialized;
    strcpy(ctx -> namespace -> defined_vars[ctx -> namespace -> defined_vars_vals], name);
    strcpy(ctx -> namespace -> var_types[ctx -> namespace -> defined_vars_vals], type);
    ctx -> namespace -> defined_vars_vals++;
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

char* x86_64_compile_statements(lqdStatementsNode* statements, lqdCompilerContext* ctx, char* namespace);
char* x86_64_compile_stmnt(lqdASTNode statement, lqdCompilerContext* ctx) {
    lqdStatementsNode* stmnts = lqdStatementsNode_new(1);
    lqdStatementsNode_push(stmnts, statement);
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    char* code;
    if (statement.type == NT_Statements) {
        lqdCompilerContext* child_ctx = create_context(ctx -> code, ctx -> filename, 1, ++cc -> child_contexts);
        child_ctx -> is_lib = ctx -> is_lib;
        if (ctx -> is_lib) {
            child_ctx -> lib_name = malloc(strlen(ctx -> lib_name) + 1);
            strcpy(child_ctx -> lib_name, ctx -> lib_name);
        }
        child_ctx -> parent_ctx = ctx;
        code = x86_64_compile_statements(stmnts, child_ctx, NULL);
        delete_context(child_ctx);
    } else {
        code = x86_64_compile_statements(stmnts, ctx, NULL);
    }
    free(stmnts -> statements);
    free(stmnts);
    return code;
}

char* x86_64_NoStmnt() {
    return ""; // Only for lines like `while true;`
};
char* x86_64_Number(lqdNumberNode* node, lqdCompilerContext* ctx) {
    char* num = malloc(1);
    num[0] = 0;
    strconcat(&num, "    push ");
    strconcat(&num, node -> tok.value);
    strconcat(&num, "\n");
    return num;
};
char* x86_64_Char(lqdCharNode* node, lqdCompilerContext* ctx) {
    char* chr = malloc(1);
    chr[0] = 0;
    strconcat(&chr, "    push ");
    strconcat(&chr, node -> tok.value);
    strconcat(&chr, "\n");
    return chr;
};
char* x86_64_Array(lqdArrayNode* node, lqdCompilerContext* ctx) {
    char* arr_body = malloc(1);
    arr_body[0] = 0;
    // Note to self bcuz I'm prolly gonna forget what this code does:
    //  - Allocate memory for the existing elements (times 2)
    //  - Put all the elements into the array
    //  - Create C-style size and value counters (uint64_t)
    lqdCompilerContext* top_lvl_ctx = ctx;
    while (top_lvl_ctx -> is_child_ctx)top_lvl_ctx = top_lvl_ctx -> parent_ctx;
    char* tmp = malloc(8);
    sprintf(tmp, "%li", ++top_lvl_ctx -> defined_arrays);
    char* tmp5 = malloc(8);
    sprintf(tmp5, "%li", node -> values -> values);
    strconcat(get_glob_section_bss(ctx), "    arr");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), ":");
    strconcat(get_glob_section_bss(ctx), "\n        arr");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "values: resq 1\n");
    strconcat(get_glob_section_bss(ctx), "        arr");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "size: resq 1\n");
    strconcat(get_glob_section_bss(ctx), "        arr");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "elements: resq ");
    char* tmp2 = malloc(8);
    sprintf(tmp2, "%li", node -> values -> size);
    strconcat(get_glob_section_bss(ctx), tmp2);
    strconcat(get_glob_section_bss(ctx), "\n");
    for (uint64_t i = 0; i < node -> values -> values; i++) {
        char* tmp3 = x86_64_compile_stmnt(node -> values -> statements[i], ctx);
        strconcat(&arr_body, tmp3);
        free(tmp3);
        strconcat(&arr_body, "    pop rax\n");
        strconcat(&arr_body, "    mov [arr");
        strconcat(&arr_body, tmp);
        strconcat(&arr_body, "elements+");
        char* tmp4 = malloc(8);
        tmp4[0] = 0;
        sprintf(tmp4, "%ld", i * 8);
        strconcat(&arr_body, tmp4);
        free(tmp4);
        strconcat(&arr_body, "], rax\n");
    }
    strconcat(&arr_body, "    mov rax, ");
    strconcat(&arr_body, tmp2);
    strconcat(&arr_body, "\n");
    strconcat(&arr_body, "    mov [arr");
    strconcat(&arr_body, tmp);
    strconcat(&arr_body, "size], rax\n");
    strconcat(&arr_body, "    mov rax, ");
    strconcat(&arr_body, tmp5);
    strconcat(&arr_body, "\n");
    strconcat(&arr_body, "    mov [arr");
    strconcat(&arr_body, tmp);
    strconcat(&arr_body, "values], rax\n");
    strconcat(&arr_body, "    push arr");
    strconcat(&arr_body, tmp);
    strconcat(&arr_body, "\n");
    free(tmp2);
    free(tmp5);
    free(tmp);
    return arr_body;
}
char* x86_64_String(lqdStringNode* node, lqdCompilerContext* ctx) {
    char* str_body = malloc(1);
    str_body[0] = 0;
    lqdCompilerContext* top_lvl_ctx = ctx;
    while (top_lvl_ctx -> is_child_ctx)top_lvl_ctx = top_lvl_ctx -> parent_ctx;
    char* tmp = malloc(21);
    sprintf(tmp, "%li", ++top_lvl_ctx -> defined_arrays);
    char* tmp5 = malloc(21);
    sprintf(tmp5, "%li", strlen(node -> tok.value));
    strconcat(get_glob_section_bss(ctx), "    str");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), ":");
    strconcat(get_glob_section_bss(ctx), "\n        str");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "values: resq 1\n");
    strconcat(get_glob_section_bss(ctx), "        str");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "size: resq 1\n");
    strconcat(get_glob_section_bss(ctx), "        str");
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), "elements: resq ");
    char* tmp2 = malloc(21);
    sprintf(tmp2, "%li", strlen(node -> tok.value));
    strconcat(get_glob_section_bss(ctx), tmp2);
    strconcat(get_glob_section_bss(ctx), "\n");
    for (uint64_t i = 0; i < strlen(node -> tok.value); i++) {
        strconcat(&str_body, "    mov rax, ");
        char* tmp6 = malloc(12);
        sprintf(tmp6, "%d", node -> tok.value[i]);
        strconcat(&str_body, tmp6);
        strconcat(&str_body, "\n");
        free(tmp6);
        strconcat(&str_body, "    mov [str");
        strconcat(&str_body, tmp);
        strconcat(&str_body, "elements+");
        char* tmp4 = malloc(8);
        tmp4[0] = 0;
        sprintf(tmp4, "%ld", i * 8);
        strconcat(&str_body, tmp4);
        free(tmp4);
        strconcat(&str_body, "], rax\n");
    }
    strconcat(&str_body, "    mov rax, ");
    strconcat(&str_body, tmp2);
    strconcat(&str_body, "\n");
    strconcat(&str_body, "    mov [str");
    strconcat(&str_body, tmp);
    strconcat(&str_body, "size], rax\n");
    strconcat(&str_body, "    mov rax, ");
    strconcat(&str_body, tmp5);
    strconcat(&str_body, "\n");
    strconcat(&str_body, "    mov [str");
    strconcat(&str_body, tmp);
    strconcat(&str_body, "values], rax\n");
    strconcat(&str_body, "    push str");
    strconcat(&str_body, tmp);
    strconcat(&str_body, "\n");
    free(tmp2);
    free(tmp5);
    free(tmp);
    return str_body;
};
char* x86_64_VarAccess(lqdVarAccessNode* node, lqdCompilerContext* ctx) {
    char id = is_defined(ctx, node -> path -> tokens[0].value);
    if (!id)
        lqdCompilerError(ctx, node -> path -> tokens[0].line, node -> path -> tokens[0].idx_start, node -> path -> tokens[node -> path -> values-1].idx_end, "Undefined variable!");
    char* var_access = malloc(1);
    var_access[0] = 0;
    strconcat(&var_access, "    mov rax, [");
    strconcat(&var_access, node -> path -> tokens[0].value);
    char* tmp = malloc(12);
    sprintf(tmp, "%i", id);
    strconcat(&var_access, tmp);
    strconcat(&var_access, "]\n    push rax\n");
    free(tmp);
    if (node -> has_slice) {
        strconcat(&var_access, "    pop rsi\n");
        char* tmp = x86_64_compile_stmnt(node -> slice, ctx);
        strconcat(&var_access, tmp);
        strconcat(&var_access, "    pop rax\n");
        strconcat(&var_access, "    xor rdx, rdx\n");
        strconcat(&var_access, "    mov rcx, 8\n");
        strconcat(&var_access, "    imul rcx\n");
        strconcat(&var_access, "    mov rbx, [rsi+16+rax]\n");
        strconcat(&var_access, "    push rbx\n");
        free(tmp);
    }
    return var_access;
};
char* x86_64_BinOp(lqdBinOpNode* node, lqdCompilerContext* ctx) {
    char* binop = malloc(1);
    binop[0] = 0;
    char* tmp = x86_64_compile_stmnt(node -> left, ctx);
    strconcat(&binop, tmp);
    free(tmp);
    tmp = x86_64_compile_stmnt(node -> right, ctx);
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
            strconcat(&binop, "    imul rbx\n");
            strconcat(&binop, "    xchg rax, rbx\n");
            break;
        case TT_DIV:
            // TODO: Use runtime implementation
            strconcat(&binop, "    xchg rax, rbx\n");
            strconcat(&binop, "    xor rdx, rdx\n");
            strconcat(&binop, "    idiv rbx\n");
            strconcat(&binop, "    xchg rax, rbx\n");
            break;
        case TT_MOD:
            // TODO: Use runtime implementation
            strconcat(&binop, "    xchg rax, rbx\n");
            strconcat(&binop, "    xor rdx, rdx\n");
            strconcat(&binop, "    idiv rbx\n");
            strconcat(&binop, "    mov rbx, rdx\n");
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
            lqdCompilerContext* cc = ctx;
            while (cc -> is_child_ctx && ((lqdCompilerContext*) ctx -> parent_ctx) -> is_child_ctx) cc = cc -> parent_ctx;
            sprintf(tmp, "%li", cc -> comparison++);
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
char* x86_64_Unary(lqdUnaryOpNode* node, lqdCompilerContext* ctx) {
    char* unary = malloc(1);
    unary[0] = 0;
    char* tmp = x86_64_compile_stmnt(node -> value, ctx);
    strconcat(&unary, tmp);
    free(tmp);
    strconcat(&unary, "    pop rax\n");
    strconcat(&unary, "    xor rdx, rdx\n");
    strconcat(&unary, "    mov rcx, -1\n");
    strconcat(&unary, "    imul rcx\n");
    strconcat(&unary, "    push rax\n");
    return unary;
};
char* x86_64_VarDecl(lqdVarDeclNode* node, lqdCompilerContext* ctx, char* namespace) {
    define_var(ctx, node -> name.value, node -> type.type.value, node -> initialized);
    char* var_body = malloc(1);
    var_body[0] = 0;
    if (node -> initialized) {
        char* tmp = x86_64_compile_stmnt(node -> initializer, ctx);
        strconcat(&var_body, tmp);
        free(tmp);
    } else {
        strconcat(&var_body, "    add rsp, 8\n");
    }
    strconcat(get_glob_section_bss(ctx), "    ");
    strconcat(get_glob_section_bss(ctx), node -> name.value);
    char* tmp = malloc(4);
    sprintf(tmp, "%i", ctx -> id);
    strconcat(get_glob_section_bss(ctx), tmp);
    strconcat(get_glob_section_bss(ctx), ": resq 1\n"); // Reserve 8 bytes, TODO: replace to match variable type
    strconcat(&var_body, "    pop rax\n");
    strconcat(&var_body, "    mov [");
    strconcat(&var_body, node -> name.value);
    strconcat(&var_body, tmp);
    free(tmp);
    strconcat(&var_body, "], rax\n");
    return var_body;
};
char* x86_64_FuncDecl(lqdFuncDeclNode* node, lqdCompilerContext* ctx, char* namespace) {
    define_func(ctx -> namespace, node -> name.value, node -> params, node -> is_extern);
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
    if (namespace != NULL) {
        strconcat(&func_body, namespace);
        strconcat(&func_body, "_");
    }
    strconcat(&func_body, node -> name.value);
    strconcat(&func_body, ":\n");
    strconcat(&func_body, "    push rbp\n");
    strconcat(&func_body, "    mov rbp, rsp\n");
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    char id = ++cc -> child_contexts;
    char* id_as_str = malloc(12);
    sprintf(id_as_str, "%i", id);
    lqdStatementsNode* stmnts = lqdStatementsNode_new(1);
    lqdStatementsNode_push(stmnts, node -> statement);
    char* code;
    if (node -> statement.type == NT_Statements) {
        lqdCompilerContext* child_ctx = create_context(ctx -> code, ctx -> filename, 1, id);
        child_ctx -> is_lib = ctx -> is_lib;
        if (ctx -> is_lib) {
            child_ctx -> lib_name = malloc(strlen(ctx -> lib_name) + 1);
            strcpy(child_ctx -> lib_name, ctx -> lib_name);
        }
        for (uint64_t i = 0; i < node -> params -> values; i++) {
            strconcat(get_glob_section_bss(ctx), "    ");
            strconcat(get_glob_section_bss(ctx), node -> params -> params[i].name.value);
            strconcat(get_glob_section_bss(ctx), id_as_str);
            strconcat(get_glob_section_bss(ctx), ": resq 1\n");

            char* tmp = malloc(8);
            sprintf(tmp, "%li", (i + 2) * 8);
            strconcat(&func_body, "    mov rax, [rbp+");
            strconcat(&func_body, tmp);
            strconcat(&func_body, "]\n");
            free(tmp);
            strconcat(&func_body, "    mov [");
            strconcat(&func_body, node -> params -> params[i].name.value);
            strconcat(&func_body, id_as_str);
            strconcat(&func_body, "], rax\n");

            define_var(child_ctx, node -> params -> params[i].name.value, "undefined", 1);
        }
        free(id_as_str);
        lqdStatementsNode* _stmnts = lqdStatementsNode_new(1);
        lqdStatementsNode_push(_stmnts, node -> statement);
        child_ctx -> parent_ctx = ctx;
        code = x86_64_compile_statements(_stmnts, child_ctx, NULL);
        delete_context(child_ctx);
        free(_stmnts -> statements);
        free(_stmnts);
    } else {
        lqdCompilerError(ctx, node -> name.line, node -> name.idx_start, node -> name.idx_end, "Function body must be a multi-line statement!");
    }
    free(stmnts -> statements);
    free(stmnts);
    strconcat(&func_body, code);
    free(code);
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
char* x86_64_Statements(lqdStatementsNode* node, lqdCompilerContext* ctx) {
    char* statements_body = malloc(1);
    statements_body[0] = 0;
    for (int i = 0; i < node -> values; i++) {
        char* stmnt = x86_64_compile_stmnt(node -> statements[i], ctx);
        strconcat(&statements_body, stmnt);
        free(stmnt);
    }
    return statements_body;
};
char* x86_64_FuncCall(lqdFuncCallNode* node, lqdCompilerContext* ctx) {
    char found = is_defined_func(ctx, node -> call_path);
    char is_nested = found == 2;
    char is_extern = found == 3;
    if (found == -1)
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[node -> call_path -> values - 1].idx_end, "Undefined function!");
    char* call_body = malloc(1);
    call_body[0] = 0;
    for (int i = node -> args -> values - 1; i >= 0; i--) {
        char* tmp = x86_64_compile_stmnt(node -> args -> statements[i], ctx);
        strconcat(&call_body, tmp);
        free(tmp);
    }
    strconcat(&call_body, "    call ");
    if (is_nested)
        strconcat(&call_body, ".");
    if (ctx -> is_lib && !is_extern) {
        strconcat(&call_body, ctx -> lib_name);
        strconcat(&call_body, "_");
    }
    for (int i = 0; i < node -> call_path -> values; i++) {
        strconcat(&call_body, node -> call_path -> tokens[i].value);
        if (i != node -> call_path -> values - 1)
            strconcat(&call_body, "_");
    }
    strconcat(&call_body, "\n");
    return call_body;
};
char* x86_64_If(lqdIfNode* node, lqdCompilerContext* ctx) {
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
        char* tmp3 = x86_64_compile_stmnt(node -> conditions -> statements[i], ctx);
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
        tmp3 = x86_64_compile_stmnt(node -> bodies -> statements[i], ctx);
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
        char* else_case = x86_64_compile_stmnt(node -> else_case, ctx);
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
char* x86_64_For(lqdForNode* node, lqdCompilerContext* ctx) {
    int exists = 0;
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx) {
        for (uint64_t i = 0; i < current_ctx -> namespace -> defined_vars_vals; i++)
            if (!strcmp(current_ctx -> namespace -> defined_vars[i], node -> arr -> tokens[0].value)) {
                exists = 1;
            }
        if (!exists) {
            current_ctx = current_ctx -> parent_ctx;
        } else {
            break;
        }
    }
    if (!exists)
        lqdCompilerError(ctx, node -> arr -> tokens[0].line, node -> arr -> tokens[0].idx_start, node -> arr -> tokens[node -> arr -> values-1].idx_end, "Undefined array!");
    char* for_body = malloc(1);
    for_body[0] = 0;
    char* tmp = malloc(20);
    sprintf(tmp, "%li", ctx -> comparison++);
    char* tmp2 = malloc(20);
    sprintf(tmp2, "%i", ctx -> id + 1);
    strconcat(get_glob_section_bss(ctx), "    ");
    strconcat(get_glob_section_bss(ctx), node -> iterator.value);
    strconcat(get_glob_section_bss(ctx), tmp2);
    strconcat(get_glob_section_bss(ctx), ": resq 1\n");
    strconcat(&for_body, "    mov rax, [");
    char id = is_defined(ctx, node -> arr -> tokens[0].value);
    if (!id)
        lqdCompilerError(ctx, node -> arr -> tokens[0].line, node -> arr -> tokens[0].idx_start, node -> arr -> tokens[0].idx_end, "Array not defined!");
    char* tmp3 = malloc(8);
    sprintf(tmp3, "%i", id);
    strconcat(&for_body, node -> arr -> tokens[0].value);
    strconcat(&for_body, tmp3);
    free(tmp3);
    strconcat(&for_body, "]\n");
    strconcat(&for_body, "    mov r8, [rax]\n");
    strconcat(&for_body, "    lea r9, [rax+16]\n");
    strconcat(&for_body, "    cmp r8, 0\n");
    strconcat(&for_body, "    je .for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "_true_end\n");
    strconcat(&for_body, ".for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, ":\n");
    strconcat(&for_body, "    mov r10, [r9]\n");
    strconcat(&for_body, "    mov [");
    strconcat(&for_body, node -> iterator.value);
    strconcat(&for_body, tmp2);
    free(tmp2);
    strconcat(&for_body, "], r10\n");
    lqdStatementsNode* stmnts = lqdStatementsNode_new(1);
    lqdStatementsNode_push(stmnts, node -> body);
    char* code;
    if (node -> body.type == NT_Statements) {
        lqdCompilerContext* cc = ctx;
        while (cc -> is_child_ctx) cc = cc -> parent_ctx;
        lqdCompilerContext* child_ctx = create_context(ctx -> code, ctx -> filename, 1, ++cc -> child_contexts);
        child_ctx -> is_lib = ctx -> is_lib;
        child_ctx -> lib_name = ctx -> lib_name;
        child_ctx -> for_end = malloc(100);
        sprintf(child_ctx -> for_end, ".for_%li_end", ctx -> comparison - 1);
        child_ctx -> true_for_end = malloc(100);
        sprintf(child_ctx -> true_for_end, ".for_%li_true_end", ctx -> comparison - 1);
        define_var(child_ctx, node -> iterator.value, "undefined", 1); // TODO: yk
        child_ctx -> parent_ctx = ctx;
        code = x86_64_compile_statements(stmnts, child_ctx, NULL);
        free(child_ctx -> for_end);
        child_ctx -> for_end = NULL;
        free(child_ctx -> true_for_end);
        child_ctx -> true_for_end = NULL;
        child_ctx -> is_lib = 0;
        child_ctx -> lib_name = NULL;
        delete_context(child_ctx);
    } else {
        code = x86_64_compile_statements(stmnts, ctx, NULL);
    }
    free(stmnts -> statements);
    free(stmnts);
    strconcat(&for_body, code);
    strconcat(&for_body, ".for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "_end:\n");
    strconcat(&for_body, "    add r9, 8\n");
    strconcat(&for_body, "    sub r8, 1\n");
    strconcat(&for_body, "    cmp r8, 0\n");
    strconcat(&for_body, "    jg .for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "\n");
    strconcat(&for_body, ".for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "_true_end:\n");
    free(tmp);
    free(code);
    return for_body;
};
char* x86_64_While(lqdWhileNode* node, lqdCompilerContext* ctx) {
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
    ctx -> while_end = malloc(100);
    sprintf(ctx -> while_end, ".while_%li_end", ctx -> comparison - 1);
    ctx -> true_while_end = malloc(100);
    sprintf(ctx -> true_while_end, ".while_%li_true_end", ctx -> comparison - 1);
    char* tmp2 = x86_64_compile_stmnt(node -> body, ctx);
    free(ctx -> while_end);
    ctx -> while_end = NULL;
    free(ctx -> true_while_end);
    ctx -> true_while_end = NULL;
    strconcat(&while_body, tmp2);
    free(tmp2);
    strconcat(&while_body, ".while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "_end:\n");
    tmp2 = x86_64_compile_stmnt(node -> condition, ctx);
    strconcat(&while_body, tmp2);
    strconcat(&while_body, "    pop rax\n");
    strconcat(&while_body, "    cmp rax, 1\n");
    strconcat(&while_body, "    je .while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "\n");
    strconcat(&while_body, ".while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "_true_end:\n");
    free(tmp2);
    free(tmp);
    return while_body;
};
char* x86_64_VarReassign(lqdVarReassignNode* node, lqdCompilerContext* ctx) {
    int id = is_defined(ctx, node -> var -> tokens[0].value);
    if (!id)
        lqdCompilerError(ctx, node -> var -> tokens[0].line, node -> var -> tokens[0].idx_start, node -> var -> tokens[node -> var -> values-1].idx_end, "Undefined variable!");
    char* var_reassign = malloc(1);
    var_reassign[0] = 0;
    char* tmp2 = x86_64_compile_stmnt(node -> value, ctx);
    strconcat(&var_reassign, tmp2);
    free(tmp2);
    strconcat(&var_reassign, "    pop rdi\n");
    strconcat(&var_reassign, "    mov rbx, ");
    char* tmp = malloc(12);
    sprintf(tmp, "%i", id);
    strconcat(&var_reassign, node -> var -> tokens[0].value);
    strconcat(&var_reassign, tmp);
    free(tmp);
    strconcat(&var_reassign, "\n");
    if (node -> has_slice) {
        strconcat(&var_reassign, "    mov rsi, [rbx]\n");
        char* tmp = x86_64_compile_stmnt(node -> slice, ctx);
        strconcat(&var_reassign, tmp);
        strconcat(&var_reassign, "    pop rax\n");
        strconcat(&var_reassign, "    xor rdx, rdx\n");
        strconcat(&var_reassign, "    mov rcx, 8\n");
        strconcat(&var_reassign, "    imul rcx\n");
        strconcat(&var_reassign, "    lea rbx, [rsi+16+rax]\n");    
        free(tmp);
    }
    strconcat(&var_reassign, "    mov [rbx], rdi\n");
    return var_reassign;
};
char* x86_64_Return(lqdReturnNode* node, lqdCompilerContext* ctx) {
    char* ret_body = malloc(1);
    ret_body[0] = 0;
    strconcat(&ret_body, "    mov rsp, rbp\n");
    strconcat(&ret_body, "    pop rbp\n");
    if (node -> returns) {
        char* tmp = x86_64_compile_stmnt(node -> return_value, ctx);
        strconcat(&ret_body, tmp);
        free(tmp);
        strconcat(&ret_body, "    pop rax\n");
        strconcat(&ret_body, "    pop rbx\n");
        strconcat(&ret_body, "    push rax\n");
        strconcat(&ret_body, "    push rbx\n");
    }
    strconcat(&ret_body, "    ret\n");
    ctx -> returns = 1;
    return ret_body;
};
char* x86_64_Continue(lqdContinueNode* node, lqdCompilerContext* ctx) {
    char* cont = malloc(1);
    cont[0] = 0;
    if (ctx -> while_end == NULL && ctx -> for_end == NULL)
        lqdCompilerError(ctx, node -> tok.line, node -> tok.idx_start, node -> tok.idx_end, "Cannot continue outside of loop!\n");
    strconcat(&cont, "    jmp ");
    if (ctx -> while_end)
        strconcat(&cont, ctx -> while_end);
    else
        strconcat(&cont, ctx -> for_end);
    return cont;
};
char* x86_64_Break(lqdBreakNode* node, lqdCompilerContext* ctx) {
    char* brk = malloc(1);
    brk[0] = 0;
    if (ctx -> true_while_end == NULL && ctx -> true_for_end == NULL)
        lqdCompilerError(ctx, node -> tok.line, node -> tok.idx_start, node -> tok.idx_end, "Cannot break outside of loop!\n");
    strconcat(&brk, "    jmp ");
    if (ctx -> while_end)
        strconcat(&brk, ctx -> true_while_end);
    else
        strconcat(&brk, ctx -> true_for_end);
    return brk;
};
char* x86_64_Construct(lqdConstructorNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* x86_64_Struct(lqdStructNode* node, lqdCompilerContext* ctx) {
    return "";
};
char* x86_64_Employ(lqdEmployNode* node, lqdCompilerContext* ctx) {
    char* employee = malloc(1);
    employee[0] = 0;
    char* path = malloc(256);
    path[0] = 0;
    strconcat(&path, "./");
    for (uint64_t i = 0; i < node -> lib_path -> values; i++) {
        strconcat(&path, node -> lib_path -> tokens[i].value);
        if (i != node -> lib_path -> values - 1)
            strconcat(&path, "/");
    }
    strconcat(&path, ".lqd");
    // Code above turns something like "some.folder.some.lib" into "./some/folder/some/lib.lqd"
    char* code = malloc(2);
    uint64_t code_len = 0;
    uint64_t code_max = 2;
    FILE* file = fopen(path, "r");
    if (file == NULL)
        lqdCompilerError(ctx, node -> lib_path -> tokens[0].line, node -> lib_path -> tokens[0].idx_start, node -> lib_path -> tokens[0].idx_end, "Library not found!");
    lqdchar ch;
    do {
        if (code_len == code_max) {
            code_max *= 2;
            code = realloc(code, code_max);
            if (code == NULL) {
                fprintf(stderr, "Not enough memory to load file!\n");
                exit(1);
            }
        }
        ch = fgetc(file);
        code[code_len++] = ch;
    } while(ch != EOF);
    code = realloc(code, code_max + 1);
    code[--code_len] = 0;
    lqdTokenArray* tokens = tokenize(code, path);
    lqdStatementsNode* AST = parse(tokens, code, path);
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    lqdCompilerContext* nctx = create_context(code, path, 0, ++cc -> child_contexts);
    nctx -> lib_name = malloc(strlen(node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value) + 1);
    strcpy(nctx -> lib_name, node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value);
    nctx -> is_lib = 1;
    char* new_code = x86_64_compile_statements(AST, nctx, node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value);
    strconcat(&employee, new_code);
    free(new_code);
    free(path);
    fclose(file);

    define_lib(ctx, node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value);
    for (int i = 0; i < nctx -> namespace -> defined_funcs_vals; i++) {
        define_func(ctx -> namespace -> libraries[ctx -> namespace -> libraries_vals - 1], nctx -> namespace -> defined_funcs[i], nctx -> namespace -> func_params[i], nctx -> namespace -> is_extern_func[i]);
    }

    strconcat(get_glob_section_bss(ctx), *get_glob_section_bss(nctx));
    cc -> child_contexts += nctx -> child_contexts - cc -> child_contexts + 1;
    cc -> defined_arrays += nctx -> defined_arrays - cc -> defined_arrays + 1;
    delete_context(nctx);
    free(code);
    lqdTokenArray_delete(tokens);
    lqdStatementsNode_delete(AST);
    return employee;
};

char* x86_64_compile_statements(lqdStatementsNode* statements, lqdCompilerContext* ctx, char* namespace) {
    char* section_text = malloc(1);
    section_text[0] = 0;
    for (uint64_t i = 0; i < statements -> values; i++) {
        switch (statements -> statements[i].type) {
            case NT_NoStmnt:
                strconcat(&section_text, x86_64_NoStmnt());
                break;
            case NT_Number: {
                char* num = x86_64_Number((lqdNumberNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, num);
                free(num);
                break;
            }
            case NT_Char: {
                char* chr = x86_64_Char((lqdCharNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, chr);
                free(chr);
                break;
            }
            case NT_String: {
                char* str = x86_64_String((lqdStringNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, str);
                free(str);
                break;
            }
            case NT_Arr: {
                char* arr = x86_64_Array((lqdArrayNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, arr);
                free(arr);
                break;
            }
            case NT_VarAccess: {
                char* var_access = x86_64_VarAccess((lqdVarAccessNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, var_access);
                free(var_access);
                break;
            }
            case NT_BinOp: {
                char* op = x86_64_BinOp((lqdBinOpNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, op);
                free(op);
                break;
            }
            case NT_Unary:
                strconcat(&section_text, x86_64_Unary((lqdUnaryOpNode*)statements -> statements[i].node, ctx));
                break;
            case NT_VarDecl: {
                char* var_decl = x86_64_VarDecl((lqdVarDeclNode*)statements -> statements[i].node, ctx, namespace);
                strconcat(&section_text, var_decl);
                free(var_decl);
                break;
            }
            case NT_FuncDecl: {
                char* decl = x86_64_FuncDecl((lqdFuncDeclNode*)statements -> statements[i].node, ctx, namespace);
                strconcat(&section_text, decl);
                free(decl);
                break;
            }
            case NT_Statements: {
                char* stmnts = x86_64_Statements((lqdStatementsNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, stmnts);
                free(stmnts);
                break;
            }
            case NT_FuncCall: {
                /* TODO: dynamic calling convention*/
                char* call = x86_64_FuncCall((lqdFuncCallNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, call);
                free(call);
                break;
            }
            case NT_If: {
                char* if_body = x86_64_If((lqdIfNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, if_body);
                free(if_body);
                break;
            }
            case NT_For: {
                char* for_body = x86_64_For((lqdForNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, for_body);
                free(for_body);
                break;
            }
            case NT_While: {
                char* while_body = x86_64_While((lqdWhileNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, while_body);
                free(while_body);
                break;
            }
            case NT_Struct:
                strconcat(&section_text, x86_64_Struct((lqdStructNode*)statements -> statements[i].node, ctx));
                break;
            case NT_VarReassign: { // lib
                char* reassign_body = x86_64_VarReassign((lqdVarReassignNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, reassign_body);
                free(reassign_body);
                break;
            }
            case NT_Return: {
                char* ret = x86_64_Return((lqdReturnNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, ret);
                free(ret);
                break;
            }
            case NT_Continue: {
                char* cont = x86_64_Continue((lqdContinueNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, cont);
                free(cont);
                break;
            }
            case NT_Break: {
                char* brk = x86_64_Break((lqdBreakNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, brk);
                free(brk);
                break;
            }
            case NT_Construct:
                strconcat(&section_text, x86_64_Construct((lqdConstructorNode*)statements -> statements[i].node, ctx));
                break;
            case NT_Employ: {
                char* employee = x86_64_Employ((lqdEmployNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, employee);
                free(employee);
                break;
            }
            default:
                break;
        }
    }
    return section_text;
}

char* x86_64_compile(lqdStatementsNode* statements, char* code, char* filename) {
    char* section_text = malloc(1);
    lqdCompilerContext* ctx = create_context(code, filename, 0, 0);
    section_text[0] = 0;
    
    char* _code = x86_64_compile_statements(statements, ctx, NULL);
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
#ifdef __WIN64
    strconcat(&final_code, "global WinMain\n");
    strconcat(&final_code, "WinMain:\n");
#else
    strconcat(&final_code, "global _start\n");
    strconcat(&final_code, "_start:\n");
#endif
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