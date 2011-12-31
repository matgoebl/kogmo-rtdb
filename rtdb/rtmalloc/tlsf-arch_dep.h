/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.2.0
 *
 * Written by Miguel Masmano Tello <mmasmano@disca.upv.es>
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

#ifndef	_ARCH_DEP_H_
#define _ARCH_DEP_H_

#ifdef _IA32_

static inline int _ffs(int x) {
        int r;

        __asm__("bsfl %1,%0\n\t"
                "jnz 1f\n\t"
                "movl $-1,%0\n"
                "1:" : "=r" (r) : "g" (x));
        return r;
}

static inline int _fls(int x) {
        int r;

        __asm__("bsrl %1,%0\n\t"
                "jnz 1f\n\t"
		"movl $-1,%0\n"
                "1:" : "=r" (r) : "g" (x));
        return r;
}

#else

static inline int _ffs (int x) {
  int r = 0;
  
  if (!x)
    return -1;
  
  if (!(x & 0xffff)) {
    x >>= 16;
    r += 16;
  }

  if (!(x & 0xff)) {
    x >>= 8;
    r += 8;
  }

  if (!(x & 0xf)) {
    x >>= 4;
    r += 4;
  }

  if (!(x & 0x3)) {
    x >>= 2;
    r += 2;
  }

  if (!(x & 0x1)) {
    x >>= 1;
    r += 1;
  }

  return r;
}

static inline int _fls (int x) {
  int r = 31;

  if (!x)
    return -1;
  
  if (!(x & 0xffff0000)) {
    x <<= 16;
    r -= 16;
  }
  if (!(x & 0xff000000)) {
    x <<= 8;
    r -= 8;
  }
  if (!(x & 0xf0000000)) {
    x <<= 4;
    r -= 4;
  }
  if (!(x & 0xc0000000)) {
    x <<= 2;
    r -= 2;
  }
  if (!(x & 0x80000000)) {
    x <<= 1;
    r -= 1;
  }
  return r;
}

#endif

#endif 	    /* !_ARCH_DEP_H_ */
