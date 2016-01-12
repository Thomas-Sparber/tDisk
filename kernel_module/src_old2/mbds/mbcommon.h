/*
 *  mbcommon.h - common definitions
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef MBCOMMON_H
#define MBCOMMON_H

typedef int    (*MBcmpfn)(const void *, const void *);
typedef void   (*MBforfn)(void *);
typedef void   (*MBdeletefn)(void *);
typedef void * (*MBgetfn)(void *);

//Not valid in kernel space
//typedef void   (*MBprintfn)(const void *, FILE *);

#endif /* MBCOMMON_H */
