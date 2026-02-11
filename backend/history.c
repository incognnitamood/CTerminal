#include "history.h"
#include "utils.h"

static HistoryList hist;

void history_init(int max_size) {
    hist.head = 0;
    hist.tail = 0;
    hist.size = 0;
    hist.max_size = max_size;
}

void history_add(const char *cmd) {
    HistoryNode *n;
    if (!cmd || cmd[0] == 0) return;
    n = (HistoryNode *)u_malloc(sizeof(HistoryNode));
    n->command = u_strdup(cmd);
    n->prev = hist.tail;
    n->next = 0;
    if (!hist.head) hist.head = n;
    if (hist.tail) hist.tail->next = n;
    hist.tail = n;
    hist.size++;
    if (hist.size > hist.max_size) {
        HistoryNode *old = hist.head;
        hist.head = old->next;
        if (hist.head) hist.head->prev = 0;
        if (old->command) u_free(old->command);
        u_free(old);
        hist.size--;
    }
}

int history_get_all(char ***out, int *count) {
    HistoryNode *cur = hist.head;
    int c = hist.size;
    int i = 0;
    if (c == 0) {
        *out = 0;
        *count = 0;
        return 0;
    }
    *out = (char **)u_malloc(sizeof(char *) * c);
    while (cur) {
        (*out)[i++] = u_strdup(cur->command);
        cur = cur->next;
    }
    *count = c;
    return 0;
}
