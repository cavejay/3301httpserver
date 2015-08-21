/** s4262468 - Assignment 1 Comp3301 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h> // events
#include <err.h> // erroring
#include <errno.h> // errors and the like
#include <sys/queue.h>

#include <sys/types.h> // For network interface
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "http-parser/http_parser.h"
#include "request.h"
#include "http_callbacks.h"

struct conn {
  SLIST_ENTRY(conn) conns;
  int sock;
  struct event event;
};

SLIST_HEAD(SOCKS, conn) lsocks;

void usage(void); // __dead at front TODO
void start_listening(struct SOCKS *, int, const char *, const char *);
void accept_connection(int, short, void *);

void // __dead at front TODO
usage(void)
{
  extern char *__progname;
  fprintf(stderr, "usage: %s [-46d] [-a access.log] [-l address] [-p port] \
directory\n", __progname);
  exit(1);
}

int
main(int argc, char *argv[])
{
	int family = PF_INET;
	const char *logfile = NULL, *dir = NULL, *addr = "localhost", *port = "http";
	int opt, d = 0;
  struct conn *node;
  struct request_info *arg_info;

	while ((opt = getopt(argc, argv, "46da:l:p:")) != -1){
		switch (opt){
		case '4':
			family = PF_INET;
			break;
		case '6':
			family = PF_INET6;
			break;
    case 'd':
      d = 1;
      break;
		case 'a':
			logfile = optarg;
			break;
		case 'l':
			addr = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	switch (argc){
	case 1:
		dir = argv[0];
		break;
	default:
		usage();
	}

	/* Daemonise if we're going to. */
  if (d != 1 && daemon(0,0) == -1)
    errx(1, "Failed to perform daemonisation");

  /* Start listening on our address and port. */
  SLIST_INIT(&lsocks);
  start_listening(&lsocks, family, addr, port);

  /* Create a request_info struct to pass the logfile and dir to */
  arg_info = malloc(sizeof(*arg_info));
  arg_info->dir = dir;
  arg_info->logfile = logfile;

  /* Create an event listening on the server's socket */
  event_init();
  SLIST_FOREACH(node, &lsocks, conns){
    event_set(&(node->event), node->sock, EV_READ | EV_PERSIST, accept_connection, arg_info);
    event_add(&(node->event), NULL);
  }
  event_dispatch();
	return 0;
}

void
start_listening(struct SOCKS *headp, int family, const char* addr, const char* port)
{
  int on = 1;
  struct addrinfo hints, *res, *res0;
  struct conn *c;
  int error;
  int s, nsock;
  const char *cause = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  error = getaddrinfo(addr, port, &hints, &res0);
  if (error) {
    errx(1, "%s", gai_strerror(error));
    /*NOTREACHED*/
  }

  nsock = 0;
  for (res = res0; res; res = res->ai_next){
    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s < 0) {
      cause = "socket";
      continue;
    }

    if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int)) == -1) {
      cause = "sockopt";
      close(s);
      continue;
    }

    if (bind(s, res->ai_addr, res->ai_addrlen) < 0){
      cause = "bind";
      close(s);
      continue;
    }

    if (ioctl(s, FIONBIO, &on) == -1){
      cause = "listener ioctl(FIONBIO)";
      close(s);
      continue;
    }

    if (listen(s, 5) == -1){
      cause = "listen";
      close(s);
      continue;
    }

    c = malloc(sizeof(struct conn));
    c->sock = s;
    SLIST_INSERT_HEAD(headp, c, conns);

    nsock++;
  }
  if (nsock == 0){
    err(1, "%s", cause);
    /*NOTREACHED*/
  }

  freeaddrinfo(res0);
}

void
accept_connection(int sock, short revents, void *arg_info)
{
  struct sockaddr_storage ss;
  socklen_t socklen = sizeof(ss);
  int fd, on = 1;
  http_parser *parser;
  http_parser_settings settings;
  struct request_info *r;

  /* Accept the connection and check for errors */
  fd = accept(sock, (struct sockaddr *)&ss, &socklen);
  if (fd == -1) {
    switch (errno) {
    case EINTR:
    case EWOULDBLOCK:
    case ECONNABORTED:
      /* the connection dropped */
      return;
    default:
      err(1, "accept");
    }
  }

  /* Make sure it's non-blocking */
  if (ioctl(fd, FIONBIO, &on) == -1)
    err(1, "conn ioctl(FIONBIO)");

  // Temporary print statement stuff //TODO
  char hoststr[NI_MAXHOST]; char portstr[NI_MAXSERV];
  getnameinfo((struct sockaddr *)&ss, socklen, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
  printf("fd %d: accepted connection from %s:%s\n", fd, hoststr, portstr);
  fflush(stdout);

  // create the setup of the html_parser
	settings.on_url = url_cb;
  settings.on_body = body_cb;
  settings.on_header_field = header_field_cb;
  settings.on_header_value = header_value_cb;
  settings.on_headers_complete = headers_complete_cb;
  settings.on_message_begin = message_begin_cb;
  settings.on_message_complete = message_complete_cb;
  settings.on_status = status_cb;
  settings.on_chunk_header = chunk_header_cb;
  settings.on_chunk_complete = chunk_complete_cb;

  /* Make us a structure for this connection and it's events */
	r = malloc(sizeof(*r));
	if (r == NULL) {
		warn("request alloc");
		close(fd);
		return;
	}

  r->buf = evbuffer_new();
  if (r->buf == NULL) {
    warn("request buf alloc");
    free(r);
    close(fd);
    return;
  }

  /* set the log file and access file */
  r->fd = fd;
  r->read_failed = 0;
  r->dir = ((struct request_info *)arg_info)->dir;
  r->logfile = ((struct request_info *)arg_info)->logfile;
  r->settings = settings;

  /* initialise the parser to use on this connection */
  parser = malloc(sizeof(http_parser));
  http_parser_init(parser, HTTP_REQUEST);
  parser->data = r;

  /* set the events for this connection */
  event_set(&r->rd_ev, fd, EV_READ, read_request, parser);
  event_set(&r->wr_ev, fd, EV_WRITE, write_out, parser);

  event_add(&r->rd_ev, NULL);
}
