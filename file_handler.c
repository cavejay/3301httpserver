#include "file_handler.h"

int
read_from_file(struct request_info *r)
{
  int fd, len;

  fd = open(r->file, O_RDONLY | O_NONBLOCK);
  if(fd == -1) {
    // give internal server error and cleanup
    warn("file open");
    r->read_failed = 1;
    return -1;
  }

  for(;;){
    len = evbuffer_read(r->buf, fd, 4096);
    switch (len) {
    case -1:
      switch (errno) {
      case EINTR:
      case EAGAIN:
        /* Wait for another call to read */
        return 0;
      case ECONNRESET:
        break;
      default:
        warn("read file");
        break;
      }
      /* FALLTHROUGH */
    case 0:
      // EOF let's packup
      close(fd);
      return 0;
    default:
      /* we've received part of a request */
      printf("Parsing-> fd %d: read %d bytes\n", fd, len);
      break;
    }
  }

  close(fd);
  event_add(&r->wr_ev, NULL);
  return 0;
}
