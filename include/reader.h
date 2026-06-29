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

#include <atomic>
#include <string>
#include <vector>

#define CTRL_KEY(k) ((k) & 0x1f)

typedef struct EditorCfg {
	int screenrows;
	int screencols;
    int speed;
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



template <typename T>
class
SCPCQueue
{
private:
    std::vector<T> buff;
    const size_t capacity;

    alignas(64) std::atomic<size_t> head;
    alignas(64) std::atomic<size_t> tail;

public:
    SCPCQueue(size_t size)
        : buff(size + 1), capacity(size + 1), head(0), tail(0) {}

    SCPCQueue(const SCPCQueue&) = delete;
    SCPCQueue& operator=(const SCPCQueue&) = delete;

    bool
    push(const T& data)
    {
        const size_t cur_tail = tail.load(std::memory_order_relaxed);
        const size_t next_tail = (cur_tail + 1) % capacity;

        if (next_tail == head.load(std::memory_order_acquire)) {
            /* Queue is full */
            return false;
        }

        buff[cur_tail] = data;

        tail.store(next_tail, std::memory_order_release);

        return true;
    }
    bool pop(T &result)
    {
	    const size_t cur_head = head.load(std::memory_order_relaxed);

	    if (cur_head == tail.load(std::memory_order_acquire)) {
            return false;
	    }
	    result = buff[cur_head];

	    const size_t next_head = (cur_head + 1) % capacity;

	    head.store(next_head, std::memory_order_release);

        return true;
    }
};

void producer(SCPCQueue<char> &queue);

void consumer(SCPCQueue<char> &queue, EditorCfg *cfg);
