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

#include "dtparser.h"

#define TEST_ARR_SZ 7
int main(void)
{
        int i;
        const char *arr[TEST_ARR_SZ] = {
                "15-OCT-2010 03:19:52 +1100",
                " 5-Oct-2010 03:19:52 +1100",
                " 3-jan-2009 04:05    -0400",
                "Tue, 20 Jun 2017 00:49:38 +0000",
                " 3-jan-2009 04:05    +0400",
                " 3-jan-09 04:05    -0400",
                " 3-jan-09 04:05  "
        };

        for (i=0; i < TEST_ARR_SZ; i++) {
                time_t t = 0;

                printf(">> Input : %s\n", arr[i]);
                parse_time(arr[i], &t);
                printf(">> Output: %s\n", ctime(&t));
                printf("-------------\n");
        }

        exit(EXIT_SUCCESS);
}
