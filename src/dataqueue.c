/**********************************************************************
* Filename:     dataqueue.c
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 10, 2018
*
* Description:  This is the data queue application programming interface
*               (API) which consists of function definitions of a set of
*               entry point functions for application code needing to
*               deploy a file-based data queue in their system.
*
* History
* 10-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#include "psl.h"
#include "fsal.h"

#include "dataqueue.h"

/**
 * List of currently opened data queues
 */
static DataQ_File_t DataQ_FileHandleList[ DATA_QUEUE_FILE_HANDLE_LIST_MAX ] = { { { DATA_QUEUE_FILE_HANDLE_INVALID } } };

/** @brief Performs initialization of the first-in, first-out (FIFO)
 *         data queue engine
 *
 *  This function initializes the data queue engine including the
 *  underlying filesystem abstraction layer.
 *
 *  @param none
 *
 *  @return none
 */
void DataQ_InitEngine( void )
{
	/* call the underlying filesystem abstraction layer */
	FSAL_Init();
}

/** @brief Creates a first-in, first-out (FIFO) data queue.
 *
 *  This function creates a specific type of data queue where first
 *  data in is also the first data out. In nominal scenario, the
 *  operation sets up the associated metadata and lookup table with
 *  an empty (although pre-allocated) data queue.
 *
 *  @param[in] fifo_name - the reference by which the data queue is
 *                         identified
 *
 *  @param[in] max_entries - the maximum allowable entries for the
 *                           data queue to be created
 *
 *  @param[in] max_entry_size - the maximum allowable size of one
 *                              data queue entry
 *
 *  @param[in] flags - the bit mask information of the characteristics
 *                     specific to the data queue:
 *
 *                     FLAGS_MESSAGE_LOG
 *                     FLAGS_RANDOM_ACCESS
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_QUEUE_EXISTS
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoCreate( char * fifo_name, uint8_t max_entries, size_t max_entry_size, uint16_t flags )
{
	DataQ_Hdr_t fifo_hdr = {
		.size = 0,
		.max_entry_size = max_entry_size,
		.max_entries = max_entries,
		.num_of_entries = 0,
		.head_lut_offs = 0,
		.tail_lut_offs = 0,
		.seek_lut_offs = 0,
		.reference_count = 0,
		.flags = flags,
	};
	DataQ_LUT_Entry_t fifo_lut_entry;
	FSAL_File_t fsal_handle = -1;
	int fsal_flags = FSAL_FLAGS_CREATE | FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_WRITE;
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( fifo_name == (char *) 0 ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check mandatory arguments for invalid values */
	if ( ( max_entries == 0 ) || ( max_entry_size == 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check if data queue already exists */
	if ( FSAL_ChangeDirectory(fifo_name) == FSAL_STATUS_OK ) {

		/* data queue exists so return back and exit */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_EXISTS;
	}

	/* create (and change to) the directory associated with the fifo */
	if ( (FSAL_MakeDirectory(fifo_name) == FSAL_ERROR_DIR_ACCESS) ||
		 (FSAL_ChangeDirectory(fifo_name) == FSAL_ERROR_DIR_ACCESS) ) {

		/* file system access error */
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* create the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", fsal_flags, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* delete the previously created directory */
		FSAL_ChangeDirectory( "../" );
		FSAL_RemoveDirectory( fifo_name );

		/* file system access error */
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* create the lut file associated with the fifo */
	if ( FSAL_OpenFile(".lut", fsal_flags, &fsal_handle) == FSAL_STATUS_OK ) {

		/* invalidate a generic LUT entry */
		PSL_memset( &fifo_lut_entry, 0, sizeof(DataQ_LUT_Entry_t) );

		/* pre-populate the lut with max number of entries */
		for ( index = 0; index < max_entries; index++ ) {

			/* write one lut entry at a time */
			if ( FSAL_WriteFile(
					fsal_handle,
					(uint8_t *)&fifo_lut_entry,
					sizeof(DataQ_LUT_Entry_t)) < 0 ) {

				/* at least one write operation failed */
				FSAL_CloseFile( fsal_handle );

				/* delete the previously created directory */
				FSAL_ChangeDirectory( "../" );
				FSAL_RemoveDirectory( fifo_name );

				/* file system access error */
				return CODE_ERROR_FS_ACCESS_FAIL;
			}
		}

		/* lut file fully initialized */
		FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );

		/* operation succeeded */
		return CODE_STATUS_OK;
	}

	/* delete the previously created file and directory */
	FSAL_ChangeDirectory( "../" );
	FSAL_RemoveDirectory( fifo_name );

	/* file system access error */
	return CODE_ERROR_FS_ACCESS_FAIL;
}

/** @brief Destroys a first-in, first-out (FIFO) data queue.
 *
 *  This function destroys a specified data queue and frees up all
 *  allocated resources associated with it. If the data queue does
 *  not exist, it does not do anything and considers a successful
 *  operation. If the data queue is currently opened at least by
 *  one process, then operation will fail and the error code is set
 *  to busy to indicate a retry can be done.
 *
 *  @param[in] fifo_name - the reference of the data queue to be
 *                         destroyed
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_QUEUE_IS_BUSY
 *
 */
int DataQ_FifoDestroy( char * fifo_name )
{
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( fifo_name == (char *) 0 ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_name ) == FSAL_ERROR_DIR_ACCESS ) {
		/* data queue does not exist so do nothing */
		return CODE_STATUS_OK;
	}

	/* check if fifo is opened by this process */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX; index++ ) {

		/* locate the fifo name in the currently opened queue list */
		if( (DataQ_FileHandleList[index].handle != DATA_QUEUE_FILE_HANDLE_INVALID) &&
			(strcmp(fifo_name, DataQ_FileHandleList[index].name) == 0) ) {

			/* fifo is still opened and maybe busy */
			FSAL_ChangeDirectory( "../" );
			return CODE_ERROR_QUEUE_IS_BUSY;
		}
	}

	/* check if fifo is opened by another process by listing lock files */
	if ( (FSAL_ListFile( ".rolock" ) == FSAL_STATUS_OK) ||
	     (FSAL_ListFile( ".wolock" ) == FSAL_STATUS_OK) ||
		 (FSAL_ListFile( ".rwlock" ) == FSAL_STATUS_OK) ) {

		/* fifo is still opened by another and maybe busy */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_BUSY;

	}

	/* delete the folder (all files within) associated with data queue */
	FSAL_ChangeDirectory( "../" );
	FSAL_RemoveDirectory( fifo_name );

	/* operation succeeded */
	return CODE_STATUS_OK;
}

/** @brief Opens a first-in, first-out (FIFO) data queue for
 *         access.
 *
 *  This function opens a specified data queue for access as
 *  specified by the type of access (read-only, write-only or
 *  read and write) and mode of access (normal or unpacked, or
 *  binary or packed). If the operation succeeded, the output
 *  parameter is updated with the fifo handle.
 *
 *  If the data queue is already opened by this process, it
 *  checks if the current access type and mode match the new
 *  one - if it does, the function updates the output parameter
 *  with the already opened fifo handle and considers a successful
 *  operation; otherwise, the function considers a failed operation.
 *
 *  If the data queue is already opened by another process, the
 *  function checks if the fifo is opened for read-only access.
 *  If it does, the passed access parameter should also be read
 *  only so the operation can proceed. In all other conditions,
 *  the function considers a failed operation and the error code
 *  is set to busy to indicate a retry can be done.
 *
 *  In nominal case, the function will create lock files so that
 *  concurrent access to the data queue by at least 2 separate
 *  processes are synchronized and would not result to data queue
 *  corruption. If the access type is read-only, the associated
 *  lock file is either 1) created (if no other process has opened
 *  the data queue) and write a byte value representing the number
 *  of users the data queue have (in this case, it will be 1); or
 *  2) opened (if there is at least another process that has the
 *  data queue opened) and had the current byte value incremented
 *  by one. For all other access type (write-only and read/write),
 *  the associated lock files are created empty because only one
 *  user is allowed for such access types.
 *
 *
 *  @param[in] fifo_name - the reference of the data queue to be
 *                         accessed
 *
 *  @param[in] access - the access type to be performed on the data
 *                      data queue:
 *
 *						ACCESS_TYPE_READ_ONLY
 *						ACCESS_TYPE_WRITE_ONLY
 *						ACCESS_TYPE_READ_WRITE
 *
 *  @param[in] mode - the access mode to be considered on the data
 *                    data queue:
 *
 *					  ACCESS_MODE_UNPACKED
 *					  ACCESS_MODE_BINARY_PACKED
 *
 *  @param[out] fifo_handle - the reference where the pointer to the
 *                            fifo handle is copied.
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_OPENED
 *                CODE_ERROR_QUEUE_IS_BUSY
 *                CODE_ERROR_FS_ACCESS_FAIL
 *                CODE_ERROR_HANDLE_NOT_AVAIL
 *
 */
int DataQ_FifoOpen( char * fifo_name, int access, int mode, DataQ_File_t ** fifo_handle )
{
	FSAL_File_t fsal_handle = -1;
	int fsal_flags = FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_WRITE;
	int fsal_status = FSAL_ERROR_FILE_ACCESS;
	uint8_t users = 1;
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( ( fifo_name == (char *) 0 ) || ( fifo_handle == (DataQ_File_t **) 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check mandatory arguments for invalid values */
	if ( ( access >= ACCESS_TYPE_MAX ) || ( mode >= ACCESS_MODE_MAX ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened by this process */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX; index++ ) {

		/* locate the fifo name in the currently opened queue list */
		if( (DataQ_FileHandleList[index].handle != DATA_QUEUE_FILE_HANDLE_INVALID) &&
			(strcmp(fifo_name, DataQ_FileHandleList[index].name) == 0) ) {

			/* found an active fifo handle */
			if ( (access == DataQ_FileHandleList[index].access) &&
				 (mode == DataQ_FileHandleList[index].mode) ) {

				/* fifo is already opened with the correct access parameters */
				PSL_memcpy( *fifo_handle, &DataQ_FileHandleList[index], sizeof(DataQ_File_t) );
				 FSAL_ChangeDirectory( "../" );
				return CODE_STATUS_OK;
			}

			/* fifo is already opened but with different access parameters */
			FSAL_ChangeDirectory( "../" );
			return CODE_ERROR_QUEUE_OPENED;
		}
	}

	/* check if fifo is already opened by another process by listing lock files */
	if ( (FSAL_ListFile( ".wolock" ) == FSAL_STATUS_OK) ||
		 (FSAL_ListFile( ".rwlock" ) == FSAL_STATUS_OK) ) {

		/* fifo is already opened by another process with at least write access */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_BUSY;

	} else if ( (FSAL_ListFile( ".rolock" ) == FSAL_STATUS_OK) &&
			    (access != ACCESS_TYPE_READ_ONLY) ) {

		/**
		 *  fifo is already opened by another process with read-only
		 * access but write access is required
		 */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_BUSY;

	}

	/* create the appropriate lock file */
	if ( access == ACCESS_TYPE_READ_ONLY ) {

		/* determine if another process is currently using the data queue */
		if ( FSAL_ListFile( ".rolock" ) == FSAL_ERROR_FILE_ACCESS ) {

			/* this process is first user so create the lock file */
			if ( FSAL_OpenFile(".rolock", fsal_flags | FSAL_FLAGS_CREATE, &fsal_handle) == FSAL_STATUS_OK ) {

				/* initialize lock file with default user count (set to 1) */
				if ( FSAL_WriteFile(fsal_handle, (uint8_t *)&users, sizeof(users)) == sizeof(users) )
					fsal_status = FSAL_STATUS_OK;

				/* done creating the lock file */
				FSAL_CloseFile( fsal_handle );
			}

		} else {

			/* another process is first user so modify the lock file instead */
			if ( FSAL_OpenFile(".rolock", fsal_flags, &fsal_handle) == FSAL_STATUS_OK ) {

				/* read the current number of users from the lock file */
				if ( FSAL_ReadFile(fsal_handle, (uint8_t *)&users, sizeof(users)) == sizeof(users) ) {

					/* increment the number of users */
					users++;

					/* write the updated current number of users to the lock file */
					if ( FSAL_WriteFile(fsal_handle, (uint8_t *)&users, sizeof(users)) == sizeof(users) )
						fsal_status = FSAL_STATUS_OK;
				}

				/* done modifying the lock file */
				FSAL_CloseFile( fsal_handle );
			}
		}

	} else if ( access == ACCESS_TYPE_WRITE_ONLY ) {

		/* create an empty write-only lock file (user count assumed only one) */
		fsal_status = FSAL_OpenFile( ".wolock", fsal_flags | FSAL_FLAGS_CREATE, &fsal_handle );
		if ( fsal_status == FSAL_STATUS_OK ) {
			FSAL_CloseFile( fsal_handle );
		}

	} else {

		/* create an empty read/write lock file (user count assumed only one) */
		fsal_status = FSAL_OpenFile( ".rwlock", fsal_flags | FSAL_FLAGS_CREATE, &fsal_handle );
		if ( fsal_status == FSAL_STATUS_OK ) {
			FSAL_CloseFile( fsal_handle );
		}

	}

	/* check if appropriate lock file is added */
	if ( fsal_status != FSAL_STATUS_OK ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* get a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX; index++ ) {

		/* locate an available fifo handle from the opened queue list */
		if( DataQ_FileHandleList[index].handle == DATA_QUEUE_FILE_HANDLE_INVALID ) {

			/* found an available fifo handle */
			DataQ_FileHandleList[index].handle = index;
			DataQ_FileHandleList[index].access = access;
			DataQ_FileHandleList[index].mode = mode;
			strncpy( DataQ_FileHandleList[index].name,
					fifo_name,
					sizeof(DataQ_FileHandleList[index].name) );

			/* store the fifo handle pointer to the output parameter */
			*fifo_handle = &DataQ_FileHandleList[index];

			/* operation succeeded */
			FSAL_ChangeDirectory( "../" );
			return CODE_STATUS_OK;
		}
	}

	/* no more available handles */
	FSAL_ChangeDirectory( "../" );
	return CODE_ERROR_HANDLE_NOT_AVAIL;
}


/** @brief Closes a first-in, first-out (FIFO) data queue for
 *         access.
 *
 *  This function closes a specified data queue previously opened
 *  for access. If the data queue is already closed, it does not
 *  do anything and considers a successful operation. If the data
 *  queue is missing, it returns an error code.
 *
 *  In nominal case, the function checks for the lock files as
 *  indications of data queues opened by other processes. If
 *  the lock file associated to a fifo that has been opened
 *  with read-only exists, this function will read a byte value
 *  (corresponds to the number of current user) and decrement
 *  it by one. If the result is a zero value, the lock file
 *  is deleted. For all other lock files (write-only or read
 *  write access types), the lock file is deleted immediately
 *  as such access types only allows one user at a time.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           closed
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoClose( DataQ_File_t * fifo_handle )
{
	FSAL_File_t fsal_handle = -1;
	int fsal_flags = FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_WRITE;
	int fsal_status = FSAL_STATUS_OK;
	uint8_t users;
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( fifo_handle == (DataQ_File_t *) 0 ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is actually opened by this process */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if( fifo_handle == &DataQ_FileHandleList[index] ) {

			/* locate the read-only lock file associated with the data queue */
			if ( FSAL_OpenFile(".rolock", fsal_flags, &fsal_handle) == FSAL_STATUS_OK ) {

				/* read the current number of users from the lock file */
				if ( FSAL_ReadFile(fsal_handle, (uint8_t *)&users, sizeof(users)) == sizeof(users) ) {

					/* decrement the number of users */
					users--;

					/* write the updated current number of users to the lock file */
					if ( FSAL_WriteFile(fsal_handle, (uint8_t *)&users, sizeof(users)) == sizeof(users) )
						fsal_status = FSAL_STATUS_OK;
				}

				/* done modifying the lock file */
				FSAL_CloseFile( fsal_handle );

				/* determine if no other processes are using the data queue */
				if ( users == 0 ) {

					/* delete the lock file */
					fsal_status = FSAL_DeleteFile( ".rolock" );
				}
			}

			/* locate the write-only lock file associated with the data queue */
			if ( FSAL_ListFile( ".wolock" ) == FSAL_STATUS_OK ) {

				/* delete the lock file (only one user allowed) */
				fsal_status = FSAL_DeleteFile( ".wolock" );
			}

			/* locate the read/write lock file associated with the data queue */
			if ( FSAL_ListFile( ".rwlock" ) == FSAL_STATUS_OK ) {

				/* delete the lock file (only one user allowed) */
				fsal_status = FSAL_DeleteFile( ".rwlock" );
			}

			/* invalidate the handle for reuse */
			memset( &DataQ_FileHandleList[index], DATA_QUEUE_FILE_HANDLE_INVALID, sizeof(DataQ_File_t) );

			/* found the handle so stop looking */
			break;
		}
	}

	FSAL_ChangeDirectory( "../" );

	/* check if appropriate lock file is updated and/or deleted */
	if ( fsal_status != FSAL_STATUS_OK ) {
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* operation succeeded */
	return CODE_STATUS_OK;
}


/** @brief Enqueues an entry into the specified first-in, first-out
 *         (FIFO) data queue.
 *
 *  This function enqueues or inserts a new entry into the specified
 *  data queue. For a FIFO type of queue, insertion happens on the
 *  'tail' end only (where the youngest entry can be found) and not
 *  anywhere else. If the data queue is full, then the oldest entry
 *  in the queue, located in the 'head' end, is removed from the queue
 *  before the new entry is inserted.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           accessed for the enqueue operation.
 *
 *  @param[in] data - the reference of the data to be enqueued.
 *
 *  @param[in] size - the size of the data to be enqueued.
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_INVALID_HANDLE
 *                CODE_ERROR_QUEUE_READ_ONLY
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_CLOSED
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoEnqueue( DataQ_File_t * fifo_handle, void * data, size_t size )
{
	FSAL_File_t fsal_handle = -1;
	DataQ_Hdr_t fifo_hdr;
	DataQ_LUT_Entry_t fifo_lut_entry;
	uint8_t fifo_lut_cache[ DATAQ_LUT_FILE_SIZE_MAX ] = { 0 };
	char fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE + 1];
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( ( fifo_handle == (DataQ_File_t *) 0 ) || ( data == (void *) 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check mandatory arguments for invalid values */
	if ( size == 0 ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check for a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX + 1; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if ( index < DATA_QUEUE_FILE_HANDLE_LIST_MAX ) {

			/* a valid fifo handle references one data queue on the list */
			if( fifo_handle == &DataQ_FileHandleList[index] ) {

				/* found it */
				break;
			}

			/* try next one */
			continue;
		}

		/* can not find the handle */
		return CODE_ERROR_INVALID_HANDLE;
	}

	/* check if write access is allowed */
	if ( ( fifo_handle->access != ACCESS_TYPE_WRITE_ONLY ) &&
		 ( fifo_handle->access != ACCESS_TYPE_READ_WRITE ) ) {
		return CODE_ERROR_QUEUE_READ_ONLY;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened for writing */
	if ( (FSAL_ListFile( ".wolock" ) == FSAL_ERROR_FILE_ACCESS) &&
		 (FSAL_ListFile( ".rwlock" ) == FSAL_ERROR_FILE_ACCESS) ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_CLOSED;
	}

	/* open and read the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* create a cache which directly mirrors the LUT file associated with the fifo */
	if ( (FSAL_OpenFile(".lut", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)fifo_lut_cache, DATAQ_LUT_FILE_SIZE_MAX) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* increment reference count */
	fifo_hdr.reference_count++;

	/* initialize the LUT entry by converting reference count  as a revolving reference string */
	for ( int base10 = 1, index = 0; index < sizeof(fifo_lut_entry.reference); base10 *= 10, index++ ) {
		fifo_lut_entry.reference[ DATA_QUEUE_LUT_ENTRY_SIZE - index - 1] = '0' + ((fifo_hdr.reference_count % (base10 * 10)) / base10);
	}

	/* copy the LUT entry reference array and terminate to make it a string */
	PSL_memcpy( fifo_lut_entry_reference, fifo_lut_entry.reference, DATA_QUEUE_LUT_ENTRY_SIZE);
	fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE] = '\0';

	/* create a new file to contain the enqueued data */
	if ( (FSAL_OpenFile(fifo_lut_entry_reference, FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY | FSAL_FLAGS_CREATE, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)data, size) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* determine if fifo is empty */
	if ( (fifo_hdr.num_of_entries == 0) &&
		 (fifo_hdr.head_lut_offs == fifo_hdr.tail_lut_offs)  ) {

		/* add the new entry by copying the LUT entry indicated either by the head or tail offset */
		PSL_memcpy( fifo_lut_cache + (fifo_hdr.tail_lut_offs * sizeof(DataQ_LUT_Entry_t)), &fifo_lut_entry, sizeof(DataQ_LUT_Entry_t) );

		/* increment entry counter */
		fifo_hdr.num_of_entries++;

	/* determine if fifo is already full */
    } else if ( (fifo_hdr.num_of_entries == fifo_hdr.max_entries) &&
   		        (((fifo_hdr.head_lut_offs == 0) && (fifo_hdr.tail_lut_offs == (fifo_hdr.max_entries - 1))) ||
		         ((fifo_hdr.head_lut_offs > 0) && (fifo_hdr.tail_lut_offs == (fifo_hdr.head_lut_offs - 1)))) ) {

		/* adjust seek offset if seek is set to the head end of the data queue
		 * (wrapping around if the seek reach the end of the queue) */
		if ( fifo_hdr.seek_lut_offs == fifo_hdr.head_lut_offs ) {
			fifo_hdr.seek_lut_offs = (fifo_hdr.seek_lut_offs + 1) % fifo_hdr.max_entries;
		}

		/* remove the oldest entry by invalidating the LUT entry indicated by the
		 * head offset and then incrementing the head offset (wrapping around if
		 * the head reach the end of the queue)
		 */
		PSL_memset( fifo_lut_cache + (fifo_hdr.head_lut_offs * sizeof(DataQ_LUT_Entry_t)), 0, sizeof(DataQ_LUT_Entry_t) );
		fifo_hdr.head_lut_offs = (fifo_hdr.head_lut_offs + 1) % fifo_hdr.max_entries;

		/* add the new entry by incrementing the tail offset (wrapping around if the
		 * tail reach the end of the queue) and copying the LUT entry indicated by
		 * the new tail offset
		 */
		fifo_hdr.tail_lut_offs = (fifo_hdr.tail_lut_offs + 1) % fifo_hdr.max_entries;
		PSL_memcpy( fifo_lut_cache + (fifo_hdr.tail_lut_offs * sizeof(DataQ_LUT_Entry_t)), &fifo_lut_entry, sizeof(DataQ_LUT_Entry_t) );

	} else {

		/* add the new entry by incrementing the tail offset (wrapping around if the
		 * tail reach the end of the queue) and copying the LUT entry indicated by
		 * the new tail offset
		 */
		fifo_hdr.tail_lut_offs = (fifo_hdr.tail_lut_offs + 1) % fifo_hdr.max_entries;
		PSL_memcpy( fifo_lut_cache + (fifo_hdr.tail_lut_offs * sizeof(DataQ_LUT_Entry_t)), &fifo_lut_entry, sizeof(DataQ_LUT_Entry_t) );

		/* increment entry counter */
		fifo_hdr.num_of_entries++;
	}

	/* update the LUT file associated with the fifo */
	if ( (FSAL_OpenFile(".lut", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)fifo_lut_cache, DATAQ_LUT_FILE_SIZE_MAX) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* update the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* operation succeeded */
	FSAL_ChangeDirectory( "../" );
	return CODE_STATUS_OK;
}


/** @brief Dequeues an entry from the specified first-in, first-out
 *         (FIFO) data queue.
 *
 *  This function dequeues or removes a new entry on the specified
 *  data queue. For a FIFO type of queue, deletion happens on the
 *  'head' end only (where the oldest entry can be found) and not
 *  anywhere else. On successful operation, the contents and size
 *  of the data removed from the queue is copied into the non-null
 *  output parameters. The queue needs to have at least one entry
 *  for the operation to succeed.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           access for the dequeue operation.
 *
 *  @param[out] data - if not null, the reference where the data to be
 *                     dequeued is to be copied (otherwise, it is not
 *                     copied).
 *
 *  @param[out] size - if not null, the reference where the data size
 *                     of the data to be dequeued is set (otherwise,
 *                     it is not set).
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_INVALID_HANDLE
 *                CODE_ERROR_QUEUE_READ_ONLY
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_CLOSED
 *                CODE_ERROR_FS_ACCESS_FAIL
 *                CODE_ERROR_QUEUE_IS_EMPTY
 *
 */
int DataQ_FifoDequeue( DataQ_File_t * fifo_handle, void * data, size_t * size )
{
	FSAL_File_t fsal_handle = -1;
	DataQ_Hdr_t fifo_hdr;
	uint8_t fifo_lut_cache[ DATAQ_LUT_FILE_SIZE_MAX ] = { 0 };
	char fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE + 1];
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( ( fifo_handle == (DataQ_File_t *) 0 ) || ( data == (void *) 0 ) || ( size == (size_t *) 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check for a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX + 1; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if ( index < DATA_QUEUE_FILE_HANDLE_LIST_MAX ) {

			/* a valid fifo handle references one data queue on the list */
			if( fifo_handle == &DataQ_FileHandleList[index] ) {

				/* found it */
				break;
			}

			/* try next one */
			continue;
		}

		/* can not find the handle */
		return CODE_ERROR_INVALID_HANDLE;
	}

	/* check if write access is allowed */
	if ( ( fifo_handle->access != ACCESS_TYPE_WRITE_ONLY ) &&
		 ( fifo_handle->access != ACCESS_TYPE_READ_WRITE ) ) {
		return CODE_ERROR_QUEUE_READ_ONLY;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened for writing */
	if ( (FSAL_ListFile( ".wolock" ) == FSAL_ERROR_FILE_ACCESS) &&
		 (FSAL_ListFile( ".rwlock" ) == FSAL_ERROR_FILE_ACCESS) ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_CLOSED;
	}

	/* open and read the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* create a cache which directly mirrors the LUT file associated with the fifo */
	if ( (FSAL_OpenFile(".lut", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)fifo_lut_cache, DATAQ_LUT_FILE_SIZE_MAX) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* determine if fifo is empty */
	if ( (fifo_hdr.num_of_entries == 0) &&
		 (fifo_hdr.head_lut_offs == fifo_hdr.tail_lut_offs)  ) {

		/* nothing to return */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_EMPTY;

	} else {

		/* adjust seek offset if seek is set to the head end of the data queue
		 * (wrapping around if the seek reach the end of the queue) */
		if ( fifo_hdr.seek_lut_offs == fifo_hdr.head_lut_offs ) {
			fifo_hdr.seek_lut_offs = (fifo_hdr.seek_lut_offs + 1) % fifo_hdr.max_entries;
		}

		/* copy the LUT entry reference array and terminate to make it a string */
		PSL_memcpy( fifo_lut_entry_reference, fifo_lut_cache + (fifo_hdr.head_lut_offs * sizeof(DataQ_LUT_Entry_t)), DATA_QUEUE_LUT_ENTRY_SIZE);
		fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE] = '\0';

		/* copy the dequeued data and store it if possible */
		if ( (FSAL_OpenFile(fifo_lut_entry_reference, FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
			 (*size = FSAL_ReadFile(fsal_handle, (uint8_t *)data, *size) < 0) ||
			 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

			/* file system access error */
			if ( fsal_handle != -1 )
				FSAL_CloseFile( fsal_handle );
			FSAL_ChangeDirectory( "../" );
			return CODE_ERROR_FS_ACCESS_FAIL;
		}

		/* delete the file */
		FSAL_DeleteFile( fifo_lut_entry_reference );

		/* remove the oldest entry by invalidating the LUT entry indicated by the
		 * head offset and then incrementing the head offset (wrapping around if
		 * the head reach the end of the queue)
		 */
		PSL_memset( fifo_lut_cache + (fifo_hdr.head_lut_offs * sizeof(DataQ_LUT_Entry_t)), 0, sizeof(DataQ_LUT_Entry_t) );
		fifo_hdr.head_lut_offs = (fifo_hdr.head_lut_offs + 1) % fifo_hdr.max_entries;

		/* decrement entry counter */
		fifo_hdr.num_of_entries--;
	}

	/* update the LUT file associated with the fifo */
	if ( (FSAL_OpenFile(".lut", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)fifo_lut_cache, DATAQ_LUT_FILE_SIZE_MAX) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* update the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* operation succeeded */
	FSAL_ChangeDirectory( "../" );
	return CODE_STATUS_OK;
}


/** @brief Seeks to an entry from the specified first-in, first-out
 *         (FIFO) data queue.
 *
 *  This function sets the current position of the 'seek' pointer
 *  to a particular entry from the specified data queue. The seek
 *  operation can be either seek to the 'head' end, seek to the
 *  'tail' end, or seek to a specific position between 'head' and
 *  'tail' ends. The data queue should be seekable and should have
 *  at least one entry for the operation to succeed.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           accessed for the seek operation.
 *
 *  @param[in] seek_type - the type of seek to be performed:
 *
 *                         SEEK_TYPE_HEAD
 *                         SEEK_TYPE_TAIL
 *                         SEEK_TYPE_POSITION
 *
 *  @param[in] position - if SEEK_TYPE_POSITION is specified as the
 *                        seek type, the position where the 'seek'
 *                        pointer would be set (otherwise, this
 *                        parameter is ignored).
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_INVALID_HANDLE
 *                CODE_ERROR_INVALID_SEEK
 *                CODE_ERROR_QUEUE_WRITE_ONLY
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_CLOSED
 *                CODE_ERROR_QUEUE_IS_EMPTY
 *                CODE_ERROR_QUEUE_NOT_SEEKABLE
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoSeek( DataQ_File_t * fifo_handle, int seek_type, int position )
{
	FSAL_File_t fsal_handle = -1;
	DataQ_Hdr_t fifo_hdr;
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( fifo_handle == (DataQ_File_t *) 0 )  {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check mandatory arguments for invalid values */
	if ( seek_type >= SEEK_TYPE_MAX ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check for a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX + 1; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if ( index < DATA_QUEUE_FILE_HANDLE_LIST_MAX ) {

			/* a valid fifo handle references one data queue on the list */
			if( fifo_handle == &DataQ_FileHandleList[index] ) {

				/* found it */
				break;
			}

			/* try next one */
			continue;
		}

		/* can not find the handle */
		return CODE_ERROR_INVALID_HANDLE;
	}

	/* at least read access is allowed */
	if ( fifo_handle->access == ACCESS_TYPE_WRITE_ONLY ) {
		return CODE_ERROR_QUEUE_WRITE_ONLY;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened for reading */
	if ( (FSAL_ListFile( ".rolock" ) == FSAL_ERROR_FILE_ACCESS) &&
		 (FSAL_ListFile( ".rwlock" ) == FSAL_ERROR_FILE_ACCESS) ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_CLOSED;
	}

	/* open and read the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* determine if fifo is seekable */
	if ( (fifo_hdr.flags & FLAGS_RANDOM_ACCESS) == 0 ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_NOT_SEEKABLE;
	}

	/* determine if fifo is empty */
	if ( (fifo_hdr.num_of_entries == 0) &&
		 (fifo_hdr.head_lut_offs == fifo_hdr.tail_lut_offs)  ) {

		/* nothing to return */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_EMPTY;

	}

	/* determine if seek position is outside the range */
	if ( position >= fifo_hdr.num_of_entries ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_INVALID_SEEK;
	}

	/* perform seek operation */
	if ( seek_type == SEEK_TYPE_HEAD ) {

		/* set seek offset to current head offset in the LUT entry */
		fifo_hdr.seek_lut_offs = fifo_hdr.head_lut_offs;

	} else if ( seek_type == SEEK_TYPE_TAIL ) {

		/* set seek offset to current tail offset in the LUT entry */
		fifo_hdr.seek_lut_offs = fifo_hdr.tail_lut_offs;

	} else {

		/* set seek offset to anywhere between head and tail offsets
		 * in relative to the head offset in the LUT entry
		 */
		fifo_hdr.seek_lut_offs = (fifo_hdr.head_lut_offs + position) % fifo_hdr.max_entries;
	}

	/* update the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* operation succeeded */
	FSAL_ChangeDirectory( "../" );
	return CODE_STATUS_OK;
}


/** @brief Copies an entry from the specified first-in, first-out
 *         (FIFO) data queue.
 *
 *  This function copies an entry from the specified data queue,
 *  particularly where the 'seek' pointer is currently positioned.
 *  After the contents and size of the entry are copied and set to
 *  the corresponding output parameters, the position of the 'seek'
 *  pointer is advanced to the next entry. If the old 'seek' pointer
 *  position is the 'head' end of the queue, then the new position
 *  would be the 'tail' end of the queue. The queue needs to have
 *  at least one entry for the operation to succeed.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           accessed for the copy operation.
 *
 *  @param[out] data - the reference where the data is to be copied.
 *
 *  @param[out] size - the reference where the size of the copied
 *                     data is to be set.
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_INVALID_HANDLE
 *                CODE_ERROR_QUEUE_WRITE_ONLY
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_CLOSED
 *                CODE_ERROR_QUEUE_IS_EMPTY
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoGetEntry( DataQ_File_t * fifo_handle, void * data, size_t * size )
{
	FSAL_File_t fsal_handle = -1;
	DataQ_Hdr_t fifo_hdr;
	DataQ_LUT_Entry_t fifo_lut_entry;
	uint8_t fifo_lut_cache[ DATAQ_LUT_FILE_SIZE_MAX ] = { 0 };
	char fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE + 1];
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( ( fifo_handle == (DataQ_File_t *) 0 ) || ( data == (void *) 0 ) || ( size == (size_t *) 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check for a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX + 1; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if ( index < DATA_QUEUE_FILE_HANDLE_LIST_MAX ) {

			/* a valid fifo handle references one data queue on the list */
			if( fifo_handle == &DataQ_FileHandleList[index] ) {

				/* found it */
				break;
			}

			/* try next one */
			continue;
		}

		/* can not find the handle */
		return CODE_ERROR_INVALID_HANDLE;
	}

	/* at least read access is allowed */
	if ( fifo_handle->access == ACCESS_TYPE_WRITE_ONLY ) {
		return CODE_ERROR_QUEUE_WRITE_ONLY;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened for reading */
	if ( (FSAL_ListFile( ".rolock" ) == FSAL_ERROR_FILE_ACCESS) &&
		 (FSAL_ListFile( ".rwlock" ) == FSAL_ERROR_FILE_ACCESS) ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_CLOSED;
	}

	/* open and read the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* create a cache which directly mirrors the LUT file associated with the fifo */
	if ( (FSAL_OpenFile(".lut", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)fifo_lut_cache, DATAQ_LUT_FILE_SIZE_MAX) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* determine if fifo is empty */
	if ( (fifo_hdr.num_of_entries == 0) &&
		 (fifo_hdr.head_lut_offs == fifo_hdr.tail_lut_offs)  ) {

		/* nothing to return */
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_IS_EMPTY;

	}

	/* retrieve the entry by copying the LUT entry indicated by the seek offset */
	PSL_memcpy( &fifo_lut_entry,
			    fifo_lut_cache + (fifo_hdr.seek_lut_offs * sizeof(DataQ_LUT_Entry_t)),
			    sizeof(DataQ_LUT_Entry_t) );

	/* copy the LUT entry reference array and terminate to make it a string */
	PSL_memcpy( fifo_lut_entry_reference, fifo_lut_entry.reference, DATA_QUEUE_LUT_ENTRY_SIZE);
	fifo_lut_entry_reference[DATA_QUEUE_LUT_ENTRY_SIZE + 1] = '\0';

	/* extract the data from the LUT entry as indicated by the reference */
	if ( (FSAL_OpenFile(fifo_lut_entry_reference, FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)data, *size) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* increment the seek offset if it have not yet reached the tail offset */
	if ( fifo_hdr.seek_lut_offs != fifo_hdr.tail_lut_offs ) {
		fifo_hdr.seek_lut_offs = (fifo_hdr.seek_lut_offs + 1) % fifo_hdr.max_entries;
	}

	/* update the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_WRITE_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_WriteFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* operation succeeded */
	FSAL_ChangeDirectory( "../" );
	return CODE_STATUS_OK;
}


/** @brief Retrieves the current number of entries of the specified
 *         first-in, first-out (FIFO) data queue.
 *
 *  This function gets the number of entries the specified data queue
 *  currently has. If the operation succeeds, the number of entries
 *  is stored in the output parameter.
 *
 *
 *  @param[in] fifo_handle - the reference of the data queue to be
 *                           accessed for the get length operation.
 *
 *  @param[out] length - the reference where the number of entries
 *                       is to be stored.
 *
 *  @return int - the status or error code of the operation after
 *                the call:
 *
 *                CODE_STATUS_OK
 *                CODE_ERROR_INVALID_ARG
 *                CODE_ERROR_INVALID_HANDLE
 *                CODE_ERROR_QUEUE_MISSING
 *                CODE_ERROR_QUEUE_CLOSED
 *                CODE_ERROR_FS_ACCESS_FAIL
 *
 */
int DataQ_FifoGetLength( DataQ_File_t * fifo_handle, size_t * length )
{
	FSAL_File_t fsal_handle = -1;
	DataQ_Hdr_t fifo_hdr;
	int index;

	/* check mandatory arguments for NULL pointers */
	if ( ( fifo_handle == (DataQ_File_t *) 0 ) || ( length == (size_t *) 0 ) ) {
		return CODE_ERROR_INVALID_ARG;
	}

	/* check for a valid fifo handle */
	for ( index = 0; index < DATA_QUEUE_FILE_HANDLE_LIST_MAX + 1; index++ ) {

		/* locate the fifo handle in the currently opened queue list */
		if ( index < DATA_QUEUE_FILE_HANDLE_LIST_MAX ) {

			/* a valid fifo handle references one data queue on the list */
			if( fifo_handle == &DataQ_FileHandleList[index] ) {

				/* found it */
				break;
			}

			/* try next one */
			continue;
		}

		/* can not find the handle */
		return CODE_ERROR_INVALID_HANDLE;
	}

	/* check if folder associated with data queue is present by changing directory */
	if ( FSAL_ChangeDirectory( fifo_handle->name ) == FSAL_ERROR_DIR_ACCESS ) {
		return CODE_ERROR_QUEUE_MISSING;
	}

	/* check if fifo is already opened */
	if ( (FSAL_ListFile( ".rolock" ) == FSAL_ERROR_FILE_ACCESS) &&
	     (FSAL_ListFile( ".wolock" ) == FSAL_ERROR_FILE_ACCESS) &&
		 (FSAL_ListFile( ".rwlock" ) == FSAL_ERROR_FILE_ACCESS) ) {
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_QUEUE_CLOSED;
	}

	/* open and read the header (or metadata) file associated with the fifo */
	if ( (FSAL_OpenFile(".header", FSAL_FLAGS_BINARY | FSAL_FLAGS_READ_ONLY, &fsal_handle) == FSAL_ERROR_FILE_ACCESS) ||
		 (FSAL_ReadFile(fsal_handle, (uint8_t *)&fifo_hdr, sizeof(fifo_hdr)) < 0) ||
		 (FSAL_CloseFile(fsal_handle) == FSAL_ERROR_FILE_ACCESS) ) {

		/* file system access error */
		if ( fsal_handle != -1 )
			FSAL_CloseFile( fsal_handle );
		FSAL_ChangeDirectory( "../" );
		return CODE_ERROR_FS_ACCESS_FAIL;
	}

	/* copy the current number of fifo entries */
	*length = fifo_hdr.num_of_entries;

	/* operation succeeded */
	FSAL_ChangeDirectory( "../" );
	return CODE_STATUS_OK;
}
