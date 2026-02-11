#ifndef COMMANDS_H
#define COMMANDS_H

#include "parser.h"
#include "stack.h"
#include "hashmap.h"
#include "logger.h"
#include "trie.h"

typedef struct {
    int status;         /* 0 ok, nonzero error */
    char *stdout_text;  /* main output */
    char *stderr_text;  /* error message, may be empty */
    char **suggestions; /* for autocomplete */
    int suggestion_count;
} CommandResult;

void commands_init();
CommandResult cmd_execute(TokenArray *tokens);

#endif

