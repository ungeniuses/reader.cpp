/* main.cpp - reader: RSVP-style terminal word reader */

#include <thread>
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "../include/reader.h"

int
main(int argc, char *argv[])
{
	EditorCfg                 *cfg;
	FILE                      *fp;
	long                      size;
	std::string               content;
	std::vector<std::string>  data;
	size_t                    i;
	std::string               *k;
    SCPCQueue<char>           queue(1024);

	cfg   = (EditorCfg *)malloc(sizeof(EditorCfg));
	cfg->speed = 250000;
    int& speed = cfg->speed;
	if (!cfg)
		die("[err]: malloc failed\n");

	enable_raw(cfg);
	init_editor(cfg);

	if (argc < 2)
		die("[err]: no file provided!\n");
	if (argc > 3)
		die("[err]: too many arguments!\n");

	if (argc == 3) {
		if (!is_number(argv[2]))
			die("[err]: invalid speed interval provided!\n");
		speed = atoi(argv[2]);
	}

	fp = fopen(argv[1], "r");
	if (!fp)
		die("[err]: fopen failed\n");

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	content.resize(size, '\0');
	fread(&content[0], 1, size, fp);
	fclose(fp);

	split(content, data, " \t\r\n,;:!?");

	std::thread input_thread(producer, std::ref(queue));

	for (;;) {
		for (i = 0; i < data.size(); i++) {
			k = &data[i];
			if (k->empty())
				continue;
			window_size_callback(cfg);
			editor_refresh_scrn(cfg);
            consumer(queue, cfg);
			{
				int         mid       = (int)k->size() / 2;
				int         remaining = (int)k->size() - mid - 1;
				const char *cstr      = k->c_str();
                const char  message[]   = "Current delay: ";
                char        buf[32];
                int         len{};


				write(STDOUT_FILENO, cstr, mid);
				write(STDOUT_FILENO, "\x1b[31m", 5);
				write(STDOUT_FILENO, cstr + mid, 1);
				write(STDOUT_FILENO, "\x1b[m", 3);
				write(STDOUT_FILENO, cstr + mid + 1, remaining);

                write(STDOUT_FILENO, "\x1b[H", 3);
                write(STDOUT_FILENO, message, sizeof(message)-1);
                len = snprintf(buf, sizeof(buf), "%d", cfg->speed);
                write(STDOUT_FILENO, buf, len);
			}
			usleep(speed);
		}
	}

	free(cfg);
	return 0;
}
