
/* os-libcdio.h
   Operating system specific libburn definitions and declarations. Included
   by os.h in case of compilation for
           Unknown X/Open-like systems
           with GNU libcdio MMC transport adapter sg-libcdio.c

   Copyright (C) 2009 - 2013 Thomas Schmitt <scdbackup@gmx.net>
   Provided under GPLv2+
*/


/** List of all signals which shall be caught by signal handlers and trigger
    a graceful abort of libburn. (See man 7 signal.)
*/
/* Once as system defined macros */
#define BURN_OS_SIGNAL_MACRO_LIST \
 SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, \
 SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, \
 SIGUSR1, SIGUSR2, SIGXCPU

/* Once as text 1:1 list of strings for messages and interpreters */
#define BURN_OS_SIGNAL_NAME_LIST \
 "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGABRT", \
 "SIGFPE", "SIGSEGV", "SIGPIPE", "SIGALRM", "SIGTERM", \
 "SIGUSR1", "SIGUSR2", "SIGXCPU"

/* The number of above list items */
#define BURN_OS_SIGNAL_COUNT 13


/** The list of all signals which shall surely not be caught.
    It depends on the particular signal whether it can be ignored or whether
    it will lead to sudden death of the process.
    Some signals are not POSIX,
    but nevertheless ought to be ignored if they are defined.
*/

#ifdef SIGWINCH
#define BURN_OS_SIG_WINCH ,SIGWINCH
#define BURN_OS_SIG_WINCH_CNT 1
#else
#define BURN_OS_SIG_WINCH 
#define BURN_OS_SIG_WINCH_CNT 0
#endif

#ifdef SIGURG
#define BURN_OS_SIG_URG ,SIGURG
#define BURN_OS_SIG_URG_CNT 1
#else
#define BURN_OS_SIG_URG 
#define BURN_OS_SIG_URG_CNT 0
#endif

/** The combined list of all signals which shall not be caught.
 */
#define BURN_OS_NON_SIGNAL_MACRO_LIST \
SIGKILL, SIGCHLD, SIGSTOP, SIGTSTP, SIGCONT, SIGTTIN, SIGTTOU \
BURN_OS_SIG_WINCH BURN_OS_SIG_URG

/* The number of above list items */
#define BURN_OS_NON_SIGNAL_COUNT \
( 7 + BURN_OS_SIG_WINCH_CNT + BURN_OS_SIG_URG_CNT )


/* The maximum size for a (SCSI) i/o transaction */
/* My Blu-ray burner LG GGW-H20 writes junk if stream recording is combined
   with buffer size 32 kB. So stream recording is allowed only with size 64k.
   Older BSD info says that 32 kB is maximum. But 64 kB seems to work well
   on 8-STABLE. It is by default only used with BD in streaming mode.
   So older systems should still be quite safe with this buffer max size.
*/
/* Important : MUST be at least 32768 ! */
#define BURN_OS_TRANSPORT_BUFFER_SIZE 65536


/* To hold the position of the most recently delivered address from
   device enumeration.
*/
struct burn_drive_enumerator_struct {
	char **ppsz_cd_drives;
	char **pos;
};

#define BURN_OS_DEFINE_DRIVE_ENUMERATOR_T \
typedef struct burn_drive_enumerator_struct burn_drive_enumerator_t;


/* The list of operating system dependent elements in struct burn_drive.
   Usually they are initialized in  sg-*.c:enumerate_common().
*/
#define BURN_OS_TRANSPORT_DRIVE_ELEMENTS \
	void *p_cdio; /* actually a pointer to CdIo_t */ \
	char libcdio_name[4096]; /* The drive path as used by libcdio */ \

