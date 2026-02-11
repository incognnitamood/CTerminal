#include <stdio.h>
#include "filesystem.h"
#include "history.h"
#include "parser.h"
#include "commands.h"
#include "utils.h"
#include "logger.h"

static void json_escape_and_print(const char *s) {
    int i = 0;
    char c;
    putchar('"');
    if (s) {
        while ((c = s[i]) != 0) {
            if (c == '"' || c == '\\') {
                putchar('\\');
                putchar(c);
            } else if (c == '\n') {
                putchar('\\');
                putchar('n');
            } else {
                putchar(c);
            }
            i++;
        }
    }
    putchar('"');
}

static void write_json_response(CommandResult *res, const char *cwd) {
    int i;
    printf("{");
    printf("\"ok\":%s,", res->status == 0 ? "true" : "false");
    printf("\"stdout\":");
    json_escape_and_print(res->stdout_text ? res->stdout_text : "");
    printf(",");
    printf("\"stderr\":");
    json_escape_and_print(res->stderr_text ? res->stderr_text : "");
    printf(",");
    printf("\"cwd\":");
    json_escape_and_print(cwd ? cwd : "/");
    printf(",");
    printf("\"suggestions\":[");
    for (i = 0; i < res->suggestion_count; i++) {
        if (i > 0) printf(",");
        json_escape_and_print(res->suggestions[i]);
    }
    printf("]}");
    printf("\n");
    fflush(stdout);
}

int main(void) {
    char line[512];
    int len;

    fs_init();
    history_init(100);
    commands_init();

    while (1) {
        int ch;
        len = 0;
        while (1) {
            ch = getchar();
            if (ch == EOF) return 0;
            if (ch == '\n') break;
            if (len < 511) {
                line[len++] = (char)ch;
            }
        }
        line[len] = 0;
        if (len == 0) {
            continue;
        }
        if (u_strcmp(line, "exit") == 0) {
            break;
        }
        history_add(line);

        {
            TokenArray tokens;
            CommandResult res;
            char *cwd;
            parser_init(&tokens);
            parser_tokenize(line, &tokens);
            res = cmd_execute(&tokens);
            cwd = fs_pwd();
            write_json_response(&res, cwd);
            if (cwd) u_free(cwd);
            if (res.stdout_text) u_free(res.stdout_text);
            if (res.stderr_text) u_free(res.stderr_text);
            if (res.suggestions) {
                int i;
                for (i = 0; i < res.suggestion_count; i++) {
                    if (res.suggestions[i]) u_free(res.suggestions[i]);
                }
                u_free(res.suggestions);
            }
            parser_free(&tokens);
        }
    }

    return 0;
}

