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

void debug_lex(const char *json) {
    JsonLexer lx = { json, 0, strlen(json) };
    for (;;) {
        JsonToken t = next_token(&lx);
        printf("type=%d value=%s\n", t.type, t.value ? t.value : "(nil)");
        free(t.value);
        if (t.type == TOK_EOF) break;
    }
}

void test_parse_person()
{
    debug_lex("{\"name\":\"Tobias\",\"age\":25,\"gender\":\"man\", \"skills\":[\"C\",\"C++\",\"Python\"]}");
}