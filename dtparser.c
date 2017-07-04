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
        ['\t'] = 1,
        ['\r'] = 1,
        ['\n'] = 1,
};

static const char separators[256] = {
        [' ']  = 1,
        [',']  = 1,
        ['-']  = 1,
        ['+']  = 1,
        [':']  = 1,
};

static const char * const monthnames[12] = {
        "JAN",
        "FEB",
        "MAR",
        "APR",
        "MAY",
        "JUN",
        "JUL",
        "AUG",
        "SEP",
        "OCT",
        "NOV",
        "DEC"
};

static const char *const wdays[7] = {
        "SUN",
        "MON",
        "TUE",
        "WED",
        "THU",
        "FRI",
        "SAT"
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
        UAlpha = 2,
        LAlpha = 4,
        Digit = 8,
        TZSign = 16,
};

static const long charset[257] = {
        ['0' + 1 ... '9' + 1] = Digit,
        ['A' + 1 ... 'Z' + 1] = Alpha | UAlpha,
        ['a' + 1 ... 'z' + 1] = Alpha | LAlpha,
        ['+' + 1] = TZSign,
        ['-' + 1] = TZSign
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

static int skip_ws(struct tbuf *buf, int skipcomment)
{
        int c = buf->str[buf->offset];

        while (c != EOB) {
                if (special[c]) {
                        c = get_next_char(buf);
                        continue;
                }

                break;
        }

        return 1;
}

#define MAX_BUF_LEN 32
static int get_next_token(struct tbuf *buf, char **str, int *len)
{
        int c, ret = 1;
        long ch;
        static char cache[MAX_BUF_LEN];

        memset(cache, 1, MAX_BUF_LEN);

        c = get_current_char(buf);
        if (c == EOB) {
                ret = 0;
                goto failed;
        }

        *len = 0;
        for (;;) {
                if (special[c] || separators[c]) {
                        //c = get_next_char(buf);
                        break;
                }

                ch = charset[c + 1];
                if (!(ch & (Alpha | Digit)))
                        break;

                cache[*len] = c;
                *len += 1;

                c = get_next_char(buf);
                if (c == EOB) {
                        ret = 0;
                        break;
                }
        }

failed:
        *str = cache;

        return ret;
}

static inline int to_int(char *str, int len)
{
        int i, num = 0;

        for (i = 0; i<len; i++) {
                if (charset[str[i] + 1] & Digit)
                        num = num * 10 + (str[i] - '0');
                else {
                        num = -9999;
                        break;
                }
        }

        return num;
}

static inline int to_upper(char **str, int len)
{
        int i;

        for (i=0; i<len; i++) {
                int c = str[0][i];
                if (charset[c + 1] & LAlpha)
                        str[0][i] = str[0][i] - 32;
        }

        return 1;
}

static int compute_tzoffset(char *str, int len, int sign)
{
        int offset = 0;
        int i;

        for (i = 0; i<len; i++) {
                if (!(charset[str[i] + 1] & Digit))
                        return 0;
        }

        if (len == 4) {         /* The number timezone offset */
                offset = ((str[0] - '0') * 10 + (str[1] - '0')) * 60 +
                        (str[2] - '0') * 10 +
                        (str[3] - '0');

                return sign == '+' ? offset : -offset;
        }
}

/*
  date-time = [ ([FWS] day-name) "," ]
              ([FWS] 1*2DIGIT FWS)
              month
              (FWS 4*DIGIT FWS)
              2DIGIT ":" 2DIGIT [ ":" 2DIGIT ]
              (FWS ( "+" / "-" ) 4DIGIT)
              [CFWS]

   day-name = "Mon" / "Tue" / "Wed" / "Thu" / "Fri" / "Sat" / "Sun"
   month = "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" / "Jul" / "Aug" /
           "Sep" / "Oct" / "Nov" / "Dec"
 */

static int tokenise_and_create_tm(struct tbuf *buf, struct tm *tm,
                                  int *tz_offset)
{
        long ch;
        int c, i, len;
        char *str_token = NULL;

        /* Skip leading WS, if any */
        skip_ws(buf, 0);

        c = get_current_char(buf);
        if (c == EOB)
                goto failed;

        ch = charset[c + 1];
        if (ch & Alpha) {
                if (!get_next_token(buf, &str_token, &len))
                        goto failed;

                /* We might have a weekday token here, which we should skip*/
                if (len == 3) {
                        /* The weekday is foll wed by a ',', consume that. */
                        if (get_current_char(buf) == ',')
                                get_next_char(buf);
                        else
                                goto failed;
                }
                skip_ws(buf, 0);
        }

        /** DATE **/
        /* date (1 or 2 digits) */
        if (!get_next_token(buf, &str_token, &len))
                goto failed;

        if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                goto failed;

        tm->tm_mday = to_int(str_token, len);
        if (tm->tm_mday == -9999)
                goto failed;

        printf(">>tm_mday:%d\n", tm->tm_mday);

        /* month name */
        get_next_char(buf);     /* Consume a character, either a '-' or ' ' */

        if (!get_next_token(buf, &str_token, &len) ||
            len != 3 ||
            !(charset[str_token[0] + 1] & Alpha))
                goto failed;

        to_upper(&str_token, len);

        for (i = 0; i < 12; i++) {
                if (memcmp(monthnames[i], str_token, 3) == 0) {
                        tm->tm_mon = i;
                        break;
                }
        }
        if (i == 12)
                goto failed;
        printf(">>tm_mon:%d\n", tm->tm_mon);

        /* year 2, 4 or >4 digits */
        get_next_char(buf);     /* Consume a character, either a '-' or ' ' */

        if (!get_next_token(buf, &str_token, &len))
                goto failed;

        tm->tm_year = to_int(str_token, len);
        if (tm->tm_year == -9999)
                goto failed;

        if (len == 2) {
                /* A 2 digit year */
                if (tm->tm_year < 70)
                        tm->tm_year += 100;
        } else {
                if (tm->tm_year < 1900)
                        goto failed;
                tm->tm_year -= 1900;
        }

        printf(">>tm_year:%d\n", tm->tm_year);

        /** TIME **/
        skip_ws(buf, 0);
        /* hour */
        if (!get_next_token(buf, &str_token, &len))
                goto failed;

        if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                goto failed;

        tm->tm_hour = to_int(str_token, len);
        if (tm->tm_hour == -9999)
                goto failed;

        printf(">>tm_hour:%d\n", tm->tm_hour);

        /* minutes */
        if (get_current_char(buf) == ':')
                get_next_char(buf); /* Consume ':' */
        else
                goto failed;    /* Something is broken */

        if (!get_next_token(buf, &str_token, &len))
                goto failed;

        if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                goto failed;

        tm->tm_min = to_int(str_token, len);
        if (tm->tm_min == -9999)
                goto failed;

        printf(">>tm_min:%d\n", tm->tm_min);

        /* seconds[optional] */
        if (get_current_char(buf) == ':') {
                get_next_char(buf); /* Consume ':' */

                if (!get_next_token(buf, &str_token, &len))
                        goto failed;

                if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                        goto failed;

                tm->tm_sec = to_int(str_token, len);
                if (tm->tm_sec == -9999)
                        goto failed;

                printf(">>tm_sec:%d\n", tm->tm_sec);
        }

        /* timezone */
        skip_ws(buf, 0);
        c = get_current_char(buf); /* the '+' or '-' in the timezone */
        get_next_char(buf);        /* consume '+' or '-' */

        if (!get_next_token(buf, &str_token, &len)) {
                printf("no timezone\n");
                *tz_offset = 0;
        } else {
                *tz_offset = compute_tzoffset(str_token, len, c);
        }

        /* dst */
        tm->tm_isdst = -1;
        return 1;
failed:
        return 0;

}

int parse_time(const char *str, time_t *t)
{
        struct tbuf buf;
        struct tm tm;
        time_t tmp_gmtime;
        int tzone_offset;

        if (!str)
                goto baddate;

        buf.str = str;
        buf.len = strlen(str);
        buf.offset = 0;
        buf.token = &zero_token;

        printf(">>>%s\n", buf.str);
        tokenise_and_create_tm(&buf, &tm, &tzone_offset);

        tmp_gmtime = timegm(&tm);
        if (tmp_gmtime == -1)
                goto baddate;

        *t = tmp_gmtime - tzone_offset * 60;

        return 1;

baddate:
        return -1;
}

