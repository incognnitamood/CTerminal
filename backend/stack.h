#ifndef STACK_H
#define STACK_H

typedef enum {
    OP_CREATE_FILE,
    OP_DELETE_FILE,
    OP_CREATE_DIR,
    OP_DELETE_DIR,
    OP_WRITE_FILE,
    OP_MOVE,
    OP_RENAME
} OperationType;

typedef struct {
    OperationType type;
    char *path;
    char *old_content;
    char *new_content;
} Operation;

typedef struct {
    Operation *items;
    int size;
    int capacity;
} OpStack;

void stack_init(OpStack *s);
void stack_push(OpStack *s, Operation op);
int stack_pop(OpStack *s, Operation *out);
void stack_clear(OpStack *s);

#endif

#ifndef STACK_H
#define STACK_H

#include "utils.h"
#include "filesystem.h"

typedef enum {
    OP_CREATE_FILE,
    OP_CREATE_DIR,
    OP_DELETE_FILE,
    OP_DELETE_DIR,
    OP_WRITE_FILE
} OperationType;

typedef struct {
    OperationType type;
    char path[MAX_PATH_LEN];
    char* old_content;
    int old_readable;
    int old_writable;
} Operation;

typedef struct StackNode {
    Operation op;
    struct StackNode* next;
} StackNode;

typedef struct {
    StackNode* top;
    int count;
} Stack;

void stack_init(Stack* stack);
void stack_push(Stack* stack, Operation op);
int stack_pop(Stack* stack, Operation* op);
void stack_clear(Stack* stack);
void stack_cleanup(Stack* stack);

#endif
