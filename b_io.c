/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Yahya Obeid, Siarhei Pushkin, Philip Karnatsevich
* Student IDs:: 922368561, 922907437, 922912455
* GitHub-Name:: yahyaobeid, spushkin, kapitoshcka
* Group-Name:: Team of 3
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int linuxFd;
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();

	if (returnFd < 0) return -1;    // no available FCB

    fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
    if (fcbArray[returnFd].buf == NULL) return -1;  // allocation failed

    fcbArray[returnFd].index = 0;
    fcbArray[returnFd].buflen = 0;
    fcbArray[returnFd].linuxFd = open(filename, flags, PERMISSIONS);
    if (fcbArray[returnFd].linuxFd < 0) {
        free(fcbArray[returnFd].buf);
        fcbArray[returnFd].buf = NULL;
        return -1;
    }				// get our own file descriptor
										// check for error - all used FCB's
	return (returnFd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  // Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS) || (fcbArray[fd].buf == NULL)) {
        return (-1);  // Invalid file descriptor
    }

    int bytesWritten = 0;
    while (count > 0) {
        int spaceInBuffer = B_CHUNK_SIZE - fcbArray[fd].index;
        int bytesToWrite = (spaceInBuffer < count) ? spaceInBuffer : count;

        memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer + bytesWritten, bytesToWrite);
        fcbArray[fd].index += bytesToWrite;
        bytesWritten += bytesToWrite;
        count -= bytesToWrite;

        if (fcbArray[fd].index == B_CHUNK_SIZE) {
            int bytes = write(fcbArray[fd].linuxFd, fcbArray[fd].buf, B_CHUNK_SIZE);
            if (bytes != B_CHUNK_SIZE) return -1;  // Write error
            fcbArray[fd].index = 0;
        }
    }

    return bytesWritten;
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  // Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS) || (fcbArray[fd].buf == NULL)) {
        return (-1);  // Invalid file descriptor
    }

    int bytesRead = 0;
    while (count > 0) {
        if (fcbArray[fd].index == fcbArray[fd].buflen) {
            fcbArray[fd].buflen = read(fcbArray[fd].linuxFd, fcbArray[fd].buf, B_CHUNK_SIZE);
            fcbArray[fd].index = 0;
            if (fcbArray[fd].buflen == 0) break;  // EOF
            if (fcbArray[fd].buflen < 0) return -1;  // Read error
        }

        int bytesToCopy = (fcbArray[fd].buflen - fcbArray[fd].index < count) ? fcbArray[fd].buflen - fcbArray[fd].index : count;
        memcpy(buffer + bytesRead, fcbArray[fd].buf + fcbArray[fd].index, bytesToCopy);
        fcbArray[fd].index += bytesToCopy;
        bytesRead += bytesToCopy;
        count -= bytesToCopy;
    }

    return bytesRead;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
		if (startup == 0) b_init();  // Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS) || (fcbArray[fd].buf == NULL)) {
        return (-1);  // Invalid file descriptor
    }

    // Free the buffer and close the Linux file descriptor
    free(fcbArray[fd].buf);
    fcbArray[fd].buf = NULL;
    close(fcbArray[fd].linuxFd);

    return 0;  // Success
	}
