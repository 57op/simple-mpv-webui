#include "commands.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// used for mpv_command
#define CMD_SIZE 256
static char cmd_args[CMD_SIZE];

// used for returning json
#define RET_MAX_SIZE 4096 // 4kB [might it be not enough?]
static char ret_val[RET_MAX_SIZE];

static double validate_double_param(void *param) {
  double number;
  sscanf(param, "%lf", &number);
  return number;
}

static int64_t validate_int_param(void *param) {
  int64_t number;
  sscanf(param, "%ld", &number);
  return number;
}

static void *command_status(mpv_handle *mpv, void *param) {
  char *_filename = mpv_get_property_string(mpv, "filename");
  char *_duration = mpv_get_property_string(mpv, "duration");
  char *_position = mpv_get_property_string(mpv, "time-pos");
  char *_pause = mpv_get_property_string(mpv, "pause");
  char *_remaining = mpv_get_property_string(mpv, "playtime-remaining");
  char *_sub_delay = mpv_get_property_osd_string(mpv, "sub-delay");
  char *_audio_delay = mpv_get_property_osd_string(mpv, "audio-delay");
  char *_metadata = mpv_get_property_string(mpv, "metadata");
  char *_volume = mpv_get_property_string(mpv, "volume");
  char *_volume_max = mpv_get_property_string(mpv, "volume-max");
  char *_playlist = mpv_get_property_string(mpv, "playlist");
  char *_track_list = mpv_get_property_string(mpv, "track-list");
  char *_fullscreen = mpv_get_property_string(mpv, "fullscreen");
  char *_loop_file = mpv_get_property_string(mpv, "loop-file");
  char *_loop_playlist = mpv_get_property_string(mpv, "loop-playlist");
  char *_chapters = mpv_get_property_string(mpv, "chapters");

  const char *filename = _filename == NULL ? "" : _filename;
  const char *position = _position == NULL ? "" : _position;
  const char *duration = _duration == NULL ? "" : _duration;
  const char *pause = _pause == NULL ? "" : _pause;
  const char *remaining = _remaining == NULL ? "" : _remaining;
  const char *sub_delay = _sub_delay == NULL ? "" : _sub_delay;
  const char *audio_delay = _audio_delay == NULL ? "" : _audio_delay;
  const char *metadata = _metadata == NULL ? "" : _metadata;
  const char *volume = _volume == NULL ? "" : _volume;
  const char *volume_max = _volume_max == NULL ? "" : _volume_max;
  const char *playlist = _playlist == NULL ? "" : _playlist;
  const char *track_list = _track_list == NULL ? "" : _track_list;
  const char *fullscreen = _fullscreen == NULL ? "" : _fullscreen;
  const char *loop_file = _loop_file == NULL ? "" : _loop_file;
  const char *loop_playlist = _loop_playlist == NULL ? "" : _loop_playlist;
  const char *chapters = _chapters == NULL ? "" : _chapters;
  int64_t chapter = 0;

  if (strcmp(chapters, "0") != 0) {
    mpv_get_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter);
  }

  snprintf(
    ret_val, RET_MAX_SIZE,
    "{"
    "\"audio-delay\":%.*s," // substr: remove ms
    "\"chapter\":%ld,"
    "\"chapters\":%s,"
    "\"duration\":%s," // round?
    "\"filename\":\"%s\","
    "\"fullscreen\":%s,"
    "\"loop-file\":\"%s\","
    "\"loop-playlist\":\"%s\","
    "\"metadata\":%s,"
    "\"pause\":%s,"
    "\"playlist\":%s,"
    "\"position\":%s," // round?
    "\"remaining\":%s," // round?
    "\"sub-delay\":%.*s," // substr: remove ms
    "\"track-list\":%s,"
    "\"volume\":%s," // round?
    "\"volume-max\":%s" // round?
    "}",
    (int) (strlen(audio_delay) - 3), audio_delay,
    chapter,
    chapters,
    duration,
    filename,
    strcmp(fullscreen, "yes") == 0 ? "true" : "false",
    loop_file, // bool, but leave it as yes/no
    loop_playlist, // same as above
    metadata,
    strcmp(pause, "yes") == 0 ? "true" : "false",
    playlist,
    position,
    remaining,
    (int) (strlen(sub_delay) - 3), sub_delay,
    track_list,
    volume,
    volume_max);

  mpv_free(_filename);
  mpv_free(_position);
  mpv_free(_duration);
  mpv_free(_pause);
  mpv_free(_remaining);
  mpv_free(_sub_delay);
  mpv_free(_audio_delay);
  mpv_free(_metadata);
  mpv_free(_volume);
  mpv_free(_volume_max);
  mpv_free(_playlist);
  mpv_free(_track_list);
  mpv_free(_fullscreen);
  mpv_free(_loop_file);
  mpv_free(_loop_playlist);
  mpv_free(_chapters);

  return ret_val;
}

static void *command_play(mpv_handle *mpv, void *param) {
  mpv_set_property_string(mpv, "pause", "no");
  return NULL;
}

static void *command_pause(mpv_handle *mpv, void *param) {
  mpv_set_property_string(mpv, "pause", "yes");
  return NULL;
}

static void *command_toggle_pause(mpv_handle *mpv, void *param) {
  char *current = mpv_get_property_string(mpv, "pause");
  char *next;

  if (strcmp(current, "no") == 0) {
    next = "yes";
  } else {
    next = "no";
  }

  mpv_set_property_string(mpv, "pause", next);
  mpv_free(current);
  return NULL;
}

static void *command_fullscreen(mpv_handle *mpv, void *param) {
  char *current = mpv_get_property_string(mpv, "fullscreen");
  char *next;

  if (strcmp(current, "no") == 0) {
    next = "yes";
  } else {
    next = "no";
  }

  mpv_set_property_string(mpv, "fullscreen", next);
  mpv_free(current);
  return NULL;
}

static void *command_seek(mpv_handle *mpv, void *param) {
  snprintf(cmd_args, CMD_SIZE, "seek %s", (char *) param);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_sub_seek(mpv_handle *mpv, void *param) {
  snprintf(cmd_args, CMD_SIZE, "sub-seek %s", (char *) param);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_set_position(mpv_handle *mpv, void *param) {
  snprintf(cmd_args, CMD_SIZE, "seek %s absolute", (char *) param);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_playlist_prev(mpv_handle *mpv, void *param) {
  char *position = mpv_get_property_string(mpv, "time-pos");
  double pos = 0;

  if (position != NULL) {
    pos = validate_double_param(position);
  }

  if (pos > 1) {
    snprintf(cmd_args, CMD_SIZE, "seek 0 absolute");
  } else {
    snprintf(cmd_args, CMD_SIZE, "playlist-prev");
  }

  mpv_command_string(mpv, cmd_args);
  mpv_free(position);
  return NULL;
}

static void *command_playlist_next(mpv_handle *mpv, void *param) {
  snprintf(cmd_args, CMD_SIZE, "playlist-next");
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_playlist_jump(mpv_handle *mpv, void *param) {
  int64_t pos = validate_int_param(param);
  mpv_set_property(mpv, "playlist-pos", MPV_FORMAT_INT64, &pos);
  return NULL;
}

static void *command_playlist_remove(mpv_handle *mpv, void *param) {
  int64_t pos = validate_int_param(param);
  snprintf(cmd_args, CMD_SIZE, "playlist-remove %ld", pos);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_playlist_move_up(mpv_handle *mpv, void *param) {
  int64_t pos = validate_int_param(param);

  if (pos > 0) {
    snprintf(cmd_args, CMD_SIZE, "playlist-move %ld %ld", pos, pos - 1);
    mpv_command_string(mpv, cmd_args);
  }

  return NULL;
}

static void *command_playlist_shuffle(mpv_handle *mpv, void *param) {
  mpv_command_string(mpv, "playlist-shuffle");
  return NULL;
}

static void *command_loop_file(mpv_handle *mpv, void *param) {
  // TODO: param: { inf, no }
  mpv_set_property_string(mpv, "loop-file", param);
  return NULL;
}

static void *command_loop_playlist(mpv_handle *mpv, void *param) {
  // TODO: param { inf, no, force }
  mpv_set_property_string(mpv, "loop-playlist", param);
  return NULL;
}

static void *command_add_volume(mpv_handle *mpv, void *param) {
  int64_t v = validate_int_param(param);
  snprintf(cmd_args, CMD_SIZE, "add volume %ld", v);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_set_volume(mpv_handle *mpv, void *param) {
  int64_t v = validate_int_param(param);
  snprintf(cmd_args, CMD_SIZE, "set volume %ld", v);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_add_sub_delay(mpv_handle *mpv, void *param) {
  double ms = validate_double_param(param);
  snprintf(cmd_args, CMD_SIZE, "add sub-delay %lf", ms);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_set_sub_delay(mpv_handle *mpv, void *param) {
  double ms = validate_double_param(param);
  snprintf(cmd_args, CMD_SIZE, "set sub-delay %lf", ms);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_add_sub_scale(mpv_handle *mpv, void *param) {
  double s = validate_double_param(param);
  snprintf(cmd_args, CMD_SIZE, "add sub-scale %lf", s);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_add_audio_delay(mpv_handle *mpv, void *param) {
  double ms = validate_double_param(param);
  snprintf(cmd_args, CMD_SIZE, "add audio-delay %lf", ms);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_set_audio_delay(mpv_handle *mpv, void *param) {
  double ms = validate_double_param(param);
  snprintf(cmd_args, CMD_SIZE, "set audio-delay %lf", ms);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

static void *command_cycle_sub(mpv_handle *mpv, void *param) {
  mpv_command_string(mpv, "cycle sub");
  return NULL;
}

static void *command_cycle_audio(mpv_handle *mpv, void *param) {
  mpv_command_string(mpv, "cycle audio");
  return NULL;
}

static void *command_cycle_audio_device(mpv_handle *mpv, void *param) {
  mpv_command_string(mpv, "cycle audio-device");
  return NULL;
}

static void *command_add_chapter(mpv_handle *mpv, void *param) {
  int64_t num = validate_int_param(param);

  snprintf(cmd_args, CMD_SIZE, "add chapter %ld", num);
  mpv_command_string(mpv, cmd_args);
  return NULL;
}

struct command COMMANDS[] = {
  { "status", 0, command_status },
  { "play", 0, command_play },
  { "pause", 0, command_pause },
  { "toggle_pause", 0, command_toggle_pause },
  { "fullscreen", 0, command_fullscreen },
  { "seek", 1, command_seek },
  { "sub_seek", 1, command_sub_seek },
  { "set_position", 1, command_set_position },
  { "playlist_prev", 0, command_playlist_prev },
  { "playlist_next", 0, command_playlist_next },
  { "playlist_jump", 1, command_playlist_jump },
  { "playlist_remove", 1, command_playlist_remove },
  { "playlist_move_up", 1, command_playlist_move_up },
  { "playlist_shuffle", 0, command_playlist_shuffle },
  { "loop_file", 1, command_loop_file },
  { "loop_playlist", 1, command_loop_playlist },
  { "add_volume", 1, command_add_volume },
  { "set_volume", 1, command_set_volume },
  { "add_sub_delay", 1, command_add_sub_delay },
  { "set_sub_delay", 1, command_set_sub_delay },
  { "add_sub_scale", 1, command_add_sub_scale },
  { "add_audio_delay", 1, command_add_audio_delay },
  { "set_audio_delay", 1, command_set_audio_delay },
  { "cycle_sub", 0, command_cycle_sub },
  { "cycle_audio", 0, command_cycle_audio },
  { "cycle_audio_device", 0, command_cycle_audio_device },
  { "cycle_add_chapter", 1, command_add_chapter },
  // terminator
  { 0 }
};
