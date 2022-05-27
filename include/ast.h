#pragma once

#include "token.h"
#include "lqdint.h"

typedef enum {
    NT_NULL,
    NT_NoStmnt,
    NT_Number,
    NT_Char,
    NT_String,
    NT_VarAccess,
    NT_BinOp,
    NT_Unary,
    NT_Arr,

    NT_VarDecl,
    NT_FuncDecl,

    NT_Statements,
    NT_FuncCall,
    NT_Employ,
    NT_If,
    NT_For,
    NT_While,
    NT_Struct,

    NT_VarReassign,

    NT_Return,
    NT_Continue,
    NT_Break,

    NT_Construct,
} lqdNodeType;


typedef struct {
    lqdToken type;
} lqdType;

typedef struct {
    lqdType type;
    lqdToken element_type;
    lqdToken name;
} lqdParameter;

typedef struct {
    lqdParameter* params;
    uint64_t values;
    uint64_t size;
} lqdParameterArray;
lqdParameterArray* lqdParameterArray_new(uint64_t size);
void lqdParameterArray_push(lqdParameterArray* arr, lqdParameter param);
void lqdParameterArray_delete(lqdParameterArray* arr);


typedef struct {
    lqdNodeType type;
    void* node;
} lqdASTNode;

typedef struct {
    lqdToken tok;
} lqdNumberNode;

typedef lqdNumberNode lqdCharNode;
typedef lqdNumberNode lqdStringNode;
typedef lqdNumberNode lqdContinueNode;
typedef lqdNumberNode lqdBreakNode;

typedef struct {
    lqdTokenArray* path;
    lqdASTNode slice;
    char has_slice;
} lqdVarAccessNode;

typedef struct {
    lqdASTNode left;
    lqdToken op;
    lqdASTNode right;
} lqdBinOpNode;

typedef struct {
    lqdToken op;
    lqdASTNode value;
} lqdUnaryOpNode;

typedef struct {
    lqdType type;
    lqdToken name;
    lqdASTNode initializer;
    char initialized;
    char has_element_type;
    lqdToken element_type;
} lqdVarDeclNode;

typedef struct {
    lqdType ret_type;
    lqdToken name;
    lqdParameterArray* params;
    char is_void;
    lqdASTNode statement;
    char is_extern;
} lqdFuncDeclNode;

typedef struct {
    lqdTokenArray* lib_path;
    lqdToken alias;
    char is_aliased;
} lqdEmployNode;

typedef struct {
    lqdASTNode* statements;
    uint64_t values;
    uint64_t size;
} lqdStatementsNode;

typedef struct {
    char returns;
    lqdASTNode return_value;
} lqdReturnNode;

typedef struct {
    lqdStatementsNode* conditions;
    lqdStatementsNode* bodies;
    lqdASTNode else_case;
    char has_else_case;
} lqdIfNode;

typedef struct {
    lqdTokenArray* call_path;
    lqdStatementsNode* args;
} lqdFuncCallNode;

typedef struct {
    lqdToken iterator;
    lqdTokenArray* arr;
    lqdASTNode body;
} lqdForNode;

typedef struct {
    lqdASTNode condition;
    lqdASTNode body;
} lqdWhileNode;

typedef struct {
    lqdToken name;
    lqdStatementsNode* body;
} lqdStructNode;

typedef struct {
    lqdTokenArray* var;
    lqdASTNode value;
    lqdASTNode slice;
    char has_slice;
} lqdVarReassignNode;

typedef struct {
    lqdTokenArray* struct_path;
    lqdStatementsNode* args;
} lqdConstructorNode;

typedef struct {
    lqdStatementsNode* values;
    char is_reserved;
    lqdToken reserved;
} lqdArrayNode;

lqdStatementsNode* lqdStatementsNode_new(uint64_t size);
void lqdStatementsNode_push(lqdStatementsNode* arr, lqdASTNode statement);
void lqdStatementsNode_delete(lqdStatementsNode* arr);

void lqdASTNode_print(lqdASTNode node, uint8_t indent);
void lqdASTNode_delete(lqdASTNode node);