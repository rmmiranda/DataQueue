/**********************************************************************
* Filename:     psl.h
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 20, 2018
*
* Description:  This is the header file associated with the top-level
*               Platform Software Layer or PSL. This pulls in the header
*               file associated to a specific platform software or OS
*               (usually found at psl/<psl-name>/psl.h) and contains
*               macro definitions and prototypes which are referenced
*               by the data queue application API.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifndef __PSL_H__
#define __PSL_H__

#if defined( PSL_LINUX )
#include "../psl/linux/psl.h"
#else
#error "Please define one of the supported Platform Software Layers"
#endif

/**
 * Platform specific macro definitons
 */
#define DATA_QUEUE_FILE_HANDLE_LIST_MAX			PSL_FILE_HANDLE_LIST_MAX
#define DATA_QUEUE_FILE_HANDLE_INVALID			PSL_FILE_HANDLE_INVALID


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
extern void * PSL_memset ( void * block, int c, size_t size );


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
extern void * PSL_memcpy (void *restrict to, const void *restrict from, size_t size);

#endif /* __PSL_H__ */

