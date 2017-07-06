/*
 * dtparser
 *
 *
 * dtparser is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 * Copyright (c) 2017 Partha Susarla <mail@spartha.org>
 */

#ifndef _DT_PARSER_H
#define _DT_PARSER_H

#include <time.h>
#include <sys/time.h>

int parse_from_rfc5322(const char *str, time_t *t);
int parse_to_rfc5322(time_t date, char *buf, size_t len);

#endif  /* _DT_PARSER_H */
