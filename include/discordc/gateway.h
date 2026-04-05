#ifndef DISCORDC_GATEWAY_H
#define DISCORDC_GATEWAY_H

#include <stdbool.h>
#include <stdint.h>

#include <cjson/cJSON.h>

#include "discordc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dc_gateway dc_gateway;

typedef void (*dc_event_handler)(
    const char *event_name,
    const cJSON *event_payload,
    void *user_data
);

typedef struct dc_gateway_config {
    const char *bot_token;
    uint64_t intents;
    dc_event_handler on_event;
    void *user_data;
} dc_gateway_config;

dc_status dc_gateway_init(dc_gateway **out, const dc_gateway_config *config);
void dc_gateway_cleanup(dc_gateway *gateway);

dc_status dc_gateway_run(dc_gateway *gateway);
void dc_gateway_stop(dc_gateway *gateway);

#ifdef __cplusplus
}
#endif

#endif
