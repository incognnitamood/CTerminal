#include "trie.h"
#include "utils.h"

TrieNode *trie_create() {
    int i;
    TrieNode *n = (TrieNode *)u_malloc(sizeof(TrieNode));
    n->is_end = 0;
    for (i = 0; i < 256; i++) n->children[i] = 0;
    return n;
}

void trie_insert(TrieNode *root, const char *word) {
    TrieNode *cur = root;
    int i = 0;
    unsigned char c;
    while ((c = (unsigned char)word[i]) != 0) {
        if (!cur->children[c]) {
            cur->children[c] = trie_create();
        }
        cur = cur->children[c];
        i++;
    }
    cur->is_end = 1;
}

static void trie_collect_rec(TrieNode *node, char *buf, int depth,
                             char ***out_words, int *out_count, int *out_cap) {
    int i;
    if (!node) return;
    if (node->is_end) {
        int len = depth;
        char *s;
        int j;
        if (*out_cap == 0) {
            *out_cap = 4;
            *out_words = (char **)u_malloc(sizeof(char *) * (*out_cap));
        } else if (*out_count >= *out_cap) {
            int newcap = (*out_cap) * 2;
            char **nw = (char **)u_malloc(sizeof(char *) * newcap);
            for (j = 0; j < *out_count; j++) nw[j] = (*out_words)[j];
            u_free(*out_words);
            *out_words = nw;
            *out_cap = newcap;
        }
        s = (char *)u_malloc(len + 1);
        for (i = 0; i < len; i++) s[i] = buf[i];
        s[len] = 0;
        (*out_words)[(*out_count)++] = s;
    }
    for (i = 0; i < 256; i++) {
        if (node->children[i]) {
            if (depth < 255) {
                buf[depth] = (char)i;
                trie_collect_rec(node->children[i], buf, depth + 1,
                                 out_words, out_count, out_cap);
            }
        }
    }
}

int trie_complete(TrieNode *root, const char *prefix,
                  char ***out_words, int *out_count) {
    TrieNode *cur = root;
    int i = 0;
    unsigned char c;
    char buf[256];
    int depth = 0;
    int cap = 0;
    *out_words = 0;
    *out_count = 0;
    while ((c = (unsigned char)prefix[i]) != 0) {
        if (!cur->children[c]) return 0;
        cur = cur->children[c];
        buf[depth++] = (char)c;
        i++;
    }
    trie_collect_rec(cur, buf, depth, out_words, out_count, &cap);
    return 0;
}

