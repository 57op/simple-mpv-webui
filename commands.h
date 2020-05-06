#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <mpv/client.h>

struct command {
  const char *name;
  char *(*callback)(mpv_handle *mpv, const char *param);
};

extern struct command COMMANDS[];

#endif // COMMANDS_H