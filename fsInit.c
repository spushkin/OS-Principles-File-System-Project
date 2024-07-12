#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
    
    // Allocate memory for the Volume Control Block (VCB)
    VolumeControlBlock *vcb = (VolumeControlBlock *)malloc(blockSize);
    if (vcb == NULL) {
        printf("Failed to allocate memory for VCB\n");
        return -1;
    }

    // Read first block into memory
    int readResult = LBAread(vcb, 1, 0);
    if (readResult != 1) {
        printf("Failed to read block 0 into VCB\n");
        free(vcb);
        return -1;
    }

    // Check signature to see if block is already formatted
    if (vcb->signature != VCB_SIGNATURE) {
        printf("Volume not formatted, starting initialization...\n");
        
        // Initialize the VCB
        initializeVCB(numberOfBlocks, blockSize);
        printf("VCB initialized successfully\n");
        
        // Initialize the free space
        initializeFreeSpace(blockSize);
        printf("Free space initialized successfully\n");
        
        // Initialize the root directory
        initializeRootDirectory(blockSize);
        printf("Root directory initialized successfully\n");
        
        // Free the VCB memory and indicate success
        free(vcb);
        return 0;
    } else {
        printf("Volume already formatted\n");
    }

    // Free the VCB memory and indicate no formatting needed
    free(vcb);
    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
}

// Initialize the Volume Control Block
void initializeVCB(uint64_t numberOfBlocks, uint64_t blockSize) {
    VolumeControlBlock vcb;
    vcb.signature = VCB_SIGNATURE;
    vcb.blockSizeBytes = blockSize;
    vcb.totalBlockCount = numberOfBlocks;
    vcb.availableBlocks = numberOfBlocks - 6;
    vcb.rootDirectoryBlock = 6;
    vcb.freeSpaceMapBlock = 1;
    
    // Write it to the first block
    int writeResult = LBAwrite(&vcb, 1, 0);
    if (writeResult != 1) {
        printf("Failed to write VCB to block 0\n");
    }
}

// Initialize the free space
void initializeFreeSpace(uint64_t blockSize) {
    uint64_t freeSpaceSize = 5 * blockSize;
    
    // Allocate memory for the free space map
    void *freeSpace = malloc(freeSpaceSize);
    if (freeSpace == NULL) {
        printf("Failed to allocate memory for free space map\n");
        return;
    }

    // Mark all the blocks as free
    memset(freeSpace, 0, freeSpaceSize);

    // Mark the first 6 blocks as used for the VCB and free space map
    for (int i = 0; i < 6; i++) {
        ((char *)freeSpace)[i / 8] |= (1 << (i % 8));
    }

    // Write the free space map to the disk at block 1
    int writeResult = LBAwrite(freeSpace, 5, 1);
    if (writeResult != 5) {
        printf("Failed to write free space map to disk\n");
    }

    // Free the memory of the free space map
    free(freeSpace);
}

// Initialize root directory
void initializeRootDirectory(uint64_t blockSize) {
    uint64_t rootDirSize = 6 * blockSize;
    
    // Allocate memory for root directory
    void *rootDir = malloc(rootDirSize);
    if (rootDir == NULL) {
        printf("Failed to allocate memory for root directory\n");
        return;
    }

    // Initialize root directory
    memset(rootDir, 0, rootDirSize);

    // Cast directory entries to a directory entry array
    directoryEntry *entries = (directoryEntry *)rootDir;

    // Initialize the "." entry
    strcpy(entries[0].file_name, ".");
    entries[0].creation_timestamp = time(NULL);
    entries[0].modification_timestamp = time(NULL);
    entries[0].last_access_timestamp = time(NULL);
    entries[0].starting_block = 6;
    entries[0].file_size_bytes = rootDirSize;
    entries[0].owner_id = 0;
    entries[0].group_id = 0;
    entries[0].file_attributes = ATTR_DIRECTORY;

    // Initialize the ".." entry
    strcpy(entries[1].file_name, "..");
    entries[1].creation_timestamp = time(NULL);
    entries[1].modification_timestamp = time(NULL);
    entries[1].last_access_timestamp = time(NULL);
    entries[1].starting_block = 6;
    entries[1].file_size_bytes = rootDirSize;
    entries[1].owner_id = 0;
    entries[1].group_id = 0;
    entries[1].file_attributes = ATTR_DIRECTORY;

    // Write root directory to disk
    int writeResult = LBAwrite(rootDir, 6, 6);
    if (writeResult != 6) {
        printf("Failed to write root directory to disk\n");
    }

    // Free the memory
    free(rootDir);
}
