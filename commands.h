#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <mpv/client.h>

struct command {
  const char *name;
  uint8_t has_param;
  unsigned char *(*callback)(mpv_handle *mpv, void *param);
};

extern struct command COMMANDS[];

#define RET_MAX_SIZE 4096 // 4kB [might it be not enough?]

#endif // COMMANDS_H