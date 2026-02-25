/* Determine the time when the machine last booted.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

#include <config.h>

/* Specification.  */
#include "boot-time.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_SYS_SYSCTL_H && !(defined __GLIBC__ && defined __linux__) && !defined __minix
# if HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif
# include <sys/sysctl.h>
#endif

#include "idx.h"
#include "readutmp.h"
#include "stat-time.h"

/* Each of the FILE streams in this file is only used in a single thread.  */
#include "unlocked-io.h"

/* Some helper functions.  */
#include "boot-time-aux.h"

/* The following macros describe the 'struct UTMP_STRUCT_NAME',
   *not* 'struct gl_utmp'.  */
#undef UT_USER

/* Accessor macro for the member named ut_user or ut_name.  */
#if (HAVE_UTMPX_H ? HAVE_STRUCT_UTMPX_UT_NAME \
     : HAVE_UTMP_H && HAVE_STRUCT_UTMP_UT_NAME)
# define UT_USER(UT) ((UT)->ut_name)
#else
# define UT_USER(UT) ((UT)->ut_user)
#endif

#if !HAVE_UTMPX_H && HAVE_UTMP_H && defined UTMP_NAME_FUNCTION
# if !HAVE_DECL_ENDUTENT /* Android */
void endutent (void);
# endif
#endif

static int
get_boot_time_uncached (struct timespec *p_boot_time)
{
  struct timespec found_boot_time = {0};

  /* Try to find the boot time in the /var/run/utmp file.  */

  /* Ignore the return value for now.
     Solaris' utmpname returns 1 upon success -- which is contrary
     to what the GNU libc version does.  In addition, older GNU libc
     versions are actually void.   */
  UTMP_NAME_FUNCTION ((char *) UTMP_FILE);

  SET_UTMP_ENT ();

  void const *entry;

  while ((entry = GET_UTMP_ENT ()) != NULL)
    {
      struct UTMP_STRUCT_NAME const *ut = (struct UTMP_STRUCT_NAME const *) entry;

      struct timespec ts =
        #if (HAVE_UTMPX_H ? 1 : HAVE_STRUCT_UTMP_UT_TV)
        { .tv_sec = ut->ut_tv.tv_sec, .tv_nsec = ut->ut_tv.tv_usec * 1000 };
        #else
        { .tv_sec = ut->ut_time, .tv_nsec = 0 };
        #endif

      if (ut->ut_type == BOOT_TIME)
        found_boot_time = ts;

    }

  END_UTMP_ENT ();

# if HAVE_SYS_SYSCTL_H && HAVE_SYSCTL \
     && defined CTL_KERN && defined KERN_BOOTTIME \
     && !defined __minix
  if (found_boot_time.tv_sec == 0)
    {
      get_bsd_boot_time_final_fallback (&found_boot_time);
    }
# endif

  if (found_boot_time.tv_sec != 0)
    {
      *p_boot_time = found_boot_time;
      return 0;
    }
  else
    return -1;
}

int
get_boot_time (struct timespec *p_boot_time)
{
  /* Cache the result from get_boot_time_uncached.  */
  static int volatile cached_result = -1;
  static struct timespec volatile cached_boot_time;

  if (cached_result < 0)
    {
      struct timespec boot_time;
      int result = get_boot_time_uncached (&boot_time);
      cached_boot_time = boot_time;
      cached_result = result;
    }

  if (cached_result == 0)
    {
      *p_boot_time = cached_boot_time;
      return 0;
    }
  else
    return -1;
}
