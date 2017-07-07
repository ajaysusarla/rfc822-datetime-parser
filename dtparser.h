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

int rfc5322_date_parse(const char *str, size_t len, time_t *t);
int rfc5322_date_create(time_t date, char *buf, size_t len);

#endif  /* _DT_PARSER_H */
