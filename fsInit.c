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

VolumeControlBlock *vcpPoint;
directoryEntry *rootDirectory;
directoryEntry *cwd;
int BLOCKSIZE;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    VolumeControlBlock *vcpPoint = malloc(blockSize);
    LBAread(vcpPoint, 1, 0);

    if(vcpPoint->signature == VCB_SIGNATURE){
        printf("Volume already formatted");
    }else{
        vcpPoint->signature = VCB_SIGNATURE;
        vcpPoint->totalBlockCount = numberOfBlocks;
        vcpPoint->blockSizeBytes = blockSize;
        vcpPoint->freeSpaceMapBlock = initializeFreeSpace(numberOfBlocks, blockSize);
        vcpPoint->rootDirectoryBlock = initDir(50, NULL);
    }
    directoryEntry *root = malloc(blockSize);
    LBAread(root, 1, vcpPoint->rootDirectoryBlock);
    directoryEntry *rootDirectory = loadDir(root);
    directoryEntry *currentDir = rootDirectory;
    LBAwrite(vcpPoint,1,0);
    return 0;
}

void exitFileSystem(){
    free(vcpPoint);
    free(rootDirectory);
    if(rootDirectory != cwd){
        free(cwd);
    }
    writeFreeSpace();
    printf("System exiting\n");
    freeFreeSpace();
}