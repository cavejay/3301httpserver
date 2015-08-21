#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include "request.h"
#include "http-parser/http_parser.h"

int read_from_file(struct request_info *);

#endif /* FILE_HANDLER_H */
