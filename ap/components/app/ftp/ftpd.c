/*
 * Copyright (c) 2002 Florian Schulze.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ftpd.c - This file is part of the FTP daemon for lwIP
 *
 */
#include <common/bk_include.h>
#include <components/log.h>
#if CONFIG_FTP_SERVER
#include <os/mem.h>
#include "lwip/debug.h"
#include "lwip/stats.h"

#include "ftpd.h"

#include "lwip/tcp.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#if CONFIG_VFS
#include "bk_posix.h"
#endif
#include <sys/stat.h>


#define TAG "FTP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define FTP_USER        "bk7258"
#define FTP_PASSWORD    "123456"

uint32_t ftp_is_running = 0;

#if 0
#define EINVAL 1
#define ENOMEM 2
#define ENODEV 3
#endif

#define msg110 "110 MARK %s = %s."
/*
         110 Restart marker reply.
             In this case, the text is exact and not left to the
             particular implementation; it must read:
                  MARK yyyy = mmmm
             Where yyyy is User-process data stream marker, and mmmm
             server's equivalent marker (note the spaces between markers
             and "=").
*/
#define msg120 "120 Service ready in nnn minutes."
#define msg125 "125 Data connection already open; transfer starting."
#define msg150 "150 File status okay; about to open data connection."
#define msg150recv "150 Opening BINARY mode data connection for %s (%i bytes)."
#define msg150stor "150 Opening BINARY mode data connection for %s."
#define msg200 "200 Command okay."
#define msg202 "202 Command not implemented, superfluous at this site."
#define msg211 "211 System status, or system help reply."
#define msg212 "212 Directory status."
#define msg213 "213 %ld"
#define msg214 "214 %s."
/*
             214 Help message.
             On how to use the server or the meaning of a particular
             non-standard command.  This reply is useful only to the
             human user.
*/
#define msg214SYST "214 %s system type."
/*
         215 NAME system type.
             Where NAME is an official system name from the list in the
             Assigned Numbers document.
*/
#define msg220 "220 lwIP FTP Server ready."
/*
         220 Service ready for new user.
*/
#define msg221 "221 Goodbye."
/*
         221 Service closing control connection.
             Logged out if appropriate.
*/
#define msg225 "225 Data connection open; no transfer in progress."
#define msg226 "226 Closing data connection."
#define msg226_1 "226 Transfer ok."
/*
             Requested file action successful (for example, file
             transfer or file abort).
*/
#define msg227 "227 Entering Passive Mode (%i,%i,%i,%i,%i,%i)."
/*
         227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
*/
#define msg230 "230 User logged in, proceed."
#define msg250 "250 Requested file action okay, completed."
#define msg257PWD "257 \"%s\" is current directory."
#define msg257 "257 \"%s\" created."
/*
         257 "PATHNAME" created.
*/
#define msg331 "331 User name okay, need password."
#define msg332 "332 Need account for login."
#define msg350 "350 Requested file action pending further information."
#define msg421 "421 Service not available, closing control connection."
/*
             This may be a reply to any command if the service knows it
             must shut down.
*/
#define msg425 "425 Can't open data connection."
#define msg426 "426 Connection closed; transfer aborted."
#define msg450 "450 Requested file action not taken."
/*
             File unavailable (e.g., file busy).
*/
#define msg451 "451 Requested action aborted: local error in processing."
#define msg452 "452 Requested action not taken."
/*
             Insufficient storage space in system.
*/
#define msg500 "500 Syntax error, command unrecognized."
/*
             This may include errors such as command line too long.
*/
#define msg501 "501 Syntax error in parameters or arguments."
#define msg502 "502 Command not implemented."
#define msg503 "503 Bad sequence of commands."
#define msg504 "504 Command not implemented for that parameter."
#define msg530 "530 Not logged in."
#define msg532 "532 Need account for storing files."
#define msg550 "550 Requested action not taken."
/*
             File unavailable (e.g., file not found, no access).
*/
#define msg551 "551 Requested action aborted: page type unknown."
#define msg552 "552 Requested file action aborted."
/*
             Exceeded storage allocation (for current directory or
             dataset).
*/
#define msg553 "553 Requested action not taken."


#define msg_FEAT   "211-Extension supported\r\n UTF8\r\n MLSD\r\n CLNT\r\n SIZE\r\n211 End."

/*
             File name not allowed.
*/

enum ftpd_state_e {
	FTPD_USER,
	FTPD_PASS,
	FTPD_IDLE,
	FTPD_NLST,
	FTPD_LIST,
	FTPD_RETR,
	FTPD_RNFR,
	FTPD_STOR,
	FTPD_QUIT,
	FTPD_MLSD,
};

static const char *month_table[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dez"
};

/*
------------------------------------------------------------
	SFIFO 1.3
------------------------------------------------------------
 * Simple portable lock-free FIFO
 * (c) 2000-2002, David Olofson
 *
 * Platform support:
 *	gcc / Linux / x86:		Works
 *	gcc / Linux / x86 kernel:	Works
 *	gcc / FreeBSD / x86:		Works
 *	gcc / NetBSD / x86:		Works
 *	gcc / Mac OS X / PPC:		Works
 *	gcc / Win32 / x86:		Works
 *	Borland C++ / DOS / x86RM:	Works
 *	Borland C++ / Win32 / x86PM16:	Untested
 *	? / Various Un*ces / ?:		Untested
 *	? / Mac OS / PPC:		Untested
 *	gcc / BeOS / x86:		Untested
 *	gcc / BeOS / PPC:		Untested
 *	? / ? / Alpha:			Untested
 *
 * 1.2: Max buffer size halved, to avoid problems with
 *	the sign bit...
 *
 * 1.3:	Critical buffer allocation bug fixed! For certain
 *	requested buffer sizes, older version would
 *	allocate a buffer of insufficient size, which
 *	would result in memory thrashing. (Amazing that
 *	I've manage to use this to the extent I have
 *	without running into this... *heh*)
 */

/*
 * Porting note:
 *	Reads and writes of a variable of this type in memory
 *	must be *atomic*! 'int' is *not* atomic on all platforms.
 *	A safe type should be used, and  sfifo should limit the
 *	maximum buffer size accordingly.
 */
typedef int sfifo_atomic_t;
#ifdef __TURBOC__
#	define	SFIFO_MAX_BUFFER_SIZE	0x7fff
#else /* Kludge: Assume 32 bit platform */
#	define	SFIFO_MAX_BUFFER_SIZE	0x7fffffff
#endif

typedef struct sfifo_t {
	char *buffer;
	int size;			/* Number of bytes */
	sfifo_atomic_t readpos;		/* Read position */
	sfifo_atomic_t writepos;	/* Write position */
} sfifo_t;

#define SFIFO_SIZEMASK(x)	((x)->size - 1)

#define sfifo_used(x)	(((x)->writepos - (x)->readpos) & SFIFO_SIZEMASK(x))
#define sfifo_space(x)	((x)->size - 1 - sfifo_used(x))

#define DBG(x)

#define FTP_TEMP_BUFFER_LEN  1024
#define DATA_CMD_MAX_SIZE  640
#define FULL_PATH_SIZE 640
#define MAX_PRE_READ_BUFFER_SIZE 30*1024//8000
#define MAX_READ_BUFFER_SIZE 30*1024//8*1024

static char *strprepend(char *dest,int size,const char *pre_str)
{
	if (dest == NULL || pre_str == NULL || size == 0) return NULL;
    size_t src_len = strlen(pre_str);
    size_t dest_len = strlen(dest);
    size_t total_needed = src_len + dest_len + 1;

    if (total_needed > size) {
        LOGE("%s size %d, str1 %s,str2 %s\r\n",__func__,size,dest,pre_str);
        return NULL;
    }
	memmove(dest + src_len, dest, dest_len + 1);
	memcpy(dest, pre_str, src_len);
	return dest;
}

static int sfifo_init(sfifo_t *f, int size)
{
	memset(f, 0, sizeof(sfifo_t));

	if (size > SFIFO_MAX_BUFFER_SIZE)
		return -EINVAL;

	f->size = 1;
	for (; f->size <= size; f->size <<= 1)
		;

	if (0 == (f->buffer = (void *)ftp_malloc(f->size)))
		return -ENOMEM;

	return 0;
}

static void sfifo_close(sfifo_t *f)
{
	if (f->buffer)
		os_free(f->buffer);
}

static int sfifo_write(sfifo_t *f, const void *_buf, int len)
{
	int total;
	int i;
	const char *buf = (const char *)_buf;

	if (!f->buffer)
		return -ENODEV;	/* No buffer! */

	total = sfifo_space(f);
	DBG(LOGI("sfifo_space() = %d\r\n", total));
	if (len > total)
		len = total;
	else
		total = len;

	i = f->writepos;
	if (i + len > f->size) {
		memcpy(f->buffer + i, buf, f->size - i);
		buf += f->size - i;
		len -= f->size - i;
		i = 0;
	}
	memcpy(f->buffer + i, buf, len);
	f->writepos = i + len;

	return total;
}

struct ftpd_datastate {
	int connected;
	DIR* vfs_dir;              // vfs_dir_t *vfs_dir;
	struct dirent* vfs_dirent; //vfs_dirent_t *vfs_dirent;
	int fd;                    //vfs_file_t *vfs_file;
	sfifo_t fifo;
	struct tcp_pcb *msgpcb;
	struct ftpd_msgstate *msgfs;
};

struct ftpd_msgstate {
	enum ftpd_state_e state;
	sfifo_t fifo;
	// vfs_t *vfs;
	ip_addr_t dataip;
	u16_t dataport;
	struct tcp_pcb *datapcb;
	struct ftpd_datastate *datafs;
	int passive;
	char *renamefrom;
	off_t restart_offset;
};

static void send_msg(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, char *msg, ...);

static void ftpd_dataerr(void *arg, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	LOGE("ftpd_dataerr: %s (%i)\r\n", lwip_strerr(err), err);
	if (fsd == NULL)
		return;
	fsd->msgfs->datafs = NULL;
	fsd->msgfs->state = FTPD_IDLE;
	os_free(fsd);
}

static void ftpd_dataclose(struct tcp_pcb *pcb, struct ftpd_datastate *fsd)
{
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	fsd->msgfs->datafs = NULL;
	fsd->msgfs->passive = 0;
	sfifo_close(&fsd->fifo);
	os_free(fsd);
	tcp_arg(pcb, NULL);
	tcp_close(pcb);
}

static void send_data(struct tcp_pcb *pcb, struct ftpd_datastate *fsd)
{
	err_t err;
	u16_t len;

	if (sfifo_used(&fsd->fifo) > 0) {
		int i;

		/* We cannot send more data than space available in the send
		   buffer. */
		if (tcp_sndbuf(pcb) < sfifo_used(&fsd->fifo))
			len = tcp_sndbuf(pcb);
		else
			len = (u16_t) sfifo_used(&fsd->fifo);

		i = fsd->fifo.readpos;
		if ((i + len) > fsd->fifo.size) {
			err = tcp_write(pcb, fsd->fifo.buffer + i, (u16_t)(fsd->fifo.size - i), 1);
			if (err != ERR_OK) {
				LOGE("send_data: error writing! err %d\r\n",err);
				return;
			}
			len -= fsd->fifo.size - i;
			fsd->fifo.readpos = 0;
			i = 0;
		}

		err = tcp_write(pcb, fsd->fifo.buffer + i, len, 1);
		if (err != ERR_OK) {
			LOGE("send_data: error writing! err %d\r\n",err);
			return;
		}
		fsd->fifo.readpos += len;
	}
}

static void send_file(struct ftpd_datastate *fsd, struct tcp_pcb *pcb)
{
	uint8_t *buffer = NULL;
	uint32_t temp_len = MAX_READ_BUFFER_SIZE;
	int len;

	if (!fsd->connected)
		return;

	if (-1 != fsd->fd) {

		buffer = ftp_malloc(temp_len);
		if (buffer == NULL) {
			LOGE(" buffer test malloc fail len %d\r\n",temp_len);
			goto error;
		}

		len = sfifo_space(&fsd->fifo);

		if (len == 0) {
			send_data(pcb, fsd);
			goto error;
		}

		if (len > temp_len)
			len = temp_len;

		len = read(fsd->fd, buffer, len);

		if (len <= 0) {
			if (feof(fsd->fd) == 0)
				goto error;

			close(fsd->fd);
			fsd->fd = -1;
			goto error;
		}

		sfifo_write(&fsd->fifo, buffer, len);
		os_free(buffer);
		send_data(pcb, fsd);
	} else {
		struct ftpd_msgstate *fsm;
		struct tcp_pcb *msgpcb;

		if (sfifo_used(&fsd->fifo) > 0) {
			send_data(pcb, fsd);
			return;
		}
		fsm = fsd->msgfs;
		msgpcb = fsd->msgpcb;

		// close(fsd->fd);
		// fsd->fd = -1;
		ftpd_dataclose(pcb, fsd);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		fsm->state = FTPD_IDLE;
		send_msg(msgpcb, fsm, msg226);
		return;
	}

	return;
error:
	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}
}

static void send_next_directory(struct ftpd_datastate *fsd, struct tcp_pcb *pcb, int list_type)
{
	char *cwd_path = NULL;
	char *cwd_buffer = NULL;
	char *buffer = NULL;
	char *path = NULL;
	int len;

	LOGV("%s list_type %d\r\n",__func__,list_type);

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);
	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	cwd_buffer = os_malloc(MAX_PATH_LEN);
	if(cwd_buffer == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(cwd_buffer, 0, MAX_PATH_LEN);

	while (1) {
		if (fsd->vfs_dirent == NULL) {
			fsd->vfs_dirent = readdir(fsd->vfs_dir);
		}

		if (fsd->vfs_dirent) {
			if (list_type == FTPD_NLST) {
				len = sprintf(buffer, "%s\r\n", fsd->vfs_dirent->d_name);
				if (sfifo_space(&fsd->fifo) < len) {
					send_data(pcb, fsd);
					goto exit;
				}
				sfifo_write(&fsd->fifo, buffer, len);
				fsd->vfs_dirent = NULL;
				LOGV("NLST : %s",buffer);
			} else {
				struct stat st = {0};
				time_t current_time = {0};
				int current_year = 0;
				struct tm *s_time = NULL;

#if CONFIG_NTP_SYNC_RTC
				extern time_t timestamp_get();
				current_time = timestamp_get();
#else
				/* Fallback to time() if NTP is not configured */
				current_time = time(NULL);
#endif

				s_time = gmtime(&current_time);
				current_year = s_time->tm_year;
				memset(cwd_buffer,0,MAX_PATH_LEN);
				cwd_path = getcwd(cwd_buffer, MAX_PATH_LEN);
				LOGV("%s cwd_path %s\r\n",__func__,cwd_path);
				if (!cwd_path || cwd_path[0] == '\0' || (cwd_path[0] == '/' && cwd_path[1] == '\0')) {
					snprintf(path, FULL_PATH_SIZE, "%s/%s", FTP_MOUNT_PATH, fsd->vfs_dirent->d_name);
				} else {
					if (cwd_path[0] == '/' && cwd_path[1] == '/')
						cwd_path++;
					snprintf(path, FULL_PATH_SIZE, "%s/%s", cwd_path, fsd->vfs_dirent->d_name);
				}
				LOGV("%s path %s\r\n",__func__,path);
				stat(path, &st);

				/* If st_mtime is 0 (file system doesn't support timestamps),
				 * use current time instead */
				if (st.st_mtime == 0) {
					/* Display UTC (Greenwich Mean Time) */
					s_time = gmtime(&current_time);
				} else {
					s_time = gmtime(&st.st_mtime);
				}
				if(list_type == FTPD_LIST){
					if (s_time->tm_year == current_year) {
						len = sprintf(buffer, "-rw-rw-rw-   1 user     ftp  %11ld %s %02i %02i:%02i %s\r\n", st.st_size, month_table[s_time->tm_mon], s_time->tm_mday, s_time->tm_hour, s_time->tm_min, fsd->vfs_dirent->d_name);
					} else {
						len = sprintf(buffer, "-rw-rw-rw-   1 user     ftp  %11ld %s %02i %5i %s\r\n", st.st_size, month_table[s_time->tm_mon], s_time->tm_mday, s_time->tm_year + 1900, fsd->vfs_dirent->d_name);
					}
					if (S_ISDIR(st.st_mode))
						buffer[0] = 'd';
					LOGV("LIST : %s",buffer);
				} else {
					//MLSD
					if (S_ISDIR(st.st_mode))
					{
						/* MLSD format: modify=YYYYMMDDHHmmss */
						len = sprintf(buffer, "type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d; %s\r\n",
									 "dir","elrwx",
									 s_time->tm_year + 1900,
									 s_time->tm_mon + 1,
									 s_time->tm_mday,
									 s_time->tm_hour,
									 s_time->tm_min,
									 s_time->tm_sec,
									 fsd->vfs_dirent->d_name);
					} 
					else 
					{
						/* MLSD format: modify=YYYYMMDDHHmmss */
						len = sprintf(buffer, "type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d;size=%ld; %s\r\n", 
									"file","rwx",
									s_time->tm_year + 1900,
									s_time->tm_mon + 1,
									s_time->tm_mday,
									s_time->tm_hour,
									s_time->tm_min,
									s_time->tm_sec,
									st.st_size,
									fsd->vfs_dirent->d_name);
					}
				}
				LOGV("MLSD : %s",buffer);

				if (sfifo_space(&fsd->fifo) < len) {
					send_data(pcb, fsd);
					goto exit;
				}

				sfifo_write(&fsd->fifo, buffer, len);
				fsd->vfs_dirent = NULL;
			} 
		} else {
			struct ftpd_msgstate *fsm;
			struct tcp_pcb *msgpcb;

			if (sfifo_used(&fsd->fifo) > 0) {
				send_data(pcb, fsd);
				goto exit;
			}
			fsm = fsd->msgfs;
			msgpcb = fsd->msgpcb;
			if(fsd->vfs_dir) {
				closedir(fsd->vfs_dir);
				fsd->vfs_dir = NULL;
			}
			ftpd_dataclose(pcb, fsd);
			fsm->datapcb = NULL;
			fsm->datafs = NULL;
			fsm->state = FTPD_IDLE;
			send_msg(msgpcb, fsm, msg226_1);
			goto exit;
		}
	}

exit:
	if (path)
		os_free(path);
	if (cwd_buffer)
		os_free(cwd_buffer);
	if (buffer)
		os_free(buffer);
}

static err_t ftpd_datasent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	struct ftpd_datastate *fsd = arg;

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static err_t ftpd_datarecv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	if (err == ERR_OK && p != NULL) {
		struct pbuf *q;
		u16_t tot_len = 0;

		for (q = p; q != NULL; q = q->next) {
			int len;

			len = write(fsd->fd, q->payload, q->len);
			tot_len += len;
			if (len != q->len)
				break;
		}

		tcp_recved(pcb, tot_len);
		pbuf_free(p);
	}
	if (err == ERR_OK && p == NULL) {
		struct ftpd_msgstate *fsm;
		struct tcp_pcb *msgpcb;

		fsm = fsd->msgfs;
		msgpcb = fsd->msgpcb;
		if(-1 != fsd->fd) {
			close(fsd->fd);
			fsd->fd = -1;
		}
		ftpd_dataclose(pcb, fsd);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		fsm->passive = 0;
		fsm->state = FTPD_IDLE;
		send_msg(msgpcb, fsm, msg226);
	}

	return ERR_OK;
}

static err_t ftpd_dataconnected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	fsd->msgfs->datapcb = pcb;
	fsd->connected = 1;

	tcp_recv(pcb, ftpd_datarecv);
	tcp_sent(pcb, ftpd_datasent);
	tcp_err(pcb, ftpd_dataerr);

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static err_t ftpd_dataaccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	tcp_close(fsd->msgfs->datapcb);
	fsd->msgfs->datapcb = pcb;
	fsd->connected = 1;

	tcp_recv(pcb, ftpd_datarecv);
	tcp_sent(pcb, ftpd_datasent);
	tcp_err(pcb, ftpd_dataerr);

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static int open_dataconnection(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (fsm->passive) {
		return 0;
	}

	fsm->datafs = (struct ftpd_datastate *)ftp_malloc(sizeof(struct ftpd_datastate));
	if (fsm->datafs == NULL) {
		send_msg(pcb, fsm, msg451);
		return 1;
	}
	memset(fsm->datafs, 0, sizeof(struct ftpd_datastate));
	fsm->datafs->msgfs = fsm;
	fsm->datafs->msgpcb = pcb;
	sfifo_init(&fsm->datafs->fifo, MAX_PRE_READ_BUFFER_SIZE);
	fsm->datapcb = tcp_new();
	ip_set_option(fsm->datapcb, SOF_REUSEADDR);
	tcp_bind(fsm->datapcb, (ip_addr_t *)&pcb->local_ip, 20);
	tcp_arg(fsm->datapcb, fsm->datafs);
	ip_addr_t dataip;
	ip_addr_copy(dataip, fsm->dataip);
	tcp_connect(fsm->datapcb, &dataip, fsm->dataport, ftpd_dataconnected);

	return 0;
}

static void cmd_user(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (!strcmp(arg,FTP_USER))
	{
		send_msg(pcb, fsm, msg331);
		fsm->state = FTPD_PASS;
	}
	else
	{
		send_msg(pcb, fsm, msg530);
	}

}

static void cmd_pass(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (!strcmp(arg,FTP_PASSWORD))
	{
		send_msg(pcb, fsm, msg230);
		fsm->state = FTPD_IDLE;
	}
	else
	{
		send_msg(pcb, fsm, msg530);
	}
}

static void cmd_port(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int nr;
	unsigned pHi, pLo;
	unsigned ip[4];

	nr = sscanf(arg, "%u,%u,%u,%u,%u,%u", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]), &pHi, &pLo);
	if (nr != 6)
		send_msg(pcb, fsm, msg501);
	else {
		IP4_ADDR(&fsm->dataip, (u8_t) ip[0], (u8_t) ip[1], (u8_t) ip[2], (u8_t) ip[3]);
		fsm->dataport = ((u16_t) pHi << 8) | (u16_t) pLo;
		send_msg(pcb, fsm, msg200);
	}
}

static void cmd_quit(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg221);
	fsm->state = FTPD_QUIT;
}

static void cmd_cwd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *last_flash = NULL;
	char *new_path = NULL;
	char *path = NULL;

	new_path = os_malloc(MAX_PATH_LEN);
	if(new_path == NULL) {
		LOGE("cmd_cwd: malloc memory failed\r\n");
		goto exit;
	}
	os_memset(new_path, 0, MAX_PATH_LEN);

	if (strcmp(arg, "..") == 0) {
		if (chdir("..") != 0) {
			LOGE("%s chdir feiled\r\n",__func__);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}

		path = getcwd(new_path, MAX_PATH_LEN);

		if ((last_flash = strrchr(path,'/')) != NULL) {
			if (strcmp(last_flash,"/..") == 0) {
				*last_flash = '\0';
			}

			char *prev_slash = strrchr(path,'/');
			if (prev_slash != NULL) {
				*prev_slash = '\0';
			}

			if (path == NULL || path[0] == '\0') {
				path = "/";
			}
		}

		if (path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
			if (chdir(FTP_MOUNT_PATH) != 0)
				send_msg(pcb, fsm, msg550);
			else
				send_msg(pcb, fsm, msg250);
		} else {
			if (chdir(path) != 0)
				send_msg(pcb, fsm, msg550);
			else
				send_msg(pcb, fsm, msg250);
		}
	} else {
		const char *dir = arg;
		if (arg[0] == '\0' || (arg[0] == '/' && arg[1] == '\0') || strcmp(arg, FTP_MOUNT_PATH) == 0)
			dir = FTP_MOUNT_PATH;
		if (chdir(dir) == 0)
			send_msg(pcb, fsm, msg250);
		else
			send_msg(pcb, fsm, msg550);
	}

exit:
	if (new_path)
		os_free(new_path);
}

static void cmd_cdup(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *last_flash = NULL;
	char *new_path = NULL;
	char *path = NULL;

	new_path = os_malloc(MAX_PATH_LEN);
	if(new_path == NULL) {
		LOGE("%s: malloc memory failed\r\n",__func__);
		goto exit;
	}
	os_memset(new_path, 0, MAX_PATH_LEN);

	path = getcwd(new_path, MAX_PATH_LEN);

	if (chdir("..") != 0) {
		LOGE("%s chdir feiled\r\n",__func__);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	path = getcwd(new_path, MAX_PATH_LEN);
	if (path && (path[0] == '\0' || (path[0] == '/' && path[1] == '\0')))
		chdir(FTP_MOUNT_PATH);

	if ((last_flash = strrchr(path,'/')) != NULL) {
		if (strcmp(last_flash,"/..") == 0) {
			*last_flash = '\0';
		}

		char *prev_slash = strrchr(path,'/');
		if (prev_slash != NULL) {
			*prev_slash = '\0';
		}

		if (path == NULL || path[0] == '\0') {
			path = "/";
		}
	}

	send_msg(pcb, fsm, msg250,"CDUP");

exit:
	if (new_path)
		os_free(new_path);
}

static void cmd_pwd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *path = NULL;
	char *buffer = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_pwd: malloc memory failed\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = getcwd(buffer, MAX_PATH_LEN);
	if (!path)
		goto exit;
	if (path[0] == '/' && path[1] == '/')
		path++;
	if (path[0] == '\0' || (path[0] == '/' && path[1] == '\0'))
		path = (char *)FTP_MOUNT_PATH;
	send_msg(pcb, fsm, msg257PWD, path);

exit:
	if (buffer)
		os_free(buffer);
}

static void cmd_list_common(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, int list_type)
{
	DIR *vfs_dir;
	char *cwd;
	char *buffer = NULL;
	char *path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_list_common: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_list_common: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	
	do {
		cwd = getcwd(buffer, MAX_PATH_LEN);
		if ((!cwd)) {
			send_msg(pcb, fsm, msg451);
			break;
		}
		if (cwd[0] == '/' && cwd[1] == '/')
			cwd++;
		if (cwd[0] == '\0' || (cwd[0] == '/' && cwd[1] == '\0'))
			snprintf(path, MAX_PATH_LEN, "%s", FTP_MOUNT_PATH);
		else
			snprintf(path, MAX_PATH_LEN, "%s", cwd);
		LOGV("%s %d path %s\r\n",__func__,__LINE__,path);
		vfs_dir = opendir(path);
		if (!vfs_dir) {
			send_msg(pcb, fsm, msg451);
			break;
		}

		if (open_dataconnection(pcb, fsm) != 0) {
			closedir(vfs_dir);
			break;
		}

		fsm->datafs->vfs_dir = vfs_dir;
		fsm->datafs->vfs_dirent = NULL;

		LOGV("%s set fsm->state %d\r\n",__func__,list_type);
		fsm->state = list_type;

		send_msg(pcb, fsm, msg150);
	} while(0);

exit:
	if (path)
		os_free(path);
	if (buffer)
		os_free(buffer);
}

static void cmd_nlst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	cmd_list_common(arg, pcb, fsm, FTPD_NLST);
}

static void cmd_list(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	cmd_list_common(arg, pcb, fsm, FTPD_LIST);
}

static void cmd_retr(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int ret = -1;
	int fd = -1;
	struct stat st;
	off_t transfer_size;
	char *path = NULL;
	char *temp_path = NULL;
	char *buffer = NULL;

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else {
		if (arg[0] == '/') {
			if (strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
			    (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/'))
				snprintf(path, MAX_PATH_LEN, "%s", arg);
			else
				snprintf(path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
		} else {
			snprintf(path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
		}
	}
	LOGV("%s path %s\r\n",__func__,path);
	ret = stat(path, &st);
	if (0 != ret || !S_ISREG(st.st_mode)) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	if (fsm->restart_offset > 0) {
		if (lseek(fd, fsm->restart_offset, SEEK_SET) == (off_t)-1) {
			close(fd);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}

		if (fsm->restart_offset >= st.st_size) {
			close(fd);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}
		transfer_size = st.st_size - fsm->restart_offset;
	} else {
		transfer_size = st.st_size;
	}

	send_msg(pcb, fsm, msg150recv, arg, transfer_size);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_RETR;
	fsm->restart_offset = 0;

exit:
	if (path)
		os_free(path);
	if (buffer)
		os_free(buffer);
}

static void cmd_stor(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int fd = -1; //vfs_file_t *vfs_file;
	char *path = NULL;
	char *buffer = NULL;
	char *temp_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else {
		if (arg[0] == '/') {
			if (strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
			    (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/'))
				snprintf(path, MAX_PATH_LEN, "%s", arg);
			else
				snprintf(path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
		} else {
			snprintf(path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
		}
	}
	LOGV("%s path %s\r\n",__func__,path);
	fd = open(path, O_RDWR | O_CREAT | O_APPEND);
	if (-1 == fd) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	send_msg(pcb, fsm, msg150stor, arg);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_STOR;

exit:
	if (path)
		os_free(path);
    if (buffer)
	    os_free(buffer);
}

static void cmd_noop(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg200);
}

static void cmd_syst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg214SYST, "UNIX");
}

static void cmd_pasv(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	static u16_t port = FTPD_COMMON_PORT;
	static u16_t start_port = FTPD_COMMON_PORT;
	struct tcp_pcb *temppcb;

	fsm->datafs = (struct ftpd_datastate *)ftp_malloc(sizeof(struct ftpd_datastate));

	if (fsm->datafs == NULL) {
		send_msg(pcb, fsm, msg451);
		return;
	}
	memset(fsm->datafs, 0, sizeof(struct ftpd_datastate));

	fsm->datapcb = tcp_new();

	if (!fsm->datapcb) {
		os_free(fsm->datafs);
		send_msg(pcb, fsm, msg451);
		return;
	}

	sfifo_init(&fsm->datafs->fifo, MAX_PRE_READ_BUFFER_SIZE);

	start_port = port;

	while (1) {
		err_t err;

		if (++port > 0x7fff)
			port = FTPD_COMMON_PORT;

		fsm->dataport = port;
		err = tcp_bind(fsm->datapcb, (ip_addr_t *)&pcb->local_ip, fsm->dataport);
		if (err == ERR_OK)
			break;
		if (start_port == port)
			err = ERR_CLSD;
		if (err == ERR_USE)
			continue;
		if (err != ERR_OK) {
			ftpd_dataclose(fsm->datapcb, fsm->datafs);
			fsm->datapcb = NULL;
			fsm->datafs = NULL;
			return;
		}
	}

	fsm->datafs->msgfs = fsm;
	temppcb = tcp_listen(fsm->datapcb);

	if (!temppcb) {

		ftpd_dataclose(fsm->datapcb, fsm->datafs);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		return;
	}
	fsm->datapcb = temppcb;
	fsm->passive = 1;
	fsm->datafs->connected = 0;
	fsm->datafs->msgpcb = pcb;

	tcp_arg(fsm->datapcb, fsm->datafs);
	tcp_accept(fsm->datapcb, ftpd_dataaccept);
	send_msg(pcb, fsm, msg227, ip4_addr1(&pcb->local_ip), ip4_addr2(&pcb->local_ip), ip4_addr3(&pcb->local_ip), ip4_addr4(&pcb->local_ip), (fsm->dataport >> 8) & 0xff, (fsm->dataport) & 0xff);
}

static void cmd_abrt(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (fsm->datafs != NULL) {
		tcp_arg(fsm->datapcb, NULL);
		tcp_sent(fsm->datapcb, NULL);
		tcp_recv(fsm->datapcb, NULL);
		tcp_arg(fsm->datapcb, NULL);
		tcp_abort(pcb);
		sfifo_close(&fsm->datafs->fifo);
		os_free(fsm->datafs);
		fsm->datafs = NULL;
		fsm->passive = 0;
	}
	fsm->state = FTPD_IDLE;
}

static void cmd_type(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	LOGI("Got TYPE -%s-\r\n", arg);
	send_msg(pcb, fsm, msg200);
}

static void cmd_mode(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	LOGI("Got MODE -%s-\r\n", arg);
	send_msg(pcb, fsm, msg502);
}

static void cmd_rnfr(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (arg == NULL) {
		send_msg(pcb, fsm, msg501);
		return;
	}
	if (*arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}
	if (fsm->renamefrom)
		os_free(fsm->renamefrom);
	fsm->renamefrom = (char *)ftp_malloc(strlen(arg) + 1);
	if (fsm->renamefrom == NULL) {
		send_msg(pcb, fsm, msg451);
		return;
	}
	strcpy(fsm->renamefrom, arg);
	fsm->state = FTPD_RNFR;
	send_msg(pcb, fsm, msg350);
}

static void cmd_rnto(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *cwd_path = NULL;
	char *buffer = NULL;
	char *old_full_path = NULL;
	char *new_full_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);
	old_full_path = os_malloc(MAX_PATH_LEN);
	if (old_full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(old_full_path, 0, MAX_PATH_LEN);
	new_full_path = os_malloc(MAX_PATH_LEN);
	if (new_full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		goto exit;
	}
	os_memset(new_full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (fsm->state != FTPD_RNFR) {
		send_msg(pcb, fsm, msg503);
		goto exit;
	}
	fsm->state = FTPD_IDLE;

	cwd_path = getcwd(buffer, MAX_PATH_LEN);
	if (cwd_path == NULL) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (cwd_path[0] == '/' && cwd_path[1] == '/')
		cwd_path++;
	if (cwd_path[0] == '\0' || (cwd_path[0] == '/' && cwd_path[1] == '\0')) {
		snprintf(old_full_path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, fsm->renamefrom[0] == '/' ? fsm->renamefrom + 1 : fsm->renamefrom);
		snprintf(new_full_path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else {
		snprintf(old_full_path, MAX_PATH_LEN, "%s/%s", cwd_path, fsm->renamefrom[0] == '/' ? fsm->renamefrom + 1 : fsm->renamefrom);
		snprintf(new_full_path, MAX_PATH_LEN, "%s/%s", cwd_path, arg[0] == '/' ? arg + 1 : arg);
	}
	LOGV("%s old_full_path %s new_full_path %s\r\n",__func__,old_full_path,new_full_path);
	if (rename(old_full_path, new_full_path))
		send_msg(pcb, fsm, msg450);
	else
		send_msg(pcb, fsm, msg250);

exit:
	if (old_full_path)
		os_free(old_full_path);
	if (new_full_path)
		os_free(new_full_path);
	if (buffer)
		os_free(buffer);
}

static void cmd_mkd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *full_path = NULL;
	char *buffer = NULL;
	char *temp_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else if (arg[0] == '/' && strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
		   (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/')) {
		snprintf(full_path, MAX_PATH_LEN, "%s", arg);
	} else if (arg[0] == '/') {
		snprintf(full_path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
	} else {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
	}
	LOGV("%s full_path %s\r\n",__func__,full_path);
	if (mkdir(full_path, 0777) != 0)
		send_msg(pcb, fsm, msg550);
	else
		send_msg(pcb, fsm, msg257, arg);

exit:
	if (full_path)
		os_free(full_path);
	if (buffer)
		os_free(buffer);
}

static void cmd_rmd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	struct stat st;
	char *full_path = NULL;
	char *buffer = NULL;
	char *temp_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else if (arg[0] == '/' && strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
		   (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/')) {
		snprintf(full_path, MAX_PATH_LEN, "%s", arg);
	} else if (arg[0] == '/') {
		snprintf(full_path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
	} else {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
	}
	LOGV("%s full_path %s\r\n",__func__,full_path);
	if (stat(full_path, &st) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISDIR(st.st_mode)) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (rmdir(full_path) != 0)
		send_msg(pcb, fsm, msg550);
	else
		send_msg(pcb, fsm, msg250);

exit:
	if (full_path)
		os_free(full_path);
	if (buffer)
		os_free(buffer);
}

static void cmd_dele(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	struct stat st;
	char *cwd_path;
	char *buffer = NULL;
	char *full_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	cwd_path = getcwd(buffer, MAX_PATH_LEN);
	if (cwd_path == NULL) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (cwd_path[0] == '/' && cwd_path[1] == '/')
		cwd_path++;
	if (cwd_path[0] == '\0' || (cwd_path[0] == '/' && cwd_path[1] == '\0')) {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else if (arg[0] == '/' && strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
		   (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/')) {
		snprintf(full_path, MAX_PATH_LEN, "%s", arg);
	} else if (arg[0] == '/') {
		snprintf(full_path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
	} else {
		snprintf(full_path, MAX_PATH_LEN, "%s/%s", cwd_path, arg);
	}
	LOGV("%s full_path %s\r\n",__func__,full_path);
	if (stat(full_path, &st) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISREG(st.st_mode)) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (unlink(full_path) != 0)
		send_msg(pcb, fsm, msg550);
	else
		send_msg(pcb, fsm, msg250);

exit:
	if (full_path)
		os_free(full_path);
	if (buffer)
		os_free(buffer);
}

static void cmd_size(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	struct stat st;
	char *path = NULL;
	char *buffer = NULL;
	char *temp_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = os_malloc(MAX_PATH_LEN);
	if (path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	
	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else {
		if (arg[0] == '/') {
			if (strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
			    (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/'))
				snprintf(path, MAX_PATH_LEN, "%s", arg);
			else
				snprintf(path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
		} else {
			snprintf(path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
		}
	}
	LOGV("%s path %s\r\n",__func__,path);
	if (stat(path, &st) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISREG(st.st_mode)) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	send_msg(pcb, fsm, msg213, st.st_size);

exit:
	if (path)
		os_free(path);
	if (buffer)
		os_free(buffer);
}

static void cmd_reset(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	long offset;
	char *endptr = NULL;

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	offset = strtol(arg, &endptr, 10);
	if (*endptr != '\0' || offset < 0) {
		send_msg(pcb, fsm, msg501);
		return;
	}

	fsm->restart_offset = (off_t)offset;
	send_msg(pcb, fsm, msg350);
}

static void cmd_clnt(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	/* Log client name for debugging */
	LOGI("FTP client: %s\r\n", arg);

	send_msg(pcb, fsm, msg200);
}

static void cmd_feat(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg_FEAT);
}

static void cmd_opts(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	/* Parse option and value */
	if (strncmp(arg, "UTF8", 4) == 0) {
		/* UTF8 option - we support it */
		if (strlen(arg) > 5 && arg[4] == ' ') {
			/* OPTS UTF8 ON or OPTS UTF8 OFF */
			send_msg(pcb, fsm, msg200);
		} else {
			/* OPTS UTF8 */
			send_msg(pcb, fsm, msg200);
		}
	} else {
		/* Unknown option - return 501 */
		send_msg(pcb, fsm, msg501);
	}
}

static void cmd_appe(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int fd = -1;
	char *path = NULL;
	char *buffer = NULL;
	char *temp_path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = os_malloc(MAX_PATH_LEN);
	if (path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	temp_path = getcwd(buffer, MAX_PATH_LEN);
	if (temp_path && temp_path[0] == '/' && temp_path[1] == '/')
		temp_path++;
	if ((!temp_path || temp_path[0] == '\0') || (temp_path[0] == '/' && temp_path[1] == '\0')) {
		snprintf(path, MAX_PATH_LEN, "%s/%s", FTP_MOUNT_PATH, arg[0] == '/' ? arg + 1 : arg);
	} else if (arg[0] == '/' && strncmp(arg, FTP_MOUNT_PATH, strlen(FTP_MOUNT_PATH)) == 0 &&
		   (arg[strlen(FTP_MOUNT_PATH)] == '\0' || arg[strlen(FTP_MOUNT_PATH)] == '/')) {
		snprintf(path, MAX_PATH_LEN, "%s", arg);
	} else if (arg[0] == '/') {
		snprintf(path, MAX_PATH_LEN, "%s%s", FTP_MOUNT_PATH, arg);
	} else {
		snprintf(path, MAX_PATH_LEN, "%s/%s", temp_path, arg);
	}
	LOGV("%s path %s\r\n",__func__,path);
	fd = open(path, O_RDWR | O_CREAT | O_APPEND);
	if (-1 == fd) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	send_msg(pcb, fsm, msg150stor, arg);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_STOR;

exit:
	if (path)
		os_free(path);
	if (buffer)
		os_free(buffer);
}

static void cmd_mlsd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	cmd_list_common(arg, pcb, fsm, FTPD_MLSD);
}

static void cmd_mlst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	struct stat st = {0};
	struct tm *s_time = NULL;
	time_t current_time = {0};
	char *buffer = NULL;
	char *path_to_stat = NULL;
	char *cwd = NULL;
	char *cwd_buffer = NULL;
	char *full_path = NULL;

	buffer = os_malloc(FTP_TEMP_BUFFER_LEN);

	if (NULL == buffer) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, FTP_TEMP_BUFFER_LEN);

#if CONFIG_NTP_SYNC_RTC
	extern time_t timestamp_get();
	current_time = timestamp_get();
#else
	/* Fallback to time() if NTP is not configured */
	current_time = time(NULL);
#endif

	/* If no argument, use current directory */
	if (arg == NULL || *arg == '\0') {
		cwd_buffer = os_malloc(MAX_PATH_LEN);
		os_memset(cwd_buffer, 0, MAX_PATH_LEN);
		if (cwd_buffer == NULL) {
			LOGE("%s: memory malloc failed\r\n",__func__);
			send_msg(pcb, fsm, msg451);
			goto exit;
		}
		cwd = getcwd(cwd_buffer, MAX_PATH_LEN);
		if (cwd == NULL) {
			LOGE("%s: getcwd failed\r\n",__func__);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}
		path_to_stat = cwd;
	} else {
		path_to_stat = (char *)arg;
	}

	full_path = os_malloc(FULL_PATH_SIZE);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, FULL_PATH_SIZE);

	char *tmp = strchr(path_to_stat,'/');
	if(tmp) {
		path_to_stat = ++tmp;
	}

	LOGV("%s path_to_stat %s\r\n",__func__,path_to_stat);
	snprintf(full_path, FULL_PATH_SIZE, "%s/%s", FTP_MOUNT_PATH, path_to_stat);
	/* Get file/directory information */
	if (stat(full_path, &st) == -1) {
		LOGE("%s: stat failed\r\n",__func__);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	/* If st_mtime is 0 (file system doesn't support timestamps),
	 * use current time instead */
	if (st.st_mtime == 0) {
		s_time = gmtime(&current_time);
	} else {
		s_time = gmtime(&st.st_mtime);
	}

	/* MLST command returns MLSD format on control connection
	 * Format: 250- <MLSD line>
	 *         250 End
	 */
	if (S_ISDIR(st.st_mode)) {
		/* MLSD format: modify=YYYYMMDDHHmmss */
		sprintf(buffer, "250- type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d; %s",
				"dir","elrwx",
				s_time->tm_year + 1900,
				s_time->tm_mon + 1,
				s_time->tm_mday,
				s_time->tm_hour,
				s_time->tm_min,
				s_time->tm_sec,
				path_to_stat);
	} else {
		/* MLSD format: modify=YYYYMMDDHHmmss */
		sprintf(buffer, "250- type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d;size=%ld; %s",
				"file","rwx",
				s_time->tm_year + 1900,
				s_time->tm_mon + 1,
				s_time->tm_mday,
				s_time->tm_hour,
				s_time->tm_min,
				s_time->tm_sec,
				st.st_size,
				path_to_stat);
	}

	/* Send MLSD line on control connection */
	send_msg(pcb, fsm, buffer);
	/* Send final 250 response */
	send_msg(pcb, fsm, msg250);

exit:
	if (full_path)
		os_free(full_path);
	if (cwd_buffer)
		os_free(cwd_buffer);
	if (buffer)
		os_free(buffer);
}

struct ftpd_command {
	char *cmd;
	void (*func)(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm);
};

static struct ftpd_command ftpd_commands[] = {
	{"USER", cmd_user},
	{"PASS", cmd_pass},
	{"PORT", cmd_port},
	{"QUIT", cmd_quit},
	{"CWD", cmd_cwd},
	{"CDUP", cmd_cdup},
	{"PWD", cmd_pwd},
	{"XPWD", cmd_pwd},
	{"NLST", cmd_nlst},
	{"LIST", cmd_list},
	{"RETR", cmd_retr},
	{"STOR", cmd_stor},
	{"NOOP", cmd_noop},
	{"SYST", cmd_syst},
	{"ABOR", cmd_abrt},
	{"TYPE", cmd_type},
	{"MODE", cmd_mode},
	{"RNFR", cmd_rnfr},
	{"RNTO", cmd_rnto},
	{"MKD", cmd_mkd},
	{"XMKD", cmd_mkd},
	{"RMD", cmd_rmd},
	{"XRMD", cmd_rmd},
	{"DELE", cmd_dele},
	{"PASV", cmd_pasv},
	{"CLNT", cmd_clnt},
	{"FEAT", cmd_feat},
	{"OPTS", cmd_opts},
	{"MLSD", cmd_mlsd},
	{"APPE", cmd_appe},
	{"REST", cmd_reset},
	{"SIZE", cmd_size},
	//{"MLST", cmd_mlst},
	{NULL, NULL}
};

static void send_msgdata(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	err_t err;
	u16_t len;

	if (sfifo_used(&fsm->fifo) > 0) {
		int i;

		/* We cannot send more data than space available in the send
		   buffer. */
		if (tcp_sndbuf(pcb) < sfifo_used(&fsm->fifo))
			len = tcp_sndbuf(pcb);
		else
			len = (u16_t) sfifo_used(&fsm->fifo);

		i = fsm->fifo.readpos;
		if ((i + len) > fsm->fifo.size) {
			err = tcp_write(pcb, fsm->fifo.buffer + i, (u16_t)(fsm->fifo.size - i), 1);
			if (err != ERR_OK) {
				LOGW("send_msgdata: error writing!\r\n");
				return;
			}
			len -= fsm->fifo.size - i;
			fsm->fifo.readpos = 0;
			i = 0;
		}

		err = tcp_write(pcb, fsm->fifo.buffer + i, len, 1);
		if (err != ERR_OK) {
			LOGW("send_msgdata: error writing!\r\n");
			return;
		}
		fsm->fifo.readpos += len;
	}
}

static void send_msg(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, char *msg, ...)
{
	va_list arg;
	char *buffer = NULL;
	int len;

	buffer = os_malloc(FTP_TEMP_BUFFER_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		goto exit;
	}
	os_memset(buffer, 0, FTP_TEMP_BUFFER_LEN);

	va_start(arg, msg);
	vsprintf(buffer, msg, arg);
	va_end(arg);
	strcat(buffer, "\r\n");
	len = strlen(buffer);
	if (sfifo_space(&fsm->fifo) < len) {
		LOGE("%s: sfifo_space is not enough\r\n",__func__);
		goto exit;
	}
	sfifo_write(&fsm->fifo, buffer, len);
	LOGI("response: %s", buffer);
	send_msgdata(pcb, fsm);

exit:
	if (buffer)
		os_free(buffer);
}

static void ftpd_msgerr(void *arg, err_t err)
{
	struct ftpd_msgstate *fsm = arg;

	LOGE("ftpd_msgerr: %s (%i)\r\n", lwip_strerr(err), err);
	if (fsm == NULL)
		return;
	if (fsm->datafs) {
		ftpd_dataclose(fsm->datapcb, fsm->datafs);
	}
	sfifo_close(&fsm->fifo);
	// vfs_close(fsm->vfs);
	// fsm->vfs = NULL;

	if (fsm->renamefrom)
		os_free(fsm->renamefrom);
	fsm->renamefrom = NULL;
	os_free(fsm);
}

static void ftpd_msgclose(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	if (fsm->datafs) {
		ftpd_dataclose(fsm->datapcb, fsm->datafs);
	}
	sfifo_close(&fsm->fifo);
	// vfs_close(fsm->vfs);
	// fsm->vfs = NULL;
	if (fsm->renamefrom)
		os_free(fsm->renamefrom);
	fsm->renamefrom = NULL;
	os_free(fsm);
	tcp_arg(pcb, NULL);
	tcp_close(pcb);
}

static err_t ftpd_msgsent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	struct ftpd_msgstate *fsm = arg;

	if (pcb->state > ESTABLISHED)
		return ERR_OK;

	if ((sfifo_used(&fsm->fifo) == 0) && (fsm->state == FTPD_QUIT)) {
		ftpd_msgclose(pcb, fsm);
		return ERR_OK;
	}

	send_msgdata(pcb, fsm);

	return ERR_OK;
}

static err_t ftpd_msgrecv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char *text;
	struct ftpd_msgstate *fsm = arg;

	if (err == ERR_OK && p != NULL) {

		/* Inform TCP that we have taken the data. */
		tcp_recved(pcb, p->tot_len);

		text = (char *)ftp_malloc(p->tot_len + 1);
		if (text) {
			char cmd[5];
			struct pbuf *q;
			char *pt = text;
			struct ftpd_command *ftpd_cmd;

			for (q = p; q != NULL; q = q->next) {
				bcopy(q->payload, pt, q->len);
				pt += q->len;
			}
			*pt = '\0';

			pt = &text[strlen(text) - 1];
			while (((*pt == '\r') || (*pt == '\n')) && pt >= text)
				*pt-- = '\0';

			LOGI("query: %s\r\n", text);

			strncpy(cmd, text, 4);
			for (pt = cmd; isalpha((int)*pt) && pt < &cmd[4]; pt++)
				*pt = toupper(*pt);
			*pt = '\0';

			for (ftpd_cmd = ftpd_commands; ftpd_cmd->cmd != NULL; ftpd_cmd++) {
				if (!strcmp(ftpd_cmd->cmd, cmd))
					break;
			}

			if (strlen(text) < (strlen(cmd) + 1))
				pt = "";
			else
				pt = &text[strlen(cmd) + 1];

			if (ftpd_cmd->func)
				ftpd_cmd->func(pt, pcb, fsm);
			else
				send_msg(pcb, fsm, msg502);

			os_free(text);
		}
		pbuf_free(p);
	}
	else if ((err == ERR_OK && p == NULL)) {
		ftpd_msgclose(pcb, fsm);
	}

	return ERR_OK;
}

static err_t ftpd_msgpoll(void *arg, struct tcp_pcb *pcb)
{
	struct ftpd_msgstate *fsm = arg;

	if (fsm == NULL)
		return ERR_OK;

	if (fsm->datafs) {
		if (fsm->datafs->connected) {
			switch (fsm->state) {
			case FTPD_LIST:
			case FTPD_NLST:
			case FTPD_MLSD:
				send_next_directory(fsm->datafs, fsm->datapcb, fsm->state);
				break;
			case FTPD_RETR:
				send_file(fsm->datafs, fsm->datapcb);
				break;
			default:
				break;
			}
		}
	}

	return ERR_OK;
}

static err_t ftpd_msgaccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_msgstate *fsm;

	fsm = (struct ftpd_msgstate *)ftp_malloc(sizeof(struct ftpd_msgstate));
	if (fsm == NULL) {
		LOGE("%s: malloc memory failed\r\n",__func__);
		return ERR_MEM;
	}
	memset(fsm, 0, sizeof(struct ftpd_msgstate));

	sfifo_init(&fsm->fifo, MAX_PRE_READ_BUFFER_SIZE);
	fsm->state = FTPD_IDLE;

	tcp_arg(pcb, fsm);
	tcp_recv(pcb, ftpd_msgrecv);
	tcp_sent(pcb, ftpd_msgsent);
	tcp_err(pcb, ftpd_msgerr);
	tcp_poll(pcb, ftpd_msgpoll, 1);
	send_msg(pcb, fsm, msg220);

	return ERR_OK;
}

beken_thread_t ftpd_server_task = NULL;

static void ftpd_server_cc_main(beken_thread_arg_t data)
{
	err_t err = ERR_OK;
	struct tcp_pcb *pcb;

	pcb = tcp_new();
    ip_set_option(pcb, SOF_REUSEADDR);
    err = tcp_bind(pcb, IP_ADDR_ANY, 21);
    if (err != ERR_OK) {
        LOGE("ftp bind failed, ret: %d\r\n", err);
    }
	pcb = tcp_listen(pcb);
	tcp_accept(pcb, ftpd_msgaccept);
	ftp_is_running = 1;

	while(ftp_is_running) {
		rtos_delay_milliseconds(100);
	}

	ftp_is_running = 0;
	tcp_close(pcb);
	ftpd_server_task = NULL;
	rtos_delete_thread(NULL);
}

#if (CONFIG_FATFS)
static int _fs_mount(void)
{
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;
	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
#if (CONFIG_SDCARD)
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
#else
	partition.part_dev.device_name = FATFS_DEV_FLASH;
#endif

	partition.mount_path = FTP_MOUNT_PATH;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}
#endif


bk_err_t ftpd_server_init(void)
{
	int ret;

#if (CONFIG_FATFS)
	ret = _fs_mount();
	if (BK_OK != ret)
	{
		bk_printf("[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
#endif

	ret = rtos_create_thread(&ftpd_server_task,
							 4,
							 "ftpd_server",
							 (beken_thread_function_t)ftpd_server_cc_main,
							 1024 * 4,
							 (beken_thread_arg_t)NULL);
	if (ret != kNoErr)
	{
		bk_printf("Error: Failed to create ftpd server: %d\n", ret);
		return kGeneralErr;
	}

	return kNoErr;
}

void ftpd_server_deinit(void)
{
	bk_err_t ret = BK_FAIL;
	LOGI("[%s]\r\n", __FUNCTION__);

	ftp_is_running = 0;

#if (CONFIG_FATFS)
	ret = umount(FTP_MOUNT_PATH);
	if (BK_OK != ret) {
		LOGE("[%s][%d] unmount fail: %d\r\n", __FUNCTION__, __LINE__, ret);
	}
#endif
	LOGI("[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);
}

#if CONFIG_CLI
void cli_wifi_ftp_server_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		bk_printf("Invalid ftp server paramter\r\n");
		goto error;
	}

	if(os_strcmp(argv[1], "server") == 0) {
		#if CONFIG_FTP_SERVER
		#if CONFIG_VFS
		ftpd_server_init();
		#endif
		#endif
	}
	else {
		bk_printf("Invalid ftp server paramter\r\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

static const struct cli_command s_ftpd_commands[] = {
    {"ftp", "ftp server", cli_wifi_ftp_server_cmd},
};

int ftpd_cli_init(void)
{
    return cli_register_commands(s_ftpd_commands, FTPD_CMD_CNT);
}
#endif
#endif
