/*
    socksend.c:

    Copyright (C) 2006 by John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include <sys/types.h>
#ifdef __OS_WINDOWS__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

extern  int     inet_aton(const char *cp, struct in_addr *inp);

typedef struct {
    OPDS    h;
    MYFLT   *asig, *ipaddress, *port, *buffersize;
    AUXCH   aux;
    int     sock, conn;
    int     bsize, wp;
    struct sockaddr_in server_addr;
} SOCKSEND;

typedef struct {
    OPDS    h;
    MYFLT   *asigl, *asigr, *ipaddress, *port, *buffersize;
    AUXCH   aux;
    int     sock, conn;
    int     bsize, wp;
    struct sockaddr_in server_addr;
} SOCKSENDS;

#define MTU (1456)

/* UDP version one channel */
static int init_send(CSOUND *csound, SOCKSEND *p)
{
    int     bsize;

    p->bsize = bsize = (int) *p->buffersize;
    if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) {
      return csound->InitError(csound, Str("The buffersize must be <= %d samples "
                                           "to fit in a udp-packet."),
                               (int) (MTU / sizeof(MYFLT)));
    }
    p->wp = 0;

    p->sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    inet_aton((const char *) p->ipaddress,
              &p->server_addr.sin_addr);    /* the server IP address */
    p->server_addr.sin_port = htons((int) *p->port);    /* the port */

    /* create a buffer to write the interleaved audio to  */
    if (p->aux.auxp == NULL || (long) (bsize * sizeof(MYFLT)) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * sizeof(MYFLT)), &p->aux);
    else {
      memset(p->aux.auxp, 0, sizeof(MYFLT) * bsize);
    }
    return OK;
}

static int send_send(CSOUND *csound, SOCKSEND *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    int     i, wp, ksmps = csound->ksmps;
    int     buffersize = p->bsize;
    MYFLT   *asig = p->asig;
    MYFLT   *out = (MYFLT *) p->aux.auxp;

    for (i = 0, wp = p->wp; i < ksmps; i++, wp++) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, out, buffersize * sizeof(MYFLT), 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, Str("sendto failed"));
        }
        wp = 0;
      }
      out[wp] = asig[i];
    }
    p->wp = wp;

    return OK;
}

/* UDP version 2 channels */
static int init_sendS(CSOUND *csound, SOCKSENDS *p)
{
    int     bsize;

    p->bsize = bsize = (int) *p->buffersize;
    if (UNLIKELY((sizeof(MYFLT) * bsize) > MTU)) {
      return csound->InitError(csound, Str("The buffersize must be <= %d samples "
                                           "to fit in a udp-packet."),
                               (int) (MTU / sizeof(MYFLT)));
    }
    p->wp = 0;

    p->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }
    /* create server address: where we want to send to and clear it out */
    memset(&p->server_addr, 0, sizeof(p->server_addr));
    p->server_addr.sin_family = AF_INET;    /* it is an INET address */
    inet_aton((const char *) p->ipaddress,
              &p->server_addr.sin_addr);    /* the server IP address */
    p->server_addr.sin_port = htons((int) *p->port);    /* the port */

    /* create a buffer to write the interleaved audio to */
    if (p->aux.auxp == NULL || (long) (bsize * sizeof(MYFLT)) > p->aux.size)
      /* allocate space for the buffer */
      csound->AuxAlloc(csound, (bsize * sizeof(MYFLT)), &p->aux);
    else {
      memset(p->aux.auxp, 0, sizeof(MYFLT) * bsize);
    }
    return OK;
}

static int send_sendS(CSOUND *csound, SOCKSENDS *p)
{
    const struct sockaddr *to = (const struct sockaddr *) (&p->server_addr);
    MYFLT   *asigl = p->asigl;
    MYFLT   *asigr = p->asigr;
    MYFLT   *out = (MYFLT *) p->aux.auxp;
    int     i;
    int     buffersize = p->bsize;
    int     wp, ksmps = csound->ksmps;

    /* store the samples of the channels interleaved in the packet */
    /* (left, right) */
    for (i = 0, wp = p->wp; i < ksmps; i++, wp += 2) {
      if (wp == buffersize) {
        /* send the package when we have a full buffer */
        if (UNLIKELY(sendto(p->sock, out, buffersize * sizeof(MYFLT), 0, to,
                            sizeof(p->server_addr)) < 0)) {
          return csound->PerfError(csound, Str("sendto failed"));
        }
        wp = 0;
      }
      out[wp] = asigl[i];
      out[wp + 1] = asigr[i];
    }
    p->wp = wp;

    return OK;
}

/* TCP version */
static int init_ssend(CSOUND *csound, SOCKSEND *p)
{
    socklen_t clilen;

    /* create a STREAM (TCP) socket in the INET (IP) protocol */
    p->sock = socket(PF_INET, SOCK_STREAM, 0);

    if (UNLIKELY(p->sock < 0)) {
      return csound->InitError(csound, Str("creating socket"));
    }

    /* create server address: where we want to connect to */

    /* clear it out */
    memset(&(p->server_addr), 0, sizeof(p->server_addr));

    /* it is an INET address */
    p->server_addr.sin_family = AF_INET;

    /* the server IP address, in network byte order */
    inet_aton((const char *) p->ipaddress, &(p->server_addr.sin_addr));

    /* the port we are going to listen on, in network byte order */
    p->server_addr.sin_port = htons((int) *p->port);

    /* associate the socket with the address and port */
    if (UNLIKELY(bind
        (p->sock, (struct sockaddr *) &p->server_addr, sizeof(p->server_addr))
                 < 0)) {
      return csound->InitError(csound, Str("bind failed"));
    }

    /* start the socket listening for new connections -- may wait */
    if (UNLIKELY(listen(p->sock, 5) < 0)) {
      return csound->InitError(csound, Str("listen failed"));
    }
    clilen = sizeof(p->server_addr);
    p->conn = accept(p->sock, (struct sockaddr *) &p->server_addr, &clilen);

    if (UNLIKELY(p->conn < 0)) {
      return csound->InitError(csound, Str("accept failed"));
    }
    return OK;
}

static int send_ssend(CSOUND *csound, SOCKSEND *p)
{
    int     n = sizeof(MYFLT) * csound->ksmps;

    if (UNLIKELY(n != write(p->conn, p->asig, sizeof(MYFLT) * csound->ksmps))) {
      return csound->PerfError(csound, Str("write to socket failed"));
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "socksend", S(SOCKSEND), 5, "", "aSii", (SUBR) init_send, NULL,
    (SUBR) send_send },
  { "socksends", S(SOCKSENDS), 5, "", "aaSii", (SUBR) init_sendS, NULL,
    (SUBR) send_sendS },
  { "stsend", S(SOCKSEND), 5, "", "aSi", (SUBR) init_ssend, NULL,
    (SUBR) send_ssend }
};

LINKAGE

