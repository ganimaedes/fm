#include "copy.h"

// https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
int cp(const char *from, const char *to)
{
  int fd_to, fd_from;
  char buf[4096];
  ssize_t nread;
  int saved_errno;

  fd_from = open(from, O_RDONLY);
  if (fd_from < 0) {
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "fd_from < 0\n");
    return -1;
  }

  fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd_to < 0) {
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "fd_to < 0\n");
    goto out_error;
  }

  while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
    char *out_ptr = buf;
    ssize_t nwritten;

    do {
      nwritten = write(fd_to, out_ptr, nread);

      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR) {
        fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
        fprintf(stdout, "errno != EINTR\n");
        goto out_error;
      }
    } while (nread > 0);
  }

  if (nread == 0) {
    if (close(fd_to) < 0) {
      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      fprintf(stdout, "close(fd_to) < 0\n");
      fd_to = -1;
      goto out_error;
    }
    close(fd_from);

    /* Success! */
    return 0;
  }

out_error:
  saved_errno = errno;

  close(fd_from);
  if (fd_to >= 0) {
    close(fd_to);
  }

  errno = saved_errno;
  return -1;
}
