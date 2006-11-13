/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */


/* >>> ts A61021 : for testing the new arrangement of code 
		   please outcomment these defines : */

/* Keeps alive old enumerate_common(). New version delegates much work
   to methods in drive, mmc, spc, and sbc .
*/
#define Scsi_freebsd_make_own_enumeratE 1


/* Keeps alive old sg_enumerate(). New version delegates most work to
   sg_give_next_adr().
*/
#define Scsi_freebsd_old_sg_enumeratE 1


#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <camlib.h>
#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>

#include <err.h> /* XXX */


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

static void enumerate_common(char *fname, int bus_no, int host_no,
			     int channel_no, int target_no, int lun_no);

/* ts A51221 */
int burn_drive_is_banned(char *device_address);


/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
int mmc_function_spy(char * text);


#ifdef Scsi_freebsd_old_sg_enumeratE

int sg_give_next_adr(burn_drive_enumerator_t *idx,
		     char adr[], int adr_size, int initialize)
{
	return (0);
}

int sg_is_enumerable_adr(char* adr)
{
	return (0);
}

int sg_obtain_scsi_adr(char *path, int *bus_no, int *host_no, int *channel_no,
                       int *target_no, int *lun_no)
{
	return (0);
}

#else /* Scsi_freebsd_old_sg_enumeratE */

/* ts A61021 : Moved most code from sg_enumerate under sg_give_next_adr() */
/* Some helper functions for sg_give_next_adr() */

static int sg_init_enumerator(burn_drive_enumerator_t *idx)
{
	idx->skip_device = 0;

	if ((idx->fd = open(XPT_DEVICE, O_RDWR)) == -1) {
		warn("couldn't open %s", XPT_DEVICE);
		return -1;
	}

	bzero(&(idx->ccb), sizeof(union ccb));

	idx->ccb.ccb_h.path_id = CAM_XPT_PATH_ID;
	idx->ccb.ccb_h.target_id = CAM_TARGET_WILDCARD;
	idx->ccb.ccb_h.target_lun = CAM_LUN_WILDCARD;

	idx->ccb.ccb_h.func_code = XPT_DEV_MATCH;
	idx->bufsize = sizeof(struct dev_match_result) * 100;
	idx->ccb.cdm.match_buf_len = idx->bufsize;
	idx->ccb.cdm.matches = (struct dev_match_result *)malloc(idx->bufsize);
	if (idx->ccb.cdm.matches == NULL) {
		warnx("can't malloc memory for matches");
		close(idx->fd);
		return -1;
	}
	idx->ccb.cdm.num_matches = 0;
	idx->i = idx->ccb.cdm.num_matches; /* to trigger buffer load */

	/*
	 * We fetch all nodes, since we display most of them in the default
	 * case, and all in the verbose case.
	 */
	idx->ccb.cdm.num_patterns = 0;
	idx->ccb.cdm.pattern_buf_len = 0;

	return 1;
}


static int sg_next_enumeration_buffer(burn_drive_enumerator_t *idx)
{
	/*
	 * We do the ioctl multiple times if necessary, in case there are
	 * more than 100 nodes in the EDT.
	 */
	if (ioctl(idx->fd, CAMIOCOMMAND, &(idx->ccb)) == -1) {
		warn("error sending CAMIOCOMMAND ioctl");
		return -1;
	}

	if ((idx->ccb.ccb_h.status != CAM_REQ_CMP)
	    || ((idx->ccb.cdm.status != CAM_DEV_MATCH_LAST)
		&& (idx->ccb.cdm.status != CAM_DEV_MATCH_MORE))) {
		warnx("got CAM error %#x, CDM error %d\n",
		      idx->ccb.ccb_h.status, idx->ccb.cdm.status);
		return -1;
	}
	return 1;
}


/** Returns the next index number and the next enumerated drive address.
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
	int ret;

	if (initialize == 1) {
		ret = sg_init_enumerator(idx);
		if (ret<=0)
			return ret;
	} else if (initialize == -1) {
		if(idx->fd != -1)
			close(idx->fd);
		idx->fd = -1;
		return 0;
	}


try_item:; /* This spaghetti loop keeps the number of tabs small  */

	/* Loop content from old sg_enumerate() */

	while (idx->i >= idx->ccb.cdm.num_matches) {
		ret = sg_next_enumeration_buffer(idx);
		if (ret<=0)
			return -1;
		if (!((idx->ccb.ccb_h.status == CAM_REQ_CMP)
			&& (idx->ccb.cdm.status == CAM_DEV_MATCH_MORE)) )
			return 0;
		idx->i = 0;
	}

	switch (idx->ccb.cdm.matches[idx->i].type) {
	case DEV_MATCH_BUS:
		break;
	case DEV_MATCH_DEVICE: {
		struct device_match_result* result;

		result = &(idx->ccb.cdm.matches[i].result.device_result);
		if (result->flags & DEV_RESULT_UNCONFIGURED)
			idx->skip_device = 1;
		else
			idx->skip_device = 0;
		break;
	}
	case DEV_MATCH_PERIPH: {
		struct periph_match_result* result;
		char buf[64];

		result = &(idx->ccb.cdm.matches[i].result.periph_result);
		if (idx->skip_device || 
		    strcmp(result->periph_name, "pass") == 0)
			break;
		snprintf(buf, sizeof (buf), "/dev/%s%d",
			 result->periph_name, result->unit_number);
		if(adr_size <= strlen(buf)
			return -1;
		strcpy(adr, buf);

		/* Found next enumerable address */
		return 1;

	}
	default:
		/* printf(stderr, "unknown match type\n"); */
		break;
	}

	(idx->i)++;
	goto try_item; /* Regular function exit is return 1 above  */
}


int sg_is_enumerable_adr(char* adr)
{
	burn_drive_enumerator_t idx;
	int initialize = 1;
	char buf[64];

	while(1) {
		ret = sg_give_next_adr(&idx, buf, sizeof(buf), initialize);
		initialize = 0;
		if (ret <= 0)
	break;
		if (strcmp(adr, buf) == 0) {
			sg_give_next_adr(&idx, buf, sizeof(buf), -1);
			return 1;
		}
	}
	sg_give_next_adr(&idx, buf, sizeof(buf), -1);
	return (0);
}


/** Try to obtain SCSI address parameters.
    @return  1 is success , 0 is failure
*/
int sg_obtain_scsi_adr(char *path, int *bus_no, int *host_no, int *channel_no,
                       int *target_no, int *lun_no)
{
	burn_drive_enumerator_t idx;
	int initialize = 1;
	char buf[64];
	struct periph_match_result* result;

	while(1) {
		ret = sg_give_next_adr(&idx, buf, sizeof(buf), initialize);
		initialize = 0;
		if (ret <= 0)
	break;
		if (strcmp(adr, buf) != 0)
	continue;
		result = &(idx->ccb.cdm.matches[i].result.periph_result);
		*bus_no = result->path_id;
		*host_no = result->path_id;
		*channel_no = 0;
		*target_no = result->target_id
		*lun_no = result->target_lun;
		sg_give_next_adr(&idx, buf, sizeof(buf), -1);
		return 1;
	}
	sg_give_next_adr(&idx, buf, sizeof(buf), -1);
	return (0);
}

#endif /* ! Scsi_freebsd_old_sg_enumeratE */


int sg_close_drive(struct burn_drive * d)
{
	if (d->cam != NULL) {
		cam_close_device(d->cam);
		d->cam = NULL;
	}
	return 0;
}

int sg_drive_is_open(struct burn_drive * d)
{
	return (d->cam != NULL);
}


void ata_enumerate(void)
{
	/* ts A61021: Only a dummy function is needed in FreeBSD */
	/* The difference between sg and ata should be encapsulated
	   in sg-linux.c */
	;
}


void sg_enumerate(void)
{

#ifdef Scsi_freebsd_old_sg_enumeratE

	union ccb ccb;
	int bufsize, fd;
	unsigned int i;
	int skip_device = 0;

	if ((fd = open(XPT_DEVICE, O_RDWR)) == -1) {
		warn("couldn't open %s", XPT_DEVICE);
		return;
	}

	bzero(&ccb, sizeof(union ccb));

	ccb.ccb_h.path_id = CAM_XPT_PATH_ID;
	ccb.ccb_h.target_id = CAM_TARGET_WILDCARD;
	ccb.ccb_h.target_lun = CAM_LUN_WILDCARD;

	ccb.ccb_h.func_code = XPT_DEV_MATCH;
	bufsize = sizeof(struct dev_match_result) * 100;
	ccb.cdm.match_buf_len = bufsize;
	ccb.cdm.matches = (struct dev_match_result *)malloc(bufsize);
	if (ccb.cdm.matches == NULL) {
		warnx("can't malloc memory for matches");
		close(fd);
		return;
	}
	ccb.cdm.num_matches = 0;

	/*
	 * We fetch all nodes, since we display most of them in the default
	 * case, and all in the verbose case.
	 */
	ccb.cdm.num_patterns = 0;
	ccb.cdm.pattern_buf_len = 0;

	/*
	 * We do the ioctl multiple times if necessary, in case there are
	 * more than 100 nodes in the EDT.
	 */
	do {
		if (ioctl(fd, CAMIOCOMMAND, &ccb) == -1) {
			warn("error sending CAMIOCOMMAND ioctl");
			break;
		}

		if ((ccb.ccb_h.status != CAM_REQ_CMP)
		 || ((ccb.cdm.status != CAM_DEV_MATCH_LAST)
		    && (ccb.cdm.status != CAM_DEV_MATCH_MORE))) {
			warnx("got CAM error %#x, CDM error %d\n",
			      ccb.ccb_h.status, ccb.cdm.status);
			break;
		}

		for (i = 0; i < ccb.cdm.num_matches; i++) {
			switch (ccb.cdm.matches[i].type) {
			case DEV_MATCH_BUS:
				break;
			case DEV_MATCH_DEVICE: {
				struct device_match_result* result;

				result = &ccb.cdm.matches[i].result.device_result;

				if (result->flags & DEV_RESULT_UNCONFIGURED)
					skip_device = 1;
				else
					skip_device = 0;

				break;
			}
			case DEV_MATCH_PERIPH: {
				struct periph_match_result* result;
				char buf[64];

				result = &ccb.cdm.matches[i].result.periph_result;
				if (skip_device || strcmp(result->periph_name, "pass") == 0)
					break;
				snprintf(buf, sizeof (buf), "/dev/%s%d", result->periph_name, result->unit_number);
				/* ts A51221 */
				if (burn_drive_is_banned(buf))
					break;

				enumerate_common(buf, result->path_id, result->path_id, 0,
						 result->target_id, result->target_lun);
				break;
			}
			default:
				fprintf(stdout, "unknown match type\n");
				break;
			}
		}

	} while ((ccb.ccb_h.status == CAM_REQ_CMP)
		&& (ccb.cdm.status == CAM_DEV_MATCH_MORE));

	close(fd);

#else /* Scsi_freebsd_old_sg_enumeratE */

	burn_drive_enumerator_t idx;
	int initialize = 1;
	char buf[64];

	while(1) {
		ret = sg_give_next_adr(&idx, buf, sizeof(buf), initialize);
		initialize = 0;
		if (ret <= 0)
	break;
		if (burn_drive_is_banned(buf))
	continue; 
		enumerate_common(buf, idx.result->path_id, idx.result->path_id,
				0, idx.result->target_id, 
				idx.result->target_lun);
	}
	sg_give_next_adr(&idx, buf, sizeof(buf), -1);

#endif /* ! Scsi_freebsd_old_sg_enumeratE */

}


#ifdef Scsi_freebsd_make_own_enumeratE

/* ts A61021: The old version which mixes SCSI and operating system adapter
*/
static void enumerate_common(char *fname, int bus_no, int host_no,
			     int channel_no, int target_no, int lun_no)
{
	struct burn_drive *t;
	struct burn_drive out;

	/* ts A60923 */
	out.bus_no = bus_no;
	out.host = host_no;
	out.id = target_no;
	out.channel = channel_no;
	out.lun = lun_no;

	out.devname = burn_strdup(fname);
	out.cam = NULL;

	out.start_lba= -2000000000;
	out.end_lba= -2000000000;
	out.read_atip = mmc_read_atip;

	out.grab = sg_grab;
	out.release = sg_release;
	out.drive_is_open= sg_drive_is_open;
	out.issue_command = sg_issue_command;
	out.getcaps = spc_getcaps;
	out.released = 1;
	out.status = BURN_DISC_UNREADY;

	out.eject = sbc_eject;
	out.load = sbc_load;
	out.lock = spc_prevent;
	out.unlock = spc_allow;
	out.read_disc_info = spc_sense_write_params;
	out.get_erase_progress = spc_get_erase_progress;
	out.test_unit_ready = spc_test_unit_ready;
	out.probe_write_modes = spc_probe_write_modes;
	out.read_toc = mmc_read_toc;
	out.write = mmc_write;
	out.erase = mmc_erase;
	out.read_sectors = mmc_read_sectors;
	out.perform_opc = mmc_perform_opc;
	out.set_speed = mmc_set_speed;
	out.send_parameters = spc_select_error_params;
	out.send_write_parameters = spc_select_write_params;
	out.send_cue_sheet = mmc_send_cue_sheet;
	out.sync_cache = mmc_sync_cache;
	out.get_nwa = mmc_get_nwa;
	out.close_disc = mmc_close_disc;
	out.close_session = mmc_close_session;
	out.close_track_session = mmc_close;
	out.read_buffer_capacity = mmc_read_buffer_capacity;
	out.idata = malloc(sizeof(struct burn_scsi_inquiry_data));
	out.idata->valid = 0;
	out.mdata = malloc(sizeof(struct scsi_mode_data));
	out.mdata->valid = 0;
	if (out.idata == NULL || out.mdata == NULL) {
		libdax_msgs_submit(libdax_messenger, -1, 0x00020108,
			LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
			"Could not allocate new drive object", 0, 0);
		return;
	}
	memset(&out.params, 0, sizeof(struct params));
	t = burn_drive_register(&out);

/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("enumerate_common : -------- doing grab");

/* try to get the drive info */
	if (t->grab(t)) {
		burn_print(2, "getting drive info\n");
		t->getcaps(t);
		t->unlock(t);
		t->released = 1;
	} else {
		burn_print(2, "unable to grab new located drive\n");
	}

/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("enumerate_common : ----- would release ");

}

#else /* Scsi_freebsd_make_own_enumeratE */

/* The new, more concise version of enumerate_common */
static void enumerate_common(char *fname, int bus_no, int host_no,
			     int channel_no, int target_no, int lun_no)
{
	int ret;
	struct burn_drive out;

	/* General libburn drive setup */
	burn_setup_drive(&out, fname);

	/* This transport adapter uses SCSI-family commands and models
	   (seems the adapter would know better than its boss, if ever) */
	ret = burn_scsi_setup_drive(&out, bus_no, host_no, channel_no,
                                 target_no, lun_no, 0);
        if (ret<=0)
                return;

	/* Operating system adapter is CAM */
	/* Adapter specific handles and data */
	out.cam = NULL;
	/* Adapter specific functions */
	out.grab = sg_grab;
	out.release = sg_release;
	out.drive_is_open = sg_drive_is_open;
	out.issue_command = sg_issue_command;

	/* Finally register drive and inquire drive information */
	burn_drive_finish_enum(&out);
}

#endif /* ! Scsi_freebsd_make_own_enumeratE */

/* ts A61021: do not believe this:
	we use the sg reference count to decide whether we can use the
	drive or not.
	if refcount is not one, drive is open somewhere else.
*/
int sg_grab(struct burn_drive *d)
{
	int count;
	struct cam_device *cam;

	mmc_function_spy("sg_grab");

	assert(d->cam == NULL);

	cam = cam_open_device(d->devname, O_RDWR);
	if (cam == NULL) {
		libdax_msgs_submit(libdax_messenger, d->global_index, 0x00020003,
		LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
		"Could not grab drive", 0/*os_errno*/, 0);
		return 0;
	}
/*	er = ioctl(fd, SG_GET_ACCESS_COUNT, &count);*/
	count = 1;
	if (1 == count) {
		d->cam = cam;
		fcntl(cam->fd, F_SETOWN, getpid());
		d->released = 0;
		return 1;
	}
	burn_print(1, "could not acquire drive - already open\n");
	sg_close_drive(d);
	return 0;
}


/*
	non zero return means you still have the drive and it's not
	in a state to be released? (is that even possible?)
*/

int sg_release(struct burn_drive *d)
{
	mmc_function_spy("sg_release");

	if (d->cam == NULL) {
		burn_print(1, "release an ungrabbed drive.  die\n");
		return 0;
	}

	mmc_function_spy("sg_release ----------- closing.");

	sg_close_drive(d);
	return 0;
}

int sg_issue_command(struct burn_drive *d, struct command *c)
{
	int done = 0;
	int err;
	union ccb *ccb;

	char buf[161];
	snprintf(buf, sizeof (buf), "sg_issue_command  d->cam=%p d->released=%d",
		(void*)d->cam, d->released);
	mmc_function_spy(buf);

	if (d->cam == NULL) {
		c->error = 0;
		return 0;
	}

	c->error = 0;

	ccb = cam_getccb(d->cam);
	cam_fill_csio(&ccb->csio,
				  1,                              /* retries */
				  NULL,                           /* cbfncp */
				  CAM_DEV_QFRZDIS,                /* flags */
				  MSG_SIMPLE_Q_TAG,               /* tag_action */
				  NULL,                           /* data_ptr */
				  0,                              /* dxfer_len */
				  sizeof (ccb->csio.sense_data),  /* sense_len */
				  0,                              /* cdb_len */
				  30*1000);                       /* timeout */
	switch (c->dir) {
	case TO_DRIVE:
		ccb->csio.ccb_h.flags |= CAM_DIR_OUT;
		break;
	case FROM_DRIVE:
		ccb->csio.ccb_h.flags |= CAM_DIR_IN;
		break;
	case NO_TRANSFER:
		ccb->csio.ccb_h.flags |= CAM_DIR_NONE;
		break;
	}

	ccb->csio.cdb_len = c->oplen;
	memcpy(&ccb->csio.cdb_io.cdb_bytes, &c->opcode, c->oplen);
	
	memset(&ccb->csio.sense_data, 0, sizeof (ccb->csio.sense_data));

	if (c->page) {
		ccb->csio.data_ptr  = c->page->data;
		if (c->dir == FROM_DRIVE) {
			ccb->csio.dxfer_len = BUFFER_SIZE;
/* touch page so we can use valgrind */
			memset(c->page->data, 0, BUFFER_SIZE);
		} else {
			assert(c->page->bytes > 0);
			ccb->csio.dxfer_len = c->page->bytes;
		}
	} else {
		ccb->csio.data_ptr  = NULL;
		ccb->csio.dxfer_len = 0;
	}

	do {
		err = cam_send_ccb(d->cam, ccb);
		if (err == -1) {
			libdax_msgs_submit(libdax_messenger,
				d->global_index, 0x0002010c,
				LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
				"Failed to transfer command to drive",
				errno, 0);
			cam_freeccb(ccb);
			sg_close_drive(d);
			d->released = 1;
			d->busy = BURN_DRIVE_IDLE;
			c->error = 1;
			return -1;
		}
		/* XXX */
		memcpy(c->sense, &ccb->csio.sense_data, ccb->csio.sense_len);
		if ((ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
			if (!c->retry) {
				c->error = 1;
				cam_freeccb(ccb);
				return 1;
			}
			switch (scsi_error(d, c->sense, 0)) {
			case RETRY:
				done = 0;
				break;
			case FAIL:
				done = 1;
				c->error = 1;
				break;
			}
		} else {
			done = 1;
		}
	} while (!done);
	cam_freeccb(ccb);
	return 1;
}

enum response scsi_error(struct burn_drive *d, unsigned char *sense,
			 int senselen)
{
	int key, asc, ascq;

	senselen = senselen;
	key = sense[2];
	asc = sense[12];
	ascq = sense[13];

	burn_print(12, "CONDITION: 0x%x 0x%x 0x%x on %s %s\n",
		   key, asc, ascq, d->idata->vendor, d->idata->product);

	switch (asc) {
	case 0:
		burn_print(12, "NO ERROR!\n");
		return RETRY;

	case 2:
		burn_print(1, "not ready\n");
		return RETRY;
	case 4:
		burn_print(1,
			   "logical unit is in the process of becoming ready\n");
		return RETRY;
	case 0x20:
		if (key == 5)
			burn_print(1, "bad opcode\n");
		return FAIL;
	case 0x21:
		burn_print(1, "invalid address or something\n");
		return FAIL;
	case 0x24:
		if (key == 5)
			burn_print(1, "invalid field in cdb\n");
		else
			break;
		return FAIL;
	case 0x26:
		if (key == 5)
			burn_print( 1, "invalid field in parameter list\n" );
		return FAIL;
	case 0x28:
		if (key == 6)
			burn_print(1,
				   "Not ready to ready change, medium may have changed\n");
		else
			break;
		return RETRY;
	case 0x3A:
		burn_print(12, "Medium not present in %s %s\n",
			   d->idata->vendor, d->idata->product);

		d->status = BURN_DISC_EMPTY;
		return FAIL;
	}
	burn_print(1, "unknown failure\n");
	burn_print(1, "key:0x%x, asc:0x%x, ascq:0x%x\n", key, asc, ascq);
	return FAIL;
}

