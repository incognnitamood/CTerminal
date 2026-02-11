#ifndef HISTORY_H
#define HISTORY_H

typedef struct HistoryNode {
    char *command;
    struct HistoryNode *prev;
    struct HistoryNode *next;
} HistoryNode;

typedef struct {
    HistoryNode *head;
    HistoryNode *tail;
    int size;
    int max_size;
} HistoryList;

void history_init(int max_size);
void history_add(const char *cmd);
int history_get_all(char ***out, int *count);

#endif

