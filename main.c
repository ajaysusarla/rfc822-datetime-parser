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

#define TEST_ARR_SZ 20

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
                "15-Oct-95 03:19:52-Y",
               " 3-jan-2009 04.05    -0400",
                "20 Jun 2017 00.49.38 +0000",
                "Thu, 29 Feb 2001 11:22:33 +1100",
                "Tue, 29 Feb 2000 11:22:33 +1100"
        };

        for (i=0; i < TEST_ARR_SZ; i++) {
                time_t t = 0;
                int ret;

                printf(">> Input : %s\n", input[i]);
                ret = rfc5322_date_parse(input[i], strlen(input[i]), &t);
                if ( ret != -1) {
                        char buf[33] = {0};
                        rfc5322_date_create(t, buf, 32);
                        printf(">> Output       : %s\n", buf);
                        printf(">> Output(ctime): %s\n", ctime(&t));
                } else {
                        printf(">> Output: FAILED Parsing!\n\n");
                }
        }

        exit(EXIT_SUCCESS);
}
