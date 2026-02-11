#include "stack.h"
#include "utils.h"

void stack_init(OpStack *s) {
    s->items = 0;
    s->size = 0;
    s->capacity = 0;
}

void stack_push(OpStack *s, Operation op) {
    int i;
    if (s->capacity == 0) {
        s->capacity = 8;
        s->items = (Operation *)u_malloc(sizeof(Operation) * s->capacity);
    } else if (s->size >= s->capacity) {
        int newcap = s->capacity * 2;
        Operation *ni = (Operation *)u_malloc(sizeof(Operation) * newcap);
        for (i = 0; i < s->size; i++) {
            ni[i] = s->items[i];
        }
        u_free(s->items);
        s->items = ni;
        s->capacity = newcap;
    }
    s->items[s->size] = op;
    s->size++;
}

int stack_pop(OpStack *s, Operation *out) {
    if (s->size == 0) return 0;
    s->size--;
    *out = s->items[s->size];
    return 1;
}

void stack_clear(OpStack *s) {
    int i;
    for (i = 0; i < s->size; i++) {
        if (s->items[i].path) u_free(s->items[i].path);
        if (s->items[i].old_content) u_free(s->items[i].old_content);
        if (s->items[i].new_content) u_free(s->items[i].new_content);
    }
    if (s->items) u_free(s->items);
    s->items = 0;
    s->size = 0;
    s->capacity = 0;
}
