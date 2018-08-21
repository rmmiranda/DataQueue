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
*               to a stub filesystem. This file contains implementation of
*               all required FSAL functions which are referenced by the
*               data queue application API.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifdef FSAL_STUB

#include "../../inc/fsal.h"

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
	return FSAL_STATUS_OK;
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
size_t FSAL_ReadFile( FSAL_File_t fsal_handle, uint8_t * buffer, size_t length )
{
	return length;
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
size_t FSAL_WriteFile( FSAL_File_t fsal_handle, uint8_t * buffer, size_t length )
{
	return length;
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
	return FSAL_STATUS_OK;
}


#endif /* FSAL_STUB */
