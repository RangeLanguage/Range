/* Development driver: parse every source in a manifest and report a census. */
#include "parser.h"
#include "evaluator.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *readFile(const char *path, size_t *size)
{
    FILE *stream = fopen(path, "rb");
    if (!stream) return NULL;
    fseek(stream, 0, SEEK_END);
    long length = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    char *buffer = malloc((size_t)length + 1);
    if (!buffer) return NULL;
    if (fread(buffer, 1, (size_t)length, stream) != (size_t)length) {
        fclose(stream); free(buffer); return NULL;
    }
    fclose(stream);
    buffer[length] = '\0';
    *size = (size_t)length;
    return buffer;
}

/* Structural dump: one indented line per node, used by the recovery gate to
 * assert that difficult forms are actually represented rather than skipped. */
static void dumpTree(const RangeNode *node, int depth)
{
    if (!node) return;
    for (int step = 0; step < depth; ++step) fputs("  ", stdout);
    fputs(rangeNodeKindName(node->kind), stdout);
    if (node->name) printf(" name=%s", node->name);
    if (node->typeName) printf(" type=%s", node->typeName);
    if (node->flags) printf(" flags=%d", node->flags);
    if (node->rhsEnd > node->rhsStart) {
        printf(" rhs=%zu:%zu application=%s", node->rhsStart, node->rhsEnd,
               node->flags & RangeFlagApplication ? "yes" : "no");
    }
    if (node->spanEnd > node->spanStart) {
        printf(" span=%zu", node->spanEnd - node->spanStart);
    }
    if (node->itemCount) printf(" items=%zu", node->itemCount);
    putchar('\n');
    dumpTree(node->a, depth + 1);
    dumpTree(node->b, depth + 1);
    dumpTree(node->c, depth + 1);
    dumpTree(node->annotations, depth + 1);
    dumpTree(node->generics, depth + 1);
    dumpTree(node->rhsReference, depth + 1);
    for (size_t index = 0; index < node->itemCount; ++index) {
        dumpTree(node->items[index], depth + 1);
    }
}

int main(int argc, char **argv)
{
    RangeArena arena;
    rangeArenaInit(&arena);
    int treeMode = 0;
    const char *entry = NULL;
    int first = 1;
    if (argc > 1 && strcmp(argv[1], "--tree") == 0) { treeMode = 1; first = 2; }
    if (argc > 2 && strcmp(argv[1], "--run") == 0) { entry = argv[2]; first = 3; }
    if (first >= argc) { fprintf(stderr, "usage: recovery [--tree | --run function] sources...\n"); return 64; }
    RangeNode **units = calloc((size_t)argc, sizeof(*units));
    if (!units) return 70;
    size_t unitCount = 0;
    long counts[RangeNodeKindCount];
    memset(counts, 0, sizeof(counts));
    int failures = 0;
    for (int index = first; index < argc; ++index) {
        size_t size = 0;
        char *source = readFile(argv[index], &size);
        if (!source) { fprintf(stderr, "cannot read %s\n", argv[index]); return 66; }
        char error[512];
        RangeNode *unit = rangeParseUnit(&arena, argv[index], source, size,
                                         error, sizeof(error));
        free(source);
        if (!unit) { fprintf(stderr, "%s\n", error); failures += 1; continue; }
        units[unitCount++] = unit;
        if (treeMode) { dumpTree(unit, 0); continue; }
        for (size_t item = 0; item < unit->itemCount; ++item) {
            counts[unit->items[item]->kind] += 1;
        }
    }
    if (entry && !failures) {
        int64_t result = 0;
        char error[512];
        if (!rangeExecute(&arena, units, unitCount, entry, &result, error, sizeof(error))) {
            fprintf(stderr, "%s\n", error);
            failures += 1;
        } else printf("result=%" PRId64 "\n", result);
    }
    free(units);
    if (entry || treeMode) { rangeArenaDestroy(&arena); return failures ? 65 : 0; }
    printf("nodes=%zu construct=%ld enum=%ld function=%ld macro=%ld main=%ld failures=%d\n",
           arena.nodeCount, counts[RangeNodeConstruct], counts[RangeNodeEnum],
           counts[RangeNodeFunction], counts[RangeNodeMacro], counts[RangeNodeMain],
           failures);
    rangeArenaDestroy(&arena);
    return failures ? 65 : 0;
}
