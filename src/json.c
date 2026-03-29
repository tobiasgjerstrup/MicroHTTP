#include "microhttp.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


typedef enum
{
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_COMMA,
    TOK_STRING,
    TOK_NUMBER,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_EOF,
    TOK_ERROR
} JsonTokenType;

typedef struct
{
    JsonTokenType type;
    char *value;
} JsonToken;

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
} JsonLexer;

static int is_at_end(JsonLexer *lexer) {
    return lexer->pos >= lexer->len;
}

// Peeks at the current character without advancing the lexer
static char peek(JsonLexer *lexer) {
    return is_at_end(lexer) ? '\0' : lexer->src[lexer->pos];
}

// Advances the lexer and returns the current character
static char advance(JsonLexer *lexer) {
    return is_at_end(lexer) ? '\0' : lexer->src[lexer->pos++];
}

static void skip_whitespace(JsonLexer *lexer) {
    while (!is_at_end(lexer) && isspace((unsigned char)peek(lexer))) {
        lexer->pos++;
    }
}

// Utility function to duplicate a substring from the source
static char *dup_slice(const char *s, size_t start, size_t end) {
    size_t n = end - start;
    char *out = malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s + start, n);
    out[n] = '\0';
    return out;
}

// Creates a token with the given type and value
static JsonToken make_token(JsonTokenType type, char *value) {
    JsonToken token;
    token.type = type;
    token.value = value;
    return token;
}

static JsonToken error_token(void) {
    return make_token(TOK_ERROR, NULL);
}

static JsonToken lex_string(JsonLexer *lx) {
    /* opening quote already consumed */
    size_t start = lx->pos;

    while (!is_at_end(lx)) {
        char c = advance(lx);
        if (c == '\\') {
            if (!is_at_end(lx)) advance(lx); /* skip escaped char */
            continue;
        }
        if (c == '"') {
            size_t end = lx->pos - 1; /* exclude closing quote */
            char *s = dup_slice(lx->src, start, end);
            return make_token(TOK_STRING, s);
        }
    }
    return error_token(); /* unterminated string */
}

static JsonToken lex_number(JsonLexer *lx, size_t start) {
    while (isdigit((unsigned char)peek(lx))) advance(lx);

    if (peek(lx) == '.') {
        advance(lx);
        while (isdigit((unsigned char)peek(lx))) advance(lx);
    }

    if (peek(lx) == 'e' || peek(lx) == 'E') {
        advance(lx);
        if (peek(lx) == '+' || peek(lx) == '-') advance(lx);
        while (isdigit((unsigned char)peek(lx))) advance(lx);
    }

    return make_token(TOK_NUMBER, dup_slice(lx->src, start, lx->pos));
}

static int match_word(JsonLexer *lx, const char *word) {
    size_t n = strlen(word);
    if (lx->pos + n > lx->len) return 0;
    if (strncmp(lx->src + lx->pos, word, n) != 0) return 0;
    lx->pos += n;
    return 1;
}

JsonToken next_token(JsonLexer *lx) {
    skip_whitespace(lx);
    if (is_at_end(lx)) return make_token(TOK_EOF, NULL);

    char c = advance(lx);

    switch (c) {
        case '{': return make_token(TOK_LBRACE, NULL);
        case '}': return make_token(TOK_RBRACE, NULL);
        case '[': return make_token(TOK_LBRACKET, NULL);
        case ']': return make_token(TOK_RBRACKET, NULL);
        case ':': return make_token(TOK_COLON, NULL);
        case ',': return make_token(TOK_COMMA, NULL);
        case '"': return lex_string(lx);
        case '-':
            if (!isdigit((unsigned char)peek(lx))) return error_token();
            return lex_number(lx, lx->pos - 1);
        default:
            if (isdigit((unsigned char)c)) {
                return lex_number(lx, lx->pos - 1);
            }
            {
                size_t keyword_start = lx->pos - 1;

                lx->pos = keyword_start;
                if (match_word(lx, "true")) {
                    return make_token(TOK_TRUE, NULL);
                }

                lx->pos = keyword_start;
                if (match_word(lx, "false")) {
                    return make_token(TOK_FALSE, NULL);
                }

                lx->pos = keyword_start;
                if (match_word(lx, "null")) {
                    return make_token(TOK_NULL, NULL);
                }
            }
            return error_token();
    }
}

typedef struct {
    JsonLexer lexer;
    JsonToken current;
    const char *error;
} JsonParser;

static void parser_advance(JsonParser *parser) {
    free(parser->current.value);
    parser->current = next_token(&parser->lexer);
}

static int parser_expect(JsonParser *parser, JsonTokenType type, const char *error) {
    if (parser->current.type != type) {
        parser->error = error;
        return 0;
    }

    parser_advance(parser);
    return 1;
}

static int parse_value(JsonParser *parser);

static int parse_array(JsonParser *parser) {
    if (!parser_expect(parser, TOK_LBRACKET, "Expected '['")) {
        return 0;
    }

    if (parser->current.type == TOK_RBRACKET) {
        parser_advance(parser);
        return 1;
    }

    if (!parse_value(parser)) {
        return 0;
    }

    while (parser->current.type == TOK_COMMA) {
        parser_advance(parser);
        if (!parse_value(parser)) {
            return 0;
        }
    }

    return parser_expect(parser, TOK_RBRACKET, "Expected ']' after array elements");
}

static int parse_object(JsonParser *parser) {
    if (!parser_expect(parser, TOK_LBRACE, "Expected '{'")) {
        return 0;
    }

    if (parser->current.type == TOK_RBRACE) {
        parser_advance(parser);
        return 1;
    }

    if (parser->current.type != TOK_STRING) {
        parser->error = "Expected object key string";
        return 0;
    }
    parser_advance(parser);

    if (!parser_expect(parser, TOK_COLON, "Expected ':' after object key")) {
        return 0;
    }

    if (!parse_value(parser)) {
        return 0;
    }

    while (parser->current.type == TOK_COMMA) {
        parser_advance(parser);

        if (parser->current.type != TOK_STRING) {
            parser->error = "Expected object key string after ','";
            return 0;
        }
        parser_advance(parser);

        if (!parser_expect(parser, TOK_COLON, "Expected ':' after object key")) {
            return 0;
        }

        if (!parse_value(parser)) {
            return 0;
        }
    }

    return parser_expect(parser, TOK_RBRACE, "Expected '}' after object members");
}

static int parse_value(JsonParser *parser) {
    switch (parser->current.type) {
        case TOK_STRING:
        case TOK_NUMBER:
        case TOK_TRUE:
        case TOK_FALSE:
        case TOK_NULL:
            parser_advance(parser);
            return 1;
        case TOK_LBRACE:
            return parse_object(parser);
        case TOK_LBRACKET:
            return parse_array(parser);
        case TOK_ERROR:
            parser->error = "Lexer error";
            return 0;
        default:
            parser->error = "Invalid JSON value";
            return 0;
    }
}

static void parser_init(JsonParser *parser, const char *json) {
    parser->lexer.src = json;
    parser->lexer.pos = 0;
    parser->lexer.len = strlen(json);
    parser->current.type = TOK_EOF;
    parser->current.value = NULL;
    parser->error = NULL;
}

static int find_field_spec(const JsonFieldSpec *fields, size_t field_count, const char *key) {
    size_t i;

    for (i = 0; i < field_count; i++) {
        if (strcmp(fields[i].key, key) == 0) {
            return (int)i;
        }
    }

    return -1;
}

static int parse_object_into_struct(JsonParser *parser,
                                    void *out_struct,
                                    const JsonFieldSpec *fields,
                                    size_t field_count,
                                    unsigned char *seen,
                                    const char **error_out) {
    if (!parser_expect(parser, TOK_LBRACE, "Expected '{' at start of object")) {
        if (error_out) {
            *error_out = parser->error;
        }
        return 0;
    }

    if (parser->current.type == TOK_RBRACE) {
        parser_advance(parser);
        return 1;
    }

    for (;;) {
        int spec_index;

        if (parser->current.type != TOK_STRING || !parser->current.value) {
            if (error_out) {
                *error_out = "Expected object key string";
            }
            return 0;
        }

        spec_index = find_field_spec(fields, field_count, parser->current.value);
        parser_advance(parser);

        if (!parser_expect(parser, TOK_COLON, "Expected ':' after object key")) {
            if (error_out) {
                *error_out = parser->error;
            }
            return 0;
        }

        if (spec_index < 0) {
            if (!parse_value(parser)) {
                if (error_out) {
                    *error_out = parser->error ? parser->error : "Invalid value";
                }
                return 0;
            }
        } else {
            const JsonFieldSpec *spec = &fields[spec_index];
            char *dest = (char *)out_struct + spec->offset;

            if (spec->type == JSON_FIELD_STRING) {
                if (parser->current.type != TOK_STRING || !parser->current.value) {
                    if (error_out) {
                        *error_out = "Expected string field";
                    }
                    return 0;
                }

                if (spec->size == 0) {
                    if (error_out) {
                        *error_out = "String field size must be > 0";
                    }
                    return 0;
                }

                snprintf(dest, spec->size, "%s", parser->current.value);
                seen[spec_index] = 1;
                parser_advance(parser);
            } else if (spec->type == JSON_FIELD_INT) {
                char *endptr;
                long parsed;

                if (parser->current.type != TOK_NUMBER || !parser->current.value) {
                    if (error_out) {
                        *error_out = "Expected numeric field";
                    }
                    return 0;
                }

                parsed = strtol(parser->current.value, &endptr, 10);
                if (*endptr != '\0') {
                    if (error_out) {
                        *error_out = "Expected integer number";
                    }
                    return 0;
                }

                *(int *)dest = (int)parsed;
                seen[spec_index] = 1;
                parser_advance(parser);
            } else if (spec->type == JSON_FIELD_BOOL) {
                if (parser->current.type == TOK_TRUE) {
                    *(int *)dest = 1;
                    seen[spec_index] = 1;
                    parser_advance(parser);
                } else if (parser->current.type == TOK_FALSE) {
                    *(int *)dest = 0;
                    seen[spec_index] = 1;
                    parser_advance(parser);
                } else {
                    if (error_out) {
                        *error_out = "Expected boolean field";
                    }
                    return 0;
                }
            } else {
                if (error_out) {
                    *error_out = "Unsupported field type";
                }
                return 0;
            }
        }

        if (parser->current.type == TOK_COMMA) {
            parser_advance(parser);
            continue;
        }

        if (parser->current.type == TOK_RBRACE) {
            parser_advance(parser);
            return 1;
        }

        if (error_out) {
            *error_out = "Expected ',' or '}' in object";
        }
        return 0;
    }
}

int parse_json(const char *json,
               void *out_struct,
               const JsonFieldSpec *fields,
               size_t field_count,
               const char **error_out) {
    JsonParser parser;
    unsigned char *seen = NULL;
    size_t i;
    const char *err = NULL;
    int ok = 0;

    if (!json) {
        if (error_out) {
            *error_out = "JSON input cannot be NULL";
        }
        return 0;
    }

    parser_init(&parser, json);
    parser.current = next_token(&parser.lexer);

    if (parser.current.type == TOK_ERROR) {
        err = "Lexer error at start of input";
        goto done;
    }

    if (!out_struct || !fields || field_count == 0) {
        if (!parse_value(&parser)) {
            err = parser.error ? parser.error : "Unknown parse error";
            goto done;
        }
    } else {
        seen = calloc(field_count, sizeof(*seen));
        if (!seen) {
            err = "Out of memory";
            goto done;
        }

        if (!parse_object_into_struct(&parser, out_struct, fields, field_count, seen, &err)) {
            goto done;
        }

        for (i = 0; i < field_count; i++) {
            if (fields[i].required && !seen[i]) {
                err = "Missing required field";
                goto done;
            }
        }
    }

    if (parser.current.type != TOK_EOF) {
        err = "Unexpected trailing tokens";
        goto done;
    }

    ok = 1;

done:
    if (error_out) {
        *error_out = ok ? NULL : (err ? err : "Unknown parse error");
    }
    free(seen);
    free(parser.current.value);
    return ok;
}