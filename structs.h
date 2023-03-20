#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdlib.h>
#include <string.h>

#define SpaceChar(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')
#define NotArg(c) (SpaceChar(c) || (c) == '&' || (c) == '|' || (c) == '>' || (c) == '<')

#define TokenType_None          0x00
#define TokenType_Argument      0x01
#define TokenType_String        0x02
#define TokenType_Special       0x03

#define Special_None    0x00
#define Special_AND     0x01
#define Special_LESS    0x02
#define Special_GREATER 0x03
#define Special_OR      0x04
#define Special_AND_AND 0x05
#define Special_LSHIFT  0x06
#define Special_RSHIFT  0x07
#define Special_OR_OR   0x08
#define Special_COMMENT 0x09

struct string {
    size_t size, mx_size;
    char *data;
};
struct string *string_new();
void string_set(struct string *res, const struct string *a);
void string_clear(struct string *res);
void string_free(struct string *res);
void string_resize(struct string *res, size_t size);


struct token {
    short type;
    short subtype;

    struct string *data;
};
struct token *token_new();
void token_set(struct token *res, const struct token *a);
void token_clear(struct token *res);
void token_free(struct token *res);


struct token_list {
    size_t size, mx_size;
    struct token **data;
};
struct token_list *token_list_new();
void token_list_clear(struct token_list *res);
void token_list_free(struct token_list *res);
void token_list_resize(struct token_list *res, size_t size);


struct parser{
    struct string *data;
    size_t pos;

    struct token_list *list;
};
struct parser *parser_new();
void parser_clear(struct parser *res);
void parser_free(struct parser *res);

#define CmdType_None    0x00
#define CmdType_FileAp  0x01
#define CmdType_File    0x02
#define CmdType_OR      0x03
#define CmdType_OPOR    0x04
#define CmdType_OPAND   0x05
#define CmdType_AND     0x06

struct cmd_ctx{
    char *name;
    char **argv;
    size_t argc;

    int type;
    struct cmd_ctx *first, *second;
};
struct cmd_ctx *cmd_ctx_new();
void cmd_ctx_set_self(struct cmd_ctx *res);
void cmd_ctx_free(struct cmd_ctx *res);


#endif //STRUCTS_H
