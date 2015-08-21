#include "http_callbacks.h"

int
url_cb(http_parser* parser, const char* chunk, size_t len)
{
  struct request_info *r = parser->data;
  size_t dir_len = strlen(r->dir);
  char *url = malloc(len+dir_len+1);


  strncpy(url, r->dir, dir_len);
  strncpy(&url[dir_len], chunk, len);
  url[len+dir_len] = '\0';

  r->file = url;
  printf("File we want is: %s\n", r->file);
  return 0;
}

int
body_cb(http_parser* parser, const char* chunk, size_t len)
{

  // printf("body_cb: %s\n", chunk);
  return 0;
}

int
header_field_cb(http_parser* parser, const char* chunk, size_t len)
{
  char *field = malloc(len+1);

  strncpy(field, chunk, len);
  field[len] = '\0';

  printf("Header: %s\n", field);
  return 0;
}

int
header_value_cb(http_parser* parser, const char* chunk, size_t len)
{
  char *value = malloc(len+1);

  strncpy(value, chunk, len);
  value[len] = '\0';

  printf("Header Value: %s\n", value);
  return 0;
}

int
headers_complete_cb(http_parser* parser)
{
  // printf("headercomplete\n");
  return 0;
}

int
message_begin_cb(http_parser* parser)
{
  struct request_info *r = parser->data;
  printf("begin\nThis request is a %d request\n", parser->method);
  switch (parser->method) {
  case HTTP_GET:
    printf("%s\n", "We got a GET request");
    break;
  case HTTP_HEAD:
    printf("%s\n","We got a HEAD request" );
    break;
  default:
    printf("Not a method we want\n" );
    // Send back an unsupported method signal
    send_error(r, 405);
  }

  return 0;
}

int
message_complete_cb(http_parser* parser)
{
  struct request_info *r = parser->data;
  struct stat file_stat;
  int exists;
  char datestring[256];
  struct tm *tm;

  if (strstr(r->file, "..") != NULL) {
    send_error(r, 404);
    return 0;
  }

  exists = stat(r->file, &file_stat);
  if (exists != 0){
    switch (errno){
    case EACCES:
      send_error(r, 403);
      break;
    default:
      send_error(r, 404);
      break;
    }
    return 0;

  } else { /* The file exists */
    /* Test if it's a folder */
    if (S_ISDIR(file_stat.st_mode) > 0) {
      send_error(r, 404);
      return 0;
    }

    tm = localtime(&file_stat.st_mtime);

    /* Get localized date string. */
    strftime(datestring, sizeof(datestring), DATE_FORMAT, tm);
    evbuffer_add_printf(r->buf, "HTTP/1.0 200 OK\r\n");
    evbuffer_add_printf(r->buf, "Content-Length: %9jd\r\n",
        (intmax_t)file_stat.st_size);
    evbuffer_add_printf(r->buf, "Last-Modified: %s\r\n", datestring);
    add_headers(r->buf);
  }

  if (parser->method == HTTP_HEAD) {
    event_add(&r->wr_ev, NULL);
  } else {
    // Open the file for parsing to body
    if (read_from_file(r) == -1) {
      evbuffer_drain(r->buf, EVBUFFER_LENGTH(r->buf));
      send_error(r, 500);
      return 0;
    }
    // Send reponse with body now :)
    event_add(&r->wr_ev, NULL);
  }

  printf("complete\n");
  return 0;
}

int
status_cb(http_parser* parser, const char* chunk, size_t len)
{
  char *status = malloc(len+1);

  strncpy(status, chunk, len);
  status[len] = '\0';

  printf("Status: %s\n", status);
  return 0;
}

int
chunk_header_cb(http_parser* parser)
{
  return 0;
}

int
chunk_complete_cb(http_parser* parser)
{
  return 0;
}
