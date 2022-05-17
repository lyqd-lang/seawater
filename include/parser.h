#pragma once

#include "ast.h"
#include "token.h"
#include "lqdint.h"

typedef struct {
    lqdToken tok;
    lqdTokenArray* toks;
    uint64_t tok_idx;
    char* code;
    char* filename;
} lqdParserCtx;

lqdStatementsNode* parse(lqdTokenArray* tokens, char* code, char* filename);