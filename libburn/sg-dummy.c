/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/*

This is the main operating system dependent SCSI part of libburn. It implements
the transport level aspects of SCSI control and command i/o.

Present implementation: default dummy which enables libburn only to work
                        with stdio: pseudo drive addresses.
                        For real implementations see sg-linux.c or sg-freebsd.c

*/


#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#ifdef Libburn_os_has_statvfS
#include <sys/statvfs.h>
#endif /* Libburn_os_has_stavtfS */

#include "transport.h"
#include "drive.h"
#include "sg.h"
#include "spc.h"
#include "mmc.h"
#include "sbc.h"
#include "debug.h"
#include "toc.h"
#include "util.h"

#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;


/** Returns the next index number and the next enumerated drive address.
    The enumeration has to cover all available and accessible drives. It is
    allowed to return addresses of drives which are not available but under
    some (even exotic) circumstances could be available. It is on the other
    hand allowed, only to hand out addresses which can really be used right
    in the moment of this call. (This implementation chooses the former.)
    @param idx An opaque handle. Make no own theories about it.
    @param adr Takes the reply
    @param adr_size Gives maximum size of reply including final 0
    @param initialize  1 = start new,
                       0 = continue, use no other values for now
                      -1 = finish
    @return 1 = reply is a valid address , 0 = no further address available
           -1 = severe error (e.g. adr_size too small)
*/
int sg_give_next_adr(burn_drive_enumerator_t *idx,
		     char adr[], int adr_size, int initialize)
{
	return 0;
}


/** Brings all available, not-whitelist-banned, and accessible drives into
    libburn's list of drives.
*/
/* ts A61115: replacing call to sg-implementation internals from drive.c */
int scsi_enumerate_drives(void)
{
	libdax_msgs_submit(libdax_messenger, -1, 0x0002016b,
		LIBDAX_MSGS_SEV_WARNING, LIBDAX_MSGS_PRIO_HIGH,
		"No MMC transport adapter is present. Running on sg-dummy.c.",
		0, 0);
	return 1;
}


/** Tells wether libburn has the given drive in use or exclusively reserved.
    If it is "open" then libburn will eventually call sg_release() on it when
    it is time to give up usage resp. reservation.
*/
/** Published as burn_drive.drive_is_open() */
int sg_drive_is_open(struct burn_drive * d)
{
	return 0;
}


/** Opens the drive for SCSI commands and - if burn activities are prone
    to external interference on your system - obtains an exclusive access lock
    on the drive. (Note: this is not physical tray locking.)
    A drive that has been opened with sg_grab() will eventually be handed
    over to sg_release() for closing and unreserving.
*/
int sg_grab(struct burn_drive *d)
{
	libdax_msgs_submit(libdax_messenger, d->global_index, 0x0002016a,
		LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
		"No MMC transport adapter is present. Running on sg-dummy.c.",
		0, 0);
	return 0;
}


/** Gives up the drive for SCSI commands and releases eventual access locks.
    (Note: this is not physical tray locking.)
*/
int sg_release(struct burn_drive *d)
{
	return 0;
}


/** Sends a SCSI command to the drive, receives reply and evaluates wether
    the command succeeded or shall be retried or finally failed.
    Returned SCSI errors shall not lead to a return value indicating failure.
    The callers get notified by c->error. An SCSI failure which leads not to
    a retry shall be notified via scsi_notify_error().
    The Libburn_log_sg_commandS facility might be of help when problems with
    a drive have to be examined. It shall stay disabled for normal use.
    @return: 1 success , <=0 failure
*/
int sg_issue_command(struct burn_drive *d, struct command *c)
{
	libdax_msgs_submit(libdax_messenger, d->global_index, 0x0002016a,
		LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
		"No MMC transport adapter is present. Running on sg-dummy.c.",
		0, 0);
	return -1;
}


/** Tries to obtain SCSI address parameters.
    @return  1 is success , 0 is failure
*/
int sg_obtain_scsi_adr(char *path, int *bus_no, int *host_no, int *channel_no,
                       int *target_no, int *lun_no)
{
        libdax_msgs_submit(libdax_messenger, -1, 0x0002016c,
                LIBDAX_MSGS_SEV_DEBUG, LIBDAX_MSGS_PRIO_HIGH,
                "No MMC transport adapter is present. Running on sg-dummy.c.",
                0, 0);
	return 0;
}


/** Tells wether a text is a persistent address as listed by the enumeration
    functions.
*/
int sg_is_enumerable_adr(char *adr)
{
	return(0);
}


/** Estimate the potential payload capacity of a file address.
    @param path  The address of the file to be examined. If it does not
                 exist yet, then the directory will be inquired.
    @param bytes The pointed value gets modified, but only if an estimation is
                 possible.
    @return      -2 = cannot perform necessary operations on file object
                 -1 = neither path nor dirname of path exist
                  0 = could not estimate size capacity of file object
                  1 = estimation has been made, bytes was set
*/
int burn_os_stdio_capacity(char *path, off_t *bytes)
{
	struct stat stbuf;

#ifdef Libburn_os_has_statvfS
	struct statvfs vfsbuf;
#endif

	char testpath[4096], *cpt;
	long blocks;
	off_t add_size = 0;

	testpath[0] = 0;
	blocks = *bytes / 512;
	if (stat(path, &stbuf) == -1) {
		strcpy(testpath, path);
		cpt = strrchr(testpath, '/');
		if(cpt == NULL)
			strcpy(testpath, ".");
		else if(cpt == testpath)
			testpath[1] = 0;
		else
			*cpt = 0;
		if (stat(testpath, &stbuf) == -1)
			return -1;

#ifdef Libburn_if_this_was_linuX

	} else if(S_ISBLK(stbuf.st_mode)) {
		fd = open(path, open_mode);
		if (fd == -1)
			return -2;
		ret = ioctl(fd, BLKGETSIZE, &blocks);
		close(fd);
		if (ret == -1)
			return -2;
		*bytes = ((off_t) blocks) * (off_t) 512;

#endif /* Libburn_if_this_was_linuX */

	} else if(S_ISREG(stbuf.st_mode)) {
		add_size = stbuf.st_blocks * (off_t) 512;
		strcpy(testpath, path);
	} else
		return 0;

	if (testpath[0]) {	

#ifdef Libburn_os_has_statvfS

		if (statvfs(testpath, &vfsbuf) == -1)
			return -2;
		*bytes = add_size + ((off_t) vfsbuf.f_bsize) *
						(off_t) vfsbuf.f_bavail;

#else /* Libburn_os_has_statvfS */

		return 0;

#endif /* ! Libburn_os_has_stavtfS */

	}
	return 1;
}
