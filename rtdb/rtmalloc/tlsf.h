
/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.2.0
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 */

#ifndef _TLSF_H_
#define _TLSF_H_

// The  debug functions  only can  be used  when _DEBUG_TLSF_  is set.
//#define _DEBUG_TLSF_

// Some IMPORTANT TLSF parameters
// Unlike the preview TLSF versions, now they are statics
#define MAX_FLI 30
#define MAX_SLI 32
#define MAX_LOG2_SLI 5
#define MAX_SLI 32		/* MAX_SLI = 2^MAX_LOG2_SLI */

#include <stddef.h>
#include <stdio.h>

#define PRINT_MSG(fmt, args...) printf(fmt, ## args)
#define ERROR_MSG(fmt, args...) printf(fmt, ## args)
//fprintf(stderr, fmt, ## args)

// TYPES used by TLSF
typedef unsigned long u32_t;
typedef signed long s32_t;
typedef unsigned char u8_t;

extern int init_memory_pool(size_t, void *);
extern void destroy_memory_pool(void *);
extern void *malloc_ex(size_t, void *);
extern void free_ex(void *, void *);
extern void *realloc_ex(void *, size_t, void *);
extern void *calloc_ex(size_t, size_t, void *);

#endif
