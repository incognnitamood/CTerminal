#include <stdio.h>
#include "commands.h"
#include "filesystem.h"
#include "history.h"
#include "utils.h"

static OpStack undo_stack;
static OpStack redo_stack;
static HashMap vars;
static LogQueue logger_q;
static TrieNode *trie_root = 0;

static void cr_init(CommandResult *r) {
    r->status = 0;
    r->stdout_text = 0;
    r->stderr_text = 0;
    r->suggestions = 0;
    r->suggestion_count = 0;
}

static void cr_set_out(CommandResult *r, const char *s) {
    if (r->stdout_text) u_free(r->stdout_text);
    r->stdout_text = u_strdup(s ? s : "");
}

static void cr_set_err(CommandResult *r, const char *s) {
    if (r->stderr_text) u_free(r->stderr_text);
    r->stderr_text = u_strdup(s ? s : "");
}

void commands_init() {
    stack_init(&undo_stack);
    stack_init(&redo_stack);
    hm_init(&vars, 128);
    log_init(&logger_q, 50);
    trie_root = trie_create();

    /* Seed commands */
    trie_insert(trie_root, "mkdir");
    trie_insert(trie_root, "ls");
    trie_insert(trie_root, "cd");
    trie_insert(trie_root, "touch");
    trie_insert(trie_root, "write");
    trie_insert(trie_root, "read");
    trie_insert(trie_root, "rm");
    trie_insert(trie_root, "rmdir");
    trie_insert(trie_root, "cat");
    trie_insert(trie_root, "pwd");
    trie_insert(trie_root, "set");
    trie_insert(trie_root, "get");
    trie_insert(trie_root, "unset");
    trie_insert(trie_root, "listenv");
    trie_insert(trie_root, "history");
    trie_insert(trie_root, "undo");
    trie_insert(trie_root, "redo");
    trie_insert(trie_root, "search");
    trie_insert(trie_root, "help");
    trie_insert(trie_root, "log");
    trie_insert(trie_root, "chmod");
    trie_insert(trie_root, "export");
    trie_insert(trie_root, "import");
    trie_insert(trie_root, "cp");
    trie_insert(trie_root, "mv");
    trie_insert(trie_root, "tree");
    trie_insert(trie_root, "complete");
}

/* Simple help text */
static const char *help_text[] = {
    "mkdir <dir> - create directory",
    "ls [path] - list directory",
    "cd <path> - change directory",
    "touch <file> - create empty file",
    "write <file> <text> - write text to file",
    "read <file> - read file content",
    "rm <file> - delete file",
    "rmdir <dir> - delete empty directory",
    "cat <file> - show file with line numbers",
    "pwd - print working directory",
    "set <k> <v> - set variable",
    "get <k> - get variable",
    "unset <k> - remove variable",
    "listenv - list variables",
    "history - show command history",
    "undo - undo last operation",
    "redo - redo last undone operation",
    "search <path> <keyword> - search in files",
    "help [cmd] - show help",
    "log - show logs",
    "chmod <path> <r> <w> - set perms",
    "export <file> - export state",
    "import <file> - import state",
    "complete <prefix> - autocomplete"
};

static void append_line(UBuffer *b, const char *s) {
    ubuf_append_str(b, s);
    ubuf_append_char(b, '\n');
}

/* Filesystem commands */

static CommandResult cmd_mkdir(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "mkdir: missing operand");
        return r;
    }
    if (fs_mkdir(t->items[1]) != 0) {
        r.status = 1;
        cr_set_err(&r, "mkdir: cannot create directory");
    } else {
        cr_set_out(&r, "");
        /* record undo */
        {
            Operation op;
            op.type = OP_CREATE_DIR;
            op.path = u_strdup(t->items[1]);
            op.old_content = 0;
            op.new_content = 0;
            stack_push(&undo_stack, op);
            stack_clear(&redo_stack);
        }
    }
    return r;
}

static CommandResult cmd_ls(TokenArray *t) {
    CommandResult r;
    char **names;
    int count;
    int i;
    UBuffer b;
    cr_init(&r);
    if (fs_ls(t->count > 1 ? t->items[1] : 0, &names, &count) != 0) {
        r.status = 1;
        cr_set_err(&r, "ls: cannot access");
        return r;
    }
    ubuf_init(&b);
    for (i = 0; i < count; i++) {
        append_line(&b, names[i]);
        u_free(names[i]);
    }
    if (names) u_free(names);
    r.stdout_text = ubuf_to_string(&b);
    return r;
}

static CommandResult cmd_cd(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        fs_cd("/");
        cr_set_out(&r, "");
        return r;
    }
    if (fs_cd(t->items[1]) != 0) {
        r.status = 1;
        cr_set_err(&r, "cd: no such directory");
    } else {
        cr_set_out(&r, "");
    }
    return r;
}

static CommandResult cmd_touch(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "touch: missing file");
        return r;
    }
    if (fs_touch(t->items[1]) != 0) {
        r.status = 1;
        cr_set_err(&r, "touch: cannot create file");
    } else {
        cr_set_out(&r, "");
        {
            Operation op;
            op.type = OP_CREATE_FILE;
            op.path = u_strdup(t->items[1]);
            op.old_content = 0;
            op.new_content = 0;
            stack_push(&undo_stack, op);
            stack_clear(&redo_stack);
        }
    }
    return r;
}

static CommandResult cmd_write(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 3) {
        r.status = 1;
        cr_set_err(&r, "write: need file and text");
        return r;
    }
    {
        char *old = fs_read(t->items[1]);
        UBuffer b;
        int i;
        ubuf_init(&b);
        for (i = 2; i < t->count; i++) {
            if (i > 2) ubuf_append_char(&b, ' ');
            ubuf_append_str(&b, t->items[i]);
        }
        {
            char *text = ubuf_to_string(&b);
            if (fs_write(t->items[1], text, 0) != 0) {
                r.status = 1;
                cr_set_err(&r, "write: failed");
                u_free(text);
                if (old) u_free(old);
                return r;
            }
            {
                Operation op;
                op.type = OP_WRITE_FILE;
                op.path = u_strdup(t->items[1]);
                op.old_content = old ? u_strdup(old) : u_strdup("");
                op.new_content = u_strdup(text);
                stack_push(&undo_stack, op);
                stack_clear(&redo_stack);
            }
            u_free(text);
        }
        if (old) u_free(old);
        cr_set_out(&r, "");
    }
    return r;
}

static CommandResult cmd_read(TokenArray *t) {
    CommandResult r;
    char *c;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "read: missing file");
        return r;
    }
    c = fs_read(t->items[1]);
    if (!c) {
        r.status = 1;
        cr_set_err(&r, "read: cannot read");
    } else {
        r.stdout_text = c;
    }
    return r;
}

static CommandResult cmd_rm(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "rm: missing file");
        return r;
    }
    char *old = fs_read(t->items[1]);
    if (fs_rm(t->items[1]) != 0) {
        r.status = 1;
        cr_set_err(&r, "rm: cannot remove");
        if (old) u_free(old);
    } else {
        cr_set_out(&r, "");
        {
            Operation op;
            op.type = OP_DELETE_FILE;
            op.path = u_strdup(t->items[1]);
            op.old_content = old ? u_strdup(old) : 0;
            op.new_content = 0;
            stack_push(&undo_stack, op);
            stack_clear(&redo_stack);
        }
    }
    if (old) u_free(old);
    return r;
}

static CommandResult cmd_rmdir(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "rmdir: missing dir");
        return r;
    }
    if (fs_rmdir(t->items[1]) != 0) {
        r.status = 1;
        cr_set_err(&r, "rmdir: cannot remove");
    } else {
        cr_set_out(&r, "");
        /* not recorded for undo due to lack of subtree snapshot */
    }
    return r;
}

static CommandResult cmd_pwd(TokenArray *t) {
    CommandResult r;
    char *p = fs_pwd();
    cr_init(&r);
    r.stdout_text = p;
    return r;
}

static CommandResult cmd_cat(TokenArray *t) {
    CommandResult r;
    char *c;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "cat: missing file");
        return r;
    }
    c = fs_read(t->items[1]);
    if (!c) {
        r.status = 1;
        cr_set_err(&r, "cat: cannot read");
    } else {
        UBuffer b;
        int i = 0;
        int line = 1;
        char num[32];
        ubuf_init(&b);
        u_itoa(line, num);
        ubuf_append_str(&b, num);
        ubuf_append_str(&b, ": ");
        while (c[i] != 0) {
            char ch = c[i];
            ubuf_append_char(&b, ch);
            if (ch == '\n') {
                line++;
                u_itoa(line, num);
                ubuf_append_str(&b, num);
                ubuf_append_str(&b, ": ");
            }
            i++;
        }
        r.stdout_text = ubuf_to_string(&b);
        u_free(c);
    }
    return r;
}

/* Variables */

static CommandResult cmd_set(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 3) {
        r.status = 1;
        cr_set_err(&r, "set: need key and value");
        return r;
    }
    hm_set(&vars, t->items[1], t->items[2]);
    cr_set_out(&r, "");
    return r;
}

static CommandResult cmd_get(TokenArray *t) {
    CommandResult r;
    char *v;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "get: need key");
        return r;
    }
    v = hm_get(&vars, t->items[1]);
    if (!v) {
        r.status = 1;
        cr_set_err(&r, "get: not found");
    } else {
        cr_set_out(&r, v);
    }
    return r;
}

static CommandResult cmd_unset(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "unset: need key");
        return r;
    }
    hm_unset(&vars, t->items[1]);
    cr_set_out(&r, "");
    return r;
}

static CommandResult cmd_listenv(TokenArray *t) {
    CommandResult r;
    char **pairs;
    int count, i;
    UBuffer b;
    cr_init(&r);
    hm_list(&vars, &pairs, &count);
    ubuf_init(&b);
    for (i = 0; i < count; i++) {
        append_line(&b, pairs[i]);
        u_free(pairs[i]);
    }
    if (pairs) u_free(pairs);
    r.stdout_text = ubuf_to_string(&b);
    return r;
}

/* History */

static CommandResult cmd_history(TokenArray *t) {
    CommandResult r;
    char **lines;
    int count, i;
    UBuffer b;
    cr_init(&r);
    history_get_all(&lines, &count);
    ubuf_init(&b);
    for (i = 0; i < count; i++) {
        append_line(&b, lines[i]);
        u_free(lines[i]);
    }
    if (lines) u_free(lines);
    r.stdout_text = ubuf_to_string(&b);
    return r;
}

/* Undo / Redo: simplified (not full FS restore) */

static CommandResult cmd_undo(TokenArray *t) {
    CommandResult r;
    Operation op;
    cr_init(&r);
    if (!stack_pop(&undo_stack, &op)) {
        cr_set_err(&r, "undo: nothing to undo");
        r.status = 1;
        return r;
    }
    if (op.type == OP_WRITE_FILE) {
        fs_write(op.path, op.old_content, 0);
    } else if (op.type == OP_CREATE_FILE) {
        fs_rm(op.path);
    } else if (op.type == OP_CREATE_DIR) {
        fs_rmdir(op.path);
    } else if (op.type == OP_DELETE_FILE) {
        fs_write(op.path, op.old_content ? op.old_content : "", 0);
    } else if (op.type == OP_MOVE) {
        fs_move(op.new_content, op.old_content);
    }
    stack_push(&redo_stack, op);
    cr_set_out(&r, "");
    return r;
}

static CommandResult cmd_redo(TokenArray *t) {
    CommandResult r;
    Operation op;
    cr_init(&r);
    if (!stack_pop(&redo_stack, &op)) {
        cr_set_err(&r, "redo: nothing to redo");
        r.status = 1;
        return r;
    }
    if (op.type == OP_WRITE_FILE) {
        fs_write(op.path, op.new_content, 0);
    } else if (op.type == OP_CREATE_FILE) {
        fs_touch(op.path);
    } else if (op.type == OP_CREATE_DIR) {
        fs_mkdir(op.path);
    } else if (op.type == OP_DELETE_FILE) {
        fs_rm(op.path);
    } else if (op.type == OP_MOVE) {
        fs_move(op.old_content, op.new_content);
    }
    stack_push(&undo_stack, op);
    cr_set_out(&r, "");
    return r;
}

/* Search */

typedef struct {
    UBuffer *b;
} SearchCtx;

static void search_cb(const char *path, int line, const char *text, void *user) {
    SearchCtx *ctx = (SearchCtx *)user;
    char num[32];
    ubuf_append_str(ctx->b, path);
    ubuf_append_char(ctx->b, ':');
    u_itoa(line, num);
    ubuf_append_str(ctx->b, num);
    ubuf_append_char(ctx->b, ':');
    ubuf_append_str(ctx->b, text);
    ubuf_append_char(ctx->b, '\n');
}

static CommandResult cmd_search(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 3) {
        r.status = 1;
        cr_set_err(&r, "search: need path and keyword");
        return r;
    }
    {
        UBuffer b;
        SearchCtx ctx;
        ubuf_init(&b);
        ctx.b = &b;
        fs_search(t->items[1], t->items[2], search_cb, &ctx);
        r.stdout_text = ubuf_to_string(&b);
    }
    return r;
}

/* Help */

static CommandResult cmd_help(TokenArray *t) {
    CommandResult r;
    UBuffer b;
    int i;
    cr_init(&r);
    ubuf_init(&b);
    if (t->count < 2) {
        int lines = (int)(sizeof(help_text) / sizeof(help_text[0]));
        for (i = 0; i < lines; i++) {
            append_line(&b, help_text[i]);
        }
    } else {
        int lines = (int)(sizeof(help_text) / sizeof(help_text[0]));
        for (i = 0; i < lines; i++) {
            if (u_startswith(help_text[i], t->items[1])) {
                append_line(&b, help_text[i]);
            }
        }
    }
    r.stdout_text = ubuf_to_string(&b);
    return r;
}

/* Logging */

static CommandResult cmd_log(TokenArray *t) {
    CommandResult r;
    LogEntry *entries;
    int count, i;
    UBuffer b;
    cr_init(&r);
    log_get_all(&logger_q, &entries, &count);
    ubuf_init(&b);
    for (i = 0; i < count; i++) {
        ubuf_append_str(&b, entries[i].timestamp);
        ubuf_append_str(&b, " ");
        ubuf_append_str(&b, entries[i].message);
        ubuf_append_char(&b, '\n');
        u_free(entries[i].timestamp);
        u_free(entries[i].message);
    }
    if (entries) u_free(entries);
    r.stdout_text = ubuf_to_string(&b);
    return r;
}

/* Permissions (simplified: r w as 0/1 integers) */

static CommandResult cmd_chmod(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 4) {
        r.status = 1;
        cr_set_err(&r, "chmod: need path r w");
        return r;
    }
    {
        int rbit = u_atoi(t->items[2]);
        int wbit = u_atoi(t->items[3]);
        int rc;
        if ((rbit != 0 && rbit != 1) || (wbit != 0 && wbit != 1)) {
            r.status = 1;
            cr_set_err(&r, "chmod: r and w must be 0 or 1");
            return r;
        }
        rc = fs_chmod(t->items[1], rbit, wbit);
        if (rc != 0) {
            r.status = 1;
            cr_set_err(&r, "chmod: path not found");
        } else {
            cr_set_out(&r, "");
        }
    }
    return r;
}

/* Export / Import (very simple) */

static CommandResult cmd_export(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "export: need filename");
        return r;
    }
    {
        int st;
        fs_export_to_file(t->items[1], &st);
        if (st != 0) {
            r.status = 1;
            cr_set_err(&r, "export: failed");
        } else {
            cr_set_out(&r, "");
        }
    }
    return r;
}

static CommandResult cmd_import(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "import: need filename");
        return r;
    }
    {
        int st;
        fs_import_from_file(t->items[1], &st);
        if (st != 0) {
            r.status = 1;
            cr_set_err(&r, "import: failed");
        } else {
            cr_set_out(&r, "");
        }
    }
    return r;
}

/* cp, mv, tree, autocomplete */

static CommandResult cmd_cp(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 3) {
        r.status = 1;
        cr_set_err(&r, "cp: need src and dst");
        return r;
    }
    if (fs_copy(t->items[1], t->items[2]) != 0) {
        r.status = 1;
        cr_set_err(&r, "cp: failed");
    } else {
        cr_set_out(&r, "");
    }
    return r;
}

static CommandResult cmd_mv(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 3) {
        r.status = 1;
        cr_set_err(&r, "mv: need src and dst");
        return r;
    }
    if (fs_move(t->items[1], t->items[2]) != 0) {
        r.status = 1;
        cr_set_err(&r, "mv: failed");
    } else {
        Operation op;
        op.type = OP_MOVE;
        op.path = u_strdup(t->items[1]);
        op.old_content = u_strdup(t->items[1]);
        op.new_content = u_strdup(t->items[2]);
        stack_push(&undo_stack, op);
        stack_clear(&redo_stack);
        cr_set_out(&r, "");
    }
    return r;
}

static void tree_rec(TreeNode *n, const char *prefix, UBuffer *b) {
    int i;
    if (n != fs_get_root()) {
        ubuf_append_str(b, prefix);
        ubuf_append_str(b, "- ");
        ubuf_append_str(b, n->name);
        ubuf_append_char(b, '\n');
    }
    if (n->type != NODE_DIR) return;
    for (i = 0; i < n->child_count; i++) {
        TreeNode *ch = n->children[i];
        UBuffer np;
        ubuf_init(&np);
        ubuf_append_str(&np, prefix);
        ubuf_append_str(&np, "  ");
        tree_rec(ch, np.data, b);
        ubuf_free(&np);
    }
}

static CommandResult cmd_tree(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    {
        UBuffer b;
        TreeNode *start;
        ubuf_init(&b);
        if (t->count > 1) {
            start = fs_find_node(t->items[1]);
            if (!start) {
                r.status = 1;
                cr_set_err(&r, "tree: path not found");
                return r;
            }
        } else {
            start = fs_get_cwd();
        }
        tree_rec(start, "", &b);
        r.stdout_text = ubuf_to_string(&b);
    }
    return r;
}

/* Autocomplete */

static CommandResult cmd_complete(TokenArray *t) {
    CommandResult r;
    cr_init(&r);
    if (t->count < 2) {
        r.status = 1;
        cr_set_err(&r, "complete: need prefix");
        return r;
    }
    {
        char **words;
        int count;
        int i;
        trie_complete(trie_root, t->items[1], &words, &count);
        r.suggestions = words;
        r.suggestion_count = count;
        cr_set_out(&r, "");
    }
    return r;
}

CommandResult cmd_execute(TokenArray *tokens) {
    CommandResult r;
    cr_init(&r);
    if (tokens->count == 0) {
        cr_set_out(&r, "");
        return r;
    }
    if (u_strcmp(tokens->items[0], "mkdir") == 0) return cmd_mkdir(tokens);
    if (u_strcmp(tokens->items[0], "ls") == 0) return cmd_ls(tokens);
    if (u_strcmp(tokens->items[0], "cd") == 0) return cmd_cd(tokens);
    if (u_strcmp(tokens->items[0], "touch") == 0) return cmd_touch(tokens);
    if (u_strcmp(tokens->items[0], "write") == 0) return cmd_write(tokens);
    if (u_strcmp(tokens->items[0], "read") == 0) return cmd_read(tokens);
    if (u_strcmp(tokens->items[0], "rm") == 0) return cmd_rm(tokens);
    if (u_strcmp(tokens->items[0], "rmdir") == 0) return cmd_rmdir(tokens);
    if (u_strcmp(tokens->items[0], "pwd") == 0) return cmd_pwd(tokens);
    if (u_strcmp(tokens->items[0], "cat") == 0) return cmd_cat(tokens);

    if (u_strcmp(tokens->items[0], "set") == 0) return cmd_set(tokens);
    if (u_strcmp(tokens->items[0], "get") == 0) return cmd_get(tokens);
    if (u_strcmp(tokens->items[0], "unset") == 0) return cmd_unset(tokens);
    if (u_strcmp(tokens->items[0], "listenv") == 0) return cmd_listenv(tokens);

    if (u_strcmp(tokens->items[0], "history") == 0) return cmd_history(tokens);
    if (u_strcmp(tokens->items[0], "undo") == 0) return cmd_undo(tokens);
    if (u_strcmp(tokens->items[0], "redo") == 0) return cmd_redo(tokens);

    if (u_strcmp(tokens->items[0], "search") == 0) return cmd_search(tokens);
    if (u_strcmp(tokens->items[0], "help") == 0) return cmd_help(tokens);
    if (u_strcmp(tokens->items[0], "log") == 0) return cmd_log(tokens);
    if (u_strcmp(tokens->items[0], "chmod") == 0) return cmd_chmod(tokens);
    if (u_strcmp(tokens->items[0], "export") == 0) return cmd_export(tokens);
    if (u_strcmp(tokens->items[0], "import") == 0) return cmd_import(tokens);
    if (u_strcmp(tokens->items[0], "cp") == 0) return cmd_cp(tokens);
    if (u_strcmp(tokens->items[0], "mv") == 0) return cmd_mv(tokens);
    if (u_strcmp(tokens->items[0], "tree") == 0) return cmd_tree(tokens);
    if (u_strcmp(tokens->items[0], "complete") == 0) return cmd_complete(tokens);

    r.status = 1;
    cr_set_err(&r, "unknown command");
    return r;
}

