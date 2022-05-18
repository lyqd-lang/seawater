#include "token.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lqdint.h"

void lqdTokenizerError(lqdToken tok, char* message, char* code, char* filename) {
    fprintf(stderr, "╭────────────────────[ Tokenization Error ]────→\n");
    fprintf(stderr, "│ %s\n", message);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ ");
    int idx = 0;
    int i = 0;
    for (i = 0; i < tok.idx_start; i++)
        if (code[i] == '\n') idx = i;
    i = 1;
    while (code[idx + i] && code[idx + i] != '\n') {
        fprintf(stderr, "%c", code[idx + i]);
        i++;
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "│ ");
    for (i = 0; i < tok.idx_start - idx; i++)
        fprintf(stderr, " ");
    fprintf(stderr, "^");
    for (i = 0; i < tok.idx_end - tok.idx_start; i++)
        fprintf(stderr, "~");
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "│ Line: %ld, File: %s\n", tok.line, filename);
    fprintf(stderr, "╯\n");
    exit(1);
}

lqdTokenArray* tokenize(char* code, char* filename) {
    lqdTokenArray* tokens = lqdTokenArray_new(2);
    uint64_t idx = 0;
    uint64_t line = 1;
    while (code[idx]) {
        switch (code[idx]) {
            case '+':
                lqdTokenArray_push(tokens, lqdToken_new(TT_ADD, NULL, idx, idx, line));
                idx++;
                break;
            case '-':
                lqdTokenArray_push(tokens, lqdToken_new(TT_SUB, NULL, idx, idx, line));
                idx++;
                break;
            case '*':
                lqdTokenArray_push(tokens, lqdToken_new(TT_MUL, NULL, idx, idx, line));
                idx++;
                break;
            case '/':
                if (code[idx+1] == '/') {
                    while (code[idx] != '\n')idx++;
                    idx++;
                } else {
                    lqdTokenArray_push(tokens, lqdToken_new(TT_DIV, NULL, idx, idx, line));
                    idx++;
                }
                break;
            case '(':
                lqdTokenArray_push(tokens, lqdToken_new(TT_LPAREN, NULL, idx, idx, line));
                idx++;
                break;
            case ')':
                lqdTokenArray_push(tokens, lqdToken_new(TT_RPAREN, NULL, idx, idx, line));
                idx++;
                break;
            case '[':
                lqdTokenArray_push(tokens, lqdToken_new(TT_LBRACKET, NULL, idx, idx, line));
                idx++;
                break;
            case ']':
                lqdTokenArray_push(tokens, lqdToken_new(TT_RBRACKET, NULL, idx, idx, line));
                idx++;
                break;
            case '{':
                lqdTokenArray_push(tokens, lqdToken_new(TT_LBRACE, NULL, idx, idx, line));
                idx++;
                break;
            case '}':
                lqdTokenArray_push(tokens, lqdToken_new(TT_RBRACE, NULL, idx, idx, line));
                idx++;
                break;
            case '.':
                lqdTokenArray_push(tokens, lqdToken_new(TT_PERIOD, NULL, idx, idx, line));
                idx++;
                break;
            case ',':
                lqdTokenArray_push(tokens, lqdToken_new(TT_COMMA, NULL, idx, idx, line));
                idx++;
                break;
            case ':':
                lqdTokenArray_push(tokens, lqdToken_new(TT_COLON, NULL, idx, idx, line));
                idx++;
                break;
            case ';':
                lqdTokenArray_push(tokens, lqdToken_new(TT_SEMICOLON, NULL, idx, idx, line));
                idx++;
                break;
            case '<': {
                lqdTokenType type = TT_LT;
                if (*(code + idx + 1) == '=') {
                    type = TT_LTE;
                    idx++;
                }
                lqdTokenArray_push(tokens, lqdToken_new(type, NULL, type == TT_LTE ? idx - 1 : idx, idx, line));
                idx++;
            } break;
            case '>': {
                lqdTokenType type = TT_GT;
                if (*(code + idx + 1) == '=') {
                    type = TT_GTE;
                    idx++;
                }
                lqdTokenArray_push(tokens, lqdToken_new(type, NULL, type == TT_GTE ? idx - 1 : idx, idx, line));
                idx++;
            } break;
            case '!': {
                lqdTokenType type = TT_NOT;
                if (*(code + idx + 1) == '=') {
                    type = TT_NE;
                    idx++;
                }
                lqdTokenArray_push(tokens, lqdToken_new(type, NULL, type == TT_NE ? idx - 1 : idx, idx, line));
                idx++;
            } break;
            case '=': {
                lqdTokenType type = TT_EQ;
                if (*(code + idx + 1) == '=') {
                    type = TT_EQEQ;
                    idx++;
                }
                lqdTokenArray_push(tokens, lqdToken_new(type, NULL, type == TT_EQEQ ? idx - 1 : idx, idx, line));
                idx++;
            } break;
            default: {
                if (
                    (code[idx] >= 'A' && code[idx] <= 'Z') ||
                    (code[idx] >= 'a' && code[idx] <= 'z') ||
                    code[idx] == '_'
                ) {
                    uint64_t idx_start = idx;
                    char* value = malloc(16);
                    uint64_t value_size = 16;
                    uint64_t value_ptr = 0;
                    while (
                        (code[idx] >= 'A' && code[idx] <= 'Z') ||
                        (code[idx] >= 'a' && code[idx] <= 'z') ||
                        (code[idx] >= '0' && code[idx] <= '9') ||
                        code[idx] == '_'
                    ) {
                        if (value_ptr >= value_size) {
                            value_size *= 2;
                            value = realloc(value, value_size);
                            if (value == NULL) {
                                lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "Identifier too long! Ran out of memory\n", code, filename);
                                exit(1);
                            }
                        }
                        value[value_ptr++] = code[idx];
                        idx++;
                    }
                    value = realloc(value, value_size+1);
                    value[value_ptr++] = 0;
                    lqdTokenArray_push(tokens, lqdToken_new(TT_IDEN, value, idx_start, idx-1, line));
                    break;
                }
                if ((code[idx] >= '0' && code[idx] <= '9') || code[idx] == '.') {
                    uint64_t idx_start = idx;
                    char* value = malloc(16);
                    uint64_t value_size = 16;
                    uint64_t value_ptr = 0;
                    char dots = 0;
                    while ((code[idx] >= '0' && code[idx] <= '9') || code[idx] == '.') {
                        if (value_ptr >= value_size) {
                            value_size *= 2;
                            value = realloc(value, value_size);
                            if (value == NULL) {
                                lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "Number too long! Ran out of memory\n", code, filename);
                                exit(1);
                            }
                        }
                        if (code[idx] == '.')
                            dots++;
                        value[value_ptr++] = code[idx];
                        idx++;
                    }
                    if (dots > 1) {
                        lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "Malformed number", code, filename);
                        exit(1);
                    }
                    value = realloc(value, value_size+1);
                    value[value_ptr++] = 0;
                    lqdTokenArray_push(tokens, lqdToken_new(dots == 0 ? TT_INT : TT_FLOAT, value, idx_start, idx-1, line));
                    break;
                }
                if (code[idx] == '"') {
                    uint64_t idx_start = idx;
                    char* value = malloc(16);
                    uint64_t value_size = 16;
                    uint64_t value_ptr = 0;
                    idx++;
                    while (code[idx] && code[idx] != '"') {
                        if (value_ptr >= value_size) {
                            value_size *= 2;
                            value = realloc(value, value_size);
                            if (value == NULL) {
                                lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "String too long! Ran out of memory\n", code, filename);
                                exit(1);
                            }
                        }
                        if (code[idx] == '\\') {
                            idx++;
                            while (value_ptr+6 >= value_size) {
                                value_size *= 2;
                                value = realloc(value, value_size);
                                if (value == NULL) {
                                    lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "String too long! Ran out of memory\n", code, filename);
                                    exit(1);
                                }
                            }
                            switch (code[idx]) {
                                case 'n':
                                    value[value_ptr++] = 10;
                                    break;
                                case 't':
                                    value[value_ptr++] = 9;
                                    break;
                                case '0':
                                    value[value_ptr++] = 0;
                                    break;
                                default:
                                    value[value_ptr++] = code[idx];
                                    break;
                            }
                        } else {
                            value[value_ptr++] = code[idx];
                        }
                        idx++;
                    }
                    if (!code[idx])
                        lqdTokenizerError(lqdToken_new(0, NULL, idx_start, idx, line), "Reached EOF when tokenizing string", code, filename);
                    idx++;
                    value = realloc(value, value_size+1);
                    value[value_ptr++] = 0;
                    lqdTokenArray_push(tokens, lqdToken_new(TT_STR, value, idx_start, idx-1, line));
                    break;
                }
                if (code[idx] == '\'') {
                    uint64_t idx_start = idx;
                    char* chr = malloc(4);
                    if (code[++idx] == '\\') {
                        switch (code[++idx]) {
                            case 'n':
                                chr[0] = '1';
                                chr[1] = '0';
                                chr[2] = 0;
                            case 't':
                                chr[0] = '9';
                                chr[1] = 0;
                        }
                    } else {
                        chr[0] = '\'';
                        chr[1] = code[idx];
                        chr[2] = '\'';
                        chr[3] = 0;
                    }
                    idx += 2;
                    lqdTokenArray_push(tokens, lqdToken_new(TT_CHAR, chr, idx_start, idx-1, line));
                    break;
                }
                if (code[idx] == '\n') {
                    line++;
                }
                idx++;
                break;
            }
        }
    }
    lqdTokenArray_push(tokens, lqdToken_new(TT_EOF, NULL, idx, idx, line));
    return tokens;
}