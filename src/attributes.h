/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

#ifdef __GNUC__
#define CHECK_PRINTF_ARGS __attribute__((format(printf, 1, 2)))
#define NO_RETURN __attribute__((noreturn))
#define IGNORABLE __attribute__((unused))
#else
#define CHECK_PRINTF_ARGS
#define NO_RETURN
#define IGNORABLE
#endif

#endif  /* _ATTRIBUTES_H_ */
