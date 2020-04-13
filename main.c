#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <libgen.h>
#define __USE_GNU
#include <dlfcn.h>

#include <libwebsockets.h>
#include <mpv/client.h>

#include "commands.h"
#define SCRIPT_OPT_PREFIX "ws-webui-"

struct web_message {
  char command[256];
  char param[256];
};

struct ws_data {
  struct web_message message;
  mpv_handle *mpv;
  volatile uint8_t running;
};

struct ws_config {
  uint8_t use_ipv6;
  char *iface;
  int port;
  char *webui_dir;
};

static signed char ws_request_parse_callback(struct lejp_ctx *ctx, char reason) {
  if (reason & LEJP_FLAG_CB_IS_VALUE) {
    struct web_message *message = (struct web_message *) ctx->user;

    if (strcmp(ctx->path, "command") == 0) {
      strncpy(message->command, ctx->buf, sizeof(message->command));
    } else if (strcmp(ctx->path, "param") == 0) {
      strncpy(message->param, ctx->buf, sizeof(message->param));
    }
  }

  return 0;
}

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
  struct lws_context *context = lws_get_context(wsi);
  if (context == NULL) {
    fprintf(stderr, "[lws_get_context] failed to get `context`\n");
    return -1;
  }
  struct ws_data *data = (struct ws_data *) lws_context_user(context);
  if (data == NULL) {
    fprintf(stderr, "[lws_context_user] failed to get `data`\n");
    return -1;
  }
  struct web_message *message = &data->message;

  switch (reason) {
    case LWS_CALLBACK_RECEIVE: {
      message->command[0] = '\0';
      message->param[0] = '\0';

      struct lejp_ctx lejp;
      lejp_construct(&lejp, ws_request_parse_callback, NULL, NULL, 0);
      lejp.user = data;

      if (lejp_parse(&lejp, (unsigned char *) in, len) != 0) {
        fprintf(stderr, "[lejp_parse] failed to parse json\n");
        return -1;
      }

      size_t i = 0;
      struct command *cmd = NULL;
      void *res;

      while (cmd == NULL && COMMANDS[i].name != NULL) {
        if (strcmp(message->command, COMMANDS[i].name) == 0) {
          cmd = &COMMANDS[i];
        }

        i++;
      }

      if (cmd == NULL) {
        printf("{ command: %s, param: %s }\n", message->command, message->param);
        printf("Command not implemented\n");
      } else if ((res = cmd->callback(data->mpv, cmd->has_param ? message->param : NULL)) != NULL) {
        // prepend LWS_SEND_BUFFER_PRE_PADDING
        size_t res_len =  strlen(res);
        size_t response_size = LWS_SEND_BUFFER_PRE_PADDING + res_len + LWS_SEND_BUFFER_POST_PADDING;
        char *response = malloc(response_size);

        if (response == NULL) {
          fprintf(stderr, "[malloc] failed to allocate `response`\n");
          return -1;
        }

        strncpy(&response[LWS_SEND_BUFFER_PRE_PADDING], res, res_len);
        lws_write(wsi, &response[LWS_SEND_BUFFER_PRE_PADDING], res_len, LWS_WRITE_TEXT);

        free(response);
      }

      break;
    }
  }

  return 0;
}

void parse_script_options(char *script_opts, struct ws_config *config) {
  size_t prefix_len = sizeof(SCRIPT_OPT_PREFIX) - 1;
  char *entry_ptr;
  char *entry = strtok_r(script_opts, ",", &entry_ptr);

  while (entry != NULL) {
    char *sub_entry_ptr;
    char *key = strtok_r(entry, "=", &sub_entry_ptr);
    char *value = strtok_r(NULL, "=", &sub_entry_ptr);

    if (strncmp(SCRIPT_OPT_PREFIX, key, prefix_len) == 0) {
      // remove prefix
      key = &key[prefix_len];

      if (strcmp(key, "ipv6") == 0) {
        if (strcmp(value, "yes") != 0) {
          config->use_ipv6 = 0;
        }
      } else if (strcmp(key, "interface") == 0) {
        if (strcmp(value, "all") != 0) {
          config->iface = value;
        }
      } else if (strcmp(key, "port") == 0) {
        sscanf(value, "%d", &config->port);
      } else if (strcmp(key, "dir") == 0) {
        config->webui_dir = value;
      }
    }

    entry = strtok_r(NULL, ",", &entry_ptr);
  }
}

void *start_ws_thread(void *arg0) {
  struct ws_data *data = (struct ws_data *) arg0;
  mpv_handle *handle = data->mpv;

  // parse script options starting with ws-webui: ipv6:str [ default: yes, values yes or no], interface:str [default:all, any value], port:int [default: 8080], dir:str [default "ws-webui-page"]
  struct ws_config config;
  // set default values
  config.use_ipv6 = 1;
  config.iface = NULL;
  config.port = 8080;
  config.webui_dir = "ws-webui-page";

  char *script_opts = mpv_get_property_string(handle, "script-opts");
  if (script_opts != NULL) {
    parse_script_options(script_opts, &config);
  }

  // get plugin path
  Dl_info dl_info;
  if (dladdr((void*) start_ws_thread, &dl_info) == 0) {
    fprintf(stderr, "[dladdr] failed to fetch `plugin_path`\n");
    return NULL;
  }
  char *plugin_path = dirname((char *) dl_info.dli_fname);

  // path / dir \0
  size_t origin_dir_size = strlen(plugin_path) + 1 + strlen(config.webui_dir) + 1;
  char *origin_dir = malloc(origin_dir_size);

  if (origin_dir == NULL) {
    fprintf(stderr, "[malloc] failed to allocate `origin_dir`\n");
    return NULL;
  }
  snprintf(origin_dir, origin_dir_size, "%s/%s", plugin_path, config.webui_dir);

  // log level
  int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
  lws_set_log_level(logs, NULL);

  // mount point
  struct lws_http_mount mount;
  memset(&mount, 0, sizeof(mount));
  mount.mountpoint = "/";
  mount.origin = origin_dir;
  mount.def = "index.html";
  mount.origin_protocol = LWSMPRO_FILE;
  mount.mountpoint_len = 1;

  // servers to host
  struct lws_protocols protocols[] = {
    // simple http webserver for static file hosting
    { "http", lws_callback_http_dummy, 0 },
    // websocket for mpv/webui communication
    { "ws", ws_callback, 0 },
    { NULL, NULL, 0 } /* terminator */
  };

  // server info
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  //info.vhost_name = NULL;
  info.iface = config.iface; // NULL -> all interfaces
  info.port = config.port;
  info.mounts = &mount;
  info.protocols = protocols;
  info.ws_ping_pong_interval = 10;
  info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

  if (config.use_ipv6 == 0) {
    info.options |= LWS_SERVER_OPTION_DISABLE_IPV6;
  }

  // shared data within ws server
  info.user = data;

  // serve
  struct lws_context *context = lws_create_context(&info);

  if (context == NULL) {
    fprintf(stderr, "[lws_create_context] failed to create `context`\n");
    return NULL;
  }

  printf("[ws-webui] serving on interface: %s, port %d, ipv6 %s | %s\n", config.iface == NULL ? "all" : config.iface, config.port, config.use_ipv6 == 0 ? "no" : "yes", origin_dir);
  while (data->running && lws_service(context, 0) >= 0);

  lws_context_destroy(context);
  free(origin_dir);
  mpv_free(script_opts);

  return NULL;
}

int mpv_open_cplugin(mpv_handle *handle) {
  struct ws_data *data = malloc(sizeof(*data));
  if (data == NULL) {
    fprintf(stderr, "[malloc] failed to allocate `data`\n");
    return -1;
  }

  data->mpv = handle;
  data->running = 1;
  pthread_t ws_thread;

  if (pthread_create(&ws_thread, NULL, start_ws_thread, data) != 0) {
    fprintf(stderr, "[pthread_create] failed to create thread `ws_thread`\n");
    return -1;
  }

  while (1) {
    mpv_event *event = mpv_wait_event(handle, -1);

    if (event->event_id == MPV_EVENT_SHUTDOWN) {
      data->running = 0;
      break;
    }
  }

  //pthread_join(ws_thread, NULL);
  free(data);
  return 0;
}
