#ifndef TRIE_H
#define TRIE_H

typedef struct TrieNode {
    int is_end;
    struct TrieNode *children[256];
} TrieNode;

TrieNode *trie_create();
void trie_insert(TrieNode *root, const char *word);
int trie_complete(TrieNode *root, const char *prefix,
                  char ***out_words, int *out_count);

#endif

