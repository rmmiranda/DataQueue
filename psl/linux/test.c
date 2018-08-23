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
int main( int argc, char * argv[] )
{
	printf( "\r\nCommand Line Interface (CLI) for TR M7 DataQ" );
	printf( "\r\nCopyright 2018, Swift Labs" );
	printf( "\r\n" );
	printf( "\r\nAvailable Commands:" );
	printf( "\r\n  create <fifo-name> <fifo-size> <fifo-item-size> <fifo-type>" );
	printf( "\r\n         - creates a first-in, first-out (FIFO) data queue called" );
	printf( "\r\n           <fifo-name> and can store up to maximum of <fifo-size>" );
	printf( "\r\n           items with each fifo item has size of <fifo-item-size>" );
	printf( "\r\n           bytes; the <fifo-type> can either be MESSAGE or LOG" );
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
	printf( "\r\n" );

	return 0;
}

