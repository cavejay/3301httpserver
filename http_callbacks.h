#ifndef HTTP_CALLBACKS_H
#define HTTP_CALLBACKS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "http-parser/http_parser.h"
#include "request.h"
#include "file_handler.h"

int url_cb(http_parser *, const char *, size_t);
int body_cb(http_parser *, const char *, size_t);
int header_field_cb(http_parser *, const char *, size_t);
int header_value_cb(http_parser *, const char *, size_t);
int headers_complete_cb(http_parser *);
int message_begin_cb(http_parser *);
int message_complete_cb(http_parser *);
int status_cb(http_parser *, const char *, size_t);
int chunk_header_cb(http_parser *);
int chunk_complete_cb(http_parser *);

#endif /* HTTP_CALLBACKS_H */
