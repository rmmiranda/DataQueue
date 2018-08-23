/**********************************************************************
* Filename:     dataqueue.c
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 10, 2018
*
* Description:  This is the header file for the data queue application
*               programming interface (API) which consists of declarations
*               of entry point functions, data structures and macro
*               definitions, which are all necessary for code development
*               of an application needing to deploy a file-based data queue
*               in their system.
*
* History
* 10-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifndef __DATA_QUEUE_H__
#define __DATA_QUEUE_H__

/**
 * Data queue status code used by
 * functions to return the status
 * of the associated operation
 */
#define CODE_STATUS_OK					        0
#define CODE_ERROR_INVALID_ARG					1
#define CODE_ERROR_INVALID_HANDLE				2
#define CODE_ERROR_INVALID_SEEK				    3
#define CODE_ERROR_QUEUE_EXISTS					4
#define CODE_ERROR_QUEUE_MISSING				5
#define CODE_ERROR_QUEUE_OPENED				    6
#define CODE_ERROR_QUEUE_CLOSED			        7
#define CODE_ERROR_QUEUE_IS_FULL		        8
#define CODE_ERROR_QUEUE_IS_EMPTY		        9
#define CODE_ERROR_QUEUE_IS_BUSY		        10
#define CODE_ERROR_QUEUE_READ_ONLY				11
#define CODE_ERROR_QUEUE_WRITE_ONLY				12
#define CODE_ERROR_QUEUE_NOT_SEEKABLE       	13
#define CODE_ERROR_FS_ACCESS_FAIL       		14
#define CODE_ERROR_HANDLE_NOT_AVAIL       		15



/**
 * Data queue flags to determine which
 * message type is stored
 */
#define FLAGS_MESSAGE_LOG						0x0001
#define FLAGS_RANDOM_ACCESS						0x0002

/**
 * Data queue access type used by
 * functions to set or to determine
 * which allowed operations to be
 * done on the data queue
 */
#define ACCESS_TYPE_READ_ONLY					0
#define ACCESS_TYPE_WRITE_ONLY					1
#define ACCESS_TYPE_READ_WRITE					2
#define ACCESS_TYPE_MAX							3


/**
 * Data queue access mode used by
 * functions to set or to determine
 * how raw data are moved to/from
 * data queue
 */
#define ACCESS_MODE_UNPACKED					0
#define ACCESS_MODE_BINARY_PACKED				1
#define ACCESS_MODE_MAX							3


/**
 * Type of seek operation to be used
 * on a seekable data queue
 */
#define SEEK_TYPE_HEAD							0
#define SEEK_TYPE_TAIL							1
#define SEEK_TYPE_POSITION						2
#define SEEK_TYPE_MAX							3


/**
 * Data structure used as a handle for
 * a single instance of the data queue
 */
typedef struct DataQ_File {
	char name[32];
	int handle;
	int access;
	int mode;
} DataQ_File_t;


/**
 * Data structure used as a header
 * of a specific data queue
 */
typedef struct DataQ_Hdr {
	size_t size;
	size_t max_entry_size;
	uint8_t max_entries;
	uint8_t num_of_entries;
	uint8_t head_lut_offs;
	uint8_t tail_lut_offs;
	uint8_t seek_lut_offs;
	uint8_t reserved;
	uint16_t reference_count;
	uint16_t flags;
} DataQ_Hdr_t;


/**
 * Data structure used as a LUT
 * entry of the data queue
 */
typedef struct DataQ_LUT_Entry {
	char reference[DATA_QUEUE_LUT_ENTRY_SIZE];
} DataQ_LUT_Entry_t;

#define DATAQ_LUT_FILE_SIZE_MAX		(256 * DATA_QUEUE_LUT_ENTRY_SIZE)

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
int DataQ_FifoCreate( char * fifo_name, uint8_t max_entries, size_t max_entry_size, uint16_t flags );


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
int DataQ_FifoDestroy( char * fifo_name );


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
 *  function considers a failed operation and the error code is
 *  set to busy to indicate a retry can be done.
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
 *
 */
int DataQ_FifoOpen( char * fifo_name, int access, int mode, DataQ_File_t ** fifo_handle );


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
int DataQ_FifoClose( DataQ_File_t * fifo_handle );


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
int DataQ_FifoEnqueue( DataQ_File_t * fifo_handle, void * data, size_t size );


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
int DataQ_FifoDequeue( DataQ_File_t * fifo_handle, void * data, size_t * size );


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
int DataQ_FifoSeek( DataQ_File_t * fifo_handle, int seek_type, int position );


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
int DataQ_FifoGetEntry( DataQ_File_t * fifo_handle, void * data, size_t * size );


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
int DataQ_FifoGetLength( DataQ_File_t * fifo_handle, size_t * length );

#endif /* __DATA_QUEUE_H__ */

