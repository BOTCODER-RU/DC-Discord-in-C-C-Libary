#include "discordc/gateway.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libwebsockets.h>

struct dc_gateway {
    struct lws_context *context;
    struct lws *wsi;

    char *bot_token;
    uint64_t intents;

    dc_event_handler on_event;
    void *user_data;

    bool stop_requested;
    bool hello_received;
    bool identify_sent;
    int64_t sequence;
    int heartbeat_interval_ms;
    int64_t next_heartbeat_ms;
    bool heartbeat_pending;
    bool connected;
};

static int dc_gateway_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

static struct lws_protocols dc_protocols[] = {
    {
        .name = "discord-gateway",
        .callback = dc_gateway_callback,
        .per_session_data_size = 0,
        .rx_buffer_size = 1 << 14,
    },
    { NULL, NULL, 0, 0 }
};

static char *dc_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t n = strlen(s) + 1;
    char *d = (char *)malloc(n);
    if (!d) {
        return NULL;
    }
    memcpy(d, s, n);
    return d;
}

static int64_t dc_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static char *dc_build_identify_payload(dc_gateway *gw) {
    cJSON *root = cJSON_CreateObject();
    cJSON *d = cJSON_CreateObject();
    cJSON *properties = cJSON_CreateObject();
    if (!root || !d || !properties) {
        cJSON_Delete(root);
        cJSON_Delete(d);
        cJSON_Delete(properties);
        return NULL;
    }

    cJSON_AddNumberToObject(root, "op", 2);
    cJSON_AddItemToObject(root, "d", d);
    cJSON_AddStringToObject(d, "token", gw->bot_token);
    cJSON_AddNumberToObject(d, "intents", (double)gw->intents);
    cJSON_AddItemToObject(d, "properties", properties);

#if defined(_WIN32)
    cJSON_AddStringToObject(properties, "os", "windows");
#elif defined(__APPLE__)
    cJSON_AddStringToObject(properties, "os", "macos");
#else
    cJSON_AddStringToObject(properties, "os", "linux");
#endif
    cJSON_AddStringToObject(properties, "browser", "discordc");
    cJSON_AddStringToObject(properties, "device", "discordc");

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return payload;
}

static char *dc_build_heartbeat_payload(dc_gateway *gw) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }

    cJSON_AddNumberToObject(root, "op", 1);
    if (gw->sequence >= 0) {
        cJSON_AddNumberToObject(root, "d", (double)gw->sequence);
    } else {
        cJSON_AddNullToObject(root, "d");
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return payload;
}

static int dc_gateway_send_text(struct lws *wsi, const char *payload) {
    size_t payload_len = strlen(payload);
    size_t buf_len = LWS_PRE + payload_len;
    unsigned char *buf = (unsigned char *)malloc(buf_len);
    if (!buf) {
        return -1;
    }
    memcpy(buf + LWS_PRE, payload, payload_len);
    int written = lws_write(wsi, buf + LWS_PRE, payload_len, LWS_WRITE_TEXT);
    free(buf);
    return written;
}

static void dc_gateway_handle_dispatch(dc_gateway *gw, cJSON *root) {
    cJSON *t = cJSON_GetObjectItemCaseSensitive(root, "t");
    cJSON *d = cJSON_GetObjectItemCaseSensitive(root, "d");
    if (cJSON_IsString(t) && t->valuestring && gw->on_event) {
        gw->on_event(t->valuestring, d, gw->user_data);
    }
}

static void dc_gateway_handle_hello(dc_gateway *gw, cJSON *root) {
    cJSON *d = cJSON_GetObjectItemCaseSensitive(root, "d");
    cJSON *heartbeat_interval = cJSON_GetObjectItemCaseSensitive(d, "heartbeat_interval");
    if (!cJSON_IsNumber(heartbeat_interval)) {
        return;
    }
    gw->heartbeat_interval_ms = heartbeat_interval->valueint;
    gw->next_heartbeat_ms = dc_now_ms() + gw->heartbeat_interval_ms;
    gw->hello_received = true;
}

static void dc_gateway_handle_payload(dc_gateway *gw, const char *payload) {
    cJSON *root = cJSON_Parse(payload);
    if (!root) {
        return;
    }

    cJSON *op = cJSON_GetObjectItemCaseSensitive(root, "op");
    cJSON *s = cJSON_GetObjectItemCaseSensitive(root, "s");

    if (cJSON_IsNumber(s)) {
        gw->sequence = (int64_t)s->valuedouble;
    }

    if (!cJSON_IsNumber(op)) {
        cJSON_Delete(root);
        return;
    }

    switch (op->valueint) {
        case 0:
            dc_gateway_handle_dispatch(gw, root);
            break;
        case 10:
            dc_gateway_handle_hello(gw, root);
            break;
        case 11:
            break;
        default:
            break;
    }

    cJSON_Delete(root);
}

static int dc_gateway_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    (void)user;
    dc_gateway *gw = (dc_gateway *)lws_context_user(lws_get_context(wsi));
    if (!gw) {
        return 0;
    }

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            gw->wsi = wsi;
            gw->connected = true;
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            gw->connected = false;
            gw->stop_requested = true;
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            gw->connected = false;
            gw->stop_requested = true;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            char *payload = (char *)malloc(len + 1);
            if (!payload) {
                return -1;
            }
            memcpy(payload, in, len);
            payload[len] = '\0';
            dc_gateway_handle_payload(gw, payload);
            free(payload);
            lws_callback_on_writable(wsi);
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            if (gw->hello_received && !gw->identify_sent) {
                char *identify = dc_build_identify_payload(gw);
                if (!identify) {
                    return -1;
                }
                if (dc_gateway_send_text(wsi, identify) < 0) {
                    free(identify);
                    return -1;
                }
                gw->identify_sent = true;
                free(identify);
            }

            if (gw->heartbeat_pending) {
                char *heartbeat = dc_build_heartbeat_payload(gw);
                if (!heartbeat) {
                    return -1;
                }
                if (dc_gateway_send_text(wsi, heartbeat) < 0) {
                    free(heartbeat);
                    return -1;
                }
                free(heartbeat);
                gw->heartbeat_pending = false;
                gw->next_heartbeat_ms = dc_now_ms() + gw->heartbeat_interval_ms;
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

dc_status dc_gateway_init(dc_gateway **out, const dc_gateway_config *config) {
    if (!out || !config || !config->bot_token) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    dc_gateway *gw = (dc_gateway *)calloc(1, sizeof(*gw));
    if (!gw) {
        return DC_ERR_ALLOC;
    }

    gw->bot_token = dc_strdup(config->bot_token);
    gw->intents = config->intents;
    gw->on_event = config->on_event;
    gw->user_data = config->user_data;
    gw->sequence = -1;
    if (!gw->bot_token) {
        free(gw);
        return DC_ERR_ALLOC;
    }

    *out = gw;
    return DC_OK;
}

void dc_gateway_cleanup(dc_gateway *gateway) {
    if (!gateway) {
        return;
    }
    if (gateway->context) {
        lws_context_destroy(gateway->context);
        gateway->context = NULL;
    }
    free(gateway->bot_token);
    free(gateway);
}

dc_status dc_gateway_run(dc_gateway *gateway) {
    if (!gateway) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = dc_protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.user = gateway;

    gateway->context = lws_create_context(&info);
    if (!gateway->context) {
        return DC_ERR_GATEWAY;
    }

    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = gateway->context;
    ccinfo.address = "gateway.discord.gg";
    ccinfo.port = 443;
    ccinfo.path = "/?v=10&encoding=json";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = dc_protocols[0].name;
    ccinfo.ssl_connection = LCCSCF_USE_SSL;

    gateway->stop_requested = false;
    gateway->hello_received = false;
    gateway->identify_sent = false;
    gateway->heartbeat_interval_ms = 0;
    gateway->next_heartbeat_ms = 0;
    gateway->heartbeat_pending = false;

    gateway->wsi = lws_client_connect_via_info(&ccinfo);
    if (!gateway->wsi) {
        lws_context_destroy(gateway->context);
        gateway->context = NULL;
        return DC_ERR_GATEWAY;
    }

    while (!gateway->stop_requested) {
        int64_t now = dc_now_ms();
        if (gateway->heartbeat_interval_ms > 0 && now >= gateway->next_heartbeat_ms) {
            gateway->heartbeat_pending = true;
            if (gateway->wsi) {
                lws_callback_on_writable(gateway->wsi);
            }
        }

        if (lws_service(gateway->context, 50) < 0) {
            break;
        }
    }

    return DC_OK;
}

void dc_gateway_stop(dc_gateway *gateway) {
    if (!gateway) {
        return;
    }
    gateway->stop_requested = true;
}
