/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: FSFunctions.h
*
* Description:: This file holds our Free Space Functions declarations
*
**************************************************************/

#ifndef FSFunctions_H
#define FSFunctions_H

// Structure representing a block of free space
typedef struct space {
    int start;
    int count;
} space;

// Functions to manipulate the free space bitmap
void setBit(int loc);             // Set a bit at the given location
void clearBit(int loc);           // Clear a bit at the given location
int checkBit(int loc);            // Check if a bit at the given location is set
void freeFreeSpace();             // Free the memory allocated for the free space bitmap
void clearBits(int start, int count); // Clear a range of bits starting from the initial location

// Functions to initialize and manage free space
int initializeFreeSpace(int numberOfBlocks, int blockSize); // Initialize the free space bitmap
int loadFreeSpace(int numberOfBlocks, int blockSize);       // Load the free space bitmap from disk
space *allocateBlocks(int requiredBlocks, int minPerExtent); // Allocate blocks of free space

// Function to write the free space bitmap to disk
void writeFreeSpace();
#endif
