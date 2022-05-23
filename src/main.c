#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* output_file = "\0";
char keep_asm = 0;
char produce_binary = 0;
char* source_file = "\0";
char* dof = "a";
#ifdef __WIN64
    char* format = "win64";
    char* linker = "ld";
    char* os     = "windows 64bit";
    char* tmp    = "./";
#else
    char* format = "elf64";
    char* linker = "ld";
    char* os     = "unix 64bit";
    char* tmp    = "./";
#endif

char** object_files;
int object_values = 0;
int object_size = 4;

void append_object(char* obj) {
    if (object_values == object_size) {
        object_values *= 2;
        object_files = realloc(object_files, object_values * sizeof(char*));
        if (object_files == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
        }
    }
    object_files[object_values++] = obj;
}

void show_help(char** argv) {
    printf("Seawater compiler on %s\n", os);
    printf("Usage: %s <file.lqd>\n", argv[0]);
    printf("-v                 Print the compiler version\n");
    printf("-o <file>          Specify output file\n");
    printf("-k                 Keep the assembly code\n");
    printf("-l <file>          Link an object file\n");
    printf("-pb                Whether to link the object file into a binary\n");
}

void filter_args(int argc, char** argv) {
    if (!strcmp(argv[1], "help")) {
        show_help(argv);
        exit(0);
    }
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            char* outputfile = malloc(256);
            int p = 0;
            char* o = argv[++i];
            for (int i = 0; i < strlen(o); i++) {
                if (o[i] == '.')break;
                outputfile[p] = o[p];
                p++;
            }
            outputfile[p] = 0;
            output_file = outputfile;
        } else if (!strcmp(argv[i], "-v")) {
            printf("clyqd 0.1-dev\n");
            exit(0);
        } else if (!strcmp(argv[i], "-k")) {
            keep_asm = 1;
        } else if (!strcmp(argv[i], "-l")) {
            append_object(argv[++i]);
        } else if (!strcmp(argv[i], "-pb")) {
            produce_binary = 1;
        } else {
            source_file = argv[i];
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        show_help(argv);
        exit(1);
    }
    object_files = malloc(4 * sizeof(char*));
    filter_args(argc, argv);
    char* code = malloc(2);
    uint64_t code_len = 0;
    uint64_t code_max = 2;
    FILE* file = fopen(source_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Invalid file passed!\n");
        exit(1);
    }
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
    fclose(file);
    lqdTokenArray* tokens = tokenize(code, source_file);
    lqdStatementsNode* AST = parse(tokens, code, source_file);
    if (*output_file == 0)
        output_file = dof;
    char* new_code = x86_64_compile(AST, code, source_file);
    char* asmtmp = malloc(100);
    sprintf(asmtmp, "%slqdtmp.asm", tmp);
    file = fopen(asmtmp, "w");
    fputs(new_code, file);
    fclose(file);
    char* cmd = malloc(1024);
    sprintf(cmd, "nasm -o %s%s.o %s -f %s", tmp, output_file, asmtmp, format);
    system(cmd);
    if (produce_binary) {
        char* obj = malloc(256);
        sprintf(obj, "%s.o", output_file);
        sprintf(cmd, "%s %s%s.o -o %s bin/runtime/runtime.o", linker, tmp, output_file, output_file);
        for (int i = 0; i < object_values; i++) {
            strcat(cmd, " ");
            strcat(cmd, object_files[i]);
        }
        system(cmd);
        remove(obj);
        free(obj);
    }
    if (keep_asm) {
        sprintf(cmd, "mv %s ./%s.asm", asmtmp, output_file);
        system(cmd);
    }
    free(object_files);
    free(cmd);
    free(new_code);
    free(code);
    free(asmtmp);
    lqdTokenArray_delete(tokens);
    lqdStatementsNode_delete(AST);
    return 0;
}