/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2010 Teclo Networks AG
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <errno.h>
#include <cstdarg>
#include <cstdbool>

#include "attributes.h"

void use_syslog(const char *ident, bool include_pid);
void fail(const char *format, ...) CHECK_PRINTF_ARGS NO_RETURN;
void fail_with_errno(const char *format, ...) CHECK_PRINTF_ARGS NO_RETURN;
void fail_with_srcloc(const char *file, int line, const char *format, ...) NO_RETURN;
void warn(const char *format, ...) CHECK_PRINTF_ARGS;
void warn_with_errno(const char *format, ...) CHECK_PRINTF_ARGS;
void info(const char *format, ...) CHECK_PRINTF_ARGS;

extern bool need_fresh_line;
bool fresh_line(void);

#define bug()         fail_with_srcloc(__FILE__, __LINE__, "How did this happen?")
#define fallthrough() fail_with_srcloc(__FILE__, __LINE__, "Fallthrough.")

#endif  /* _LOG_H_ */
