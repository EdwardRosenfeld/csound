#include "csound.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    CSOUND  *csound;
    csoundInitialize(&argc, &argv, 0);
    csound = csoundCreate(NULL);
    char *name, *type;
    int n = 0;

    printf("Output ------------------------------\n");
    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int i,ndevs = csoundGetMIDIDevList(csound,NULL,1);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,1);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            printf(" %d: %s %s (%s)\n", i,
                   devs[i].device_id, devs[i].device_name, devs[i].interface_name);
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int i,ndevs = csoundGetAudioDevList(csound,NULL,1);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,1);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++)
                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
            free(devs);
        }
    }

    printf("\nInput ------------------------------\n");
    n = 0;
    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int i,ndevs = csoundGetMIDIDevList(csound,NULL,0);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,0);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++)
                printf(" %d: %s %s (%s)\n", i,
                       devs[i].device_id, devs[i].device_name, devs[i].interface_name);
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int i,ndevs = csoundGetAudioDevList(csound,NULL,0);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,0);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++)
                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
            free(devs);
        }
    }

    csoundDestroy(csound);
    return 0;
}

