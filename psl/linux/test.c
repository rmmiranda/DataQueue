/**********************************************************************
* Filename:     test.c
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 20, 2018
*
* Description:  This is the Platform Software Layer or PSL specific test
*               code to verify the data queue implementation on systems
*               based on the Linux kernel. This file contains the main
*               routine which implements a command line interface (CLI).
*
* History
* 20-Aug-2018   RMM      Main routine implemented as a CLI (test) module.
**********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "psl.h"
#include "dataqueue.h"

#define ASCII_CODE_NULL						0x00
#define ASCII_CODE_BS						0x08
#define ASCII_CODE_NL						0x0A
#define ASCII_CODE_FF						0x0C
#define ASCII_CODE_CR						0x0D
#define ASCII_CODE_ESC						0x1B
#define ASCII_CODE_DEL						0x7F

void DataQ_Create_CLI( int argc, char * argv[] );
void DataQ_Destroy_CLI( int argc, char * argv[] );
void DataQ_Enqueue_CLI( int argc, char * argv[] );
void DataQ_Dequeue_CLI( int argc, char * argv[] );
void DataQ_Fetch_CLI( int argc, char * argv[] );
void DataQ_Length_CLI( int argc, char * argv[] );

typedef enum {
	DATAQ_CMD_CREATE,
	DATAQ_CMD_DESTROY,
	DATAQ_CMD_ENQUEUE,
	DATAQ_CMD_DEQUEUE,
	DATAQ_CMD_FETCH,
	DATAQ_CMD_LENGTH,
	DATAQ_CMD_MAX,
} DataQCmdId;

typedef void (*DATAQ_CMD_FUNC)( int argc, char * argv[] );

typedef struct {
	char * name;
	DATAQ_CMD_FUNC handler;
} DataQCmd_t;

static DataQCmd_t command_list[DATAQ_CMD_MAX] = {
	{ "create",  DataQ_Create_CLI  },
	{ "destroy", DataQ_Destroy_CLI },
	{ "enqueue", DataQ_Enqueue_CLI },
	{ "dequeue", DataQ_Dequeue_CLI },
	{ "fetch",   DataQ_Fetch_CLI   },
	{ "length",  DataQ_Length_CLI  },
};

static char command_buffer[64] = {0};
static unsigned short command_buffer_length = 0;
static unsigned short command_buffer_index = 0;

void DataQ_Create_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	size_t fifo_size;
	size_t fifo_item_size;
	//char * fifo_type;
	int dataq_status;

	if ( argc < 4 ) {
		printf( "Usage:" );
		printf( "\r\ncreate <fifo-name> <fifo-size> <fifo-item-size>" );
		printf( "\r\n - creates a first-in, first-out (FIFO) data queue called" );
		printf( "\r\n   <fifo-name> and can store up to maximum of <fifo-size>" );
		printf( "\r\n   items with each fifo item has size of <fifo-item-size>" );
		printf( "\r\n   bytes" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];
	fifo_size = (size_t) atoi( argv[2] );
	fifo_item_size =(size_t) atoi( argv[3] );

	dataq_status = DataQ_FifoCreate( fifo_name, fifo_size, fifo_item_size, FLAGS_RANDOM_ACCESS );
	if ( dataq_status == CODE_STATUS_OK ) {
		printf( "Operation succeeded" );
	} else {
		printf( "Operation failed (error code = %d)", dataq_status );
	}

	return;
}

void DataQ_Destroy_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	int dataq_status;

	if ( argc < 2 ) {
		printf( "Usage:" );
		printf( "\r\ndestroy <fifo-name>" );
		printf( "\r\n - destroys the first-in, first-out (FIFO) data queue as" );
		printf( "\r\n   specified by the name <fifo-name>" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];

	dataq_status = DataQ_FifoDestroy( fifo_name );
	if ( dataq_status == CODE_STATUS_OK ) {
		printf( "Operation succeeded" );
	} else {
		printf( "Operation failed (error code = %d)", dataq_status );
	}

	return;
}

void DataQ_Enqueue_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	char * fifo_item_data;
	DataQ_File_t * fifo_handle;
	int dataq_status;

	if ( argc < 3 ) {
		printf( "Usage:" );
		printf( "\r\nenqueue <fifo-name> <fifo-item-data>" );
		printf( "\r\n - opens the first-in, first-out (FIFO) data queue called" );
		printf( "\r\n   <fifo-name>, adds a FIFO item containing the string data" );
		printf( "\r\n   specified in <fifo-item-data>, and closes the FIFO" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];
	fifo_item_data = argv[2];

	dataq_status = DataQ_FifoOpen(
						fifo_name,
						ACCESS_TYPE_READ_WRITE,
						ACCESS_MODE_BINARY_PACKED,
						&fifo_handle );

	if ( dataq_status == CODE_STATUS_OK ) {

		printf( "Open operation succeeded\n" );

		dataq_status = DataQ_FifoEnqueue( fifo_handle, fifo_item_data, strlen(fifo_item_data) );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Enqueue operation succeeded\n" );
		} else {
			printf( "Enqueue operation failed (error code = %d)\n", dataq_status );
		}

		dataq_status = DataQ_FifoClose( fifo_handle );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Close operation succeeded" );
		} else {
			printf( "Close operation failed (error code = %d)\n", dataq_status );
		}

	} else {
		printf( "Open operation failed (error code = %d)", dataq_status );
	}

	return;
}

void DataQ_Dequeue_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	char fifo_item_data[256] = {0};
	size_t fifo_item_length = sizeof(fifo_item_data);
	DataQ_File_t * fifo_handle;
	int dataq_status;

	if ( argc < 2 ) {
		printf( "Usage:" );
		printf( "\r\ndequeue <fifo-name>" );
		printf( "\r\n - opens the first-in, first-out (FIFO) data queue called" );
		printf( "\r\n   <fifo-name>, removes the oldest FIFO item, and closes the" );
		printf( "\r\n   FIFO (the string data associated with the removed item is" );
		printf( "\r\n   printed to the console)" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];

	dataq_status = DataQ_FifoOpen(
						fifo_name,
						ACCESS_TYPE_READ_WRITE,
						ACCESS_MODE_BINARY_PACKED,
						&fifo_handle );

	if ( dataq_status == CODE_STATUS_OK ) {

		printf( "Open operation succeeded\n" );

		dataq_status = DataQ_FifoDequeue( fifo_handle, fifo_item_data, &fifo_item_length );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Dequeue operation succeeded (item data: %s)\n", fifo_item_data );
		} else {
			printf( "Dequeue operation failed (error code = %d)\n", dataq_status );
		}

		dataq_status = DataQ_FifoClose( fifo_handle );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Close operation succeeded" );
		} else {
			printf( "Close operation failed (error code = %d)\n", dataq_status );
		}

	} else {
		printf( "Open operation failed (error code = %d)", dataq_status );
	}

	return;
}

void DataQ_Fetch_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	char fifo_item_data[256] = {0};
	size_t fifo_item_length = sizeof(fifo_item_data);
	int fifo_item_index = 0;
	DataQ_File_t * fifo_handle;
	int dataq_status;

	if ( argc < 2 ) {
		printf( "Usage:" );
		printf( "\r\nfetch <fifo-name> [<fifo-item-index]" );
		printf( "\r\n - opens the first-in, first-out (FIFO) data queue called" );
		printf( "\r\n   <fifo-name>, reads either the oldest FIFO item or, if the" );
		printf( "\r\n   optional <fifo-item-index> is indicated, the FIFO item as" );
		printf( "\r\n   specified by <fifo-item-index>, and closes the FIFO (the" );
		printf( "\r\n   string data associated with the read FIFO item is printed" );
		printf( "\r\n   to the console)" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];
	if ( argc == 3 ) {
		fifo_item_index = atoi( argv[2] );
	}

	dataq_status = DataQ_FifoOpen(
						fifo_name,
						ACCESS_TYPE_READ_ONLY,
						ACCESS_MODE_BINARY_PACKED,
						&fifo_handle );

	if ( dataq_status == CODE_STATUS_OK ) {

		printf( "Open operation succeeded\n" );

		dataq_status = DataQ_FifoSeek( fifo_handle, SEEK_TYPE_POSITION, fifo_item_index );
		if ( dataq_status == CODE_STATUS_OK ) {

			printf( "Seek operation succeeded\n" );

			dataq_status = DataQ_FifoGetEntry( fifo_handle, fifo_item_data, &fifo_item_length );
			if ( dataq_status == CODE_STATUS_OK ) {
				printf( "Get entry operation succeeded (item data: %s)\n", fifo_item_data );
			} else {
				printf( "Get entry operation failed (error code = %d)\n", dataq_status );
			}

		} else {
			printf( "Seek operation failed (error code = %d)\n", dataq_status );
		}

		dataq_status = DataQ_FifoClose( fifo_handle );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Close operation succeeded" );
		} else {
			printf( "Close operation failed (error code = %d)\n", dataq_status );
		}

	} else {
		printf( "Open operation failed (error code = %d)", dataq_status );
	}

	return;

}

void DataQ_Length_CLI( int argc, char * argv[] )
{
	char * fifo_name;
	DataQ_File_t * fifo_handle;
	int dataq_status;
	size_t fifo_length;

	if ( argc < 2 ) {
		printf( "Usage:" );
		printf( "\r\nlength <fifo-name>" );
		printf( "\r\n - opens the first-in, first-out (FIFO) data queue called" );
		printf( "\r\n   <fifo-name>, reads the current length of the FIFO, and" );
		printf( "\r\n   closes the FIFO (the read length is printed to the console)" );

		return;
	}

	/* get handles to the arguments */
	fifo_name = argv[1];

	dataq_status = DataQ_FifoOpen(
						fifo_name,
						ACCESS_TYPE_READ_ONLY,
						ACCESS_MODE_BINARY_PACKED,
						&fifo_handle );

	if ( dataq_status == CODE_STATUS_OK ) {

		printf( "Open operation succeeded\n" );

		dataq_status = DataQ_FifoGetLength( fifo_handle, &fifo_length );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Get length operation succeeded (length = %d)\n", (int)fifo_length );
		} else {
			printf( "Get length operation failed (error code = %d)\n", dataq_status );
		}

		dataq_status = DataQ_FifoClose( fifo_handle );
		if ( dataq_status == CODE_STATUS_OK ) {
			printf( "Close operation succeeded" );
		} else {
			printf( "Close operation failed (error code = %d)\n", dataq_status );
		}

	} else {
		printf( "Open operation failed (error code = %d)", dataq_status );
	}

	return;
}

/** @brief The main application routine.
 *
 *  This function implements the main application routine as a command
 *  line interface (CLI) which is used for testing the data queue design
 *  on a Linux system.
 *
 *  @param[in] argc - the number of command arguments
 *
 *  @param[in] argv - the command argument strings
 *
 *  @return int - the exit status code of the application
 *
 */
int PSM_main( int argc, char * argv[] )
{
	char * command_args[10];
	unsigned char command_args_index;
	DATAQ_CMD_FUNC command_handler;
	int index;

	printf( "\r\nCommand Line Interface (CLI) for TR M7 DataQ" );
	printf( "\r\nCopyright 2018, Swift Labs" );
	printf( "\r\n" );
	printf( "\r\nAvailable Commands:" );
	printf( "\r\n  create <fifo-name> <fifo-size> <fifo-item-size> <fifo-type>" );
	printf( "\r\n         - creates a first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name> and can store up to maximum of <fifo-size>" );
	printf( "\r\n           items with each fifo item has size of <fifo-item-size>" );
	printf( "\r\n           bytes" );
	printf( "\r\n  destroy <fifo-name>" );
	printf( "\r\n         - destroys the first-in, first-out (FIFO) data queue as" );
	printf( "\r\n           specified by the name <fifo-name>" );
	printf( "\r\n  enqueue <fifo-name> <fifo-item-data>" );
	printf( "\r\n         - opens the first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name>, adds a FIFO item containing the string data" );
	printf( "\r\n           specified in <fifo-item-data>, and closes the FIFO" );
	printf( "\r\n  dequeue <fifo-name>" );
	printf( "\r\n         - opens the first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name>, removes the oldest FIFO item, and closes the" );
	printf( "\r\n           FIFO (the string data associated with the removed item is" );
	printf( "\r\n           printed to the console)" );
	printf( "\r\n  fetch <fifo-name> [<fifo-item-index]" );
	printf( "\r\n         - opens the first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name>, reads either the oldest FIFO item or, if the" );
	printf( "\r\n           optional <fifo-item-index> is indicated, the FIFO item as" );
	printf( "\r\n           specified by <fifo-item-index>, and closes the FIFO (the" );
	printf( "\r\n           string data associated with the read FIFO item is printed" );
	printf( "\r\n           to the console)" );
	printf( "\r\n  length <fifo-name>" );
	printf( "\r\n         - opens the first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name>, reads the current length of the FIFO, and" );
	printf( "\r\n           closes the FIFO (the read length is printed to the console)" );
	printf( "\r\n" );
	printf( "\r\nDataQ/>" );

	/* initiliaze the data queue engine */
	DataQ_InitEngine();

	while (1 ) {

		/* retrieves the command string from standard input */
		if( fgets( command_buffer, sizeof(command_buffer), stdin ) ) {
			command_buffer_length = strlen(command_buffer);
		}

		/* process the retrieved characters */
		while( command_buffer_index < command_buffer_length ) {

			/* determine if command string is sent */
			if (command_buffer[command_buffer_index] == ASCII_CODE_NL) {

				/* terminate the command string */
				command_buffer[command_buffer_index] = ASCII_CODE_NULL;

				/* parse command buffer */
				command_args_index = 0;
				command_args[command_args_index] = strtok( command_buffer, " " );
				while( command_args[command_args_index] ) {
					command_args_index++;
					command_args[command_args_index] = strtok( NULL, " " );
				}

				if ( command_args_index ) {

					/* lookup for existing command handlers */
					for( index = 0, command_handler = NULL; index < DATAQ_CMD_MAX; index++ ) {
						if( strcmp(command_list[index].name, command_args[0]) == 0 ) {
							command_handler = command_list[index].handler;
						}
					}

					/* process command */
					if ( command_handler )
						command_handler( command_args_index, command_args );
				}

				/* reset command buffer and args indexes */
				command_buffer_length = 0;
				command_buffer_index = 0;
				command_args_index = 0;

				/* show the prompt back */
				printf( "\r\nDataQ/>" );

				continue;
			}

			/* advance to the next character received */
			command_buffer_index++;
		}
	}

	return 0;
}

