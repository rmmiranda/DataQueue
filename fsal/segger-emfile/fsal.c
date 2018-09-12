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
*               to Segger's emFile filesystem. This file implements all
*               required FSAL functions which are referenced by the data
*               data queue application API.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifdef FSAL_SEGGER_EMFILE

#include "../../inc/fsal.h"

#include <stdio.h>
#include <string.h>

#include "SEGGER.h"
#include "FS.h"

static char current_working_dir[20] = {0};

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
	/* initiliaze emFile filesystem */
	FS_Init();

	/* set current working directory to "" */
	//snprintf( current_working_dir, sizeof(current_working_dir), "" );
	memset( current_working_dir, 0, sizeof(current_working_dir) );
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
	char dir_path_name[20] = {0};

	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	snprintf( dir_path_name, sizeof(dir_path_name), "%s\\%s", current_working_dir, dir_name );

	/* create a directory (assumed with read/write access) */
	if ( FS_MkDir(dir_path_name) != 0 ) {
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
	FS_FIND_DATA fd;
	char dir_path_name[20] = {0};
	char file_name[20] = {0};
	int fsal_status = FSAL_ERROR_DIR_ACCESS;

	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	if ( strcmp(dir_name, "../") == 0 ) {

		//snprintf( current_working_dir, sizeof(current_working_dir), "" );
		//current_working_dir[ strlen("") ] = '\0';
		memset( current_working_dir, 0, sizeof(current_working_dir) );

	} else {

		snprintf( dir_path_name, sizeof(dir_path_name), "%s\\%s", current_working_dir, dir_name );

		if ( FS_FindFirstFile(&fd, dir_path_name, file_name, sizeof(file_name)) == 0 ) {

			snprintf( current_working_dir, sizeof(current_working_dir), "%s", dir_path_name );
			current_working_dir[ strlen(dir_path_name) ] = '\0';

			fsal_status = FSAL_STATUS_OK;
		}

		FS_FindClose( &fd );
	}

	return fsal_status;
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
	char dir_path_name[20] = {0};

	/* sanity checks */
	if ( dir_name == NULL ) {
		return FSAL_ERROR_DIR_ACCESS;
	}

	snprintf( dir_path_name, sizeof(dir_path_name), "%s\\%s", current_working_dir, dir_name );

	/* delete the specified directory up to 2 levels deep */
	if ( FS_DeleteDir(dir_path_name, 2) != 0 ) {
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
	/* not implemented */
	return FSAL_ERROR_DIR_ACCESS;
}

/** @brief Lists a file.
 *
 *  This function lists a file in the current directory.
 *
 *  @param[in] file_name - the name of the file to list
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                FSAL_STATUS_OK
 *                FSAL_ERROR_FILE_ACCESS
 *
 */
int FSAL_ListFile( char * file_name )
{
	FS_FILE * fd;
	char file_path[20] = {0};

	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	snprintf( file_path, sizeof(file_path), "%s\\%s", current_working_dir, file_name);

	/* open the specified file */
	fd = FS_FOpen( file_path, "rb" );
	if ( fd == 0 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	/* close the file */
	FS_FClose( fd );

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
	FS_FILE * fd;
	char file_path[20] = {0};

	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	snprintf( file_path, sizeof(file_path), "%s\\%s", current_working_dir, file_name);

	if (flags & FSAL_FLAGS_CREATE ) {
		/* create the specified file */
		fd = FS_FOpen( file_path, "wb" );
		if ( fd == 0 ) {
			return FSAL_ERROR_FILE_ACCESS;
		}
		FS_FClose( fd );
	}

	if (flags & FSAL_FLAGS_READ_ONLY ) {

		/* open the specified file for reading only */
		fd = FS_FOpen( file_path, "rb" );
		if ( fd == 0 ) {
			return FSAL_ERROR_FILE_ACCESS;
		}

	} else if (flags & FSAL_FLAGS_APPEND_ONLY ) {

		/* open the specified file for append only */
		fd = FS_FOpen( file_path, "ab" );
		if ( fd == 0 ) {
			return FSAL_ERROR_FILE_ACCESS;
		}

	} else if (flags & FSAL_FLAGS_WRITE_ONLY ) {

		/* open the specified file for writing only */
		fd = FS_FOpen( file_path, "wb" );
		if ( fd == 0 ) {
			return FSAL_ERROR_FILE_ACCESS;
		}

	} else {

		/* open the specified file for reading and writing */
		fd = FS_FOpen( file_path, "r+b" );
		if ( fd == 0 ) {
			return FSAL_ERROR_FILE_ACCESS;
		}

	}

	/* copy the handle to the output parameter */
	*fsal_handle = (FSAL_File_t)((intptr_t)fd);

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
	FS_FILE * fd = (FS_FILE *)((intptr_t)fsal_handle);

	/* sanity check */
	if ( fd == 0 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	FS_FClose( fd );

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
	FS_FILE * fd = (FS_FILE *)((intptr_t)fsal_handle);
	ssize_t actual_length = 0;

	/* sanity checks */
	if ( (fd == 0) || (buffer == NULL) ) {
		return -FSAL_ERROR_FILE_ACCESS;
	}

	/* proceed operation with nonzero length */
	if ( length ) {
		actual_length = FS_FRead( buffer, 1, length, fd );
		if ( (actual_length != length) &&
			 (FS_FError(fd) != FS_ERR_OK) ) {

			FS_ClearErr( fd );
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
	FS_FILE * fd = (FS_FILE *)((intptr_t)fsal_handle);
	ssize_t actual_length = 0;

	/* sanity checks */
	if ( (fd == 0) || (buffer == NULL) ) {
		return -FSAL_ERROR_FILE_ACCESS;
	}

	/* proceed operation with nonzero length */
	if ( length ) {
		actual_length = FS_FWrite( buffer, 1, length, fd );
		if ( (actual_length != length) &&
			 (FS_FError(fd) != FS_ERR_OK) ) {

			FS_ClearErr( fd );
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
	char file_path[20] = {0};

	/* sanity checks */
	if ( file_name == NULL ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	snprintf( file_path, sizeof(file_path), "%s\\%s", current_working_dir, file_name);

	/* delete the specified file */
	if ( FS_Remove(file_path) != 0 ) {
		return FSAL_ERROR_FILE_ACCESS;
	}

	return FSAL_STATUS_OK;
}


#endif /* FSAL_SEGGER_EMFILE */
