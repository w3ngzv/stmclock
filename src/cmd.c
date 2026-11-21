#include "cmd.h"
#include "calendar.h"

static char upper(char c)
{
	return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

static void skip_spaces(const char **p)
{
	while (**p == ' ' || **p == '\t')
		(*p)++;
}

/* 整词匹配: 词后必须是空白或行尾, 避免 "TIMEX" 被当成 "TIME" */
static int word(const char **p, const char *w)
{
	const char *s = *p;

	while (*w) {
		if (upper(*s) != *w)
			return 0;
		s++;
		w++;
	}
	if (*s != 0 && *s != ' ' && *s != '\t')
		return 0;
	*p = s;
	skip_spaces(p);
	return 1;
}

static int number(const char **p, int *out)
{
	const char *s = *p;
	int v = 0;
	int digits = 0;

	while (*s >= '0' && *s <= '9' && digits < 5) {
		v = v * 10 + (*s - '0');
		s++;
		digits++;
	}
	if (!digits)
		return 0;
	*out = v;
	*p = s;
	return 1;
}

static int sep(const char **p, char c)
{
	if (**p != c)
		return 0;
	(*p)++;
	return 1;
}

static int at_end(const char *p)
{
	skip_spaces(&p);
	return *p == 0;
}

static cmd_t make(cmd_type_t type, int a, int b, int c)
{
	cmd_t cmd = { type, a, b, c };

	return cmd;
}

static cmd_t parse_time(const char *p)
{
	int h, m, s = 0;

	if (!number(&p, &h) || !sep(&p, ':') || !number(&p, &m))
		return make(CMD_BAD, 0, 0, 0);
	if (sep(&p, ':') && !number(&p, &s))
		return make(CMD_BAD, 0, 0, 0);
	if (!at_end(p) || h > 23 || m > 59 || s > 59)
		return make(CMD_BAD, 0, 0, 0);
	return make(CMD_TIME, h, m, s);
}

static cmd_t parse_date(const char *p)
{
	int y, m, d;

	if (!number(&p, &y) || !sep(&p, '-') || !number(&p, &m) ||
	    !sep(&p, '-') || !number(&p, &d) || !at_end(p))
		return make(CMD_BAD, 0, 0, 0);
	if (y < YEAR_MIN || y > YEAR_MAX || m < 1 || m > 12 ||
	    d < 1 || d > calendar_days_in_month(y, m))
		return make(CMD_BAD, 0, 0, 0);
	return make(CMD_DATE, y, m, d);
}

static cmd_t parse_alarm(const char *p)
{
	int h, m;

	if (word(&p, "ON"))
		return at_end(p) ? make(CMD_ALARM_ON, 0, 0, 0) : make(CMD_BAD, 0, 0, 0);
	if (word(&p, "OFF"))
		return at_end(p) ? make(CMD_ALARM_OFF, 0, 0, 0) : make(CMD_BAD, 0, 0, 0);
	if (!number(&p, &h) || !sep(&p, ':') || !number(&p, &m) ||
	    !at_end(p) || h > 23 || m > 59)
		return make(CMD_BAD, 0, 0, 0);
	return make(CMD_ALARM_SET, h, m, 0);
}

cmd_t cmd_parse(const char *line)
{
	const char *p = line;

	skip_spaces(&p);
	if (*p == 0)
		return make(CMD_NONE, 0, 0, 0);

	/* 蓝牙模块在 AT 模式下会回显 OK 和 ERR, 不能再回复, 否则两端互相回复停不下来 */
	if (word(&p, "OK") || word(&p, "ERR"))
		return at_end(p) ? make(CMD_NONE, 0, 0, 0) : make(CMD_BAD, 0, 0, 0);

	if (word(&p, "TIME"))
		return parse_time(p);
	if (word(&p, "DATE"))
		return parse_date(p);
	if (word(&p, "ALARM"))
		return parse_alarm(p);
	if (word(&p, "VIB")) {
		if (word(&p, "ON"))
			return make(CMD_VIB_ON, 0, 0, 0);
		if (word(&p, "OFF"))
			return make(CMD_VIB_OFF, 0, 0, 0);
		if (word(&p, "TEST"))
			return make(CMD_VIB_TEST, 0, 0, 0);
	}
	if (word(&p, "BEEP")) {
		word(&p, "TEST");
		return make(CMD_BEEP_TEST, 0, 0, 0);
	}
	if (word(&p, "STATUS"))
		return make(CMD_STATUS, 0, 0, 0);
	if (word(&p, "IMGW"))
		return make(CMD_IMGW, 0, 0, 0);
	return make(CMD_BAD, 0, 0, 0);
}
