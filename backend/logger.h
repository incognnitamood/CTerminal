#ifndef LOGGER_H
#define LOGGER_H

typedef struct {
    char *timestamp;
    char *message;
} LogEntry;

typedef struct {
    LogEntry *entries;
    int size;
    int capacity;
    int head;
    int counter;
} LogQueue;

void log_init(LogQueue *q, int capacity);
void log_add(LogQueue *q, const char *msg);
int log_get_all(LogQueue *q, LogEntry **out, int *count);

#endif

