#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <mpv/client.h>

struct command {
  const char *name;
  uint8_t has_param;
  void *(*callback)(mpv_handle *mpv, void *param);
};

extern struct command COMMANDS[];

#endif // COMMANDS_H