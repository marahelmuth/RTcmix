/* This is set up so that all vars are defined when this file is included
   by main.C (which defines the MAIN preprocessor symbol), and declared
   extern when included by all other files.
                                                              -JGG, 2/8/00
*/
#ifndef _GLOBALS_H_ 
#define _GLOBALS_H_ 1

#include "version.h"
#include "buffers.h"

/* MAXBUS is one greater than the total number of useable buses. */
#define MAXBUS 33

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

#ifdef __cplusplus

class RTQueue;
GLOBAL RTQueue *rtQueue;

extern "C" {
#endif /* __cplusplus */

#ifdef __GNUC__
#define INLINE inline    /* GNU C compiler can inline */
#else
#define INLINE           /* MIPSpro can't */
#endif

#include <rtupdate.h>

#ifdef LINUX
GLOBAL int in_port[MAXBUS];    /* array, in case sound driver uses many devs */
GLOBAL int out_port[MAXBUS];
#endif
#ifdef MACOSX
#define Boolean Mac_Boolean /* this allows compiling of RTcmix w/X11 on OSX */
#include <CoreAudio/AudioHardware.h>
#undef Boolean
GLOBAL AudioDeviceID in_port;
GLOBAL AudioDeviceID out_port;
#endif
#ifdef SGI
#include <dmedia/audio.h>
GLOBAL ALport in_port;
GLOBAL ALport out_port;
#endif

/* Note: these 3 vars also extern in rtdefs.h, for use by insts */
GLOBAL int NCHANS;
GLOBAL int RTBUFSAMPS;
GLOBAL float SR;

GLOBAL int audioNCHANS;


/* -------------------------------------------------------------------------- */
GLOBAL int noParse;
GLOBAL int record_audio;	// was audio_on
GLOBAL int play_audio;
GLOBAL int audio_config;
GLOBAL int rtInteractive;
GLOBAL int print_is_on;
GLOBAL int rtsetparams_called;

GLOBAL int output_data_format;
GLOBAL int output_header_type;
GLOBAL int normalize_output_floats;
GLOBAL int is_float_format;
GLOBAL char *rtoutsfname;

/* contains file descriptors for data files opened with infile
   and used by gen2 and gen3
*/
#include <stdio.h>
#define MAX_INFILE_DESC 50
GLOBAL FILE *infile_desc[MAX_INFILE_DESC + 1];

/* max amplitude encountered -- reported at end of run */
GLOBAL BUFTYPE peaks[MAXBUS];
GLOBAL long peaklocs[MAXBUS];
GLOBAL int report_clipping;
GLOBAL int check_peaks;

#ifdef NETAUDIO
GLOBAL int netplay;     // for remote sound network playing
#endif

/* for more than 1 socket, set by -s flag to CMIX as offset from MYPORT */
GLOBAL int socknew;

/* used in intraverse.C, traverse.C and rtsendsamps.c */
GLOBAL unsigned long bufStartSamp;
GLOBAL long elapsed;
typedef enum {
	RT_GOOD = 0, RT_SHUTDOWN = 1, RT_PANIC = 2, RT_ERROR = 3
} RTstatus;
GLOBAL RTstatus run_status;

#include <pthread.h>
#ifdef MAIN      /* Have to do this because must be inited in definition. */
pthread_mutex_t pfieldLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t endsamp_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t audio_config_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t aux_to_aux_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t to_aux_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t to_out_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inst_bus_config_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bus_config_status_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bus_in_config_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t has_child_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t has_parent_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t aux_in_use_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t aux_out_in_use_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t out_in_use_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t revplay_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bus_slot_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t schedtime_lock = PTHREAD_MUTEX_INITIALIZER;
/* pthread_mutex_t heapLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; */
#else
GLOBAL pthread_mutex_t pfieldLock;
GLOBAL pthread_mutex_t endsamp_lock;
GLOBAL pthread_mutex_t audio_config_lock;
GLOBAL pthread_mutex_t aux_to_aux_lock;
GLOBAL pthread_mutex_t to_aux_lock;
GLOBAL pthread_mutex_t to_out_lock;
GLOBAL pthread_mutex_t inst_bus_config_lock;
GLOBAL pthread_mutex_t bus_config_status_lock;
GLOBAL pthread_mutex_t bus_in_config_lock;
GLOBAL pthread_mutex_t has_child_lock;
GLOBAL pthread_mutex_t has_parent_lock;
GLOBAL pthread_mutex_t aux_in_use_lock;
GLOBAL pthread_mutex_t aux_out_in_use_lock;
GLOBAL pthread_mutex_t out_in_use_lock;
GLOBAL pthread_mutex_t revplay_lock;
GLOBAL pthread_mutex_t bus_slot_lock;
//GLOBAL pthread_mutex_t schedtime_lock;
#endif

/* -------------------------------------------------------------------------- */

#include "rtdefs.h"
GLOBAL InputDesc inputFileTable[MAX_INPUT_FDS];

GLOBAL BufPtr audioin_buffer[MAXBUS];    /* input from ADC, not file */
GLOBAL BufPtr aux_buffer[MAXBUS];
GLOBAL BufPtr out_buffer[MAXBUS];

GLOBAL int rtfileit;
GLOBAL int rtoutfile;

/* This should probably go someplace else in this file? */
typedef enum {
  NO = 0,
  YES
} Bool;

GLOBAL short AuxToAuxPlayList[MAXBUS]; /* The playback order for AUX buses */
GLOBAL short ToOutPlayList[MAXBUS]; /* The playback order for AUX buses */
GLOBAL short ToAuxPlayList[MAXBUS]; /* The playback order for AUX buses */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _GLOBALS_H_ */
