/*
 * dtparser
 *
 *
 * dtparser is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 * Copyright (c) 2017 Partha Susarla <mail@spartha.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dtparser.h"

#define TEST_ARR_SZ 16

int main(void)
{
        int i;
        const char *input[TEST_ARR_SZ] = {
                "15-OCT-2010 03:19:52 +1100",
                "15-OCT-2010 03:19:52"
                "+1100",
                " 5-Oct-2010 03:19:52 +1100",
                " 3-jan-2009 04:05    -0400",
                "Tue, 20 Jun 2017 00:49:38 +0000",
                " 3-jan-2009 04:05    +0400",
                " 3-jan-09 04:05    -0400",
                " 3-jan-09 04:05  ",
                "foobar, 2 +5000",
                "Sun, Hello, World!",
                "-1000",
                "20 Jun 2017 00:49:38 +0000",
                "15-Oct-95 03:19:52-Z",
                "15-Oct-95 03:19:52-A",
                "15-Oct-95 03:19:52-J",
                "15-Oct-95 03:19:52-Y"
        };

        for (i=0; i < TEST_ARR_SZ; i++) {
                time_t t = 0;
                int ret;

                printf(">> Input : %s\n", input[i]);
                ret = parse_time(input[i], &t);
                if ( ret != -1) {
                        printf(">> Output: %s\n", ctime(&t));
                } else {
                        printf(">> Output: FAILED Parsing!\n\n");
                }
        }

        exit(EXIT_SUCCESS);
}
