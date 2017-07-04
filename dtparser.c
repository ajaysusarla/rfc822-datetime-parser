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
#include <ctype.h>

#include "dtparser.h"

#define EOB (-99999)            /* End Of Buffer */
static const char special[256] = {
        [' ']  = 1,
        ['-']  = 1,
        ['+']  = 1,
        [',']  = 1,
        ['\t'] = 1,
        ['\r'] = 1,
        ['\n'] = 1,
        [':']  = 1,
};

static const char * const monthname[12] = {
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Nov",
        "Dec"
};

static const char *const wday[7] = {
        "Sun",
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat"
};

enum token_type {
        TOKEN_DAY_OF_WEEK,
        TOKEN_DAY,
        TOKEN_MONTH,
        TOKEN_YEAR,
        TOKEN_HOUR,
        TOKEN_MINUTE,
        TOKEN_SECONDS,
        TOKEN_ZONE,
};

enum {
        Alpha = 1,
        Digit = 2,
        TZSign = 4,
};

static const long charset[257] = {
        ['0' + 1 ... '9' + 1] = Digit,
        ['A' + 1 ... 'Z' + 1] = Alpha,
        ['a' + 1 ... 'z' + 1] = Alpha,
        /* ['+' + 1] = TZSign, */
        /* ['-' + 1] = TZSign */
};

struct tbuf {
        const char *str;
        struct token *token;
        int len;
        int offset;
};

struct token {
        enum token_type type;
        struct token *next;
        union {
                char day_of_week[3]; /* optional  */
                char day[2];         /* 1 or 2 DIGITS */
                char month[3];       /* 3 letter month name */
                char year[5];    /* 4 or more DIGITS of year */
                char hour[2];        /* 2 DIGITS */
                char minute[2];      /* 2 DIGITS */
                char seconds[2];     /* optiona, 2 DIGITS */
                char zone[5];        /* '+'/'-' followed by 4 DIGITS*/
        };
};

static struct token zero_token;

static struct token * token_new(enum token_type type)
{
        struct token *t;

        t = calloc(1, sizeof(struct token));
        if (!t)
                return &zero_token;

        t->type = type;

        return t;
}

static void token_free(struct token *token)
{
        if (token) {
                free(token);
                token = &zero_token;
        }
}

static inline int get_next_char(struct tbuf *buf)
{
        int c;

        if (buf->offset < buf->len) {
                buf->offset++;
                c = buf->str[buf->offset];
                return c;
        }

        return EOB;
}

static inline int get_current_char(struct tbuf *buf)
{
        int offset = buf->offset;

        if (offset < buf->len)
                return buf->str[offset];
        else
                return EOB;
}

static inline int get_previous_char(struct tbuf *buf)
{
        int offset = buf->offset;

        offset--;
        if (offset >= 0)
                return buf->str[offset];
        else
                return EOB;
}

static void skip_ws(struct tbuf *buf)
{
        int c = buf->str[buf->offset];

        while (c != EOB) {
                if (!isspace(c))
                        return;
                c = get_next_char(buf);
        }

        return;
}

#define MAX_BUF_LEN 32
static int get_next_token(struct tbuf *buf, char **str, int *len)
{
        int c, ret = 1;
        long ch;
        static char cache[MAX_BUF_LEN];
        int i;

        memset(cache, 1, MAX_BUF_LEN);

        c = get_current_char(buf);
        if (c == EOB)
                goto failed;

        *len = 0;
        for (;;) {
                if (special[c]) {
                        c = get_next_char(buf);
                        break;
                }

                ch = charset[c + 1];
                if (!(ch & (Alpha | Digit))) {
                        //c = get_next_char(buf);
                        break;
                }

                cache[*len] = c;
                *len += 1;

                c = get_next_char(buf);
                if (c == EOB)
                        break;
        }

failed:
        *str = cache;

        return ret;
}
/*
  Returns a string, which needs to freed
  TODO: Use struct token instead
 */
static char * get_string_token(struct tbuf *buf)
{
        int c, offset;
        int len = 1;
        long ch;
        char str[32] = {0};           /* XXX: Should be bigger? */

        offset = buf->offset;
        c = buf->str[offset];
        str[0] = c;
        for (;;) {
                ch = charset[c + 1];
                if (!(ch & Alpha))
                        break;

                if (len >= sizeof(str))
                        break;

                c = get_next_char(buf);
                if (c == EOB)
                        break;
                str[len] = c;
                len++;
        }

        /* TODO: Check for NULL */
        return strdup(str);
}

static int get_number_token(struct tbuf *buf)
{
        int c, offset;
        int len = 1;
        int num = 0, i;
        long ch;
        char str[32] = {0};     /* XXX: Should be bigger? */

        offset = buf->offset;
        c = buf->str[offset];
        str[0] = c;
        for (;;) {
                ch = charset[c + 1];
                if (!(ch & Digit))
                        break;
                if (len > sizeof(str))
                        break;
                c = get_next_char(buf);
                if (c == EOB)
                        break;
                str[len] = c;
                len++;
        }

        for (i=0; i<len; i++)
                num = num * 10 + (str[i] - '0');

        return num;

}

/* struct tm { */
/*         int tm_sec; */
/*         int tm_min; */
/*         int tm_hour; */
/*         int tm_mday; */
/*         int tm_mon; */
/*         int tm_year; */
/*         int tm_wday; */
/*         int tm_yday; */
/*         int tm_isdst; */

/* }; */

static int tokenise(struct tbuf *buf, struct tm *tm)
{
        long ch;
        int c;
        char *str_token = NULL;
        int len, offset;
        int num_token = 0;
        int ret = 1;

        /* Skip leading WS, if any */
        skip_ws(buf);

        c = get_current_char(buf);
        if (c == EOB) {
                ret = 0;
                goto failed;
        }

        ch = charset[c + 1];
        if (ch & Alpha) {
                /* We might have a weekday token here, which we should skip*/
                /*
                str_token = get_string_token(buf);
                if (str_token && str_token[0] && (strlen(str_token) == 3)) {
                        int offset = buf->offset;

                        free(str_token);

                        if (buf->str[offset] == ',') {
                                buf->offset += 1;
                        } else {
                                ret = 0;
                                goto failed;
                        }

                        skip_ws(buf);
                }
                */
                //struct token *t = token_new(TOKEN_DAY_OF_WEEK);
                get_next_token(buf, &str_token, &len);
                printf(">>>> %s\n", str_token);
                skip_ws(buf);
        }

        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        /* Zone */
        skip_ws(buf);
        get_next_token(buf, &str_token, &len);
        printf(">>>> %s\n", str_token);

        /*tm.tm_mday */
        c = get_next_char(buf);
        if (c == EOB) {
                ret = 0;
                goto failed;
        }

        ch = charset[c + 1];
        if (!(ch & Digit)) {
                ret = 0;
                goto failed;
        }
        //tm->tm_mday = get_number_token(buf);

        /* tm.tm_mon */
        if (ch & TZSign)
                printf("TZSign\n");

failed:
        return ret;

}

int parse_time(const char *str, time_t *t)
{
        struct tbuf buf;
        struct tm tm;

        if (!str)
                goto baddate;

        buf.str = str;
        buf.len = strlen(str);
        buf.offset = 0;
        buf.token = &zero_token;

        printf(">>>%s\n", buf.str);
        #if 0
        for(;;) {
                int c = get_next_char(&buf);
                if (c == EOB)
                        break;
                printf("%c:%d\n", c, buf.offset);
        }
        #else
        tokenise(&buf, &tm);
        #endif

baddate:
        return -1;
}

