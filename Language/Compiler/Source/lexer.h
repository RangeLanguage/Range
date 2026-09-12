/* Lexer for the Range compiler.
 *
 * Operates over an explicit [start, end) span so that interpolated string
 * segments can be lexed re-entrantly without copying source. */
#ifndef RANGE_COMPILER_LEXER_H
#define RANGE_COMPILER_LEXER_H

#include <stddef.h>

typedef enum {
    RangeTokenEnd = 0,
    RangeTokenName,
    RangeTokenInteger,
    RangeTokenString,
    RangeTokenSymbol
} RangeTokenKind;

typedef struct {
    RangeTokenKind kind;
    const char *text;   /* points into the source buffer, not NUL terminated */
    size_t length;
    size_t offset;      /* byte offset of text within the whole file */
    int line;
    int column;
    /* String tokens only: span of the body between the quotes. */
    size_t bodyStart;
    size_t bodyEnd;
} RangeToken;

typedef struct {
    const char *path;
    const char *source;
    size_t length;
    size_t cursor;
    int line;
    int column;
    /* Set when lexing fails; the compiler reports and exits. */
    char error[512];
    int failed;
} RangeLexer;

void rangeLexerInit(RangeLexer *lexer, const char *path,
                    const char *source, size_t start, size_t end, int line);
int rangeLexerNext(RangeLexer *lexer, RangeToken *token);
int rangeTokenIs(const RangeToken *token, const char *text);
void rangeLexerFail(RangeLexer *lexer, const RangeToken *token,
                    const char *format, ...);

#endif
