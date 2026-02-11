#ifndef UTILS_H
#define UTILS_H

/* Basic memory helpers */
void *u_malloc(int size);
void u_free(void *p);

/* String helpers (no <string.h>) */
int u_strlen(const char *s);
int u_strcmp(const char *a, const char *b);
int u_strncmp(const char *a, const char *b, int n);
void u_strcpy(char *dst, const char *src);
void u_strcat(char *dst, const char *src);
char *u_strdup(const char *s);
int u_startswith(const char *s, const char *prefix);
int u_find_char(const char *s, char c);

/* Number helpers */
int u_atoi(const char *s);
void u_itoa(int v, char *buf);

/* Levenshtein distance for command suggestions */
int u_levenshtein(const char *s1, const char *s2);

/* File extension validation */
int u_is_valid_extension(const char *ext);
char *u_get_extension(const char *filename);

/* Simple dynamic buffer for building strings */
typedef struct {
    char *data;
    int length;
    int capacity;
} UBuffer;

void ubuf_init(UBuffer *b);
void ubuf_free(UBuffer *b);
void ubuf_append_char(UBuffer *b, char c);
void ubuf_append_str(UBuffer *b, const char *s);
char *ubuf_to_string(UBuffer *b);

#endif

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PATH_LEN 1024
#define MAX_INPUT_LEN 2048
#define MAX_NAME_LEN 256
#define MAX_CONTENT_LEN 65536

char* get_timestamp();
char* str_duplicate(const char* str);
void json_escape_string(const char* src, char* dest, int dest_size);

#endif
