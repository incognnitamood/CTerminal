#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char **items;
    int count;
    int capacity;
} TokenArray;

void parser_init(TokenArray *t);
void parser_free(TokenArray *t);
void parser_tokenize(const char *line, TokenArray *t);

#endif

