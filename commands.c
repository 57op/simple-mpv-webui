#include "commands.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libwebsockets.h>

// used for returning json
#define RET_MAX_SIZE 0x2000
static char ret_val[LWS_SEND_BUFFER_PRE_PADDING + RET_MAX_SIZE + LWS_SEND_BUFFER_POST_PADDING];

static char *command_get_property(mpv_handle *mpv, const char *prop) {
  char *value = mpv_get_property_string(mpv, prop);
  char *result = &ret_val[LWS_SEND_BUFFER_PRE_PADDING];

  if (value == NULL) {
    value = mpv_get_property_osd_string(mpv, prop);
  }

  if (value == NULL) {
    strcpy(result, "false");
  } else {
    // some properties return a json
    // we won't wrap json with quotes
    const uint8_t is_json =
      strcmp(prop, "metadata") == 0 ||
      strcmp(prop, "playlist") == 0 ||
      strcmp(prop, "track-list") == 0;

    snprintf(result, RET_MAX_SIZE, is_json ? "%s" : "\"%s\"", value);
    mpv_free(value);
  }

  return ret_val;
}

static char *command_command(mpv_handle *mpv, const char *cmd) {
  int status = mpv_command_string(mpv, cmd);

  strcpy(&ret_val[LWS_SEND_BUFFER_PRE_PADDING], status == 0 ? "true" : "false");

  return ret_val;
}

struct command COMMANDS[] = {
  { "get_property", command_get_property },
  { "run_command", command_command },
  // terminator
  { 0 }
};
