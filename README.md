# RFC5322 Date-Time string parser

This is a parser for the RFC5322 Date Time string. Given an date time
string, the `rfc5322_date_parse()` function parses it and returns it in a
`time_t` structure.

To convert a `time_t` structure to RFC5322 date-time string use the
function `rfc5322_date_create()`.

Please look at `main.c` for usage.

