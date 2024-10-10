/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: mfs.c
*
* Description: This file implements the following functions from mfs.h:
* isDirectory, isFile, loadDir, findEntryInDir, parsePath,
* fs_mkdir, fs_rmdir, fs_isDir, fs_isFile, fs_delete,
* isDirAtDeEmpty, fs_setcwd, cleanPath, fs_getcwd, fs_stat,
* fs_opendir, fs_closedir, fs_readdir.
*
**************************************************************/
#include "mfs.h"
#include <stdio.h>
#include <unistd.h>
#include "fsLow.h"
#include "constants.h"
#include <sys/types.h>
#include <time.h>
#include "initializeDirectories.h"
#include <stdlib.h>
#include <string.h>

char* cwdPath = "/"; // Current working directory path

// Check if the directory entry is a directory
int isDirectory(directoryEntry* parent) {
    return parent->isDir == 1;
}

// Check if the directory entry is a file
int isFile(directoryEntry* parent) {
    return !isDirectory(parent);
}

// Load directory entries from disk
directoryEntry* loadDir(directoryEntry* parent) {
    if (parent == NULL) {
        printf("Error: parent is NULL in loadDir\n");
        return NULL;
    }

    directoryEntry* directory = malloc(parent->file_size_bytes);
    if (directory == NULL) {
        printf("Error: Memory allocation failed in loadDir\n");
        return NULL;
    }

    space* sp = loadSpace(parent); // Load the space information
    if (sp == NULL) {
        printf("Error: loadSpace failed in loadDir\n");
        free(directory);
        return NULL;
    }

    int index = 0;

    // Read directory entries from disk
    while (sp[index].start != -1) {
        LBAread(directory + (index * sizeof(directoryEntry)), sp[index].count, sp[index].start);
        index++;
    }
    free(sp);
    return directory;
}

// Find an entry in the directory by name
int findEntryInDir(directoryEntry* parent, char* name) {
    int directories = parent->file_size_bytes / sizeof(directoryEntry);
    for (int loop = 0; loop < directories; loop++) {
        if (strcmp(parent[loop].file_name, name) == 0) {
            return loop;
        }
    }
    return -1;
}

// Parse the given path and return parent directory information
int parsePath(char* path, parentInfo* parentInfo) {
    directoryEntry* start;
    if (path == NULL || parentInfo == NULL) {
        return -1;
    }
    if (path[0] == '/') {
        start = rootDirectory; // Start from root if path is absolute
    } else {
        start = cwd; // Start from current working directory if path is relative
    }
    directoryEntry* parentDir = start;

    char* ptr;
    char* token = strtok_r(path, "/", &ptr);

    // If the path is root
    if (token == NULL) {
        if (strcmp(path, "/") == 0) {
            parentInfo->parent = parentDir;
            parentInfo->index = -1;
            parentInfo->lastElement = NULL;
            return 0;
        } else {
            return -1;
        }
    }

    // Traverse through each token in the path
    while (token != NULL) {
        int index = findEntryInDir(parentDir, token);
        char* secondToken = strtok_r(NULL, "/", &ptr);

        // If this is the last token
        if (secondToken == NULL) {
            parentInfo->index = index;
            parentInfo->parent = parentDir;
            parentInfo->lastElement = strdup(token);
            return 0;
        }

        // If the entry is not found or not a directory
        if (index == -1 || !isDirectory(&(parentDir[index]))) {
            return -2;
        }

        // Load the next directory
        directoryEntry* direct = loadDir(&(parentDir[index]));
        if (direct == NULL) {
            return -1;
        }

        if (parentDir != start) {
            free(parentDir);
        }

        parentDir = direct;
        token = secondToken;
    }
    return -1;
}

// Create a new directory
int fs_mkdir(const char *pathname, mode_t mode) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path, parent);
    if (value == -1 || value == -2 || (value == 0 && parent->index != -1)) {
        free(parent);
        free(path);
        return -1;
    }
    directoryEntry newDirectory;
    newDirectory.starting_block = initDir(50, parent->parent);
    directoryEntry *direct = calloc(1, BLOCKSIZE);
    LBAread(direct, 1, newDirectory.starting_block);
    newDirectory = direct[0];
    strcpy(newDirectory.file_name, parent->lastElement);
    int index = findEntryInDir(parent->parent, "\0");
    if (index == -1) {
        free(direct);
        free(parent);
        free(path);
        return -1;
    }
    parent->parent[index] = newDirectory;
    int blocks = parent->parent->file_size_bytes / BLOCKSIZE;
    int result = LBAwrite(parent->parent, blocks, parent->parent->starting_block);
    if (result != blocks) {
        free(direct);
        free(parent);
        free(path);
        return -1;
    }
    free(direct);
    free(parent->lastElement);
    free(parent);
    free(path);
    return 0;
}

// Remove a directory
int fs_rmdir(const char *pathname) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    if (!parent) {
        printf("Error: Memory allocation failed for parent in fs_rmdir\n");
        return -1;
    }

    char* path = strdup(pathname);
    if (!path) {
        printf("Error: Memory allocation failed for path in fs_rmdir\n");
        free(parent);
        return -1;
    }

    int value = parsePath(path, parent);
    if (value == -1 || value == -2 || parent->index == -1 || parent->index < 2 ||
        fs_isDir(path) != 1 || isDirAtDeEmpty(&parent->parent[parent->index]) == -1) {
        free(parent);
        free(path);
        return -1;
    }

    space* sp = loadSpace(&parent->parent[parent->index]);
    int index = 0;
    while (sp[index].start != -1) {
        clearBits(sp[index].start, sp[index].count);
        index++;
    }

    strcpy(parent->parent[parent->index].file_name, "\0");
    int blocksUsed = parent->parent->file_size_bytes / BLOCKSIZE;
    LBAwrite(parent->parent, blocksUsed, parent->parent->starting_block);

    free(sp);
    free(parent);
    free(path);
    return 0;
}

// Check if the given path is a directory
int fs_isDir(char *pathname) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    if (!parent) {
        printf("Error: Memory allocation failed for parent in fs_isDir\n");
        return -1;
    }

    char* path = strdup(pathname);
    if (!path) {
        printf("Error: Memory allocation failed for path in fs_isDir\n");
        free(parent);
        return -1;
    }

    int value = parsePath(path, parent);
    if (value != 0) {
        free(parent);
        free(path);
        return -1;
    }
    if (parent->index == -1) {
        free(parent);
        free(path);
        return -1;
    } else {
        directoryEntry *directory = &(parent->parent[parent->index]);
        int result = isDirectory(directory);
        free(parent);
        free(path);
        return result;
    }
}

// Check if the given path is a file
int fs_isFile(char *pathname) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    if (!parent) {
        printf("Error: Memory allocation failed for parent in fs_isFile\n");
        return -1;
    }

    char* path = strdup(pathname);
    if (!path) {
        printf("Error: Memory allocation failed for path in fs_isFile\n");
        free(parent);
        return -1;
    }

    int value = parsePath(path, parent);
    if (value == 0 && parent->index != -1) {
        directoryEntry *directory = &(parent->parent[parent->index]);
        int result = isFile(directory);
        free(parent);
        free(path);
        return result;
    } else {
        free(parent);
        free(path);
        return -1;
    }
}

// Delete a file
int fs_delete(char* filename) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    if (!parent) {
        printf("Error: Memory allocation failed for parent in fs_delete\n");
        return -1;
    }

    char* path = strdup(filename);
    if (!path) {
        printf("Error: Memory allocation failed for path in fs_delete\n");
        free(parent);
        return -1;
    }

    int value = parsePath(path, parent);
    if (value != 0 || parent->index == -1 || fs_isFile(path) != 1) {
        free(parent);
        free(path);
        return -1;
    }

    space* sp = loadSpace(&parent->parent[parent->index]);
    int index = 0;
    strcpy(parent->parent[parent->index].file_name, "\0");
    while (sp[index].start != -1) {
        clearBits(sp[index].start, sp[index].count);
        index++;
    }

    int blocks = parent->parent->file_size_bytes / BLOCKSIZE;
    LBAwrite(parent->parent, blocks, parent->parent->starting_block);

    free(sp);
    free(parent);
    free(path);
    return 0;
}

// Check if a directory is empty
int isDirAtDeEmpty(directoryEntry *dir) {
    int entryCount = dir->file_size_bytes / BLOCKSIZE;
    for (int loop = 2; loop < entryCount; loop++) {
        if (strcmp(dir[loop].file_name, "\0") != 0) {
            return -1;
        }
    }
    return 0;
}

// Set the current working directory
int fs_setcwd(char *pathname) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path, parent);
    if (value == -1 || value == -2 || parent->index == -1 || fs_isDir(path) != 1) {
        free(parent);
        free(path);
        return -1;
    }
    if (cwd != rootDirectory) {
        free(cwd);
    }
    cwd = loadDir(&(parent->parent[parent->index]));
    if (cwd == NULL) {
        free(parent);
        free(path);
        return -1;
    }
    char* new;
    if (path[0] == '/') {
        new = path;
    } else {
        new = strcat(cwdPath, "/");
        new = strcat(cwdPath, path);
    }
    cwdPath = cleanPath(new);
    if (cwdPath == NULL) {
        free(parent);
        free(path);
        return -1;
    }
    free(parent);
    free(path);
    return 0;
}

// Clean up the given path
char* cleanPath(char* path) {
    char* stateofPath;
    int index = 1;
    int tokens = 1;
    for (int i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            tokens++;
        }
    }
    char** arrayOfTokens = (char**)malloc(tokens * sizeof(char*));
    if (arrayOfTokens == NULL) {
        return NULL;
    }

    char* token = strtok_r((char*)path, "/", &stateofPath);
    arrayOfTokens[0] = "/";

    while (token != NULL) {
        arrayOfTokens[index] = (char*)malloc(strlen(token) + 1);
        if (arrayOfTokens[index] == NULL) {
            return NULL;
        }
        if (strcmp(token, "..") == 0) {
            index--;
        } else {
            strcpy(arrayOfTokens[index], token);
            index++;
        }
        token = strtok_r(NULL, "/", &stateofPath);
    }

    int resultSize = 1;
    for (int loop = 1; loop < index; loop++) {
        resultSize += strlen(arrayOfTokens[loop]) + 1;
    }

    char* result = (char*)malloc(resultSize);
    if (result == NULL) {
        return NULL;
    }

    strcpy(result, "/");
    for (int loop = 1; loop < index; loop++) {
        strcat(result, arrayOfTokens[loop]);
        if (loop != index - 1) strcat(result, "/");
    }

    for (int loop = 1; loop < index; loop++) {
        free(arrayOfTokens[loop]);
    }
    free(arrayOfTokens);

    return result;
}

// Get the current working directory
char *fs_getcwd(char *pathname, size_t size) {
    if (pathname == NULL || size <= 0) {
        return NULL;
    }
    strncpy(pathname, cwdPath, size - 1);
    pathname[size - 1] = '\0';
    return pathname;
}

// Get the status of a file or directory
int fs_stat(const char *path, struct fs_stat *buf) {
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* secondPath = strdup(path);
    int value = parsePath(secondPath, parent);
    if (value == -1 || value == -2 || parent->index == -1 || fs_isDir(secondPath) != 1) {
        free(parent);
        free(secondPath);
        return -1;
    }
    buf->st_size = parent->parent[parent->index].file_size_bytes;
    buf->st_blksize = BLOCKSIZE;
    buf->st_blocks = parent->parent[parent->index].file_size_bytes / BLOCKSIZE;
    free(parent);
    free(secondPath);
    return 0;
}

// Open a directory for reading
fdDir *fs_opendir(const char *pathname) {
    char* path = strdup(pathname);
    if (path == NULL) {
        return NULL;
    }

    parentInfo parent;
    int parseResult = parsePath(path, &parent);
    free(path);

    if (parseResult != 0 || parent.parent == NULL || !isDirectory(parent.parent)) {
        return NULL;
    }

    directoryEntry *parentDirectory = parent.parent;

    fdDir *directoryStructure = malloc(sizeof(fdDir));
    if (!directoryStructure) {
        return NULL;
    }

    directoryStructure->dirEntryPosition = 0;
    directoryStructure->di = malloc(sizeof(struct fs_diriteminfo));
    if (!directoryStructure->di) {
        free(directoryStructure);
        return NULL;
    }

    directoryStructure->di->d_reclen = sizeof(struct fs_diriteminfo);
    directoryStructure->di->fileType = FT_DIRECTORY;
    strcpy(directoryStructure->di->d_name, parentDirectory->file_name);

    return directoryStructure;
}

// Close an open directory
int fs_closedir(fdDir *dirp) {
    if (dirp == NULL) {
        return -1;
    }

    if (dirp->di != NULL) {
        free(dirp->di);
        dirp->di = NULL;
    }

    free(dirp);
    return 0;
}

// Read a directory entry
struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    if (dirp == NULL || cwd == NULL) {
        return NULL;
    }

    int numberOfEntries = cwd->file_size_bytes / sizeof(directoryEntry);

    if (dirp->dirEntryPosition >= numberOfEntries) {
        return NULL;
    }

    directoryEntry *entry = &cwd[dirp->dirEntryPosition];

    // Skip empty entries
    while (dirp->dirEntryPosition < numberOfEntries && strcmp(entry->file_name, "\0") == 0) {
        dirp->dirEntryPosition++;
        entry = &cwd[dirp->dirEntryPosition];
    }

    if (dirp->dirEntryPosition >= numberOfEntries) {
        return NULL;
    }

    dirp->dirEntryPosition++;

    dirp->di->d_reclen = sizeof(struct fs_diriteminfo);

    if (entry->isDir) {
        dirp->di->fileType = FT_DIRECTORY;
    } else {
        dirp->di->fileType = FT_REGFILE;
    }

    strncpy(dirp->di->d_name, entry->file_name, sizeof(dirp->di->d_name) - 1);
    dirp->di->d_name[sizeof(dirp->di->d_name) - 1] = '\0';

    return dirp->di;
}
