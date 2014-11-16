/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#include "strutil.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>

using std::string;
using std::vector;

vector<string> split_string_to_vector(const string& str,
                                      const string& delimiter) {
    vector<string> ret;
    size_t start = 0, end;

    do {
        end = str.find(delimiter, start);
        ret.push_back(str.substr(start, end - start));
        start = end + delimiter.size();
    } while (end != str.npos);

    return ret;
}

string vstringprintf(const char* format, va_list ap) {
    string ret;
    va_list ap_copy;

    va_copy(ap_copy, ap);
    int len = vsnprintf(&ret[0], 0, format, ap_copy);
    assert(len >= 0);
    ret.resize(len+1);
    va_end(ap_copy);

    len = vsnprintf(&ret[0], len+1, format, ap);
    ret.resize(len);

    return ret;
}

string stringprintf(const char* format, ...) {
    string ret;

    va_list ap;
    va_start(ap, format);
    ret = vstringprintf(format, ap);
    va_end(ap);

    return ret;
}
