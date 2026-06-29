/* reader.h - terminal word reader declarations */

#define _POSIX_C_SOURCE 200809L

#include <asm-generic/ioctls.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <string>
#include <vector>

#define CTRL_KEY(k) ((k) & 0x1f)

typedef struct EditorCfg {
	int screenrows;
	int screencols;
	struct termios termios_inst;
} EditorCfg;

typedef struct AppendBuf {
	char    *b;
	uint32_t len;
} AppendBuf;

void ab_append(AppendBuf *ab, const char *s, int len);
void ab_write(AppendBuf *ab);
void ab_free(AppendBuf *ab);

void die(const char *s);
void disable_raw(EditorCfg *cfg);
void enable_raw(EditorCfg *cfg);
void init_editor(EditorCfg *cfg);
void window_size_callback(EditorCfg *cfg);
void editor_refresh_scrn(EditorCfg *cfg);

char editor_readkey(void);

int get_cursor_pos(int *rows, int *cols);
int get_window_size(int *rows, int *cols);
int is_number(char *strnum);

size_t split(const std::string &txt,
             std::vector<std::string> &strings,
             const std::string &delims);
