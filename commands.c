#include "commands.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <libwebsockets.h>

// whitelist: hashmap section
// hashmap are generated with utilities/whitelist.py
#define BUCKET_NO 5
#define PROPERTIES_BUCKET_LEN 5
#define COMMANDS_BUCKET_LEN 6

static const char *properties_hashmap[BUCKET_NO][PROPERTIES_BUCKET_LEN] = {
  { "sub-delay", NULL, NULL, NULL, NULL },
  { "chapters", "filename", "playlist", "volume", "metadata" },
  { "chapter", "time-pos", "playtime-remaining", "loop-file", "audio-delay" },
  { "loop-playlist", "pause", "duration", "volume-max", NULL },
  { "fullscreen", "track-list", NULL, NULL, NULL }
};

static const char *commands_hashmap[BUCKET_NO][COMMANDS_BUCKET_LEN] = {
  { "add chapter", "playlist-shuffle", "playlist-move", "seek", NULL, NULL },
  { "set loop-playlist", NULL, NULL, NULL, NULL, NULL },
  { "sub_seek", "set playlist-pos", "cycle fullscreen", "set volume", "cycle pause", "playlist-remove" },
  { "playlist-next", "cycle sub", "set loop-file", NULL, NULL, NULL },
  { "cycle audio", "add sub-delay", "add audio-delay", "cycle audio-device", "add sub-scale", NULL }
};

static size_t hash_k33(const char *str) {
  size_t hash = 5381;
  char c;

  while ((c = *str++) != '\0') {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash % BUCKET_NO;
}
// /whitelist: hashmap section

// used for returning json
#define RET_MAX_SIZE 0x2000
static char ret_val[LWS_SEND_BUFFER_PRE_PADDING + RET_MAX_SIZE + LWS_SEND_BUFFER_POST_PADDING];

static char *command_get_property(mpv_handle *mpv, char *property) {
  // check if property is in whitelisted properties (hashmap)
  const char **bucket = properties_hashmap[hash_k33(property)];
  uint8_t found = 0;

  for (size_t i = 0; i < PROPERTIES_BUCKET_LEN && bucket[i] != NULL && found == 0; i++) {
    found = strcmp(property, bucket[i]) == 0;
  }

  char *value = NULL;
  char *result = &ret_val[LWS_SEND_BUFFER_PRE_PADDING];

  if (found == 1) {
    value = mpv_get_property_string(mpv, property);


    if (value == NULL) {
      value = mpv_get_property_osd_string(mpv, property);
    }
  }/* else {
    fprintf(stderr, "[command_get_property] unkown property: %s\n", property);
  }*/

  if (value == NULL) {
    strcpy(result, "false");
  } else {
    // some properties return a json
    // we won't wrap json with quotes
    const uint8_t is_json =
      strcmp(property, "metadata") == 0 ||
      strcmp(property, "playlist") == 0 ||
      strcmp(property, "track-list") == 0;

    snprintf(result, RET_MAX_SIZE, is_json ? "%s" : "\"%s\"", value);
    mpv_free(value);
  }

  return ret_val;
}

static char *command_run_command(mpv_handle *mpv, char *command) {
  size_t cut_position = 0;
  size_t command_len = strlen(command) - 1;

  for (size_t i = 1; cut_position == 0 && i < command_len; i++) {
    if (isspace(command[i]) && (command[i + 1] == '-' || isdigit(command[i + 1]))) {
      command[i] = '\0';
      cut_position = i;
    }
  }

  // search in the hashmap
  const char **bucket = commands_hashmap[hash_k33(command)];
  uint8_t found = 0;

  for (size_t i = 0; i < COMMANDS_BUCKET_LEN && bucket[i] != NULL && found == 0; i++) {
    // commands starts with bucket[i]
    found = strncmp(bucket[i], command, strlen(bucket[i])) == 0;
  }

  int status = -1;

  if (found == 1) {
    // restore the truncated value
    if (cut_position > 0) {
      command[cut_position] = ' ';
    }

    // execute the command
    status = mpv_command_string(mpv, command);
  }/* else {
    fprintf(stderr, "[command_command] unkown command: %s\n", command);
  }*/

  strcpy(&ret_val[LWS_SEND_BUFFER_PRE_PADDING], status == 0 ? "true" : "false");

  return ret_val;
}

struct command COMMANDS[] = {
  { "get_property", command_get_property },
  { "run_command", command_run_command },
  // terminator
  { 0 }
};
