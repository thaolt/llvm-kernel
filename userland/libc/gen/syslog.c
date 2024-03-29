/*	$NetBSD: syslog.c,v 1.47 2009/01/11 02:46:27 christos Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)syslog.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: syslog.c,v 1.47 2009/01/11 02:46:27 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "reentrant.h"
#include "extern.h"

#ifdef __weak_alias
__weak_alias(closelog,_closelog)
__weak_alias(openlog,_openlog)
__weak_alias(setlogmask,_setlogmask)
__weak_alias(syslog,_syslog)
__weak_alias(vsyslog,_vsyslog)
__weak_alias(syslogp,_syslogp)
__weak_alias(vsyslogp,_vsyslogp)

__weak_alias(closelog_r,_closelog_r)
__weak_alias(openlog_r,_openlog_r)
__weak_alias(setlogmask_r,_setlogmask_r)
__weak_alias(syslog_r,_syslog_r)
__weak_alias(vsyslog_r,_vsyslog_r)
__weak_alias(syslog_ss,_syslog_ss)
__weak_alias(vsyslog_ss,_vsyslog_ss)
__weak_alias(syslogp_r,_syslogp_r)
__weak_alias(vsyslogp_r,_vsyslogp_r)
__weak_alias(syslogp_ss,_syslogp_ss)
__weak_alias(vsyslogp_ss,_vsyslogp_ss)
#endif

static struct syslog_data sdata = SYSLOG_DATA_INIT;

static void	openlog_unlocked_r(const char *, int, int,
    struct syslog_data *);
static void	disconnectlog_r(struct syslog_data *);
static void	connectlog_r(struct syslog_data *);

#define LOG_SIGNAL_SAFE	(int)0x80000000


#ifdef _REENTRANT
static mutex_t	syslog_mutex = MUTEX_INITIALIZER;
#endif

static char hostname[MAXHOSTNAMELEN];

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

void
vsyslog(int pri, const char *fmt, va_list ap)
{
	vsyslog_r(pri, &sdata, fmt, ap);
}

/*
 * syslogp, vsyslogp --
 *	like syslog but take additional arguments for MSGID and SD
 */
void
syslogp(int pri, const char *msgid, const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	vsyslogp(pri, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
vsyslogp(int pri, const char *msgid, const char *sdfmt, const char *msgfmt, va_list ap)
{
	vsyslogp_r(pri, &sdata, msgid, sdfmt, msgfmt, ap);
}

void
openlog(const char *ident, int logstat, int logfac)
{
	openlog_r(ident, logstat, logfac, &sdata);
}

void
closelog(void)
{
	closelog_r(&sdata);
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
	return setlogmask_r(pmask, &sdata);
}

/* Reentrant version of syslog, i.e. syslog_r() */

void
syslog_r(int pri, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog_r(pri, data, fmt, ap);
	va_end(ap);
}

void
syslogp_r(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	vsyslogp_r(pri, data, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
syslog_ss(int pri, struct syslog_data *data, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog_r(pri | LOG_SIGNAL_SAFE, data, fmt, ap);
	va_end(ap);
}

void
syslogp_ss(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, ...)
{
	va_list ap;

	va_start(ap, msgfmt);
	vsyslogp_r(pri | LOG_SIGNAL_SAFE, data, msgid, sdfmt, msgfmt, ap);
	va_end(ap);
}

void
vsyslog_ss(int pri, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslog_r(pri | LOG_SIGNAL_SAFE, data, fmt, ap);
}

void
vsyslogp_ss(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, va_list ap)
{
	vsyslogp_r(pri | LOG_SIGNAL_SAFE, data, msgid, sdfmt, msgfmt, ap);
}


void
vsyslog_r(int pri, struct syslog_data *data, const char *fmt, va_list ap)
{
	vsyslogp_r(pri, data, NULL, NULL, fmt, ap);
}

void
vsyslogp_r(int pri, struct syslog_data *data, const char *msgid,
	const char *sdfmt, const char *msgfmt, va_list ap)
{
	size_t cnt, prlen, tries;
	char ch, *p, *t;
	struct timeval tv;
	struct tm tmnow;
	time_t now;
	int fd, saved_errno;
#define TBUF_LEN	2048
#define FMT_LEN		1024
#define MAXTRIES	10
	char *stdp = NULL;	/* pacify gcc */
	char tbuf[TBUF_LEN], fmt_cpy[FMT_LEN], fmt_cat[FMT_LEN] = "";
	size_t tbuf_left, fmt_left;
	char *fmt = fmt_cat;
	int signal_safe = pri & LOG_SIGNAL_SAFE;
	int opened;

	pri &= ~LOG_SIGNAL_SAFE;

#define INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		syslog_r(INTERNALLOG | signal_safe, data,
		    "syslog_r: unknown facility/priority: %x", pri);
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if (!(LOG_MASK(LOG_PRI(pri)) & data->log_mask))
		return;

	saved_errno = errno;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= data->log_fac;

	/* Build the message. */
	p = tbuf;
	tbuf_left = TBUF_LEN;

#define DEC()							\
	do {							\
		if (prlen >= tbuf_left)				\
			prlen = tbuf_left - 1;			\
		p += prlen;					\
		tbuf_left -= prlen;				\
	} while (/*CONSTCOND*/0)

	prlen = snprintf_ss(p, tbuf_left, "<%d>1 ", pri);
	DEC();

	if (!signal_safe && (gettimeofday(&tv, NULL) != -1)) {
		/* strftime() implies tzset(), localtime_r() doesn't. */
		tzset();
		now = (time_t) tv.tv_sec;
		localtime_r(&now, &tmnow);

		prlen = strftime(p, tbuf_left, "%FT%T", &tmnow);
		DEC();
		prlen = snprintf(p, tbuf_left, ".%06ld", (long)tv.tv_usec);
		DEC();
		prlen = strftime(p, tbuf_left-1, "%z", &tmnow);
		/* strftime gives eg. "+0200", but we need "+02:00" */
		if (prlen == 5) {
			p[prlen+1] = p[prlen];
			p[prlen]   = p[prlen-1];
			p[prlen-1] = p[prlen-2];
			p[prlen-2] = ':';
			prlen += 1;
		}
	} else {
		prlen = snprintf_ss(p, tbuf_left, "-");

		/* if gmtime_r() was signal-safe we could output the UTC-time:
		gmtime_r(&now, &tmnow);
		prlen = strftime(p, tbuf_left, "%FT%TZ", &tmnow);
		*/
	}
	DEC();
	prlen = snprintf_ss(p, tbuf_left, " %s ", hostname);
	DEC();

	if (data->log_stat & LOG_PERROR)
		stdp = p;
	if (data->log_tag == NULL)
		data->log_tag = getprogname();

	prlen = snprintf_ss(p, tbuf_left, "%s ",
	    data->log_tag ? data->log_tag : "-");
	DEC();

	if (data->log_stat & LOG_PID)
		prlen = snprintf_ss(p, tbuf_left, "%d ", getpid());
	else
		prlen = snprintf_ss(p, tbuf_left, "- ");
	DEC();

	/*
	 * concat the format strings, then use one vsnprintf()
	 */
	if (msgid != NULL && *msgid != '\0') {
		strlcat(fmt_cat, msgid, FMT_LEN);
		strlcat(fmt_cat, " ", FMT_LEN);
	} else
		strlcat(fmt_cat, "- ", FMT_LEN);

	if (sdfmt != NULL && *sdfmt != '\0') {
		strlcat(fmt_cat, sdfmt, FMT_LEN);
	} else
		strlcat(fmt_cat, "-", FMT_LEN);

	if (msgfmt != NULL && *msgfmt != '\0') {
		strlcat(fmt_cat, " ", FMT_LEN);
		strlcat(fmt_cat, msgfmt, FMT_LEN);
	}

	/*
	 * We wouldn't need this mess if printf handled %m, or if
	 * strerror() had been invented before syslog().
	 */
	for (t = fmt_cpy, fmt_left = FMT_LEN; (ch = *fmt) != '\0'; ++fmt) {
		if (ch == '%' && fmt[1] == 'm') {
			char ebuf[128];
			++fmt;
			if (signal_safe ||
			    strerror_r(saved_errno, ebuf, sizeof(ebuf)))
				prlen = snprintf_ss(t, fmt_left, "Error %d",
				    saved_errno);
			else
				prlen = snprintf_ss(t, fmt_left, "%s", ebuf);
			if (prlen >= fmt_left)
				prlen = fmt_left - 1;
			t += prlen;
			fmt_left -= prlen;
		} else if (ch == '%' && fmt[1] == '%' && fmt_left > 2) {
			*t++ = '%';
			*t++ = '%';
			fmt++;
			fmt_left -= 2;
		} else {
			if (fmt_left > 1) {
				*t++ = ch;
				fmt_left--;
			}
		}
	}
	*t = '\0';

	if (signal_safe)
		prlen = vsnprintf_ss(p, tbuf_left, fmt_cpy, ap);
	else
		prlen = vsnprintf(p, tbuf_left, fmt_cpy, ap);
	DEC();
	cnt = p - tbuf;

	/* Output to stderr if requested. */
	if (data->log_stat & LOG_PERROR) {
		struct iovec iov[2];

		iov[0].iov_base = stdp;
		iov[0].iov_len = cnt - (stdp - tbuf);
		iov[1].iov_base = __UNCONST("\n");
		iov[1].iov_len = 1;
		(void)writev(STDERR_FILENO, iov, 2);
	}

	/* Get connected, output the message to the local logger. */
	if (data == &sdata)
		mutex_lock(&syslog_mutex);
	opened = !data->opened;
	if (opened)
		openlog_unlocked_r(data->log_tag, data->log_stat, 0, data);
	connectlog_r(data);

	/*
	 * If the send() failed, there are two likely scenarios:
	 *  1) syslogd was restarted
	 *  2) /dev/log is out of socket buffer space
	 * We attempt to reconnect to /dev/log to take care of
	 * case #1 and keep send()ing data to cover case #2
	 * to give syslogd a chance to empty its socket buffer.
	 */
	for (tries = 0; tries < MAXTRIES; tries++) {
		if (send(data->log_file, tbuf, cnt, 0) != -1)
			break;
		if (errno != ENOBUFS) {
			disconnectlog_r(data);
			connectlog_r(data);
		} else
			(void)usleep(1);
	}

	/*
	 * Output the message to the console; try not to block
	 * as a blocking console should not stop other processes.
	 * Make sure the error reported is the one from the syslogd failure.
	 */
	if (tries == MAXTRIES && (data->log_stat & LOG_CONS) &&
	    (fd = open(_PATH_CONSOLE, O_WRONLY|O_NONBLOCK, 0)) >= 0 &&
	    (p = strchr(tbuf, '>')) != NULL) {
		struct iovec iov[2];
		iov[0].iov_base = ++p;
		iov[0].iov_len = cnt - (p - tbuf);
		iov[1].iov_base = __UNCONST("\r\n");
		iov[1].iov_len = 2;
		(void)writev(fd, iov, 2);
		(void)close(fd);
	}

	if (data == &sdata)
		mutex_unlock(&syslog_mutex);

	if (data != &sdata && opened) {
		/* preserve log tag */
		const char *ident = data->log_tag;
		closelog_r(data);
		data->log_tag = ident;
	}
}

static void
disconnectlog_r(struct syslog_data *data)
{
	/*
	 * If the user closed the FD and opened another in the same slot,
	 * that's their problem.  They should close it before calling on
	 * system services.
	 */
	if (data->log_file != -1) {
		(void)close(data->log_file);
		data->log_file = -1;
	}
	data->connected = 0;		/* retry connect */
}

static void
connectlog_r(struct syslog_data *data)
{
	/* AF_UNIX address of local logger */
	static const struct sockaddr_un sun = {
		.sun_family = AF_LOCAL,
		.sun_len = sizeof(sun),
		.sun_path = _PATH_LOG,
	};

	if (data->log_file == -1 || fcntl(data->log_file, F_GETFL, 0) == -1) {
		if ((data->log_file = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
			return;
		(void)fcntl(data->log_file, F_SETFD, FD_CLOEXEC);
		data->connected = 0;
	}
	if (!data->connected) {
		if (connect(data->log_file,
		    (const struct sockaddr *)(const void *)&sun,
		    sizeof(sun)) == -1) {
			(void)close(data->log_file);
			data->log_file = -1;
		} else
			data->connected = 1;
	}
}

static void
openlog_unlocked_r(const char *ident, int logstat, int logfac,
    struct syslog_data *data)
{
	if (ident != NULL)
		data->log_tag = ident;
	data->log_stat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		data->log_fac = logfac;

	if (data->log_stat & LOG_NDELAY)	/* open immediately */
		connectlog_r(data);

	/* We could cache this, but then it might change */
	if (gethostname(hostname, sizeof(hostname)) == -1
	    || hostname[0] == '\0') {
		/* can this really happen? */
		hostname[0] = '-';
		hostname[1] = '\0';
	}
	data->opened = 1;
}

void
openlog_r(const char *ident, int logstat, int logfac, struct syslog_data *data)
{
	if (data == &sdata)
		mutex_lock(&syslog_mutex);
	openlog_unlocked_r(ident, logstat, logfac, data);
	if (data == &sdata)
		mutex_unlock(&syslog_mutex);
}

void
closelog_r(struct syslog_data *data)
{
	if (data == &sdata)
		mutex_lock(&syslog_mutex);
	(void)close(data->log_file);
	data->log_file = -1;
	data->connected = 0;
	data->log_tag = NULL;
	if (data == &sdata)
		mutex_unlock(&syslog_mutex);
}

int
setlogmask_r(int pmask, struct syslog_data *data)
{
	int omask;

	omask = data->log_mask;
	if (pmask != 0)
		data->log_mask = pmask;
	return omask;
}
