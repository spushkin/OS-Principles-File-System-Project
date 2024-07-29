/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"
#include "initializeDirectories.h"
#include "FSFunctions.h"

VolumeControlBlock *vcpPoint;    // Pointer to the volume control block
directoryEntry *rootDirectory;   // Root directory
directoryEntry *cwd;             // Current working directory
int BLOCKSIZE;                   // Block size in bytes

// Initialize the file system
int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    BLOCKSIZE = blockSize;
    printf ("Initializing File System with %ld blocks with a block size of %ld\n",
        numberOfBlocks, blockSize);

    // Allocate memory for the volume control block
    vcpPoint = malloc(blockSize);
    LBAread(vcpPoint, 1, 0);

    // Check if the volume is already formatted
    if(vcpPoint->signature == VCB_SIGNATURE){
        printf("Volume already formatted");
    } else {
        // Initialize volume control block
        vcpPoint->signature = VCB_SIGNATURE;
        vcpPoint->totalBlockCount = numberOfBlocks;
        vcpPoint->blockSizeBytes = blockSize;
        vcpPoint->freeSpaceMapBlock = initializeFreeSpace(numberOfBlocks, blockSize);
        vcpPoint->rootDirectoryBlock = initDir(50, NULL);
    }

    // Load the root directory
    directoryEntry *root = malloc(blockSize);
    LBAread(root, 1, vcpPoint->rootDirectoryBlock);
    rootDirectory = loadDir(root);
    cwd = rootDirectory;

    // Write the volume control block to disk
    LBAwrite(vcpPoint, 1, 0);
    return 0;
}

// Clean up and exit the file system
void exitFileSystem(){
    free(vcpPoint);           // Free the volume control block
    free(rootDirectory);      // Free the root directory
    if(rootDirectory != cwd){
        free(cwd);            // Free the current working directory if different from root
    }
    writeFreeSpace();         // Write the free space map to disk
    printf("System exiting\n");
    freeFreeSpace();          // Free the free space map
}