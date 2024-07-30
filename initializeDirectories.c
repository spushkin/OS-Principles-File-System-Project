/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: initializeDirectories.c
*
* Description:: This file holds all the functions we need to
* initialize our directories and root directory.
*
**************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "FSFunctions.h"
#include "fsLow.h"
#include "constants.h"
#include "initializeDirectories.h"

#define MAX_SPACES 1024

// Write directory information to disk
int writeToDisk(directoryEntry *directory, space *freeSpace) {
    int size = 0;
    while (freeSpace[size].start != -1 && size < MAX_SPACES) {
        size++;
    }

    // Handle small directories that fit within 4 entries
    if (size <= 4) {
        for (int loop = 0; loop < size; loop++) {
            directory->spaces[loop].count = freeSpace[loop].count;
            directory->spaces[loop].start = freeSpace[loop].start;
        }
        if (size < 4) {
            directory->spaces[size].count = -1;
            directory->spaces[size].start = -1;
        }
    } else {
        // Handle large directories with multiple layers
        int sizeofSecondLayer = BLOCKSIZE / sizeof(space);
        space *NodeofSecondLayer = NULL;
        space *NodeofThirdLayer = NULL;

        int firstLayer = size < 4 ? size : 4;
        int leftover;
        for (int loop = 0; loop < firstLayer; loop++) {
            if (loop == 4 - 1 && size > 4) {
                NodeofSecondLayer = allocateBlocks(1, 1);
                space *secondLayer = malloc(BLOCKSIZE);
                if (secondLayer == NULL || NodeofSecondLayer == NULL) {
                    free(secondLayer);
                    free(NodeofSecondLayer);
                    return 0;
                }
                directory->spaces[loop].start = NodeofSecondLayer->start;
                directory->spaces[loop].count = -2;

                leftover = size - 3;
                int current = leftover < sizeofSecondLayer ? leftover : sizeofSecondLayer;

                for (int inner = 0; inner < current; inner++) {
                    if (inner == sizeofSecondLayer - 1 && leftover > sizeofSecondLayer) {
                        NodeofThirdLayer = allocateBlocks(1, 1);
                        secondLayer[inner].start = NodeofThirdLayer->start;
                        secondLayer[inner].count = -3;
                    } else {
                        secondLayer[inner].start = freeSpace[inner + 3].start;
                        secondLayer[inner].count = freeSpace[inner + 3].count;
                    }
                }
                if (current < sizeofSecondLayer) {
                    secondLayer[leftover].start = -1;
                    secondLayer[leftover].count = -1;
                }
                LBAwrite(secondLayer, 1, NodeofSecondLayer->start);
                free(secondLayer);
                free(NodeofSecondLayer);

                leftover -= current - 1;
            } else {
                directory->spaces[loop].start = freeSpace[loop].start;
                directory->spaces[loop].count = freeSpace[loop].count;
                leftover--;
            }
        }

        // Handle third layer if needed
        if (leftover > 1) {
            int index = 0;
            int *thirdLayer = malloc(BLOCKSIZE);
            if (thirdLayer == NULL) {
                free(NodeofSecondLayer);
                return 0;
            }
            int numOfLoops = 0;

            while (leftover > 0) {
                int freeSpaceIndex = (index * sizeofSecondLayer) + (sizeofSecondLayer - 1) + 3;
                NodeofSecondLayer = allocateBlocks(1, 1);

                thirdLayer[index] = NodeofSecondLayer->start;
                space *secondLayer = malloc(BLOCKSIZE);
                if (secondLayer == NULL || NodeofSecondLayer == NULL) {
                    free(secondLayer);
                    free(NodeofSecondLayer);
                    free(thirdLayer);
                    return 0;
                }
                int check = leftover < sizeofSecondLayer ? leftover : sizeofSecondLayer;

                for (int inner = 0; inner < check; inner++) {
                    if (inner == sizeofSecondLayer - 1 && leftover > 0) {
                        secondLayer[inner].start = NodeofThirdLayer->start;
                        secondLayer[inner].count = -3;
                        numOfLoops++;
                    } else {
                        secondLayer[inner].start = freeSpace[inner + freeSpaceIndex - numOfLoops].start;
                        secondLayer[inner].count = freeSpace[inner + freeSpaceIndex - numOfLoops].count;
                        leftover--;
                    }
                }
                if (check < sizeofSecondLayer) {
                    secondLayer[check].start = -1;
                    secondLayer[check].count = -1;
                }
                index++;
                LBAwrite(secondLayer, 1, NodeofSecondLayer->start);
                free(secondLayer);
                free(NodeofSecondLayer);
            }
            LBAwrite(thirdLayer, 1, NodeofThirdLayer->start);
            free(thirdLayer);
        }
    }
    return 1;
}

// Load space information from the directory
space *loadSpace(directoryEntry *directory) {
    int sizeOfLayer2 = BLOCKSIZE / sizeof(space);
    int sizeOfLayer3 = BLOCKSIZE / sizeof(int);
    int max = 4 + sizeOfLayer2 + sizeOfLayer3 + ((sizeOfLayer3 - 1) * sizeOfLayer2);
    space *sp = malloc(sizeof(space) * max);
    int index = 0;

    // Load first layer spaces
    while (directory->spaces[index].count != -2 && index < 4 && directory->spaces[index].start != -1) {
        sp[index].start = directory->spaces[index].start;
        sp[index].count = directory->spaces[index].count;
        index++;
    }

    // Load second layer spaces if necessary
    if (directory->spaces[index].count == -2) {
        int secondIndex = 0;
        space *secondLayer = malloc(BLOCKSIZE);
        LBAread(secondLayer, 1, directory->spaces[index].start);

        while (secondLayer[secondIndex].count != -3 && secondIndex < sizeOfLayer2 && secondLayer[secondIndex].start != -1) {
            sp[index].start = secondLayer[secondIndex].start;
            sp[index].count = secondLayer[secondIndex].count;
            index++;
            secondIndex++;
        }

        // Load third layer spaces if necessary
        if (secondLayer[secondIndex].count == -3) {
            int thirdIndex = 0;
            int *thirdLayer = malloc(BLOCKSIZE);
            LBAread(thirdLayer, 1, secondLayer[secondIndex].start);

            int loopsNeeded = 1;
            while (loopsNeeded == 1 && thirdIndex < sizeOfLayer3) {
                space *secondLayerTemp = malloc(BLOCKSIZE);
                LBAread(secondLayerTemp, 1, thirdLayer[thirdIndex]);

                for (int loop = 0; loop < sizeOfLayer2 && secondLayerTemp[loop].count != -3 && secondLayerTemp[loop].start != -1; loop++) {
                    sp[index].start = secondLayerTemp[loop].start;
                    sp[index].count = secondLayerTemp[loop].count;
                    index++;
                }
                loopsNeeded = secondLayerTemp[sizeOfLayer2 - 1].count == -3 ? 1 : 0;
                free(secondLayerTemp);
                thirdIndex++;
            }

            free(thirdLayer);
        }
        free(secondLayer);
    }

    // Mark the end of space array
    sp[index].start = -1;
    sp[index].count = -1;

    sp = realloc(sp, (index + 1) * sizeof(space));
    return sp;
}

// Initialize a new directory
int initDir(int directoryEntries, directoryEntry *parent) {
    int sizeOfDirectoryEntry = sizeof(directoryEntry);
    int bytesNeeded = directoryEntries * sizeOfDirectoryEntry;
    int blocks = (bytesNeeded + BLOCKSIZE - 1) / BLOCKSIZE;

    space *sp = allocateBlocks(1, blocks);
    if (sp == NULL) {
        return -1;
    }

    directoryEntry *directory = malloc(bytesNeeded);
    if (directory == NULL) {
        free(sp);
        return -1;
    }

    memset(directory, 0, bytesNeeded);
    int startingBlock = sp[0].start;
    directory[0].starting_block = startingBlock;
    directory[0].file_size_bytes = bytesNeeded;
    directory[0].isDir = 1;
    writeToDisk(&directory[0], sp);

    // Create the parent directory entry
    strcpy(directory[1].file_name, "..");
    if (parent == NULL) {
        directory[1].starting_block = directory[0].starting_block;
        directory[1].file_size_bytes = directory[0].file_size_bytes;
    } else {
        directory[1].starting_block = parent->starting_block;
        directory[1].file_size_bytes = parent->file_size_bytes;
    }
    directory[1].isDir = 1;
    writeToDisk(&directory[1], sp);

    // Write directory entries to disk
    int result = LBAwrite(directory, blocks, startingBlock);
    if (result != blocks) {
        free(directory);
        free(sp);
        return -1;
    }

    free(directory);
    free(sp);
    return startingBlock;
}
