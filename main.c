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
int main(void)
{
        const char *d1 = "15-OCT-2010 03:19:52 +1100";
        const char *d2 = " 5-Oct-2010 03:19:52 +1100";
        const char *d3 = " 3-jan-2009 04:05    -0400";
        const char *d4 = "Tue, 20 Jun 2017 00:49:38 +0000";
        const char *d5 = " 3-jan-2009 04:05    +0400";
        time_t t = 0;
        int i = 0;


        #if 1
        parse_time(d1, &t);
        printf("\n");
        printf("----\n");
        #endif

        #if 0
        parse_time(d2, &t);
        printf("\n");
        printf("----\n");
        #endif


        #if 1
        parse_time(d3, &t);
        printf("\n");
        printf("----\n");
        #endif

        #if 1
        parse_time(d4, &t);
        printf("\n");
        printf("----\n");
        #endif

        #if 1
        parse_time(d5, &t);
        printf("\n");
        printf("----\n");
        #endif

        exit(EXIT_SUCCESS);

}
