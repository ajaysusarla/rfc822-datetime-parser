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

int parse_time(const char *str, time_t *t);


#endif  /* _DT_PARSER_H */
