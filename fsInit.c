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

/*void initializeVCB(uint64_t numberOfBlocks, uint64_t blockSize);
void initializeFreeSpace(uint64_t blockSize);
void initializeRootDirectory(uint64_t blockSize);*/

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	VolumeControlBlock *vcb = (VolumeControlBlock *)malloc(blockSize); //Allocate memory for the Volume Control Block.
	//Read first block into memory.
	LBAread(vcb, 1, 0);
	//Check signature to see if block is already formatted.
	if (vcb->signature != VCB_SIGNATURE) {
		free(vcb);
		return -1;
	}

	free(vcb);
	// Indicates no formatting is needed.
	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}

//Initialize the Volume Control Block
void initializeVCB(uint64_t numberOfBlocks, uint64_t blockSize) {
	//Assign the proper values to the first block.
	VolumeControlBlock vcb;
	vcb.signature = VCB_SIGNATURE;
	vcb.blockSizeBytes = blockSize;
	vcb.totalBlockCount = numberOfBlocks;
	vcb.availableBlocks = numberOfBlocks - 6;
	vcb.rootDirectoryBlock = 6;
	vcb.freeSpaceMapBlock = 1;
	//Write it to the first block.
	LBAwrite(&vcb, 1, 0);
}
//Initialize the free space.
void initializeFreeSpace(uint64_t blockSize) {
	//Calculate the size of the free space map.
	uint64_t freeSpaceSize = 5 * blockSize;
	//Allocate memory for the free space map.
	void *freeSpace = malloc(freeSpaceSize);
	//Mark all the blocks as free.
	memset(freeSpace, 0, freeSpaceSize);
	//Mark the first 6 blocks as used for the VCB and free space map.
	for (int i=0; i<6;i++){
		((char *)freeSpace)[i/8] |= (1 << (i%8));
	}
	//Write the free space map to the disk at block 1
	LBAwrite(freeSpace, 5, 1);
	//Free the memory of the free space map.
	free(freeSpace);
}

void initializeRootDirectory(uint64_t blockSize) {

}