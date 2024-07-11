/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: vcb.h
*
* Description:: This file will serve as our structure for our
* volume control block.
*
**************************************************************/

#ifndef VCB_H
#define VCB_H

#include <stdint.h>  // Fixed-width integer types

// Define a unique signature for a valid VCB
#define VCB_SIGNATURE 0x5262742042ULL

typedef struct {
    uint64_t signature;              // Unique identifier for a valid VCB
    uint32_t blockSizeBytes;         // Size of each block in bytes
    uint32_t totalBlockCount;        // Total number of blocks in the file system
    uint32_t availableBlocks;        // Number of free blocks
    uint32_t rootDirectoryBlock;     // Block number of the root directory
    uint32_t freeSpaceMapBlock;      // Block number of the free space map
} VolumeControlBlock;

#endif
