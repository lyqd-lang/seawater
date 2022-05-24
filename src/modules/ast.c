#include "ast.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include "lqdint.h"

char* from_type(lqdTokenType type) {
    switch (type) {
        case TT_ADD:return "+";
        case TT_SUB:return "-";
        case TT_MUL:return "*";
        case TT_DIV:return "/";
        case TT_EQEQ:return "==";
        case TT_NE:return "!=";
        case TT_LT:return "<";
        case TT_GT:return ">";
        case TT_LTE:return "<=";
        case TT_GTE:return ">=";
        default:break;
    }
    return "invalid";
}

void lqdASTNode_delete(lqdASTNode node) {
    switch (node.type) {
        case NT_Number:
            free((lqdNumberNode*)node.node);
            break;
        case NT_Char:
            free((lqdCharNode*)node.node);
            break;
        case NT_Break:
            free((lqdNumberNode*)node.node);
            break;
        case NT_Continue:
            free((lqdNumberNode*)node.node);
            break;
        case NT_BinOp:
            lqdASTNode_delete(((lqdBinOpNode*)node.node) -> left);
            lqdASTNode_delete(((lqdBinOpNode*)node.node) -> right);
            free(((lqdBinOpNode*)node.node));
            break;
        case NT_Unary:
            lqdASTNode_delete(((lqdUnaryOpNode*)node.node) -> value);
            free(((lqdUnaryOpNode*)node.node));
            break;
        case NT_VarDecl:
            if (((lqdVarDeclNode*)node.node)->initialized)
                lqdASTNode_delete(((lqdVarDeclNode*)node.node) -> initializer);
            free((lqdVarDeclNode*)node.node);
            break;
        case NT_FuncDecl:
            lqdASTNode_delete(((lqdFuncDeclNode*)node.node) -> statement);
            lqdParameterArray_delete(((lqdFuncDeclNode*)node.node) -> params);
            free((lqdFuncDeclNode*)node.node);
            break;
        case NT_Statements:
            lqdStatementsNode_delete(((lqdStatementsNode*)node.node));
            break;
        case NT_Employ:
            free(((lqdEmployNode*)node.node) -> lib_path -> tokens);
            free(((lqdEmployNode*)node.node) -> lib_path);
            free((lqdEmployNode*)node.node);
            break;
        case NT_FuncCall:
            free(((lqdFuncCallNode*)node.node) -> call_path -> tokens);
            free(((lqdFuncCallNode*)node.node) -> call_path);
            lqdStatementsNode_delete(((lqdFuncCallNode*)node.node) -> args);
            free((lqdEmployNode*)node.node);
            break;
        case NT_String:
            free((lqdStringNode*)node.node);
            break;
        case NT_VarAccess:
            free(((lqdVarAccessNode*)node.node) -> path -> tokens);
            free(((lqdVarAccessNode*)node.node) -> path);
            if (((lqdVarAccessNode*)node.node) -> has_slice)
                lqdASTNode_delete(((lqdVarAccessNode*)node.node) -> slice);
            free((lqdVarAccessNode*)node.node);
            break; 
        case NT_Return:
            if (((lqdReturnNode*)node.node) -> returns)
                lqdASTNode_delete(((lqdReturnNode*)node.node) -> return_value);
            free((lqdReturnNode*)node.node);
            break;
        case NT_If:
            lqdStatementsNode_delete(((lqdIfNode*)node.node) -> conditions);
            lqdStatementsNode_delete(((lqdIfNode*)node.node) -> bodies);
            if (((lqdIfNode*)node.node) -> has_else_case)
                lqdASTNode_delete(((lqdIfNode*)node.node) -> else_case);
            free((lqdIfNode*)node.node);
            break;
        case NT_For:
            free(((lqdForNode*)node.node) -> arr -> tokens);
            free(((lqdForNode*)node.node) -> arr);
            lqdASTNode_delete(((lqdForNode*)node.node) -> body);
            free((lqdForNode*)node.node);
            break;
        case NT_While:
            lqdASTNode_delete(((lqdWhileNode*)node.node) -> condition);
            lqdASTNode_delete(((lqdWhileNode*)node.node) -> body);
            free((lqdWhileNode*)node.node);
            break;
        case NT_Struct:
            lqdStatementsNode_delete(((lqdStructNode*)node.node)->body);
            free((lqdStructNode*)node.node);
            break;
        case NT_VarReassign:
            free(((lqdVarReassignNode*)node.node) -> var -> tokens);
            free(((lqdVarReassignNode*)node.node) -> var);
            lqdASTNode_delete(((lqdVarReassignNode*)node.node) -> value);
            free((lqdVarReassignNode*)node.node);
            break;
        case NT_Construct:
            free(((lqdConstructorNode*)node.node) -> struct_path -> tokens);
            free(((lqdConstructorNode*)node.node) -> struct_path);
            lqdStatementsNode_delete(((lqdConstructorNode*)node.node) -> args);
            free((lqdConstructorNode*)node.node);
            break;
        case NT_Arr:
            lqdStatementsNode_delete(((lqdArrayNode*)node.node) -> values);
            free((lqdArrayNode*)node.node);
            break;
        default:
            break;
    }
}

lqdParameterArray* lqdParameterArray_new(uint64_t size) {
    lqdParameterArray* arr = malloc(sizeof(lqdParameterArray));
    arr -> size = size;
    arr -> values = 0;
    arr -> params = malloc(size * sizeof(lqdParameter));
    return arr;
}

void lqdParameterArray_push(lqdParameterArray* arr, lqdParameter param) {
    if (arr -> values >= arr -> size) {
        arr -> size *= 2;
        arr -> params = realloc(arr -> params, arr -> size * sizeof(lqdParameter));
        if (arr -> params == NULL) {
            fprintf(stderr, "Realloc failed, ran out of memory!\n");
            exit(1);
        }
    }
    arr -> params[arr -> values++] = param;
}

void lqdParameterArray_delete(lqdParameterArray* arr) {
    free(arr -> params);
    free(arr);
}

lqdStatementsNode* lqdStatementsNode_new(uint64_t size) {
    lqdStatementsNode* arr = malloc(sizeof(lqdStatementsNode));
    arr -> size = size;
    arr -> values = 0;
    arr -> statements = malloc(size * sizeof(lqdASTNode));
    return arr;
}

void lqdStatementsNode_push(lqdStatementsNode* arr, lqdASTNode statement) {
    if (arr -> values >= arr -> size) {
        arr -> size *= 2;
        arr -> statements = realloc(arr -> statements, arr -> size * sizeof(lqdASTNode));
        if (arr -> statements == NULL) {
            fprintf(stderr, "Realloc failed, ran out of memory!\n");
            exit(1);
        }
    }
    arr -> statements[arr -> values++] = statement;
}

void lqdStatementsNode_delete(lqdStatementsNode* arr) {
    for (int i = 0; i < arr -> values; i++)
        lqdASTNode_delete(arr -> statements[i]);
    free(arr -> statements);
    free(arr);
}
