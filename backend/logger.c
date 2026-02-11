#include "logger.h"
#include "utils.h"

void log_init(LogQueue *q, int capacity) {
    int i;
    q->capacity = capacity;
    q->size = 0;
    q->head = 0;
    q->counter = 0;
    q->entries = (LogEntry *)u_malloc(sizeof(LogEntry) * capacity);
    for (i = 0; i < capacity; i++) {
        q->entries[i].timestamp = 0;
        q->entries[i].message = 0;
    }
}

void log_add(LogQueue *q, const char *msg) {
    int idx;
    char buf[32];
    if (q->size < q->capacity) {
        idx = (q->head + q->size) % q->capacity;
        q->size++;
    } else {
        idx = q->head;
        q->head = (q->head + 1) % q->capacity;
    }
    if (q->entries[idx].timestamp) u_free(q->entries[idx].timestamp);
    if (q->entries[idx].message) u_free(q->entries[idx].message);
    u_itoa(q->counter++, buf);
    q->entries[idx].timestamp = u_strdup(buf);
    q->entries[idx].message = u_strdup(msg);
}

int log_get_all(LogQueue *q, LogEntry **out, int *count) {
    int i;
    if (q->size == 0) {
        *out = 0;
        *count = 0;
        return 0;
    }
    *out = (LogEntry *)u_malloc(sizeof(LogEntry) * q->size);
    for (i = 0; i < q->size; i++) {
        int idx = (q->head + i) % q->capacity;
        (*out)[i].timestamp = u_strdup(q->entries[idx].timestamp);
        (*out)[i].message = u_strdup(q->entries[idx].message);
    }
    *count = q->size;
    return 0;
}

