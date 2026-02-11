#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void *u_malloc(int size) {
    void *p = malloc(size);
    if (!p) {
        /* Fatal error */
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

void u_free(void *p) {
    if (p) {
        free(p);
    }
}

int u_strlen(const char *s) {
    int n = 0;
    if (!s) return 0;
    while (s[n] != 0) {
        n++;
    }
    return n;
}

int u_strcmp(const char *a, const char *b) {
    int i = 0;
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
        i++;
    }
    if (a[i] == 0 && b[i] == 0) return 0;
    return (a[i] == 0) ? -1 : 1;
}

int u_strncmp(const char *a, const char *b, int n) {
    int i = 0;
    if (n <= 0) return 0;
    while (i < n && a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
        i++;
    }
    if (i == n) return 0;
    if (a[i] == 0 && b[i] == 0) return 0;
    return (a[i] == 0) ? -1 : 1;
}

void u_strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i] != 0) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

void u_strcat(char *dst, const char *src) {
    int i = 0;
    int j = 0;
    while (dst[i] != 0) i++;
    while (src[j] != 0) {
        dst[i + j] = src[j];
        j++;
    }
    dst[i + j] = 0;
}

char *u_strdup(const char *s) {
    int n = u_strlen(s);
    char *p = (char *)u_malloc(n + 1);
    int i;
    for (i = 0; i < n; i++) {
        p[i] = s[i];
    }
    p[n] = 0;
    return p;
}

int u_startswith(const char *s, const char *prefix) {
    int i = 0;
    while (prefix[i] != 0) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

int u_find_char(const char *s, char c) {
    int i = 0;
    while (s[i] != 0) {
        if (s[i] == c) return i;
        i++;
    }
    return -1;
}

int u_atoi(const char *s) {
    int i = 0;
    int sign = 1;
    int v = 0;
    if (!s) return 0;
    if (s[0] == '-') {
        sign = -1;
        i++;
    }
    while (s[i] >= '0' && s[i] <= '9') {
        v = v * 10 + (s[i] - '0');
        i++;
    }
    return sign * v;
}

void u_itoa(int v, char *buf) {
    char tmp[32];
    int i = 0;
    int sign = 0;
    int j;
    if (v < 0) {
        sign = 1;
        v = -v;
    }
    if (v == 0) {
        tmp[i++] = '0';
    } else {
        while (v > 0) {
            int d = v % 10;
            tmp[i++] = (char)('0' + d);
            v /= 10;
        }
    }
    if (sign) {
        tmp[i++] = '-';
    }
    /* reverse into buf */
    j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = 0;
}

void ubuf_init(UBuffer *b) {
    b->capacity = 64;
    b->length = 0;
    b->data = (char *)u_malloc(b->capacity);
    b->data[0] = 0;
}

void ubuf_free(UBuffer *b) {
    if (b->data) {
        u_free(b->data);
        b->data = 0;
    }
    b->length = 0;
    b->capacity = 0;
}

static void ubuf_grow(UBuffer *b, int extra) {
    int need = b->length + extra + 1;
    if (need <= b->capacity) return;
    int newcap = b->capacity * 2;
    int i;
    while (newcap < need) newcap *= 2;
    char *nd = (char *)u_malloc(newcap);
    for (i = 0; i < b->length; i++) {
        nd[i] = b->data[i];
    }
    nd[b->length] = 0;
    u_free(b->data);
    b->data = nd;
    b->capacity = newcap;
}

void ubuf_append_char(UBuffer *b, char c) {
    ubuf_grow(b, 1);
    b->data[b->length++] = c;
    b->data[b->length] = 0;
}

void ubuf_append_str(UBuffer *b, const char *s) {
    int n = u_strlen(s);
    int i;
    ubuf_grow(b, n);
    for (i = 0; i < n; i++) {
        b->data[b->length + i] = s[i];
    }
    b->length += n;
    b->data[b->length] = 0;
}

char *ubuf_to_string(UBuffer *b) {
    char *out = (char *)u_malloc(b->length + 1);
    int i;
    for (i = 0; i < b->length; i++) {
        out[i] = b->data[i];
    }
    out[b->length] = 0;
    return out;
}

