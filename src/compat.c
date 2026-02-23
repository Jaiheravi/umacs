/* Everything under this license unless explicitly said otherwise
   Copyright 2019-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lisp.h"


////////////////////////////////////////////////////////////////////////////////
// Create a pipe, with specific opening flags.
int pipe2(int fd[2], int flags)
{
  /* https://austingroupbugs.net/view.php?id=467  */
  int tmp[2];
  tmp[0] = fd[0];
  tmp[1] = fd[1];

  /* Check the supported flags. */
  if ((flags & ~(O_CLOEXEC | O_NONBLOCK)) != 0)
  {
    errno = EINVAL;
    return -1;
  }

  /* Unix API.  */

  if (pipe(fd) < 0)
    return -1;

  /* POSIX
     <https://pubs.opengroup.org/onlinepubs/9699919799/functions/pipe.html> says
     that initially, the O_NONBLOCK and FD_CLOEXEC flags are cleared on both
     fd[0] and fd[1].  */

  /* O_NONBLOCK handling.
     On Unix platforms, O_NONBLOCK is defined by the system.  Use fcntl().  */
  if (flags & O_NONBLOCK)
  {
    int fcntl_flags;

    if ((fcntl_flags = fcntl(fd[1], F_GETFL, 0)) < 0 ||
        fcntl(fd[1], F_SETFL, fcntl_flags | O_NONBLOCK) == -1 ||
        (fcntl_flags = fcntl(fd[0], F_GETFL, 0)) < 0 ||
        fcntl(fd[0], F_SETFL, fcntl_flags | O_NONBLOCK) == -1)
      goto fail;
  }

  if (flags & O_CLOEXEC)
  {
    int fcntl_flags;

    if ((fcntl_flags = fcntl(fd[1], F_GETFD, 0)) < 0 ||
        fcntl(fd[1], F_SETFD, fcntl_flags | FD_CLOEXEC) == -1 ||
        (fcntl_flags = fcntl(fd[0], F_GETFD, 0)) < 0 ||
        fcntl(fd[0], F_SETFD, fcntl_flags | FD_CLOEXEC) == -1)
      goto fail;
  }

  return 0;

fail:
  {
    int saved_errno = errno;
    close(fd[0]);
    close(fd[1]);
    fd[0] = tmp[0];
    fd[1] = tmp[1];
    errno = saved_errno;
    return -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// copy_file_range stub

ssize_t
copy_file_range (int infd, off_t *pinoff,
                 int outfd, off_t *poutoff,
                 size_t length, unsigned int flags) {
#undef copy_file_range
  /* There is little need to emulate copy_file_range with read+write,
     since programs that use copy_file_range must fall back on
     read+write anyway.  */
  errno = ENOSYS;
  return -1;
}

int dtoastr(char *buf, size_t bufsize, int flags, int width, double x)
{
  (void)flags;
  (void)width;
  return snprintf(buf, bufsize, "%.17g", x);
}

struct face; // forward declaration if needed

int face_for_font(struct frame *f, Lisp_Object font, struct face *base)
{
  (void)f; (void)font; (void)base;
  return 0;  // default face
}


void filemodestring(const struct stat *st, char *buf)
{
  const mode_t m = st->st_mode;

  // file type
  buf[0] = S_ISDIR(m)  ? 'd' :
           S_ISLNK(m)  ? 'l' :
           S_ISCHR(m)  ? 'c' :
           S_ISBLK(m)  ? 'b' :
           S_ISFIFO(m) ? 'p' :
           S_ISSOCK(m) ? 's' : '-';

  // owner
  buf[1] = (m & S_IRUSR) ? 'r' : '-';
  buf[2] = (m & S_IWUSR) ? 'w' : '-';
  buf[3] = (m & S_ISUID) ? ((m & S_IXUSR) ? 's' : 'S')
                          : ((m & S_IXUSR) ? 'x' : '-');
  // group
  buf[4] = (m & S_IRGRP) ? 'r' : '-';
  buf[5] = (m & S_IWGRP) ? 'w' : '-';
  buf[6] = (m & S_ISGID) ? ((m & S_IXGRP) ? 's' : 'S')
                          : ((m & S_IXGRP) ? 'x' : '-');
  // other
  buf[7] = (m & S_IROTH) ? 'r' : '-';
  buf[8] = (m & S_IWOTH) ? 'w' : '-';
  buf[9] = (m & S_ISVTX) ? ((m & S_IXOTH) ? 't' : 'T')
                          : ((m & S_IXOTH) ? 'x' : '-');
  buf[10] = '\0';
}

int filenvercmp(const char *a, size_t alen, const char *b, size_t blen)
{
  (void)alen; (void)blen;
  return strcmp(a, b);
}
