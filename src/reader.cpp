/* reader.cpp - terminal raw mode and editor utilities */

#define _POSIX_C_SOURCE 200809L

#include <csignal>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <cctype>
#include <string>
#include <vector>

#include "../include/reader.h"

static EditorCfg *g_cfg = NULL;

void
die(const char *s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	fprintf(stderr, "[exit]: die() invoked, exiting..\n");
	exit(1);
}

static void
disable_raw_atexit(void)
{
	if (g_cfg)
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &(g_cfg->termios_inst));
}

static void
handle_signal(int sig)
{
	disable_raw_atexit();
	_exit(128 + sig);
}

void
disable_raw(EditorCfg *cfg)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &(cfg->termios_inst)) < 0)
		die("tcsetattr");
}

void
enable_raw(EditorCfg *cfg)
{
	struct termios raw;

	if (tcgetattr(STDIN_FILENO, &(cfg->termios_inst)) < 0)
		die("tcgetattr");

	g_cfg = cfg;

	atexit(disable_raw_atexit);

	signal(SIGINT,  handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGQUIT, handle_signal);

	raw = cfg->termios_inst;
	/* disable XON/XOFF, CR-to-NL, break, parity, strip */
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	/* disable output processing */
	raw.c_oflag &= ~(OPOST);
	/* disable echo, canonical, extended; disables terminal keybinds */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);

	raw.c_cc[VMIN]  = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
		die("tcsetattr");
}

char
editor_readkey(void)
{
	int  nread;
	char c;

	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread < 0 && errno != EAGAIN)
			die("read");
	}
	return c;
}

int
get_cursor_pos(int *rows, int *cols)
{
	char         buff[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;

	while (i < sizeof(buff) - 1) {
		if (read(STDIN_FILENO, &buff[i], 1) != 1)
			break;
		if (buff[i] == 'R')
			break;
		i++;
	}
	buff[i] = '\0';

	if (buff[0] != '\x1b' || buff[1] != '[')
		return -1;
	if (sscanf(&buff[2], "%d;%d", rows, cols) != 2)
		return -1;
	return 0;
}

int
get_window_size(int *rows, int *cols)
{
	struct winsize w_size;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_size) < 0 ||
	    w_size.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;
		return get_cursor_pos(rows, cols);
	}

	*rows = w_size.ws_row;
	*cols = w_size.ws_col;

	return 0;
}

void
window_size_callback(EditorCfg *cfg)
{
	int rows;
	int cols;

	if (get_window_size(&rows, &cols) < 0) {
		fprintf(stderr,
		    "[err]: window_size_callback: get_window_size\n");
		return;
	}
	if (cfg->screenrows != rows || cfg->screencols != cols) {
		cfg->screenrows = rows;
		cfg->screencols = cols;
	}
}

void
ab_append(AppendBuf *ab, const char *s, int len)
{
	char *n;

	if (!(n = (char *)realloc(ab->b, ab->len + len)))
		return;
	memcpy(&n[ab->len], s, len);
	ab->b    = n;
	ab->len += len;
}

void
ab_write(AppendBuf *ab)
{
	write(STDOUT_FILENO, ab->b, ab->len);
}

void
ab_free(AppendBuf *ab)
{
	free(ab->b);
	ab->b   = NULL;
	ab->len = 0;
}

void
editor_refresh_scrn(EditorCfg *cfg)
{
	AppendBuf ab = {NULL, 0};
	char      buff[32];

	ab_append(&ab, "\x1b[2J", 4);
	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "\x1b[%d;%dH",
	    cfg->screenrows / 2, cfg->screencols / 2);
	ab_append(&ab, buff, strlen(buff));
	ab_write(&ab);
	ab_free(&ab);
}

void
init_editor(EditorCfg *cfg)
{
	if (get_window_size(&(cfg->screenrows), &(cfg->screencols)) < 0)
		die("get_window_size");
}

int
is_number(char *strnum)
{
	std::string                  str = strnum;
	std::string::const_iterator  it  = str.begin();

	while (it != str.end() && std::isdigit(*it))
		++it;
	return !str.empty() && it == str.end();
}

size_t
split(const std::string &txt,
      std::vector<std::string> &strings,
      const std::string &delims)
{
	size_t initial_pos = txt.find_first_not_of(delims, 0);
	size_t pos         = txt.find_first_of(delims, initial_pos);

	while (initial_pos != std::string::npos) {
		strings.push_back(txt.substr(initial_pos, pos - initial_pos));
		initial_pos = txt.find_first_not_of(delims, pos);
		pos         = txt.find_first_of(delims, initial_pos);
	}
	return strings.size();
}
