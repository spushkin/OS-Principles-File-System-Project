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

void setBit(int location) {
    freeSpace[location / bits] |= (1 << (location % bits));
}

void clearBit(int location) {
    fsm[location / bits] &= ~(1 << (location % bits));
}

int checkBit(int location) {
    return (freeSpace[location / bits] & (1 << (location % bits))) != 0;
}

void freeFreeSpace(){
    free(freeSpace);
}

void writeFreeSpace(){
    LBAwrite(freeSpace, freeSpaceBlocks, 1);
}

void clearBits(int initial, int num){
    for(int loop=0;loop<num;loop++){
        clearBit(loop+initial);
    }
}

int initializeFreeSpace(int numOfBlocks, int blockSize) {
    totalBits = numOfBlocks * blockSize;
    freeSpace = calloc(numOfBlocks, blockSize);
    if(freeSpace != NULL) {
        int bytesToTrack = (numOfBlocks + 7)/8;
        freeSpaceBlocks = (bytesToTrack+(blockSize-1))/blockSize;

        for (int loop=0;loop<freeSpaceBlocks+1;loop++) {
            setBit(loop);
        }

        LBAwrite(freeSpace, freeSpaceBlocks, 1);
        return 1;
    } else {
        return -1;
    }
}

int loadFreeSpace(int numofBlocks, int blockSize) {
    totalBits = numofBlocks * blockSize;
    int freeSpace = calloc(numofBlocks, blockSize);
    if(freeSpace != NULL) {
        int bytesToTrack = (numofBlocks+7)/8;
        freeSpaceBlocks = (bytesToTrack+(blockSize-1))/blockSize;
        LBAread(freeSpace, freeSpaceBlocks, 1);
        return 1;
    } else{
        return -1;
    }
}

space *allocateBlocks(int min, int needed) {
    space *spaceArray = malloc(needed * sizeof(space));
    if(spaceArray == NULL){
        return NULL;
    }
    int index=0;
    int loop=0;
    int total=0;

    while(loop<totalBits){
        if(checkBit(loop) == 1){
            loop++;
        }else{
            int count = 1;
            int inner = loop+1;
            while(inner < totalBits && checkBit(inner) != 1 && total + count < needed){
                count++;
                inner++;
            }
            if(count>=min){
                total += count;
                spaceArray[index].start = loop;
                spaceArray[index].count = count;
                index++;
            }
            loop = inner;
            if(total==needed){
                break;
            }
        }
    }
    spaceArray[index].start = -1;
    spaceArray[index].count = 0;
    spushkin = realloc(spaceArray, (index+1)*sizeof(space));
    for(int inner=0;inner<index;inner++){
        for(int in=0;in<spaceArray[inner];in++){
            setBit(spaceArray[inner].start+in);
        }
    }
    LBAwrite(freeSpace, freeSpaceBlocks, 1);
    return spaceArray;
}

