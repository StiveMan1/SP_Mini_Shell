#include <stdio.h>
#include <sys/wait.h>
#include "structs.h"
#include "unistd.h"
#include "fcntl.h"

#define INPUT_END 1                              // INPUT_END means where the pipe takes input
#define OUTPUT_END 0                             // OUTPUT_END means where the pipe produces output

int run_shell = 1;
int status_exit = 0;


struct string *string_new() {
    struct string *res = malloc(sizeof(struct string));
    res->data = NULL;
    res->mx_size = res->size = 0;
    return res;
}
void string_set(struct string *res, const struct string *a) {
    if (a == NULL || a->size == 0) return string_clear(res);
    string_resize(res, a->size);
    memcpy(res->data, a->data, a->size);
}
void string_clear(struct string *res) {
    string_resize(res, 0);
}
void string_free(struct string *res) {
    if (res->data != NULL) free(res->data);
    free(res);
}
void string_resize(struct string *res, size_t size) {
    if (res->data == NULL && size != 0) {
        res->mx_size = size;
        res->data = malloc(size + 1);
        if (res->data != NULL) for (size_t i = 0; i < size + 1; i++) res->data[i] = 0;
    } else if (res->mx_size < size) {
        res->data = realloc(res->data, size * 2 + 1);
        if (res->data != NULL) for (size_t i = res->mx_size; i < size * 2 + 1; i++) res->data[i] = 0;
        res->mx_size = size * 2;
    }
    if (res->size > size)
        for (size_t i = size; i < res->size; i++) {
            res->data[i] = 0;
        }
    res->size = size;
}


struct token *token_new() {
    struct token *res = malloc(sizeof(struct token));
    res->type = TokenType_None;
    res->subtype = TokenType_None;

    res->data = string_new();
    return res;
}
void token_set(struct token *res, const struct token *a) {
    res->type = a->type;
    res->subtype = a->subtype;

    string_set(res->data, a->data);
}
void token_clear(struct token *res) {
    res->type = TokenType_None;
    res->subtype = TokenType_None;

    string_clear(res->data);
}
void token_free(struct token *res) {
    string_free(res->data);
    free(res);
}


struct token_list *token_list_new() {
    struct token_list *res = malloc(sizeof(struct token_list));
    res->mx_size = res->size = 0;
    res->data = NULL;
    return res;
}
void token_list_clear(struct token_list *res) {
    token_list_resize(res, 0);
}
void token_list_free(struct token_list *res) {
    token_list_resize(res, 0);
    if (res->data != NULL) free(res->data);
    free(res);
}
void token_list_resize(struct token_list *res, size_t size) {
    if (res->data == NULL && size != 0) {
        res->mx_size = size;
        res->data = malloc(sizeof(struct token *) * size);
        for (size_t i = 0; i < size; i++) res->data[i] = NULL;
    } else if (res->mx_size < size) {
        res->data = realloc(res->data, sizeof(struct token *) * size * 2);
        for (size_t i = res->mx_size; i < size * 2; i++) res->data[i] = NULL;
        res->mx_size = size * 2;
    }
    if (res->size > size) {
        for (size_t i = size; i < res->size; i++) {
            if (res->data[i] != NULL) token_free(res->data[i]);
            res->data[i] = NULL;
        }
    }
    res->size = size;
}


struct parser *parser_new() {
    struct parser *res = malloc(sizeof(struct parser));
    res->data = string_new();
    res->pos = 0;
    res->list = token_list_new();
    return res;
}
void parser_clear(struct parser *res) {
    string_clear(res->data);
    token_list_clear(res->list);
    res->pos = 0;
}
void parser_free(struct parser *res) {
    string_free(res->data);
    token_list_free(res->list);
    free(res);
}
void get_line_command(struct parser *parser) {
    parser_clear(parser);
    int ch, st = 0;
    int is_string = 0, count = 0;
    while (run_shell) {
        ch = getchar();
        if (ch == EOF) {
            run_shell = 0;
            break;
        }
        if (is_string == 0 && count % 2 == 0 && ch == '\n') break;

        if (ch == '\\') {
            count++;
        } else {
            if (count % 2 == 1) {
                if (ch == '\n') {
                    string_resize(parser->data, parser->data->size - 1);
                    continue;
                }
            } else {
                if (ch == '\'' || ch == '\"') {
                    if (is_string == 0) {
                        st = ch;
                        is_string = 1;
                    } else if (ch == st) {
                        is_string = 0;
                    }
                }
            }
            count = 0;
        }
        string_resize(parser->data, parser->data->size + 1);
        parser->data->data[parser->data->size - 1] = (char)ch;
    }
}


struct cmd_ctx *cmd_ctx_new() {
    struct cmd_ctx *res = malloc(sizeof(struct cmd_ctx));
    res->name = NULL;
    res->argv = NULL;
    res->first = NULL;
    res->second = NULL;
    res->type = CmdType_None;
    res->argc = 0;
    return res;
}
void cmd_ctx_set_self(struct cmd_ctx *res) {
    struct cmd_ctx *ctx = cmd_ctx_new();
    ctx->name = res->name;
    ctx->argv = res->argv;
    ctx->first = res->first;
    ctx->second = res->second;
    ctx->type = res->type;
    ctx->argc = res->argc;

    res->name = NULL;
    res->argv = NULL;
    res->first = ctx;
    res->second = NULL;
    res->type = CmdType_None;
    res->argc = 0;
}
void cmd_ctx_free(struct cmd_ctx *res) {
    if (res->name != NULL) free(res->name);
    if (res->argv != NULL) for (int i = 0; i < res->argc; i++) if (res->argv[i] != NULL) free(res->argv[i]);
    if (res->first != NULL) cmd_ctx_free(res->first);
    if (res->second != NULL) cmd_ctx_free(res->second);
    free(res);
}


short Special_OneChar(char c1) {
    switch (c1) {
        case '&':
            return Special_AND;
        case '<':
            return Special_LESS;
        case '>':
            return Special_GREATER;
        case '|':
            return Special_OR;
        case '#':
            return Special_COMMENT;
        default:
            break;
    }
    return Special_None;
}
short Special_TwoChar(char c1, char c2) {
    switch (c1) {
        case '&':
            if (c2 == '&') return Special_AND_AND;
            break;
        case '<':
            if (c2 == '<') return Special_LSHIFT;
            break;
        case '>':
            if (c2 == '>') return Special_RSHIFT;
            break;
        case '|':
            if (c2 == '|') return Special_OR_OR;
            break;
        default:
            break;
    }
    return Special_None;
}


void token_get_string(struct token *token, struct parser *parser) {
    if (parser->data->data[parser->pos] != '\'' && parser->data->data[parser->pos] != '\"') return;
    token->type = TokenType_String;
    size_t pos = parser->pos++;

    for (size_t i = 0; parser->pos < parser->data->size; parser->pos++, i++) {
        if (parser->data->data[parser->pos] == '\\') {
            if (parser->data->data[parser->pos + 1] == parser->data->data[pos] ||
                parser->data->data[parser->pos + 1] == '\\') {
                parser->pos++;
            }
            string_resize(token->data, i + 1);
            token->data->data[i] = parser->data->data[parser->pos];
        } else if (parser->data->data[parser->pos] == parser->data->data[pos]) {
            parser->pos++;
            break;
        } else {
            string_resize(token->data, i + 1);
            token->data->data[i] = parser->data->data[parser->pos];
        }
    }
}
void token_get_special(struct token *token, struct parser *parser) {
    short result = Special_OneChar(parser->data->data[parser->pos]);
    if (result != Special_None) {
        token->type = TokenType_Special;

        token->subtype = result;
        parser->pos++;

        result = Special_TwoChar(parser->data->data[parser->pos - 1], parser->data->data[parser->pos]);
        if (result != Special_None) {
            token->subtype = result;
            parser->pos++;
        }
    }
}
void token_get_argument(struct token *token, struct parser *parser) {
    token->type = TokenType_Argument;

    for (size_t i = 0; parser->pos < parser->data->size; parser->pos++, i++) {
        if (parser->data->data[parser->pos] == '\\') {
            parser->pos++;
            string_resize(token->data, i + 1);
            token->data->data[i] = parser->data->data[parser->pos];
        } else if (NotArg(parser->data->data[parser->pos])) {
            break;
        } else {
            string_resize(token->data, i + 1);
            token->data->data[i] = parser->data->data[parser->pos];
        }
    }
}


void tokenize_parse(struct token *token, struct parser *parser) {
    token_get_string(token, parser);
    if (token->type != TokenType_None) return;
    token_get_special(token, parser);
    if (token->type != TokenType_None) return;
    token_get_argument(token, parser);
    if (token->type != TokenType_None) return;
}
void tokenize(struct parser *parser) {
    parser->pos = 0;
    struct token *token = token_new();
    while (parser->pos < parser->data->size) {
        if (SpaceChar(parser->data->data[parser->pos])) {
            parser->pos++;
            continue;
        }
        tokenize_parse(token, parser);
        if (token->type == TokenType_None) goto bad_end;
        if (token->type == TokenType_Special && token->subtype == Special_COMMENT) break;

        token_list_resize(parser->list, parser->list->size + 1);
        parser->list->data[parser->list->size - 1] = token_new();
        token_set(parser->list->data[parser->list->size - 1], token);
        token_clear(token);
    }

    token_free(token);
    parser->pos = 0;
    return;

    bad_end:
    token_free(token);
    token_list_clear(parser->list);
}


void cmd_ctx_parse(struct parser *parser, struct cmd_ctx *ctx) {
    if (parser->pos >= parser->list->size) return;
    size_t pos = parser->pos;
    ctx->type = CmdType_None;

    for (; parser->pos < parser->list->size; parser->pos++) {
        if (parser->list->data[parser->pos]->type == TokenType_None ||
            parser->list->data[parser->pos]->type == TokenType_Special)
            break;
    }

    ctx->name = malloc(parser->list->data[pos]->data->size + 1);
    memcpy(ctx->name, parser->list->data[pos]->data->data, parser->list->data[pos]->data->size + 1);

    ctx->argc = parser->pos - pos;
    ctx->argv = malloc(sizeof(char *) * (ctx->argc + 1));
    ctx->argv[ctx->argc] = NULL;
    for (size_t i = 0; i < ctx->argc; i++) {
        ctx->argv[i] = malloc(parser->list->data[pos + i]->data->size + 1);
        memcpy(ctx->argv[i], parser->list->data[pos + i]->data->data, parser->list->data[pos + i]->data->size + 1);
    }
}
void cmd_next_parse(struct parser *parser, struct cmd_ctx *ctx) {
    cmd_ctx_parse(parser, ctx);
    if (parser->pos >= parser->list->size) return;
    while (parser->pos < parser->list->size) {
        if (parser->list->data[parser->pos]->subtype != Special_OR) break;
        parser->pos++;

        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_OR;
        ctx->second = cmd_ctx_new();
        cmd_ctx_parse(parser, ctx->second);
    }
}
void cmd_file_parse(struct parser *parser, struct cmd_ctx *ctx) {
    cmd_next_parse(parser, ctx);
    if (parser->pos >= parser->list->size) return;
    if (parser->list->data[parser->pos]->subtype == Special_GREATER) {
        parser->pos++;
        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_File;

        ctx->name = malloc(parser->list->data[parser->pos]->data->size + 1);
        memcpy(ctx->name, parser->list->data[parser->pos]->data->data, parser->list->data[parser->pos]->data->size + 1);
        parser->pos++;
    } else if (parser->list->data[parser->pos]->subtype == Special_RSHIFT) {
        parser->pos++;
        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_FileAp;

        ctx->name = malloc(parser->list->data[parser->pos]->data->size + 1);
        memcpy(ctx->name, parser->list->data[parser->pos]->data->data, parser->list->data[parser->pos]->data->size + 1);
        parser->pos++;
    }
}
void cmd_or_parse(struct parser *parser, struct cmd_ctx *ctx) {
    cmd_file_parse(parser, ctx);
    if (parser->pos >= parser->list->size) return;
    while (parser->pos < parser->list->size) {
        if (parser->list->data[parser->pos]->subtype != Special_OR_OR) break;
        parser->pos++;

        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_OPOR;
        ctx->second = cmd_ctx_new();
        cmd_file_parse(parser, ctx->second);
    }
}
void cmd_and_parse(struct parser *parser, struct cmd_ctx *ctx) {
    cmd_or_parse(parser, ctx);
    if (parser->pos >= parser->list->size) return;
    while (parser->pos < parser->list->size) {
        if (parser->list->data[parser->pos]->subtype != Special_AND_AND) break;
        parser->pos++;

        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_OPAND;
        ctx->second = cmd_ctx_new();
        cmd_or_parse(parser, ctx->second);
    }
}
void cmd_back_parse(struct parser *parser, struct cmd_ctx *ctx) {
    cmd_and_parse(parser, ctx);
    if (parser->pos >= parser->list->size) return;
    while (parser->pos < parser->list->size) {
        if (parser->list->data[parser->pos]->subtype != Special_AND) break;
        parser->pos++;

        cmd_ctx_set_self(ctx);
        ctx->type = CmdType_AND;
        ctx->second = cmd_ctx_new();
        cmd_and_parse(parser, ctx->second);
    }
}

int exec_cmd_ctx(struct cmd_ctx *ctx) {
    if (ctx->name == NULL) return 0;
    if (memcmp("cd\0", ctx->name, 3) == 0) return chdir(ctx->argv[1]);
    if (memcmp("exit\0", ctx->name, 5) == 0) {
        run_shell = 0;
        status_exit = 0;
        if (ctx->argc >= 2) {
            status_exit = atoi(ctx->argv[1]);
        }
        return 0;
    }
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        execvp(ctx->name, ctx->argv);
        exit(0);
    }
    waitpid(pid, &status, 0);
    return status;
}
int run_cmd_ctx(struct cmd_ctx *ctx) {
    pid_t pid1;
    pid_t pid2;
    int fd[2];
    int status = 0;
    switch (ctx->type) {
        case CmdType_None: {
            status = exec_cmd_ctx(ctx);
            break;
        }
        case CmdType_OR: {
            pipe(fd);
            pid1 = fork();
            if (pid1 == 0) {
                close(fd[OUTPUT_END]);
                dup2(fd[INPUT_END], STDOUT_FILENO);
                close(fd[INPUT_END]);
                run_cmd_ctx(ctx->first);
                exit(0);
            }
            pid2 = fork();
            if (pid2 == 0) {
                close(fd[INPUT_END]);
                dup2(fd[OUTPUT_END], STDIN_FILENO);
                close(fd[OUTPUT_END]);
                run_cmd_ctx(ctx->second);
                exit(0);
            }
            close(fd[OUTPUT_END]);
            close(fd[INPUT_END]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            break;
        }
        case CmdType_File: {
            int out = open(ctx->name, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0600);
            int save_out = dup(fileno(stdout));
            dup2(out, fileno(stdout));

            status = run_cmd_ctx(ctx->first);


            close(out);
            dup2(save_out, fileno(stdout));
            close(save_out);
            break;
        }
        case CmdType_FileAp: {
            int out = open(ctx->name, O_RDWR | O_CREAT | O_APPEND, 0600);
            int save_out = dup(fileno(stdout));
            dup2(out, fileno(stdout));

            status = run_cmd_ctx(ctx->first);

            fflush(stdout);
            close(out);
            dup2(save_out, fileno(stdout));
            close(save_out);
            break;
        }
        case CmdType_OPOR: {
            status = run_cmd_ctx(ctx->first);
            if (status == 0) break;
            status = run_cmd_ctx(ctx->second);
            break;
        }
        case CmdType_OPAND: {
            status = run_cmd_ctx(ctx->first);
            if (status != 0) break;
            status = run_cmd_ctx(ctx->second);
            break;
        }
        case CmdType_AND: {
            pid1 = fork();
            if (pid1 == 0) {
                run_cmd_ctx(ctx->first);
                exit(0);
            }
            run_cmd_ctx(ctx->second);
        }
    }
    return status;
}

int main() {
    struct parser *parser = parser_new();

    while (run_shell) {
        get_line_command(parser);
        tokenize(parser);
        if (parser->list->size == 0) continue;
        struct cmd_ctx *ctx = cmd_ctx_new();
        cmd_back_parse(parser, ctx);
        run_cmd_ctx(ctx);
        cmd_ctx_free(ctx);
    }
    parser_free(parser);
    return status_exit;
}
