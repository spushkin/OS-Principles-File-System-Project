/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: initializeDirectories.h
*
* Description:: This file will hold the declarations for our
* directory initializations.
*
**************************************************************/
#ifndef INITIALIZEDIRECTORIES_H
#define INITIALIZEDIRECTORIES_H

#include <time.h>
#include "FSFunctions.h"
#include "fsLow.h"
#include <string.h>
#include <stdio.h>

typedef struct directoryEntry {
    char file_name[256];                // Name of the file or directory (null-terminated string)
    uint64_t creation_timestamp;        // Time of creation
    uint64_t modification_timestamp;    // Time of last modification
    uint64_t last_access_timestamp;     // Time of last access
    uint32_t starting_block;            // Block number where the file starts
    uint32_t file_size_bytes;           // Size of the file in bytes
    uint32_t owner_id;                  // User ID of the file owner
    uint32_t group_id;                  // Group ID of the file owner
    uint32_t isDir;                     // 1 if directory, 0 if not a directory.
    uint32_t file_attributes;           // Attributes of the file (e.g., directory, read-only)
    space spaces[4];          
} directoryEntry;

int writeToDisk(directoryEntry *dir, space *e);
int initDir(int entries, directoryEntry *parent);
space* loadSpace(directoryEntry *dir);

#endif