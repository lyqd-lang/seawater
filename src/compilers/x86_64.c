#include "compiler.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char* name;
    char* type;
    char initialized;
    uint64_t id;
    char is_arr;
    char* element_type;
    uint64_t stack_pos;
} lqdVariable;

typedef struct {
    lqdVariable** vars;
    int values;
    int size;
} lqdVariableArr;

lqdVariable* lqdVariable_new(char* name, char* type, char initialized, uint64_t id, uint64_t stack_pos) {
    lqdVariable* var = malloc(sizeof(lqdVariable));
    var -> name = malloc(strlen(name) + 1);
    strcpy(var -> name, name);
    var -> type = malloc(strlen(type) + 1);
    strcpy(var -> type, type);
    var -> initialized = initialized;
    var -> id = id;
    var -> element_type = NULL;
    var -> is_arr = 0;
    var -> stack_pos = stack_pos;
    return var;
}

lqdVariableArr* lqdVariableArr_new(int size) {
    lqdVariableArr* arr = malloc(sizeof(lqdVariableArr));
    arr -> vars = malloc(size * sizeof(lqdVariable*));
    arr -> size = size;
    arr -> values = 0;
    return arr;
}

void lqdVariableArr_push(lqdVariableArr* arr, lqdVariable var) {
    if (arr -> values == arr -> size) {
        arr -> size *= 2;
        arr -> vars = realloc(arr -> vars, arr -> size * sizeof(lqdVariable*));
        if (arr -> vars == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
            exit(1);
        }
    }
    lqdVariable* _var = lqdVariable_new(var.name, var.type, var.initialized, var.id, var.stack_pos);
    if (var.is_arr) {
        _var -> is_arr = 1;
        _var -> element_type = malloc(strlen(var.element_type) + 1);
        strcpy(_var -> element_type, var.element_type);
    }
    free(var.name);
    free(var.type);
    if (var.is_arr)
        free(var.element_type);
    arr -> vars[arr -> values++] = _var;
}

typedef struct {
    char* name;
    char* ret_type;
    lqdParameterArray* params;
    char is_extern;
    char is_void;
} lqdFunction;

typedef struct {
    lqdFunction** funcs;
    int values;
    int size;
} lqdFuncArr;

lqdFunction* lqdFunction_new(char* name, char* ret_type, lqdParameterArray* params, char is_extern, char is_void) {
    lqdFunction* func = malloc(sizeof(lqdFunction));
    func -> name = malloc(strlen(name) + 1);
    strcpy(func -> name, name);
    if (!is_void) {
        func -> ret_type = malloc(strlen(ret_type) + 1);
        strcpy(func -> ret_type, ret_type);
    }
    func -> is_void = is_void;
    func -> params = params;
    func -> is_extern = is_extern;
    return func;
}

lqdFuncArr* lqdFuncArr_new(int size) {
    lqdFuncArr* arr = malloc(sizeof(lqdFuncArr));
    arr -> funcs = malloc(size * sizeof(lqdFunction*));
    arr -> size = size;
    arr -> values = 0;
    return arr;
}

void lqdFuncArr_push(lqdFuncArr* arr, lqdFunction* func) {
    if (arr -> values == arr -> size) {
        arr -> size *= 2;
        arr -> funcs = realloc(arr -> funcs, arr -> size * sizeof(lqdFunction*));
        if (arr -> funcs == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
            exit(1);
        }
    }
    lqdParameterArray* params = lqdParameterArray_new(1);
    for (int i = 0; i < func -> params -> values; i++)
        lqdParameterArray_push(params, func -> params -> params[i]);
    lqdFunction* _func = lqdFunction_new(func -> name, func -> ret_type, params, func -> is_extern, func -> is_void);
    free(func -> name);
    if (!func -> is_void)
        free(func -> ret_type);
    arr -> funcs[arr -> values++] = _func;
}

typedef struct {
    char* name;
    void* namespace; // Very safe :)
} lqdLib;

typedef struct {
    lqdLib** libs;
    int values;
    int size;
} lqdLibraryArr;

lqdLib* lqdLib_new(char* name, void* namespace) {
    lqdLib* lib = malloc(sizeof(lqdLib));
    lib -> name = name;
    lib -> namespace = namespace;
    return lib;
}

lqdLibraryArr* lqdLibraryArr_new(int size) {
    lqdLibraryArr* arr = malloc(sizeof(lqdLibraryArr));
    arr -> size = size;
    arr -> values = 0;
    arr -> libs = malloc(size * sizeof(lqdLib));
    return arr;
}

void lqdLibraryArr_push(lqdLibraryArr* arr, lqdLib* lib) {
    if (arr -> values == arr -> size) {
        arr -> size *= 2;
        arr -> libs = realloc(arr -> libs, arr -> size * sizeof(lqdLib));
        if (arr -> libs == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
            exit(1);
        }
    }
    arr -> libs[arr -> values++] = lib;
}

typedef struct {
    lqdFuncArr* funcs;
    lqdVariableArr* vars;
    lqdLibraryArr* libs;
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

    lqdTokenArray* toks;
    lqdStatementsNode* employ_discard;

    // TODO: implement this
    char returns;
} lqdCompilerContext;

lqdCompilerContext* create_context(char* code, char* filename, char is_child, uint64_t id) {
    lqdCompilerContext* ctx = malloc(sizeof(lqdCompilerContext));
    ctx -> code = code;
    ctx -> filename = filename;
    ctx -> namespace = malloc(sizeof(lqdNamespace));

    ctx -> namespace -> funcs = lqdFuncArr_new(1);
    ctx -> namespace -> vars = lqdVariableArr_new(1);
    ctx -> namespace -> libs = lqdLibraryArr_new(1);

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

lqdVariable is_defined(lqdCompilerContext* ctx, char* name) {
    char exists = 0;
    lqdVariable var = {NULL, NULL, 0};
    lqdCompilerContext* current_ctx = ctx;
    while (current_ctx -> is_child_ctx) {
        for (uint64_t i = 0; i < current_ctx -> namespace -> vars -> values; i++) {
            if (!strcmp(current_ctx -> namespace -> vars -> vars[i] -> name, name)) {
                exists = current_ctx -> id;
                var = *current_ctx -> namespace -> vars -> vars[i];
            }
        }
        if (!exists) {
            current_ctx = current_ctx -> parent_ctx;
        } else {
            break;
        }
    }
    return var;
}

lqdFunction is_defined_func(lqdCompilerContext* ctx, lqdTokenArray* path) {
    if (path -> values == 1) {
        lqdFunction func = {};
        lqdCompilerContext* current_ctx = ctx;
        char searching = 1;
        while (searching) {
            for (int i = 0; i < current_ctx -> namespace -> funcs -> values; i++) {
                if (!strcmp(current_ctx -> namespace -> funcs -> funcs[i] -> name, path -> tokens[0].value)) {
                    searching = 0;
                    func = *current_ctx -> namespace -> funcs -> funcs[i];
                }
            }
            if (searching) {
                if (current_ctx -> is_child_ctx) current_ctx = current_ctx -> parent_ctx;
                else searching = 0;
            } else {
                searching = 0;
            }
        }
        return func;
    }
    lqdFunction func = {NULL, NULL, NULL, 0};
    lqdCompilerContext* current_ctx = ctx;
    lqdNamespace* currentns = current_ctx -> namespace;
    char found = 0;
    char searching = 1;
    while (searching) {
        for (int i = 0; i < current_ctx -> namespace -> libs -> values; i++) {
            if (!strcmp(current_ctx -> namespace -> libs -> libs[i] -> name, path -> tokens[i].value)) {
                searching = 0;
                found = 1;
                currentns = current_ctx -> namespace  -> libs -> libs[i] -> namespace;
            }
        }
        if (searching) {
            if (current_ctx -> is_child_ctx) current_ctx = current_ctx -> parent_ctx;
            else searching = 0;
        } else {
            searching = 0;
        }
    }
    if (!found) return func;
    found = 0;
    for (int i = 1; i < path -> values - 1; i++) {
        for (int j = 0; j < current_ctx -> namespace -> libs -> values; j++) {
            if (!strcmp(current_ctx -> namespace -> libs -> libs[j] -> name, path -> tokens[i].value)) {
                found = 1;
                currentns = current_ctx -> namespace;
            }
        }
        if (!found) {
            break;
        }
    }
    if (path -> values - 2 && !found) return func;
    for (int i = 0; i < currentns -> funcs -> values; i++) {
        if (!strcmp(currentns -> funcs -> funcs[i] -> name, path -> tokens[path -> values - 1].value)) {
            func = *currentns -> funcs -> funcs[i];
        }
    }
    return func;
}

void delete_namespace(lqdNamespace* ns) {
    for (int i = 0; i < ns -> vars -> values; i++) {
        free(ns -> vars -> vars[i] -> name);
        free(ns -> vars -> vars[i] -> type);
        if (ns -> vars -> vars[i] -> is_arr)
            free(ns -> vars -> vars[i] -> element_type);
        free(ns -> vars -> vars[i]);
    }
    for (int i = 0; i < ns -> funcs -> values; i++) {
        free(ns -> funcs -> funcs[i] -> name);
        if (!ns -> funcs  -> funcs[i] -> is_void)
            free(ns -> funcs -> funcs[i] -> ret_type);
        lqdParameterArray_delete(ns -> funcs -> funcs[i] -> params);
        free(ns -> funcs -> funcs[i]);
    }
    free(ns -> funcs -> funcs);
    free(ns -> funcs);
    free(ns -> vars -> vars);
    free(ns -> vars);
    for (int i = 0; i < ns -> libs -> values; i++) {
        delete_namespace(ns -> libs -> libs[i] -> namespace);
        free(ns -> libs -> libs[i]);
    }
    free(ns -> libs -> libs);
    free(ns -> libs);
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
    fprintf(stderr, "│ Line: %ld, File: %s (%s:%ld:%i)\n", line, ctx -> filename, ctx -> filename, line, idx_start - idx + 1);
    fprintf(stderr, "╯\n");
    exit(1);
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
        child_ctx -> while_end = ctx -> while_end;
        child_ctx -> for_end = ctx -> for_end;
        child_ctx -> true_while_end = ctx -> true_while_end;
        child_ctx -> true_for_end = ctx -> true_for_end;
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
    strconcat(&arr_body, "    push qword [next_free]\n");
    strconcat(&arr_body, "    lea r11, [heap]\n");
    strconcat(&arr_body, "    add r11, [next_free]\n");
    strconcat(&arr_body, "    add qword [next_free], 16\n");
    char* tmp = malloc(20);
    if (node -> is_reserved)
        strcpy(tmp, node -> reserved.value);
    else
        sprintf(tmp, "%ld", node -> values -> size * 8);
    strconcat(&arr_body, "    add qword [next_free], ");
    strconcat(&arr_body, tmp);
    strconcat(&arr_body, "\n");
    strconcat(&arr_body, "    mov qword [r11], ");
    strconcat(&arr_body, tmp);
    strconcat(&arr_body, "\n");
    char* tmp2 = malloc(20);
    sprintf(tmp2, "%ld", node -> values -> values * 8);
    strconcat(&arr_body, "    mov qword [r11+8], ");
    strconcat(&arr_body, tmp2);
    strconcat(&arr_body, "\n");
    for (int i = 0; i < node -> values -> values; i++) {
        char* tmp3 = malloc(20);
        sprintf(tmp3, "%i", (i + 2) * 8);
        char* tmp4 = x86_64_compile_stmnt(node -> values -> statements[i], ctx);
        strconcat(&arr_body, tmp4);
        free(tmp4);
        strconcat(&arr_body, "    pop rax\n");
        strconcat(&arr_body, "    mov qword [r11+");
        strconcat(&arr_body, tmp3);
        strconcat(&arr_body, "], rax\n");
        free(tmp3);
    }
    free(tmp);
    free(tmp2);
    return arr_body;
}
char* x86_64_String(lqdStringNode* node, lqdCompilerContext* ctx) {
    char* str_body = malloc(1);
    str_body[0] = 0;
    strconcat(&str_body, "    push qword [next_free]\n");
    strconcat(&str_body, "    lea r11, [heap]\n");
    strconcat(&str_body, "    add r11, [next_free]\n");
    strconcat(&str_body, "    add qword [next_free], 16\n");
    char* tmp = malloc(20);
    sprintf(tmp, "%ld", strlen(node -> tok.value));
    strconcat(&str_body, "    add qword [next_free], ");
    strconcat(&str_body, tmp);
    strconcat(&str_body, "\n");
    strconcat(&str_body, "    mov qword [r11], ");
    strconcat(&str_body, tmp);
    strconcat(&str_body, "\n");
    char* tmp2 = malloc(20);
    sprintf(tmp2, "%ld", strlen(node -> tok.value));
    strconcat(&str_body, "    mov qword [r11+8], ");
    strconcat(&str_body, tmp2);
    strconcat(&str_body, "\n");
    for (int i = 0; i < strlen(node -> tok.value); i++) {
        char* tmp3 = malloc(20);
        sprintf(tmp3, "%i", i + 16);
        strconcat(&str_body, "    mov byte [r11+");
        strconcat(&str_body, tmp3);
        char* tmp4 = malloc(12);
        sprintf(tmp4, "%i", node -> tok.value[i]);
        strconcat(&str_body, "], ");
        strconcat(&str_body, tmp4);
        strconcat(&str_body, "\n");
        free(tmp3);
        free(tmp4);
    }
    free(tmp);
    free(tmp2);
    return str_body;
};
char* x86_64_VarAccess(lqdVarAccessNode* node, lqdCompilerContext* ctx) {
    lqdVariable var = is_defined(ctx, node -> path -> tokens[0].value);
    if (var.name == NULL)
        lqdCompilerError(ctx, node -> path -> tokens[0].line, node -> path -> tokens[0].idx_start, node -> path -> tokens[node -> path -> values-1].idx_end, "Undefined variable!");
    char* var_access = malloc(1);
    var_access[0] = 0;
    strconcat(&var_access, "    mov rax, [rbp-");
    char* tmp = malloc(20);
    sprintf(tmp, "%ld", (var.stack_pos + 1) * 8);
    strconcat(&var_access, tmp);
    strconcat(&var_access, "]\n    push rax\n");
    if (node -> has_slice) {
        strconcat(&var_access, "    pop rdx\n");
        if (strcmp(var.type, "arr") && strcmp(var.type, "str"))
            lqdCompilerError(ctx, node -> path -> tokens[0].line, node -> path -> tokens[0].idx_start, node -> path -> tokens[node -> path -> values-1].idx_end, "Attempted indexing on non-array");
        char* tmp2 = x86_64_compile_stmnt(node -> slice, ctx);
        strconcat(&var_access, tmp2);
        strconcat(&var_access, "    pop rax\n");
        if (!strcmp(var.type, "str"))
            strconcat(&var_access, "    mov rbx, [heap+rdx+16+rax]\n");
        else
            strconcat(&var_access, "    mov rbx, qword [heap+rdx+16+rax]\n");
        strconcat(&var_access, "    push rbx\n");
        free(tmp2);
    }
    free(tmp);
    return var_access;
};

char* get_type(lqdCompilerContext* ctx, lqdASTNode value) {
    if (value.type == NT_Number) {
        return "num";
    } else if (value.type == NT_String) {
        return "str";
    } else if (value.type == NT_BinOp) {
        return get_type(ctx, ((lqdBinOpNode*)value.node) -> left);
    } else if (value.type == NT_Char) {
        return "chr";
    } else if (value.type == NT_VarAccess) {
        lqdVariable var = is_defined(ctx, ((lqdVarAccessNode*)value.node) -> path -> tokens[0].value);
        if (((lqdVarAccessNode*)value.node) -> has_slice && var.is_arr)
            return var.element_type;
        return var.type;
    } else if (value.type == NT_FuncCall) {
        lqdFunction callee = is_defined_func(ctx, ((lqdFuncCallNode*)value.node) -> call_path);
        return callee.ret_type;
    } else if (value.type == NT_Unary) {
        return get_type(ctx, ((lqdUnaryOpNode*)value.node) -> value);
    } else {
        fprintf(stderr, "Unhandled: %i\n", value.type);
        exit(1);
    }
    return NULL;
}

lqdToken get_tok(lqdASTNode value) {
    if (value.type == NT_Number) {
        return ((lqdNumberNode*)value.node) -> tok;
    } else if (value.type == NT_String) {
        return ((lqdStringNode*)value.node) -> tok;
    } else if (value.type == NT_BinOp) {
        get_tok(((lqdBinOpNode*)value.node) -> left);
    } else if (value.type == NT_Char) {
        return ((lqdCharNode*)value.node) -> tok;
    } else if (value.type == NT_VarAccess) {
        return ((lqdVarAccessNode*)value.node) -> path -> tokens[((lqdVarAccessNode*)value.node) -> path -> values - 1];
    } else if (value.type == NT_FuncCall) {
        return ((lqdFuncCallNode*)value.node) -> call_path -> tokens[((lqdFuncCallNode*)value.node) -> call_path -> values - 1];
    } else {
        fprintf(stderr, "Unhandled: %i\n", value.type);
        exit(1);
    }
    lqdToken tok = {};
    return tok;
}

char* x86_64_BinOp(lqdBinOpNode* node, lqdCompilerContext* ctx) {
    char* binop = malloc(1);
    binop[0] = 0;
    char type_matching = 0;
    type_matching = !strcmp(get_type(ctx, node -> left), get_type(ctx, node -> right));
    type_matching = type_matching || !strcmp(get_type(ctx, node -> left), "chr") && !strcmp(get_type(ctx, node -> right), "num");
    type_matching = type_matching || !strcmp(get_type(ctx, node -> left), "num") && !strcmp(get_type(ctx, node -> right), "chr");
    if (!type_matching)
        lqdCompilerError(ctx, get_tok(node -> left).line, get_tok(node -> left).idx_start, get_tok(node -> right).idx_end, "Can't perform an operation on different types");
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
    char* var_body = malloc(1);
    var_body[0] = 0;
    if (node -> initialized) {
        char* tmp = x86_64_compile_stmnt(node -> initializer, ctx);
        strconcat(&var_body, tmp);
        free(tmp);
    } else {
        strconcat(&var_body, "    add rsp, 8\n");
    }

    // Without this, the code just segfaults so yeh
    if (!strcmp(node -> type.type.value, "arr") && !node -> has_element_type)
        lqdCompilerError(ctx, node -> type.type.line, node -> type.type.idx_start, node -> type.type.idx_end, "Expected arr:T where T is a type");
    
    lqdVariable* var = lqdVariable_new(node -> name.value, node -> type.type.value, node -> initialized, ctx -> id, ctx -> namespace -> vars -> values);
    
    if (!strcmp(node -> type.type.value, "str")) {
        var -> is_arr = 1;
        var -> element_type = malloc(4);
        strcpy(var -> element_type, "chr");
    } else if (!strcmp(node -> type.type.value, "arr")) {
        var -> is_arr = 1;
        var -> element_type = malloc(strlen(node -> element_type.value)+1);
        strcpy(var -> element_type, node -> element_type.value);
    }

    lqdVariableArr_push(ctx -> namespace -> vars, *var);
    free(var);
    
    return var_body;
};
char* x86_64_FuncDecl(lqdFuncDeclNode* node, lqdCompilerContext* ctx, char* namespace) {
    lqdFunction* func = lqdFunction_new(node -> name.value, node -> ret_type.type.value, node -> params, node -> is_extern, node -> is_void);
    lqdFuncArr_push(ctx -> namespace -> funcs, func);
    free(func);
    char* func_body = malloc(1);
    func_body[0] = 0;
    if (node -> is_extern) {
        strconcat(&func_body, "extern ");
        strconcat(&func_body, node -> name.value);
        strconcat(&func_body, "\n");
        return func_body;
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
            char* tmp = malloc(8);
            sprintf(tmp, "%li", (i + 2) * 8);
            strconcat(&func_body, "    mov rax, [rbp+");
            strconcat(&func_body, tmp);
            strconcat(&func_body, "]\n");
            free(tmp);
            strconcat(&func_body, "    push rax\n");

            lqdVariable* var = lqdVariable_new(node -> params -> params[i].name.value, node -> params -> params[i].type.type.value, 1, id, child_ctx -> namespace -> vars -> values);

            if (!strcmp(node -> params -> params[i].type.type.value, "str") || !strcmp(node -> params -> params[i].type.type.value, "arr")) {
                var -> is_arr = 1;
                if (!strcmp(node -> params -> params[i].type.type.value, "str")) {
                    var -> is_arr = 1;
                    var -> element_type = malloc(4);
                    strcpy(var -> element_type, "chr");
                } else if (!strcmp(node -> params -> params[i].type.type.value, "arr")) {
                    var -> is_arr = 1;
                    var -> element_type = malloc(strlen(node -> params -> params[i].element_type.value)+1);
                    strcpy(var -> element_type, node -> params -> params[i].element_type.value);
                }
            }
            lqdVariableArr_push(child_ctx -> namespace -> vars, *var);
            free(var);
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
    lqdFunction found = is_defined_func(ctx, node -> call_path);
    char is_extern = found.is_extern;
    if (found.name == NULL)
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[node -> call_path -> values - 1].idx_end, "Undefined function!");
    char* call_body = malloc(1);
    call_body[0] = 0;
    if (node -> args -> values > found.params -> values)
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[node -> call_path -> values - 1].idx_end, "Too many arguments passed!");
    if (node -> args -> values < found.params -> values)
        lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[node -> call_path -> values - 1].idx_end, "Too few arguments passed!");
    for (int i = node -> args -> values - 1; i >= 0; i--) {
        char* type = get_type(ctx, node -> args -> statements[i]);
        char matching_type = 0;
        matching_type = !strcmp(type, found.params -> params[i].type.type.value);
        matching_type = matching_type || (!strcmp("chr", found.params -> params[i].type.type.value) && !strcmp("num", type));
        if (!matching_type)
            lqdCompilerError(ctx, node -> call_path -> tokens[0].line, node -> call_path -> tokens[0].idx_start, node -> call_path -> tokens[node -> call_path -> values - 1].idx_end, "Invalid arguments passed!");
        char* tmp = x86_64_compile_stmnt(node -> args -> statements[i], ctx);
        strconcat(&call_body, tmp);
        free(tmp);
    }
    strconcat(&call_body, "    call ");
    if (ctx -> is_lib && !is_extern && node -> call_path -> values == 1) {
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
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    sprintf(tmp, "%li", cc -> comparison++);
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
        strconcat(&if_body, "    cmp rax, 0\n");
        strconcat(&if_body, "    je .if_case_");
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
    char* for_body = malloc(1);
    for_body[0] = 0;
    char* tmp = malloc(20);
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    sprintf(tmp, "%li", cc -> comparison++);
    char* tmp2 = malloc(20);
    sprintf(tmp2, "%i", ctx -> id + 1);
    strconcat(&for_body, "    mov rax, [rbp-");
    lqdVariable var = is_defined(ctx, node -> arr -> tokens[0].value);
    if (var.name == NULL)
        lqdCompilerError(ctx, node -> arr -> tokens[0].line, node -> arr -> tokens[0].idx_start, node -> arr -> tokens[0].idx_end, "Array not defined!");
    char* tmp3 = malloc(12);
    sprintf(tmp3, "%i", ctx -> namespace -> vars -> values * 8);
    strconcat(&for_body, tmp3);
    free(tmp3);
    strconcat(&for_body, "]\n");
    strconcat(&for_body, "    push 0\n"); // iterator
    strconcat(&for_body, "    mov r8, [heap+rax+8]\n");
    strconcat(&for_body, "    lea r9, [heap+rax+16]\n");
    strconcat(&for_body, "    cmp r8, 0\n");
    strconcat(&for_body, "    je .for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "_true_end\n");
    strconcat(&for_body, ".for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, ":\n");
    strconcat(&for_body, "    mov r10, [r9]\n");
    if (!strcmp(var.type, "str")) {
        strconcat(&for_body, "    cmp r10, 0\n");
        strconcat(&for_body, "    je .for_");
        strconcat(&for_body, tmp);
        strconcat(&for_body, "_true_end\n");
    }
    lqdCompilerContext* child_ctx = create_context(ctx -> code, ctx -> filename, 1, ++cc -> child_contexts);
    child_ctx -> is_lib = ctx -> is_lib;
    child_ctx -> lib_name = ctx -> lib_name;
    child_ctx -> for_end = malloc(100);
    sprintf(child_ctx -> for_end, ".for_%li_end", ctx -> comparison - 1);
    child_ctx -> true_for_end = malloc(100);
    sprintf(child_ctx -> true_for_end, ".for_%li_true_end", ctx -> comparison - 1);

    lqdVariable* _var = lqdVariable_new(node -> iterator.value, var.element_type, 1, child_ctx -> id, child_ctx -> namespace -> vars -> values);
    lqdVariableArr_push(child_ctx -> namespace -> vars, *_var);
    strconcat(&for_body, "    mov [rbp-");
    char* offset = malloc(20);
    sprintf(offset, "%ld", (_var -> stack_pos + 1) * 8);
    strconcat(&for_body, offset);
    free(offset);
    free(tmp2);
    free(_var);
    strconcat(&for_body, "], r10\n");
    lqdStatementsNode* stmnts = lqdStatementsNode_new(1);
    lqdStatementsNode_push(stmnts, node -> body);
    char* code;
    if (node -> body.type == NT_Statements) {
        lqdCompilerContext* cc = ctx;
        while (cc -> is_child_ctx) cc = cc -> parent_ctx;
        if (!var.is_arr)
            lqdCompilerError(ctx, node -> arr -> tokens[0].line, node -> arr -> tokens[0].idx_start, node -> arr -> tokens[0].idx_end, "Expected array or string!");

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
        code = x86_64_compile_statements(stmnts, child_ctx, NULL);
    }
    free(stmnts -> statements);
    free(stmnts);
    strconcat(&for_body, code);
    strconcat(&for_body, ".for_");
    strconcat(&for_body, tmp);
    strconcat(&for_body, "_end:\n");
    if (strcmp("str", var.type))
        strconcat(&for_body, "    add r9, 8\n");
    else
        strconcat(&for_body, "    add r9, 1\n");
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
    lqdCompilerContext* cc = ctx;
    while (cc -> is_child_ctx) cc = cc -> parent_ctx;
    sprintf(tmp, "%li", cc -> comparison++);
    strconcat(&while_body, "    jmp .while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, "_end\n");
    strconcat(&while_body, ".while_");
    strconcat(&while_body, tmp);
    strconcat(&while_body, ":\n");
    ctx -> while_end = malloc(100);
    sprintf(ctx -> while_end, ".while_%li_end", cc -> comparison - 1);
    ctx -> true_while_end = malloc(100);
    sprintf(ctx -> true_while_end, ".while_%li_true_end", cc -> comparison - 1);
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
    lqdVariable var = is_defined(ctx, node -> var -> tokens[0].value);
    if (var.name == NULL)
        lqdCompilerError(ctx, node -> var -> tokens[0].line, node -> var -> tokens[0].idx_start, node -> var -> tokens[node -> var -> values-1].idx_end, "Undefined variable!");
    char* var_reassign = malloc(1);
    var_reassign[0] = 0;
    char* tmp2 = x86_64_compile_stmnt(node -> value, ctx);
    strconcat(&var_reassign, tmp2);
    free(tmp2);
    strconcat(&var_reassign, "    pop rdi\n");
    strconcat(&var_reassign, "    lea rbx, [rbp-");
    char* tmp = malloc(12);
    sprintf(tmp, "%li", (var.stack_pos + 1) * 8);
    strconcat(&var_reassign, tmp);
    strconcat(&var_reassign, "]\n");
    if (node -> has_slice) {
        strconcat(&var_reassign, "    mov rsi, heap\n");
        strconcat(&var_reassign, "    add rsi, [rbx]\n");
        char* tmp = x86_64_compile_stmnt(node -> slice, ctx);
        strconcat(&var_reassign, tmp);
        strconcat(&var_reassign, "    pop rax\n");
        if (strcmp("str", var.type)) {
            strconcat(&var_reassign, "    xor rdx, rdx\n");
            strconcat(&var_reassign, "    mov rcx, 8\n");
            strconcat(&var_reassign, "    imul rcx\n");
        }
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
    strconcat(&cont, "\n");
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
    strconcat(&brk, "\n");
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
    lqdCompilerContext* nctx = create_context(code, path, 1, ++cc -> child_contexts);
    nctx -> parent_ctx = cc;
    nctx -> lib_name = malloc(strlen(node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value) + 1);
    strcpy(nctx -> lib_name, node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value);
    nctx -> is_lib = 1;
    char* new_code = x86_64_compile_statements(AST, nctx, node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value);
    strconcat(&employee, new_code);
    free(new_code);
    free(path);
    fclose(file);

    lqdNamespace* namespace = malloc(sizeof(lqdNamespace));

    namespace -> funcs = lqdFuncArr_new(1);
    namespace -> vars = lqdVariableArr_new(1);
    namespace -> libs = lqdLibraryArr_new(1);
    lqdLibraryArr_push(ctx -> namespace -> libs, lqdLib_new(node -> is_aliased ? node -> alias.value : node -> lib_path -> tokens[node -> lib_path -> values - 1].value, namespace));
    for (int i = 0; i < nctx -> namespace -> funcs -> values; i++) {
        lqdFuncArr_push(((lqdNamespace*)(ctx -> namespace -> libs -> libs[ctx -> namespace -> libs -> values - 1] -> namespace)) -> funcs, nctx -> namespace -> funcs -> funcs[i]);
        lqdParameterArray_delete(nctx -> namespace -> funcs -> funcs[i] -> params);
        free(nctx -> namespace -> funcs -> funcs[i]);
    }

    free(nctx -> section_data);
    free(nctx -> section_bss);
    free(nctx -> lib_name);
    free(nctx -> namespace -> funcs -> funcs);
    free(nctx -> namespace -> funcs);
    for (int i = 0; i < nctx -> namespace -> libs -> values; i++) {
        delete_namespace(nctx -> namespace -> libs -> libs[i] -> namespace);
        free(nctx -> namespace -> libs -> libs[i]);
    }
    free(nctx -> namespace -> libs -> libs);
    free(nctx -> namespace -> libs);
    free(nctx -> namespace -> vars -> vars);
    free(nctx -> namespace -> vars);
    free(nctx -> namespace);
    free(nctx);
    free(code);
    for (int i = 0; i < tokens -> values; i++) {
        lqdTokenArray_push(cc -> toks, tokens -> tokens[i]);
    }
    free(tokens -> tokens);
    free(tokens);
    for (int i = 0; i < AST -> values; i++)
        lqdStatementsNode_push(cc -> employ_discard, AST -> statements[i]);
    free(AST -> statements);
    free(AST);
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
            case NT_Unary: {
                char* unop = x86_64_Unary((lqdUnaryOpNode*)statements -> statements[i].node, ctx);
                strconcat(&section_text, unop);
                free(unop);
                break;
            }
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
    ctx -> toks = lqdTokenArray_new(1);
    ctx -> employ_discard = lqdStatementsNode_new(1);
    
    char* _code = x86_64_compile_statements(statements, ctx, NULL);
    strconcat(&section_text, _code);
    free(_code);

    char* final_code = malloc(1);
    final_code[0] = 0;
    char* section_bss = malloc(1);
    section_bss[0] = 0;
    strconcat(&section_bss, "section .bss\n");
    strconcat(&section_bss, "    heap: resb 660000\n"); // reserve 660KB ram
    strconcat(&section_bss, "    next_free: resq 1\n");
    strconcat(&final_code, section_bss);
    strconcat(&final_code, "section .text\n");
#ifdef __WIN64
    strconcat(&final_code, "global WinMain\n");
    strconcat(&final_code, "WinMain:\n");
    strconcat(&final_code, "    mov qword [next_free], 0\n");
    strconcat(&final_code, "    call main\n");
#else
    strconcat(&final_code, "global _start\n");
    strconcat(&final_code, "_start:\n");
    strconcat(&final_code, "    mov qword [next_free], 0\n");
    strconcat(&final_code, "    call main\n");
    strconcat(&final_code, "    mov rax, 60\n");
    strconcat(&final_code, "    mov rdi, 0\n");
    strconcat(&final_code, "    syscall\n");
#endif
    strconcat(&final_code, section_text);

    lqdStatementsNode_delete(ctx -> employ_discard);

    lqdTokenArray_delete(ctx -> toks);
    delete_context(ctx);
    free(section_text);
    free(section_bss);

    return final_code;
}