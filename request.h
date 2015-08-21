#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <event.h>
#include <errno.h>
#include <time.h>
#include <err.h>
#include "http-parser/http_parser.h"

// Structures
struct request_info {
  int logfd;
  const char *logfile; // log file

  char *file; // filename to get
  const char *dir; // the directory to share
  int read_failed;

  int fd; // for the connection

  struct event		rd_ev;
	struct event		wr_ev;
	struct evbuffer		*buf;

  http_parser_settings settings;

  char* ip;
  char* date_str;
  char* bare_url;
  int code;
  int size;
};

#define DATE_FORMAT "%a, %d %b %Y %X %z"

void cleanup(struct request_info *);
void send_error(struct request_info *, int);
void read_request(int, short, void *);
void add_headers(struct evbuffer *);
void write_out(int, short, void *);



#endif /* REQUEST_H */
