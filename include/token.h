#pragma once

#include "lqdint.h"

typedef enum {
    /* Basic types */
    TT_INT,
    TT_FLOAT,
    TT_CHAR,
    TT_STR,
    TT_IDEN,

    /* Operators */
    TT_ADD,
    TT_SUB,
    TT_MUL,
    TT_DIV,
    TT_MOD,

    /* Parentheses */
    TT_LPAREN,
    TT_RPAREN,
    TT_LBRACE,
    TT_RBRACE,
    TT_LBRACKET,
    TT_RBRACKET,

    /* Equity */
    TT_EQ,
    TT_NE,
    TT_EQEQ,
    TT_LT,
    TT_GT,
    TT_LTE,
    TT_GTE,

    /* Punctuation */
    TT_PERIOD,
    TT_COMMA,
    TT_COLON,
    TT_SEMICOLON,

    /* Bitwise */
    TT_NOT,
    TT_AND,
    TT_OR,
    TT_BITNOT,
    TT_BITAND,
    TT_BITOR,

    /* Misc */
    TT_EOF,
} lqdTokenType;

typedef struct {
    lqdTokenType type;
    char* value;
    uint64_t idx_start;
    uint64_t idx_end;
    uint64_t line;
} lqdToken;

typedef struct {
    lqdToken* tokens;
    uint64_t values;
    uint64_t size;
} lqdTokenArray;

lqdTokenArray* lqdTokenArray_new(uint64_t size);
lqdToken lqdToken_new(lqdTokenType type, char* value, uint64_t idx_start, uint64_t idx_end, uint64_t line);
void lqdTokenArray_push(lqdTokenArray* arr, lqdToken token);
void lqdTokenArray_delete(lqdTokenArray* arr);
void lqdToken_delete(lqdToken* token);