/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#ifndef _SEQUTIL_H_
#define _SEQUTIL_H_

// TCP sequence numbers are 32 bit integers operated on with modular arithmetic.

static inline uint32_t seq_lt (uint32_t a, uint32_t b) {
  return ((int32_t) (a - b)) <  0;
}

static inline uint32_t seq_leq(uint32_t a, uint32_t b) {
  return ((int32_t) (a - b)) <= 0;
}

static inline uint32_t seq_gt (uint32_t a, uint32_t b) {
  return ((int32_t) (a - b)) >  0;
}

static inline uint32_t seq_geq(uint32_t a, uint32_t b) {
  return ((int32_t) (a - b)) >= 0;
}

static inline uint32_t seq_min(uint32_t a, uint32_t b) {
  return seq_lt(a,b) ? a : b;
}

static inline uint32_t seq_max(uint32_t a, uint32_t b) {
  return seq_gt(a,b) ? a : b;
}

/* inclusive */
static inline uint32_t seq_within(uint32_t a, uint32_t min, uint32_t max)
{
    return seq_geq(a, min) && seq_leq(a, max);
}

#endif  /* _SEQUTIL_H_ */
