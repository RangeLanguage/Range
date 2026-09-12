#include "ast.h"

#include <stdlib.h>
#include <string.h>

#define RANGE_ARENA_BLOCK (1u << 20)

struct RangeArenaBlock {
    RangeArenaBlock *next;
    size_t used;
    size_t capacity;
    char data[];
};

void rangeArenaInit(RangeArena *arena)
{
    arena->head = NULL;
    arena->nodeCount = 0;
}

void rangeArenaDestroy(RangeArena *arena)
{
    while (arena->head) {
        RangeArenaBlock *next = arena->head->next;
        free(arena->head);
        arena->head = next;
    }
    arena->nodeCount = 0;
}

void *rangeArenaAllocate(RangeArena *arena, size_t size)
{
    size = (size + 15u) & ~(size_t)15u;
    if (!arena->head || arena->head->used + size > arena->head->capacity) {
        size_t capacity = size > RANGE_ARENA_BLOCK ? size : RANGE_ARENA_BLOCK;
        RangeArenaBlock *block = malloc(sizeof(RangeArenaBlock) + capacity);
        if (!block) abort();
        block->next = arena->head;
        block->used = 0;
        block->capacity = capacity;
        arena->head = block;
    }
    void *result = arena->head->data + arena->head->used;
    arena->head->used += size;
    return result;
}

const char *rangeArenaIntern(RangeArena *arena, const char *text, size_t length)
{
    char *copy = rangeArenaAllocate(arena, length + 1);
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

RangeNode *rangeNodeCreate(RangeArena *arena, RangeNodeKind kind,
                           const char *path, int line, int column)
{
    RangeNode *node = rangeArenaAllocate(arena, sizeof(RangeNode));
    memset(node, 0, sizeof(*node));
    node->kind = kind;
    node->path = path;
    node->line = line;
    node->column = column;
    arena->nodeCount += 1;
    return node;
}

void rangeNodeAppend(RangeArena *arena, RangeNode *node, RangeNode *item)
{
    if (node->itemCount == node->itemCapacity) {
        size_t capacity = node->itemCapacity ? node->itemCapacity * 2 : 4;
        RangeNode **items = rangeArenaAllocate(arena, capacity * sizeof(RangeNode *));
        if (node->itemCount) {
            memcpy(items, node->items, node->itemCount * sizeof(RangeNode *));
        }
        node->items = items;
        node->itemCapacity = capacity;
    }
    node->items[node->itemCount++] = item;
}

static const char *const RANGE_NODE_NAMES[RangeNodeKindCount] = {
    "unit", "construct", "enum", "enumCase", "function", "macro", "main",
    "member", "parameter", "attribute", "block", "local", "assign", "if",
    "while", "return", "expressionStatement", "call", "argument",
    "memberAccess", "name", "integer", "bool", "string", "stringPart",
    "case", "unary", "binary", "manyLiteral", "environment", "syntaxTemplate",
    "closure", "emission", "switch", "switchCase", "extension"
};

const char *rangeNodeKindName(RangeNodeKind kind)
{
    if (kind < 0 || kind >= RangeNodeKindCount) return "<invalid>";
    return RANGE_NODE_NAMES[kind];
}
