/**********************************************************************
* Filename:     psl.c
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 20, 2018
*
* Description:  This is the Platform Software Layer or PSL specific to
*               any systems based on the Linux kernel. This file contains
*               implementation of all required PSL functions which are
*               referenced by the data queue application API.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifdef PSL_LINUX

#include "../../inc/psl.h"

/** @brief Fills a memory block with a specific byte value.
 *
 *  This function copies the passed byte value into each of the bytes
 *  of the memory block.
 *
 *  @param[in] block - the reference of the memory block
 *
 *  @param[in] c - the byte value used to fill the memory block
 *
 *  @param[in] size - the number of bytes of the memory block to set
 *
 *  @return void* - the reference of the set memory block
 *
 */
void * PSL_memset ( void * block, int c, size_t size )
{
	return memset ( block, c, size );
}

/** @brief Copies a memory block to another memory block.
 *
 *  This function copies the contents of one memory block to another
 *  memory block.
 *
 *  @param[in] to - the reference of the destination memory block
 *
 *  @param[in] from - the reference of the source memory block
 *
 *  @param[in] size - the number of bytes to copy
 *
 *  @return void* - the reference of the destination memory block
 *
 */
void * PSL_memcpy (void *restrict to, const void *restrict from, size_t size)
{
	return memcpy ( to, from, size );
}

#endif /* PSL_LINUX */
