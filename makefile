.PATH: ${.CURDIR}/http-parser

PROG=			mirrord
SRCS=			mirrord.c
SRCS+=		http_parser.c
SRCS+=		request.c
SRCS+=		http_callbacks.c
SRCS+=		file_handler.c
LDADD=		-levent
DPADD=		${LIBEVENT}
CFLAGS+=	-Wall
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= 	-Wmissing-declarations
CFLAGS+= 	-Wshadow -Wpointer-arith
CFLAGS+=	-Wsign-compare
CFLAGS+=	-std=gnu99 -g
MAN=

.include <bsd.prog.mk>
