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
        int len;
        int offset;
};

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

/*
  TODO: Support comments as per RFC.
 */
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
                if (special[c] || separators[c])
                        break;

                ch = charset[c + 1];
                if (!(ch & (Alpha | Digit)))
                        break;

                if (*len >= MAX_BUF_LEN)
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

static inline int to_upper_str_in_place(char **str, int len)
{
        int i;

        for (i=0; i<len; i++) {
                int c = str[0][i];
                if (charset[c + 1] & LAlpha)
                        str[0][i] = str[0][i] - 32;
        }

        return 1;
}

static inline int to_upper(char ch)
{
        if (charset[ch + 1] & LAlpha)
                ch =  ch - 32;

        return ch;
}

static inline int to_lower(char ch)
{
        if (charset[ch + 1] & UAlpha)
                ch = ch + 32;

        return ch;
}

static int compute_tzoffset(char *str, int len, int sign)
{
        int offset = 0;

        if (len == 1) {         /* Military timezone */
                int ch;
                ch = to_upper(str[0]);
                if (ch < 'J')
                        return (str[0] - 'A' + 1) * 60;
                if (ch == 'J')
                        return 0;
                if (ch <= 'M')
                        return (str[0] - 'A') * 60;;
                if (ch < 'Z')
                        return ('M' - str[0]) * 60;

                return 0;
        }

        if (len == 2 &&
            to_upper(str[0]) == 'U' &&
            to_upper(str[1]) == 'T') {         /* Universal Time zone (UT) */
                return 0;
        }

        if (len == 3) {
                char *p;

                if (to_upper(str[2]) != 'T')
                        return 0;

                p = strchr("AECMPYHB", to_upper(str[0]));
                if (!p)
                        return 0;
                offset = (strlen(p) - 12) *  60;

                if (to_upper(str[1]) == 'D')
                        return offset + 60;
                if (to_upper(str[1]) == 'S')
                        return offset;
        }

        if (len == 4) {         /* The number timezone offset */
                int i;

                for (i = 0; i<len; i++) {
                        if (!(charset[str[i] + 1] & Digit))
                                return 0;
                }

                offset = ((str[0] - '0') * 10 + (str[1] - '0')) * 60 +
                        (str[2] - '0') * 10 +
                        (str[3] - '0');

                return (sign == '+') ? offset : -offset;
        }

        return 0;
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
        if (ch & Alpha) {       /* Most likely a weekday at the start. */
                if (!get_next_token(buf, &str_token, &len))
                        goto failed;

                /* We might have a weekday token here, which we should skip*/
                if (len != 3)
                        goto failed;

                /* The weekday is foll wed by a ',', consume that. */
                if (get_current_char(buf) == ',')
                        get_next_char(buf);
                else
                        goto failed;

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

        /* month name */
        get_next_char(buf);     /* Consume a character, either a '-' or ' ' */

        if (!get_next_token(buf, &str_token, &len) ||
            len != 3 ||
            !(charset[str_token[0] + 1] & Alpha))
                goto failed;

        to_upper_str_in_place(&str_token, len);
        for (i = 0; i < 12; i++) {
                if (memcmp(monthnames[i], str_token, 3) == 0) {
                        tm->tm_mon = i;
                        break;
                }
        }
        if (i == 12)
                goto failed;

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

        /* minutes */
        if (get_current_char(buf) == ':' ||
            get_current_char(buf) == '.')
                get_next_char(buf); /* Consume ':'/'.' */
        else
                goto failed;    /* Something is broken */

        if (!get_next_token(buf, &str_token, &len))
                goto failed;

        if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                goto failed;

        tm->tm_min = to_int(str_token, len);
        if (tm->tm_min == -9999)
                goto failed;


        /* seconds[optional] */
        if (get_current_char(buf) == ':' ||
            get_current_char(buf) == '.') {
                get_next_char(buf); /* Consume ':' */

                if (!get_next_token(buf, &str_token, &len))
                        goto failed;

                if (len < 1 || len > 2 || !(charset[str_token[0] + 1] & Digit))
                        goto failed;

                tm->tm_sec = to_int(str_token, len);
                if (tm->tm_sec == -9999)
                        goto failed;

        }

        /* timezone */
        skip_ws(buf, 0);
        c = get_current_char(buf); /* the '+' or '-' in the timezone */
        get_next_char(buf);        /* consume '+' or '-' */

        if (!get_next_token(buf, &str_token, &len)) {
                *tz_offset = 0;
        } else {
                *tz_offset = compute_tzoffset(str_token, len, c);
        }

        /* dst */
        tm->tm_isdst = -1;
        return buf->offset;
failed:
        return -1;

}

/*
  TODO: Expect length of string.
 */
int parse_time(const char *str, time_t *t)
{
        struct tbuf buf;
        struct tm tm;
        time_t tmp_gmtime;
        int tzone_offset;

        if (!str)
                goto baddate;

        memset(&tm, 0, sizeof(struct tm));
        *t = 0;

        buf.str = str;
        buf.len = strlen(str);
        buf.offset = 0;

        if (tokenise_and_create_tm(&buf, &tm, &tzone_offset) == -1)
                goto baddate;

        tmp_gmtime = timegm(&tm);
        if (tmp_gmtime == -1)
                goto baddate;

        *t = tmp_gmtime - tzone_offset * 60;

        return buf.offset;

baddate:
        return -1;
}

