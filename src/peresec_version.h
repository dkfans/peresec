/******************************************************************************/
// PE/DLL Rebuilder of Export Section
/******************************************************************************/
/** @file peresec_version.h
 *     Project name, version, copyrights and global definitions.
 * @par Purpose:
 *     Header file for global names and defines used by resource compiler.
 * @par Comment:
 *     Can only contain commands which resource compiler can understand.
 * @author   Tomasz Lis
 * @date     08 Jan 2010 - 20 Apr 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef PERESEC_VERSION_H
#define PERESEC_VERSION_H

/* Version definitions */
#include "../obj/ver_defs.h"
//#define VER_MAJOR         1
//#define VER_MINOR         2
//#define VER_RELEASE       3
//#define VER_BUILD         4
//#define VER_STRING        "1.2.3.4"

/* Program name, copyrights and file names */
#define PROGRAM_NAME      "PEReSec"
#define PROGRAM_FULL_NAME "PE/DLL Rebuilder of Export Section"
#define PROGRAM_AUTHORS   "Tomasz Lis"
#define COMPANY_NAME      "Community based"
#define INTERNAL_NAME     "peresec"
#define LEGAL_COPYRIGHT   "GNU General Public License"
#define LEGAL_TRADEMARKS  ""
#define FILE_VERSION VER_STRING
#define FILE_DESCRIPTION PROGRAM_NAME
#define ORIGINAL_FILENAME INTERNAL_NAME".exe"
#define PRODUCT_NAME PROGRAM_FULL_NAME
#define PRODUCT_VERSION    VER_STRING
#define DEFAULT_LOG_FILENAME INTERNAL_NAME".log"

#endif /*PERESEC_VERSION_H*/
/******************************************************************************/
