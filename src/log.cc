/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#include "log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static bool syslog_enabled = false;

void use_syslog(const char *ident, bool include_pid) {
    if (!syslog_enabled) {
        openlog(ident, LOG_NDELAY | (include_pid ? LOG_PID : 0) | LOG_NOWAIT, LOG_LOCAL0);
        syslog_enabled = true;
    }
}

static void print_header(const char* label, const char* file, int line) {
    fresh_line();

    if (file) {
        fprintf(stderr, "%s: at %s:%d. ",
                label, file, line);
    } else {
        fprintf(stderr, "%s: ",
                label);
    }
}

static void print_footer(const char* format) {
    if (format[strlen(format)-1] != '\n') {
        fputc('\n', stderr);
    }
}

void fail(const char *format, ...)
{
    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);

    print_header("Fail", NULL, -1);
    vfprintf(stderr, format, ap);
    print_footer(format);

    /* Print to syslog after we have printed to stderr. */
    if (syslog_enabled) {
        vsyslog(LOG_CRIT, format, aq);
    }

    va_end(aq);
    va_end(ap);

    abort();
}

void fail_with_errno(const char *format, ...)
{
    int orig_errno = errno;

    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);

    print_header("Fail", NULL, -1);
    vfprintf(stderr, format, ap);

    if (syslog_enabled) {
        vsyslog(LOG_CRIT, format, aq);
    }

    va_end(aq);
    va_end(ap);

    char error[256];
    if (!strerror_r(orig_errno, error, sizeof(error))) {
        fprintf(stderr, " (%s)\n", error);
    } else {
        fprintf(stderr, " (%d)\n", orig_errno);
    }

    abort();
}

void fail_with_srcloc(const char *file, int line, const char* format, ...)
{
    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);

    print_header("Fail", file, line);
    vfprintf(stderr, format, ap);
    print_footer(format);

    if (syslog_enabled) {
        vsyslog(LOG_CRIT, format, aq);
    }

    va_end(aq);
    va_end(ap);

    abort();

}

void warn(const char *format, ...)
{
    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);

    print_header("Warn", NULL, -1);
    vfprintf(stderr, format, ap);
    print_footer(format);

    if (syslog_enabled) {
        vsyslog(LOG_WARNING, format, aq);
    }

    va_end(aq);
    va_end(ap);
}

void warn_with_errno(const char *format, ...)
{
    int orig_errno = errno;

    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);

    print_header("Warn", NULL, -1);
    vfprintf(stderr, format, ap);

    if (syslog_enabled) {
        vsyslog(LOG_WARNING, format, aq);
    }

    va_end(aq);
    va_end(ap);

    char error[256];
    if (!strerror_r(orig_errno, error, sizeof(error))) {
        fprintf(stderr, " (%s)\n", error);
    } else {
        fprintf(stderr, " (%d)\n", orig_errno);
    }
}

void info(const char *format, ...)
{
    va_list ap, aq;
    va_start(ap, format);
    va_copy(aq, ap);
    print_header("Info", NULL, -1);
    vfprintf(stderr, format, ap);
    print_footer(format);

    if (syslog_enabled) {
        vsyslog(LOG_INFO, format, aq);
    }

    va_end(aq);
    va_end(ap);
}

bool need_fresh_line = false;

bool fresh_line(void)
{
    if (need_fresh_line) {
        printf("\n");
        need_fresh_line = false;
        return true;
    }
    return false;
}
