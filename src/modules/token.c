#include "token.h"
#include <stdlib.h>
#include <stdio.h>
#include "lqdint.h"

lqdTokenArray* lqdTokenArray_new(uint64_t size) {
    lqdTokenArray* arr = malloc(sizeof(lqdTokenArray));
    arr -> size = size;
    arr -> values = 0;
    arr -> tokens = malloc(size * sizeof(lqdToken));
    return arr;
}

void lqdTokenArray_push(lqdTokenArray* arr, lqdToken token) {
    if (arr -> values >= arr -> size) {
        arr -> size *= 2;
        arr -> tokens = realloc(arr -> tokens, arr -> size * sizeof(lqdToken));
        if (arr -> tokens == NULL) {
            fprintf(stderr, "Realloc failed, ran out of memory!\n");
            exit(1);
        }
    }
    arr -> tokens[arr -> values++] = token;
}

void lqdTokenArray_delete(lqdTokenArray* arr) {
    for (int i = 0; i < arr -> values; i++)
        lqdToken_delete(&arr -> tokens[i]);
    free(arr -> tokens);
    free(arr);
}

lqdToken lqdToken_new(lqdTokenType type, char* value, uint64_t idx_start, uint64_t idx_end, uint64_t line) {
    lqdToken tkn;
    tkn.type = type;
    tkn.value = value;
    tkn.idx_start = idx_start - 1;
    tkn.idx_end = idx_end - 1; // - 1 to fix strange offsets, purely for error visuals
    tkn.line = line;
    return tkn;
}

void lqdToken_delete(lqdToken* token) {
    if (token -> type >= TT_INT && token -> type <= TT_IDEN) {
        free(token -> value);
    }
}