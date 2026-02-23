// This includes all the GNU things we dont have, just to pass compilation
#pragma once

#include <byteswap.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_mode_t.h>
#include <sys/_types/_off_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/acl.h>
#include <time.h>

int pipe2(int fd[2], int flags);

typedef struct tm_zone *timezone_t;

void tzfree (timezone_t tz);
struct tm * localtime_rz (timezone_t tz, time_t const *t, struct tm *tm);

timezone_t tzalloc(const char *zone);

time_t mktime_z(timezone_t restrict tz, struct tm *restrict tm);

ssize_t copy_file_range (int infd, off_t *pinoff,
  int outfd, off_t *poutoff,
  size_t length, unsigned int flags);

extern char **environ;

static inline int stdc_bit_width_ul(unsigned long x)
{
  if (x == 0) return 0;
  return (sizeof(unsigned long) * CHAR_BIT) - __builtin_clzl(x);
}

static inline int stdc_bit_width(unsigned int x)
{
  if (x == 0) return 0;
  return (sizeof(unsigned int) * CHAR_BIT) - __builtin_clz(x);
}

static inline int stdc_count_ones(unsigned int x)
{
  return __builtin_popcount(x);
}

static inline int stdc_leading_zeros(unsigned int x)
{
  if (x == 0) return sizeof(unsigned int) * CHAR_BIT;
  return __builtin_clz(x);
}

static inline int stdc_trailing_zeros(unsigned int x)
{
  if (x == 0) return sizeof(unsigned int) * CHAR_BIT;
  return __builtin_ctz(x);
}

///////////

#define bswap_16 __builtin_bswap16
#define bswap_32 __builtin_bswap32
#define bswap_64 __builtin_bswap64

///////////

static inline void *
mempcpy (void *dest, const void *src, size_t n)
{
  return (char *) memcpy (dest, src, n) + n;
}


/* memrchr -- find the last occurrence of a byte in a memory block

   Copyright (C) 1991, 1993, 1996-1997, 1999-2000, 2003-2026 Free Software
   Foundation, Inc.

   Based on strlen implementation by Torbjorn Granlund (tege@sics.se),
   with help from Dan Sahlin (dan@sics.se) and
   commentary by Jim Blandy (jimb@ai.mit.edu);
   adaptation to memchr suggested by Dick Karpinski (dick@cca.ucsf.edu),
   and implemented by Roland McGrath (roland@ai.mit.edu).

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Search no more than N bytes of S for C.  */
static inline void *
memrchr (void const *s, int c_in, size_t n)
{
  /* On 32-bit hardware, choosing longword to be a 32-bit unsigned
     long instead of a 64-bit uintmax_t tends to give better
     performance.  On 64-bit hardware, unsigned long is generally 64
     bits already.  Change this typedef to experiment with
     performance.  */
  typedef unsigned long int longword;

  unsigned char c = (unsigned char) c_in;

  const longword *longword_ptr;

  /* Handle the last few bytes by reading one byte at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  {
    const unsigned char *char_ptr;
    for (char_ptr = (const unsigned char *) s + n;
         n > 0 && (size_t) char_ptr % sizeof (longword) != 0;
         --n)
      if (*--char_ptr == c)
        return (void *) char_ptr;

    longword_ptr = (const void *) char_ptr;
  }

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to any size longwords.  */
  {
    /* Compute auxiliary longword values:
       repeated_one is a value which has a 1 in every byte.
       repeated_c has c in every byte.  */
    longword repeated_one = 0x01010101;
    longword repeated_c = c | (c << 8);
    repeated_c |= repeated_c << 16;
    if (0xffffffffU < (longword) -1)
      {
        repeated_one |= repeated_one << 31 << 1;
        repeated_c |= repeated_c << 31 << 1;
        if (8 < sizeof (longword))
          for (size_t i = 64; i < sizeof (longword) * 8; i *= 2)
            {
              repeated_one |= repeated_one << i;
              repeated_c |= repeated_c << i;
            }
      }

    /* Instead of the traditional loop which tests each byte, we will test a
       longword at a time.  The tricky part is testing if *any of the four*
       bytes in the longword in question are equal to c.  We first use an xor
       with repeated_c.  This reduces the task to testing whether *any of the
       four* bytes in longword1 is zero.

       We compute tmp =
         ((longword1 - repeated_one) & ~longword1) & (repeated_one << 7).
       That is, we perform the following operations:
         1. Subtract repeated_one.
         2. & ~longword1.
         3. & a mask consisting of 0x80 in every byte.
       Consider what happens in each byte:
         - If a byte of longword1 is zero, step 1 and 2 transform it into 0xff,
           and step 3 transforms it into 0x80.  A carry can also be propagated
           to more significant bytes.
         - If a byte of longword1 is nonzero, let its lowest 1 bit be at
           position k (0 <= k <= 7); so the lowest k bits are 0.  After step 1,
           the byte ends in a single bit of value 0 and k bits of value 1.
           After step 2, the result is just k bits of value 1: 2^k - 1.  After
           step 3, the result is 0.  And no carry is produced.
       So, if longword1 has only non-zero bytes, tmp is zero.
       Whereas if longword1 has a zero byte, call j the position of the least
       significant zero byte.  Then the result has a zero at positions 0, ...,
       j-1 and a 0x80 at position j.  We cannot predict the result at the more
       significant bytes (positions j+1..3), but it does not matter since we
       already have a non-zero bit at position 8*j+7.

     So, the test whether any byte in longword1 is zero is equivalent to
     testing whether tmp is nonzero.  */

    while (n >= sizeof (longword))
      {
        longword longword1 = *--longword_ptr ^ repeated_c;

        if ((((longword1 - repeated_one) & ~longword1)
             & (repeated_one << 7)) != 0)
          {
            longword_ptr++;
            break;
          }
        n -= sizeof (longword);
      }
  }

  {
    const unsigned char *char_ptr = (const unsigned char *) longword_ptr;

    /* At this point, we know that either n < sizeof (longword), or one of the
       sizeof (longword) bytes starting at char_ptr is == c.  On little-endian
       machines, we could determine the first such byte without any further
       memory accesses, just by looking at the tmp result from the last loop
       iteration.  But this does not work on big-endian machines.  Choose code
       that works in both cases.  */
    while (n-- > 0)
      {
        if (*--char_ptr == c)
          return (void *) char_ptr;
      }
  }

  return NULL;
}

static inline const char *sigdescr_np(int sig)
{
  return strsignal(sig);
}

#define streq(a, b) (strcmp((a), (b)) == 0)

#ifndef SIZE_WIDTH
#define SIZE_WIDTH (sizeof(size_t) * CHAR_BIT)
#endif

#ifndef PTRDIFF_WIDTH
#define PTRDIFF_WIDTH (sizeof(ptrdiff_t) * CHAR_BIT)
#endif

#ifndef INTMAX_WIDTH
#define INTMAX_WIDTH (sizeof(intmax_t) * CHAR_BIT)
#endif

#ifndef UINTMAX_WIDTH
#define UINTMAX_WIDTH (sizeof(uintmax_t) * CHAR_BIT)
#endif

#ifndef INT_LEAST32_WIDTH
#define INT_LEAST32_WIDTH (sizeof(int_least32_t) * CHAR_BIT)
#endif

#ifndef UINT_FAST32_WIDTH
#define UINT_FAST32_WIDTH (sizeof(uint_fast32_t) * CHAR_BIT)
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

// ssize_t
// getrandom (void *buffer, size_t length, unsigned int flags);

#define O_IGNORE_CTTY 0

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 1
#define GRND_RANDOM 2
#endif

#ifndef GT_FILE
#define GT_FILE     0
#define GT_DIR      1
#define GT_NOCREATE 2
#endif

// Stub
static inline int set_binary_mode(int fd, int mode)
{
  (void)fd;
  (void)mode;
  return O_BINARY;  // which is 0
}

// Stub
static inline int qcopy_acl(const char *src, int src_fd,
                             const char *dst, int dst_fd, mode_t mode)
{
  (void)src; (void)src_fd;
  (void)dst; (void)dst_fd;
  (void)mode;
  return 0;  // Don't copy ACLs for now
}

static inline bool acl_errno_valid(int errnum)
{
  (void)errnum;
  return false;  // treat all ACL errors as "not supported"
}

static inline long num_processors(int query)
{
  (void)query;
  return 1;
}

static inline ptrdiff_t nstrftime(char *restrict s, size_t maxsize,
                    const char *restrict format,
                    const struct tm *tp, timezone_t tz, int ns)
{
  (void)tz;
  (void)ns;
  return (ptrdiff_t)strftime(s, maxsize, format, tp);
}

static inline ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
  (void)flags;
  arc4random_buf(buf, buflen);
  return (ssize_t)buflen;
}

static inline size_t __fpending(FILE *fp)
{
  (void)fp;
  return 0;  // assume nothing pending — close will flush anyway
}

/* Termcap/terminfo functions — declared here instead of including
   <term.h> which pollutes the namespace with macros for every
   capability name (e.g., `lines`, `columns`).  */
extern char *tparm(const char *, ...);
extern int tputs(const char *, int, int (*)(int));
extern char *tgetstr(const char *, char **);
extern int tgetnum(const char *);
extern int tgetflag(const char *);
extern int tgetent(char *, const char *);
extern char PC;
#ifdef TERMINFO
extern char *tigetstr(const char *);
extern int tigetflag(const char *);
extern int tigetnum(const char *);
#endif
