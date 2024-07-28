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

typedef struct space{
    int start;
    int count;
}space;

void setBit(int loc);
void clearBit(int loc);
int checkBit(int loc);
int initializeFreeSpace(int numberOfBlocks, int blockSize);
int loadFreeSpace(int numberOfBlocks, int blockSize);
space *allocateBlocks(int requiredBlocks, int minPerExtent);
void freeFreeSpace();
void clearBits(int start, int count);
void writeFreeSpace();
#endif
