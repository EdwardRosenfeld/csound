/**
 * jack_transport.c
 *
 * Copyright (c) 2008 by Cesare Marilungo. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * U S A G E
 *
 *  jack_transport 0 - to start jack transport
 *
 *  jack_transport 1 - to stop
 *
 *  jack_transport 2 - to rewind to 0.
 *
 */

#include <csdl.h> 
#include <jack/jack.h>
#include <jack/transport.h>

/* these struct definitions are taken from rtjack.c by Istvan Varga */
#define MAX_NAME_LEN    32      /* for client and port name */

typedef struct RtJackBuffer_ {
#ifdef LINUX
    pthread_mutex_t csndLock;               /* signaled by process callback */
    pthread_mutex_t jackLock;               /* signaled by audio thread     */
#else
    void    *csndLock;                      /* signaled by process callback */
    void    *jackLock;                      /* signaled by audio thread     */
#endif
    jack_default_audio_sample_t **inBufs;   /* 'nChannels' capture buffers  */
    jack_default_audio_sample_t **outBufs;  /* 'nChannels' playback buffers */
} RtJackBuffer;

typedef struct RtJackGlobals_ {
    CSOUND  *csound;                    /* Csound instance pointer          */
    int     jackState;                  /* 0: OK, 1: sr changed, 2: quit    */
    char    clientName[MAX_NAME_LEN + 1];       /* client name              */
    char    inputPortName[MAX_NAME_LEN + 1];    /* input port name prefix   */
    char    outputPortName[MAX_NAME_LEN + 1];   /* output port name prefix  */
    int     sleepTime;                  /* sleep time in us (deprecated)    */
    char    *inDevName;                 /* device name for -i adc           */
    char    *outDevName;                /* device name for -o dac           */
    int     sampleRate;                 /* sample rate in Hz                */
    int     nChannels;                  /* number of channels               */
    int     bufSize;                    /* buffer size in sample frames     */
    int     nBuffers;                   /* number of buffers (>= 2)         */
    int     inputEnabled;               /* non-zero if capture (adc) is on  */
    int     outputEnabled;              /* non-zero if playback (dac) is on */
    int     csndBufCnt;                 /* current buffer in Csound thread  */
    int     csndBufPos;                 /* buffer position in Csound thread */
    int     jackBufCnt;                 /* current buffer in JACK callback  */
    int     jackBufPos;                 /* buffer position in JACK callback */
    jack_client_t   *client;            /* JACK client pointer              */
    jack_port_t     **inPorts;          /* 'nChannels' ports for capture    */
    jack_default_audio_sample_t **inPortBufs;
    jack_port_t     **outPorts;         /* 'nChannels' ports for playback   */
    jack_default_audio_sample_t **outPortBufs;
    RtJackBuffer    **bufs;             /* 'nBuffers' I/O buffers           */
    int     xrunFlag;                   /* non-zero if an xrun has occured  */
} RtJackGlobals;
/* end of code by Istvan Varga */


enum { START, STOP, REWIND };

typedef struct
{
  OPDS h;
  MYFLT *command ;
} JACKTRANSPORT;

static int jack_transport (CSOUND *csound, JACKTRANSPORT * p)
{
    RtJackGlobals *rtjack;
    jack_client_t *client;

    rtjack = (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                                 "_rtjackGlobals");
    client = rtjack->client;

    if (client == NULL) {
      csound->InitError(csound, Str("Cannot find Jack client.\n"));
      return NOTOK;
  }
    else {
      switch ((int)(*p->command)){
      case START: 
        csound->Message(csound, Str("jack_transport playing.\n"));
        jack_transport_start(client);
        break;
      case STOP:
        csound->Message(csound, Str("jack_transport stopped.\n"));
        jack_transport_stop(client);
        break;
      case REWIND:
        csound->Message(csound, Str("jack_transport rewinded.\n"));
        jack_transport_locate(client, 0);
        break;
      default:
        csound->Message(csound, Str("jack_transport: invalid parameter.\n"));
        break;
      }
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {

  { "jack_transport",  S(JACKTRANSPORT),  1, "", "i",
                       (SUBR)jack_transport, NULL, NULL   },

};

LINKAGE
