/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
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
char* cwdPath = "/";

int isDirectory(directoryEntry* parent){
    if(parent->isDir==1){
        return 1;
    }
    return 0;
}

int isFile(directoryEntry* parent){
    if(isDirectory(parent) == 1){
        return 0;
    }
    return 1;
}

directoryEntry* loadDir(directoryEntry* parent){
    directoryEntry* directory = malloc(parent->file_size_bytes);
    if(directory == NULL){
        return NULL;
    }
    
    space* sp = loadSpace(parent);
    int index=0;

    while(sp[index].start!= -1){
        LBAread(directory + (index * sizeof(directoryEntry)), 
        sp[index].count, sp[index].start);
        index++;
    }
    free(sp);
    return directory;
}

int findEntryInDir(directoryEntry* parent, char* name) {
    int directories = parent->file_size_bytes / sizeof(directoryEntry);
    for(int loop = 0;loop<directories;loop++){
        if(strcmp(parent[loop].file_name, name) == 0){
            return loop;
        }
    }
    return -1;
}

int parsePath(char* path, parentInfo* parentInfo) {
    directoryEntry* start;
    if (path == NULL || parentInfo == NULL){
        return -1;
    }
    if(path[0] == '/'){
        start = rootDirectory;
    }else{
        start = cwd;
    }
    directoryEntry* parentDir = start;

    char* ptr;
    char* token = strtok_r(path, "/", &ptr);

    if(token==NULL){
        if(strcmp(path, "/")==0){
            parentInfo->parent = parentDir;
            parentInfo->index = -1;
            parentInfo->lastElement = NULL;
            return 0;
        }else{
            return -1;
        }
    }else{
        return -1;
    }

    while(token!=NULL){
        int index = findEntryInDir(parentDir, token);
        char* secondToken = strtok_r(NULL, "/", &ptr);

        if(secondToken==NULL){
            parentInfo->index = index;
            parentInfo->parent = parentDir;
            parentInfo->lastElement = strdup(token);
            return 0;
        }else{
            return -1;
        }

        if(index==-1){
            return -2;
        }

        if(isDirectory(&(parentDir[index]))==0){
            return -2;
        }

        directoryEntry* direct = loadDir(&(parentDir[index]));

        if(direct==NULL){
            return -1;
        }

        if(parentDir != start){
            free(parentDir);
        }

        parentDir = direct;
        token = secondToken;
    }
}

int fs_mkdir(const char *pathname, mode_t mode){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path,parent);
    if(value == -1){
        free(parent);
        free(path);
        return -1;
    }
    if(value == -2){
        free(parent);
        free(path);
        return -1;
    }
    if(value == 0 && parent->index != -1){
        free(parent);
        free(path);
        return -1;
    }
    directoryEntry newDirectory;
    newDirectory.starting_block = initDir(50,parent->parent);
    directoryEntry *direct = calloc(1,BLOCKSIZE);
    LBAread(direct,1,newDirectory.starting_block);
    newDirectory = direct[0];
    strcpy(newDirectory.file_name, parent->lastElement);
    int index = findEntryInDir(parent->parent,"\0");
    if(index == -1){
        return -1;
    }
    parent->parent[index] = newDirectory;
    int blocks = parent->parent->file_size_bytes/BLOCKSIZE;

    LBAwrite(parent->parent,blocks,parent->parent->starting_block);
    free(direct);
    free(parent);
    free(path);
}

int fs_rmdir(const char *pathname){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path,parent);
    if(value == -1){
        free(path);
        free(parent);
        return -1;
    }
    if(value == -2){
        free(path);
        free(parent);
        return -1;
    }
    if(parent->index == -1){
        free(path);
        free(parent);
        return -1;
    }
    if(parent->index < 2){
        free(parent);
        free(path);
        return -1;
    }
    if(fs_isDir(path) != 1){
        free(parent);
        free(path);
        return -1;
    }
    if(isDirAtDeEmpty(&parent->parent[parent->index]) == -1){
        free(parent);
        free(path);
        return -1;
    }
    space* sp = loadSpace(&parent->parent[parent->index]);
    int index = 0;
    while(sp[index].start != -1){
        clearBits(sp[index].start,sp[index].count);
        index++;
    }
    strcpy(parent->parent[parent->index].file_name,"\0");
    int blocksUsed = parent->parent->file_size_bytes/BLOCKSIZE;
    LBAwrite(parent->parent,blocksUsed,parent->parent->starting_block);
    free(sp);
    free(parent);
    free(path);
    return 0;
}

int fs_isDir(char * pathname){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path,parent);
    if(value != 0){
        return -1;
    }
    if(parent->index == -1){
        return -1;
    }else{
        directoryEntry *directory;
        directory = &(parent->parent[parent->index]);
        if(isDirectory(directory) == 1){
            return 1;
        }
    }
    free(parent);
    free(path);
}

int fs_isFile(char * pathname){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(pathname);
    int value = parsePath(path,parent);
    if(value == 0 && parent->index == -1){
        return -1;
    }else{
        directoryEntry *directory;
        directory = &(parent->parent[parent->index]);
        if(isFile(directory) == 1){
            return 1;
        }
    }
    free(parent);
    free(path);
}


int fs_delete(char* filename){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* path = strdup(filename);
    int value = parsePath(path,parent);
    if(value != 0){
        free(parent);
        free(path);
        return -1;
    }
    if(parent->index == -1){
        free(parent);
        free(path);
        return -1;
    }
    if(fs_isFile(path) != 1){
        free(parent);
        free(path);
        return -1;
    }
    space* sp = loadSpace(&parent->parent[parent->index]);
    int index = 0;
    strcpy(parent->parent[parent->index].file_name,"\0");
    while(sp[index].start != -1){
        clearBits(sp[index].start,sp[index].count);
        index++;
    }
    int blocks = parent->parent->file_size_bytes/BLOCKSIZE;
    LBAwrite(parent->parent, blocks,parent->parent->starting_block);
    free(sp);
    free(parent);
    free(path);
}


int isDirAtDeEmpty(directoryEntry * dir){
    int entryCount = dir->file_size_bytes/BLOCKSIZE;
    for(int loop = 2; loop < entryCount;loop++){
        if (strcmp(dir[loop].file_name, "\0") != 0){
            return -1;
        }
    }
    return 0;
}
int fs_setcwd(char *pathname){
    parentInfo *parent = malloc(sizeof(parentInfo));

    char* path = strdup(pathname);
    int value = parsePath(pathname,parent);
    if(value == -1){
        free(parent);
        free(path);
        return -1;
    }
    if(value == -2){
        free(parent);
        free(path);
        return -1;
    }
    if(parent->index == -1){
        free(parent);
        free(path);
        return -1;
    }
    if(fs_isDir(path) != 1){
        free(parent);
        free(path);
        return -1;
    }
    if(cwd != rootDirectory){
        free(cwd);
    }
    cwd = loadDir(&(parent->parent[parent->index]));
    char* new;
    if(path[0] == '/'){
        new = path;
    }else{
        new = strcat(cwdPath, "/");
        new = strcat(cwdPath, path);
    }
    cwdPath = cleanPath(new);
    if(cwdPath == NULL){
        free(parent);
        free(path);
        return -1;
    }
    free(parent);
    free(path);
    return 0;
}

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
            printf("Couldnt Allocate memory for Token Array\n");
            return NULL;
        }
        if(strcmp(token,"..") == 0){
            index--;
        }else{
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
        if(loop != index -1) strcat(result, "/");
    }

    for (int loop = 1; loop < index; loop++) {
        free(arrayOfTokens[loop]);
    }
    free(arrayOfTokens);

    return result;
}

char * fs_getcwd(char *pathname, size_t size){
    if(pathname == NULL || size <= 0){
        return NULL;
    }
    strcpy(pathname,cwdPath);
    pathname[size] = '\0';
    return pathname;
}

int fs_stat(const char *path, struct fs_stat *buf){
    parentInfo *parent = malloc(sizeof(parentInfo));
    char* secondPath = strdup(path);
    int value = parsePath(secondPath,parent);
    if(value == -1){
        free(parent);
        free(secondPath);
        return -1;
    }
    if(value == -2){
        free(parent);
        free(secondPath);
        return -1;
    }
    if(parent->index == -1){
        free(parent);
        free(secondPath);
        return -1;
    }
    if(fs_isDir(secondPath) != 1){
        free(parent);
        free(secondPath);
        return -1;
    }
    buf->st_size = parent->parent[parent->index].file_size_bytes;
    buf->st_blksize = BLOCKSIZE;
    buf->st_blocks = (parent->parent[parent->index].file_size_bytes)/BLOCKSIZE;	
    free(parent);
    free(secondPath);
    return 0;
}

fdDir * fs_opendir(const char *pathname) {
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
        return -1;
    }

    directoryStructure->dirEntryPosition = 0;
    directoryStructure->di = malloc(sizeof(struct fs_diriteminfo));
    if (!directoryStructure->di) {
        free(directoryStructure);
        return -1;
    }

    directoryStructure->di->d_reclen = sizeof(struct fs_diriteminfo);
    directoryStructure->di->fileType = FT_DIRECTORY;
    strcpy(directoryStructure->di->d_name, parentDirectory->file_name);

    return directoryStructure;
}

int fs_closedir(fdDir *dirp) {
    if (dirp == NULL) {
        return NULL;
    }

    if (dirp->di != NULL) {
        free(dirp->di);
        dirp->di = NULL;
    }

    free(dirp);
    return 0;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    if (dirp==NULL || cwd==NULL) {
        return NULL;
    }

    int numberOfEntries = cwd->file_size_bytes / sizeof(directoryEntry);

    if (dirp->dirEntryPosition >= numberOfEntries) {
        return -1;
    }

    directoryEntry *entry = &cwd[dirp->dirEntryPosition];

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
