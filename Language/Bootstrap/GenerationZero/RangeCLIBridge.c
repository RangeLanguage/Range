#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ProgramGraphBridge.h"

enum {
    RANGE_USAGE = 64,
    RANGE_DATA = 65,
    RANGE_MISSING = 66,
    RANGE_UNAVAILABLE = 69,
    RANGE_CREATE = 73
};

static int fail(const char *message, int status) {
    fprintf(stderr, "range: %s\n", message);
    return status;
}

static int usage(void) {
    fputs(
        "usage: range <version|register|projects|compile|run>\n",
        stderr
    );
    return RANGE_USAGE;
}

static bool directory_exists(const char *path) {
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

static bool file_exists(const char *path) {
    struct stat status;
    return stat(path, &status) == 0 && S_ISREG(status.st_mode);
}

static int create_directory(const char *path) {
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return directory_exists(path) ? 0 : RANGE_CREATE;
    }
    return RANGE_CREATE;
}

static int join_path(
    char *output,
    size_t capacity,
    const char *left,
    const char *right
) {
    const int count = snprintf(output, capacity, "%s/%s", left, right);
    return count < 0 || (size_t)count >= capacity ? RANGE_DATA : 0;
}

static int executable_toolchain_root(char *output, size_t capacity) {
    const char *override = getenv("RANGE_TOOLCHAIN_ROOT");
    if (override != NULL && override[0] != '\0') {
        return snprintf(output, capacity, "%s", override) >= (int)capacity
            ? RANGE_DATA : 0;
    }
    char executable[PATH_MAX];
    uint32_t executableCapacity = sizeof(executable);
    if (_NSGetExecutablePath(executable, &executableCapacity) != 0) {
        return RANGE_DATA;
    }
    char resolved[PATH_MAX];
    if (realpath(executable, resolved) == NULL) {
        return RANGE_DATA;
    }
    char *bin = strrchr(resolved, '/');
    if (bin == NULL) return RANGE_DATA;
    *bin = '\0';
    char *root = strrchr(resolved, '/');
    if (root == NULL) return RANGE_DATA;
    *root = '\0';
    return snprintf(output, capacity, "%s", resolved) >= (int)capacity
        ? RANGE_DATA : 0;
}

static int registry_paths(
    char *registry,
    char *projects,
    const char *toolchain
) {
    if (join_path(registry, PATH_MAX, toolchain, "Registry") != 0
        || join_path(projects, PATH_MAX, registry, "Projects") != 0) {
        return RANGE_DATA;
    }
    return 0;
}

static int write_text(const char *path, const char *text) {
    char temporary[PATH_MAX];
    const int count = snprintf(
        temporary,
        sizeof(temporary),
        "%s.%ld.tmp",
        path,
        (long)getpid()
    );
    if (count < 0 || (size_t)count >= sizeof(temporary)) return RANGE_DATA;
    FILE *stream = fopen(temporary, "wb");
    if (stream == NULL) return RANGE_CREATE;
    const size_t length = strlen(text);
    const bool written = fwrite(text, 1, length, stream) == length;
    const bool closed = fclose(stream) == 0;
    if (!written || !closed || rename(temporary, path) != 0) {
        unlink(temporary);
        return RANGE_CREATE;
    }
    return 0;
}

static int command_register(
    const char *toolchain,
    int argumentCount,
    char **arguments
) {
    if (argumentCount != 1 && argumentCount != 2) return usage();
    const char *identity = arguments[0];
    if (identity[0] == '\0' || strchr(identity, '/') != NULL) return usage();

    char registry[PATH_MAX];
    char records[PATH_MAX];
    if (registry_paths(registry, records, toolchain) != 0
        || create_directory(registry) != 0
        || create_directory(records) != 0) {
        return fail("could not create the project registry", RANGE_CREATE);
    }

    char defaultProjects[PATH_MAX];
    char root[PATH_MAX];
    if (join_path(defaultProjects, sizeof(defaultProjects), toolchain, "Projects")
            != 0) {
        return fail("project path is too long", RANGE_DATA);
    }
    if (argumentCount == 2) {
        if (snprintf(root, sizeof(root), "%s", arguments[1])
            >= (int)sizeof(root)) {
            return fail("project path is too long", RANGE_DATA);
        }
    } else {
        if (create_directory(defaultProjects) != 0
            || join_path(root, sizeof(root), defaultProjects, identity) != 0) {
            return fail("could not create the project root", RANGE_CREATE);
        }
    }
    if (create_directory(root) != 0) {
        return fail("could not create the project root", RANGE_CREATE);
    }

    char projectFile[PATH_MAX];
    if (join_path(projectFile, sizeof(projectFile), root, "Project.range") != 0) {
        return fail("project path is too long", RANGE_DATA);
    }
    if (!file_exists(projectFile)) {
        char declaration[1024];
        const int declarationCount = snprintf(
            declaration,
            sizeof(declaration),
            "@project\n@target(platform: .macOS, architecture: .arm64, default: true)\nconstruct %s {}\n",
            identity
        );
        if (declarationCount < 0
            || (size_t)declarationCount >= sizeof(declaration)
            || write_text(projectFile, declaration) != 0) {
            return fail("could not create Project.range", RANGE_CREATE);
        }
    }

    char record[PATH_MAX];
    if (join_path(record, sizeof(record), records, identity) != 0
        || write_text(record, root) != 0) {
        return fail("could not register the project", RANGE_CREATE);
    }
    return 0;
}

static int compare_names(const void *left, const void *right) {
    const char *const *leftName = left;
    const char *const *rightName = right;
    return strcmp(*leftName, *rightName);
}

static int command_projects(const char *toolchain) {
    char registry[PATH_MAX];
    char records[PATH_MAX];
    if (registry_paths(registry, records, toolchain) != 0) return RANGE_DATA;
    DIR *directory = opendir(records);
    if (directory == NULL) {
        return errno == ENOENT ? 0
            : fail("could not read the project registry", RANGE_MISSING);
    }
    char **names = NULL;
    size_t count = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char **expanded = realloc(names, sizeof(char *) * (count + 1));
        if (expanded == NULL) {
            closedir(directory);
            return fail("could not allocate the project list", RANGE_UNAVAILABLE);
        }
        names = expanded;
        names[count] = strdup(entry->d_name);
        if (names[count] == NULL) {
            closedir(directory);
            return fail("could not allocate a project name", RANGE_UNAVAILABLE);
        }
        count += 1;
    }
    closedir(directory);
    qsort(names, count, sizeof(char *), compare_names);
    for (size_t index = 0; index < count; ++index) {
        puts(names[index]);
        free(names[index]);
    }
    free(names);
    return 0;
}

static int command_compiler_boundary(const char *command) {
    fprintf(
        stderr,
        "range %s: compiler-body graph lowering is not materialized\n",
        command
    );
    return RANGE_UNAVAILABLE;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    if (strcmp(argv[1], "__graph-schema") == 0) {
        if (argc != 2) return usage();
        fputs(rangeProgramGraphBridgeSchema(), stdout);
        return 0;
    }
    char toolchain[PATH_MAX];
    if (executable_toolchain_root(toolchain, sizeof(toolchain)) != 0) {
        return fail("could not discover the Range installation", RANGE_MISSING);
    }
    const char *command = argv[1];
    if (strcmp(command, "version") == 0
        || strcmp(command, "--version") == 0) {
        if (argc != 2) return usage();
        puts("Range 0.1.0");
        return 0;
    }
    if (strcmp(command, "register") == 0) {
        return command_register(toolchain, argc - 2, argv + 2);
    }
    if (strcmp(command, "projects") == 0) {
        if (argc != 2) return usage();
        return command_projects(toolchain);
    }
    if (strcmp(command, "compile") == 0
        || strcmp(command, "run") == 0) {
        if (argc < 3) return usage();
        return command_compiler_boundary(command);
    }
    return usage();
}
