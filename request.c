#include "request.h"

void
cleanup(struct request_info *r) //TODO should probably use this
{
	printf("fd %d: closing\n", EVENT_FD(&r->rd_ev));

	evbuffer_free(r->buf);
	event_del(&r->wr_ev);
	event_del(&r->rd_ev);
	close(EVENT_FD(&r->rd_ev)); //TODO
	free(r);
}

void
send_error(struct request_info *r, int code)
{
	switch (code) {
	case 400:
		evbuffer_add_printf(r->buf, "HTTP/1.0 400 Bad Request\r\n");
		break;
	case 403:
		evbuffer_add_printf(r->buf, "HTTP/1.0 403 Forbidden\r\n");
		break;
	case 404:
		evbuffer_add_printf(r->buf, "HTTP/1.0 404 Not Found\r\n");
		break;
	case 405:
		evbuffer_add_printf(r->buf, "HTTP/1.0 405 Method Not Allowed\r\n");
		break;
	case 500:
		evbuffer_add_printf(r->buf, "HTTP/1.0 500 Internal Server Error\r\n");
		break;
	}
	add_headers(r->buf);
	event_add(&r->wr_ev, NULL);
}

void
read_request(int fd, short revents, void* parser)
{
  http_parser *p = parser;
  struct request_info *r = p->data;
  int len, nparsed;
  size_t bflen;
  char *request;

  len = evbuffer_read(r->buf, fd, 4096);
  switch (len) {
  case -1:
    switch (errno) {
    case EINTR:
    case EAGAIN:
      /* Wait for another call to read */
      return;
    case ECONNRESET:
      break;
    default:
      warn("read");
      break;
    }
    /* FALLTHROUGH */
  case 0:
    cleanup(r);
    // Nothing lets close the socket
    return;
  default:
    /* we've received part of a request */
    printf("fd %d: read %d bytes\n", fd, len);
    break;
  }

  /* convert evbuffer to a string */
  bflen = EVBUFFER_LENGTH(r->buf);
  request = malloc(bflen);
  len = evbuffer_remove(r->buf, request, bflen);
  if (len < 0)
    err(1,"evbuffer");

  printf("-======-\n");
  nparsed = http_parser_execute(parser, &(r->settings), request, len);

	/* Did we get a proper request? */
	if (nparsed != len) {
		printf("Improper HTTP request\n" );
		send_error(r, 400);
	}
}

void
add_headers(struct evbuffer* buf)
{
	char date_str[200];
  time_t t;
  struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL)
		errx(1, "localtime");

	if (strftime(date_str, sizeof(date_str), DATE_FORMAT, tmp) == 0)
		err(1, "strftime failed");

	evbuffer_add_printf(buf, "Date: %s\r\n" \
			"Server: mirrord/s4262468\r\n" \
			"Connection: close\r\n" \
			"\r\n\r\n", date_str);
}


void
write_out(int fd, short revents, void* parser)
{
  http_parser *p = parser;
  struct request_info *r = p->data;
  int len;

  len = evbuffer_write(r->buf, fd);
	// if (len < 0){
	// 	err(1,"evbuffer");
	// 	cleanup(r);
	// }

  printf("fd %d: wrote %d bytes\n", fd, len);
  if (len > 0) {
    event_add(&r->wr_ev, NULL);
	} else {
		// After write there's nothing else to do.
		cleanup(r);
	}

}
