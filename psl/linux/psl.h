/**********************************************************************
* Filename:     psl.h
*
* Copyright 2018, Swift Labs / TR
*
* Author:       Ronald Miranda / Emil Ekmecic
*
* Created:      August 20, 2018
*
* Description:  This is the header file associated with the Platform
*               Software Layer or PSL specific to any systems based
*               on the Linux kernel.
*
* History
* 20-Aug-2018   RMM      Initial code  based on expected APIs.
**********************************************************************/

#ifndef __PSL_LINUX_H__
#define __PSL_LINUX_H__

#ifdef PSL_LINUX

/**
 * Linux specific header files
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * Linux specific macro definitons
 */
#define PSL_FILE_HANDLE_LIST_MAX				10
#define PSL_FILE_HANDLE_INVALID					0
#define PSL_LUT_ENTRY_SIZE						4

#endif /* PSL_LINUX */

#endif /*__PSL_LINUX_H__ */

