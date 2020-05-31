#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include <mpv/client.h>

struct command {
  const char *name;
  // returns the number of bytes written to result,
  // not counting the terminating null character
  size_t (*callback)(mpv_handle *mpv, char *param, char *result);
};

extern struct command COMMANDS[];

#define WS_RESULT_SIZE 0x2000

#endif // COMMANDS_H