/**********************************************************************
* Filename:     fsal.c
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 20, 2018
*
* Description:  This is the Filesystem Abstraction Layer or FSAL specific
*               to the Linux EXT4 filesystem. This file implements all
*               required FSAL functions which are referenced by the data
*               data queue application API.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifdef FSAL_LINUX_EXT4

#include "../../inc/fsal.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

/** @brief Initializes the filesystem for use
 *
 *  This function performs the required filesystem-specific
 *  initialization sequence.
 *
 *  @param none
 *
 *  @return none
 */

void FSAL_Init( void )
{
	/* nothing to do */
	return;
}

/** @brief Creates a directory.
 *
 *  This function creates a directory into the filesystem in
 *  relative to the current directory.
 *
 *  @param[in] dir_name - the name of the directory to create
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_DIR_ACCESS
 *
 */
int FSAL_MakeDirectory( char * dir_name )
{
	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* create a directory with read/write access */
	if ( mkdir(dir_name, 0777) == -1 ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	return FSAL_STATUS_OK;
}

/** @brief Changes working directory.
 *
 *  This function changes the current working directory to another
 *  directory.
 *
 *  @param[in] dir_name - the name of the directory to change in
 *                        relative to the current directory
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_DIR_ACCESS
 *
 */
int FSAL_ChangeDirectory( char * dir_name )
{
	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* change working directory as specified */
	if ( chdir(dir_name) == -1 ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	return FSAL_STATUS_OK;
}

/** @brief Removes a directory.
 *
 *  This function removes a directory from the filesystem in
 *  relative to the current directory.
 *
 *  @param[in] dir_name - the name of the directory to remove
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_DIR_ACCESS
 *
 */
int FSAL_RemoveDirectory( char * dir_name )
{
	char file_name[32] = {0};
	struct dirent * de;
	DIR * dr;

	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* open the specified directory */
	dr = opendir( dir_name );
	if ( dr == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* list all files in the directory */
	de = readdir( dr );
	while ( de != NULL ) {
		if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0)) {
			sprintf(file_name, "%s/%s", dir_name, de->d_name);
			unlink(file_name);
		}
		de = readdir( dr );
	}

	closedir( dr );

	/* delete the specified directory */
	if ( rmdir(dir_name) == -1 ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	return FSAL_STATUS_OK;
}

/** @brief Lists all directory entries.
 *
 *  This function display a listing of all entries of the current
 *  directory.
 *
 *  @param[in] dir_name - the name of the directory to list entries
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_DIR_ACCESS
 *
 */
int FSAL_ListDirectory( char * dir_name )
{
	struct dirent * de;
	DIR * dr;

	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* open the specified directory */
	dr = opendir( dir_name );
	if ( dr == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	/* list all files in the directory */
	de = readdir( dr );
	while ( de != NULL ) {
		printf( "%s\n", de->d_name );
	}

	closedir( dr );

	return FSAL_STATUS_OK;
}

/** @brief Lists a file and retrieves its size.
 *
 *  This function lists a file in the current directory and
 *  retrieves the file size.
 *
 *  @param[in] file_name - the name of the file to list
 *
 *  @param[out] file_size - the current size of the file
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_FILE_ACCESS
 *
 */
int FSAL_ListFile( char * file_name, size_t * file_size )
{
	int fd;
	struct stat st;
	int status;

	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* open the specified file */
	fd = open( file_name, O_RDWR, 0 );
	if ( fd == -1 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* retrieve the file status */
	status = fstat( fd, &st );

	/* close the file */
	close( fd );

	/* check for error condition */
	if ( status == -1 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* copy the file size */
	*file_size = st.st_size;

	return FSAL_STATUS_OK;
}

/** @brief Opens a file.
 *
 *  This function opens a file in the filesystem in relative to
 *  the current directory.
 *
 *  @param[in] file_name - the name of the file to open
 *
 *  @param[in] flags - the access mode the file when opened
 *
 *  @param[out] fsal_handle - the handle of the opened file
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_FILE_ACCESS
 *
 */
int FSAL_OpenFile( char * file_name, int flags, FSAL_File_t * fsal_handle )
{
	int fd;
	int open_flags = 0;

	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	if (flags & FSAL_FLAGS_CREATE )
		open_flags |= O_CREAT;

	if (flags & FSAL_FLAGS_READ_ONLY )
		open_flags |= O_RDONLY;
	else if (flags & FSAL_FLAGS_WRITE_ONLY )
		open_flags |= O_WRONLY;
	else
		open_flags |= O_RDWR;

	/* open the specified file */
	fd = open( file_name, open_flags, 0777 );
	if ( fd == -1 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* copy the handle to the output parameter */
	*fsal_handle = (FSAL_File_t) fd;

	return FSAL_STATUS_OK;
}

/** @brief Closes a file.
 *
 *  This function closes a file as specified by a specific file
 *  handle.
 *
 *  @param[in] fsal_handle - the handle associated to the file
 *                           to close
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_FILE_ACCESS
 *
 */
int FSAL_CloseFile( FSAL_File_t fsal_handle )
{
	int fd = (int) fsal_handle;

	/* sanity check */
	if ( fd == -1 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* close the file */
	close( fd );

	return FSAL_STATUS_OK;
}

/** @brief Reads from a file.
 *
 *  This function reads data from a file as specified by a specific
 *  file handle.
 *
 *  @param[in] fsal_handle - the handle associated to the file
 *                           to read
 *
 *  @param[in] buffer - the location reference where the contents
 *                      of the file will be stored
 *
 *  @param[in] length - the maximum bytes the location reference
                        can store
 *
 *  @return int - the number of bytes actually read or a negative
 *                error code if the operation failed
 *
 */
ssize_t FSAL_ReadFile( FSAL_File_t fsal_handle, uint8_t * buffer, size_t length )
{
	int fd = (int) fsal_handle;
	ssize_t actual_length = 0;

	/* sanity checks */
	if ( (fd == -1) || (buffer == NULL) ) {
		return -FSAL_ERROR_FILE_ACCESS;
	}

	/* proceed operation with nonzero length */
	if ( length ) {
		actual_length = read( fd, buffer, length );
		if ( actual_length == -1 ) {
			return -FSAL_ERROR_FILE_ACCESS;
		}
	}

	return actual_length;
}

/** @brief Writes to a file.
 *
 *  This function writes data to a file as specified by a specific
 *  file handle.
 *
 *  @param[in] fsal_handle - the handle associated to the file
 *                           to write
 *
 *  @param[in] buffer - the location reference of the data to be
 *                      written to the file
 *
 *  @param[in] length - the number of bytes to write to the file
 *
 *  @return int - the number of bytes actually written or a negative
 *                error code if the operation failed
 *
 */
ssize_t FSAL_WriteFile( FSAL_File_t fsal_handle, uint8_t * buffer, size_t length )
{
	int fd = (int) fsal_handle;
	ssize_t actual_length = 0;

	/* sanity checks */
	if ( (fd == -1) || (buffer == NULL) ) {
		return -FSAL_ERROR_FILE_ACCESS;
	}

	/* proceed operation with nonzero length */
	if ( length ) {
		actual_length = write( fd, buffer, length );
		if ( actual_length == -1 ) {
			return -FSAL_ERROR_FILE_ACCESS;
		}
	}

	return actual_length;
}

/** @brief Deletes a file.
 *
 *  This function deletes a file from the filesystem in
 *  relative to the current directory.
 *
 *  @param[in] file_name - the name of the file to remove
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_DIR_ACCESS
 *
 */
int FSAL_DeleteFile( char * file_name )
{
	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* delete the specified file */
	if ( unlink(file_name) == -1 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	return FSAL_STATUS_OK;
}


#endif /* FSAL_LINUX_EXT4 */
