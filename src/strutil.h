/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#ifndef _CUTIL_STRUTIL_H_
#define _CUTIL_STRUTIL_H_

#include <string>
#include <vector>
#include <cstdarg>

#include "attributes.h"

std::vector<std::string> split_string_to_vector(const std::string& str,
                                                const std::string& delimiter);
std::string stringprintf(const char *format, ...) CHECK_PRINTF_ARGS;

#endif // _CUTIL_STRUTIL_H_

