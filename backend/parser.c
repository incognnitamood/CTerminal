#include "parser.h"
#include "utils.h"

void parser_init(TokenArray *t) {
    t->items = 0;
    t->count = 0;
    t->capacity = 0;
}

void parser_free(TokenArray *t) {
    int i;
    for (i = 0; i < t->count; i++) {
        if (t->items[i]) u_free(t->items[i]);
    }
    if (t->items) u_free(t->items);
    t->items = 0;
    t->count = 0;
    t->capacity = 0;
}

static void parser_add_token(TokenArray *t, const char *buf, int len) {
    char *s;
    int i;
    if (len <= 0) return;
    if (t->capacity == 0) {
        t->capacity = 4;
        t->items = (char **)u_malloc(sizeof(char *) * t->capacity);
    } else if (t->count >= t->capacity) {
        int newcap = t->capacity * 2;
        char **ni = (char **)u_malloc(sizeof(char *) * newcap);
        for (i = 0; i < t->count; i++) {
            ni[i] = t->items[i];
        }
        u_free(t->items);
        t->items = ni;
        t->capacity = newcap;
    }
    s = (char *)u_malloc(len + 1);
    for (i = 0; i < len; i++) {
        s[i] = buf[i];
    }
    s[len] = 0;
    t->items[t->count++] = s;
}

void parser_tokenize(const char *line, TokenArray *t) {
    int i = 0;
    char buf[512];
    int bi = 0;
    int in_quotes = 0;
    int escape = 0;
    char c;
    while (1) {
        c = line[i];
        if (c == 0) {
            if (bi > 0) {
                parser_add_token(t, buf, bi);
            }
            break;
        }
        if (escape) {
            buf[bi++] = c;
            escape = 0;
        } else if (c == '\\') {
            escape = 1;
        } else if (c == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes && (c == ' ' || c == '\t')) {
            if (bi > 0) {
                parser_add_token(t, buf, bi);
                bi = 0;
            }
        } else {
            if (bi < 511) {
                buf[bi++] = c;
            }
        }
        i++;
    }
}

