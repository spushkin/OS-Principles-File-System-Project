/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: FSFunctions.c
*
* Description:: This file will hold all of the free space functions
* that we need for the filesystem.
*
**************************************************************/
#include "FSFunctions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "fsLow.h"
#include "constants.h"
#define bits 8

int freeSpaceBlocks;
int totalBits;
char *freeSpace;

// Set a bit at the given location
void setBit(int location) {
    freeSpace[location / bits] |= (1 << (location % bits));
}

// Clear a bit at the given location
void clearBit(int location) {
    freeSpace[location / bits] &= ~(1 << (location % bits));
}

// Check if a bit at the given location is set
int checkBit(int location) {
    return (freeSpace[location / bits] & (1 << (location % bits))) != 0;
}

// Free the memory allocated for the free space bitmap
void freeFreeSpace() {
    free(freeSpace);
}

// Write the free space bitmap to disk
void writeFreeSpace() {
    LBAwrite(freeSpace, freeSpaceBlocks, 1);
}

// Clear a range of bits starting from the initial location
void clearBits(int initial, int num) {
    for (int loop = 0; loop < num; loop++) {
        clearBit(loop + initial);
    }
}

// Initialize the free space bitmap
int initializeFreeSpace(int numOfBlocks, int blockSize) {
    totalBits = numOfBlocks * blockSize;
    // Allocate memory for the bitmap
    freeSpace = calloc(numOfBlocks, blockSize);
    if (freeSpace != NULL) {
        int bytesToTrack = (numOfBlocks + 7) / 8;
         // Calculate the number of blocks needed
        freeSpaceBlocks = (bytesToTrack + (blockSize - 1)) / blockSize;

        // Mark the blocks used for the bitmap as occupied
        for (int loop = 0; loop < freeSpaceBlocks + 1; loop++) {
            setBit(loop);
        }
        // Write the bitmap to disk
        LBAwrite(freeSpace, freeSpaceBlocks, 1);
        return 1;
    } else {
        return -1; // Return -1 if memory allocation fails
    }
}

// Load the free space bitmap from disk
int loadFreeSpace(int numofBlocks, int blockSize) {
    totalBits = numofBlocks * blockSize;
    // Allocate memory for the bitmap
    freeSpace = calloc(numofBlocks, blockSize);
    if (freeSpace != NULL) {
        int bytesToTrack = (numofBlocks + 7) / 8;
        // Calculate the number of blocks needed
        freeSpaceBlocks = (bytesToTrack + (blockSize - 1)) / blockSize;
        // Read the bitmap from disk
        LBAread(freeSpace, freeSpaceBlocks, 1); 
        return 1;
    } else {
        return -1; // Return -1 if memory allocation fails
    }
}

// Allocate blocks of free space
space *allocateBlocks(int min, int needed) {
    printf("allocateBlocks: Allocating %d blocks with a minimum of %d\n", needed, min);

    space *spaceArray = malloc(needed * sizeof(space));
    if (spaceArray == NULL) {
        printf("Error: Memory allocation for spaceArray failed\n");
        return NULL;
    }

    int index = 0;
    int loop = 0;
    int total = 0;

    // Loop through the bitmap to find free blocks
    while (loop < totalBits) {
        if (checkBit(loop) == 1) {
            loop++; // Skip if the bit is set
        } else {
            int count = 1; // Count of contiguous free blocks
            int inner = loop + 1;
            while (inner < totalBits && checkBit(inner) != 1 && total + count < needed) {
                count++;
                inner++;
            }
            if (count >= min) {
                total += count;
                spaceArray[index].start = loop;
                spaceArray[index].count = count;
                index++;
            }
            loop = inner; // Move to the next range
            if (total == needed) {
                break; // Break if the needed number of blocks is allocated
            }
        }
    }

    spaceArray[index].start = -1;
    spaceArray[index].count = 0;
    spaceArray = realloc(spaceArray, (index + 1) * sizeof(space));
    if (spaceArray == NULL) {
        printf("Error: Memory reallocation for spaceArray failed\n");
        return NULL;
    }

    for (int inner = 0; inner < index; inner++) {
        for (int in = 0; in < spaceArray[inner].count; in++) {
            setBit(spaceArray[inner].start + in);
        }
    }
    // Write the updated bitmap to disk
    LBAwrite(freeSpace, freeSpaceBlocks, 1); 
    return spaceArray;
}