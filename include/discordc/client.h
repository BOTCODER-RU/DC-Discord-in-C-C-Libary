#ifndef DISCORDC_CLIENT_H
#define DISCORDC_CLIENT_H

#include <stdint.h>

#include "discordc/gateway.h"
#include "discordc/rest.h"
#include "discordc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dc_client {
    dc_rest rest;
    dc_gateway *gateway;
    uint64_t intents;
} dc_client;

typedef struct dc_client_config {
    const char *bot_token;
    uint64_t intents;
    dc_event_handler on_event;
    void *user_data;
} dc_client_config;

dc_status dc_client_init(dc_client *client, const dc_client_config *config);
void dc_client_cleanup(dc_client *client);

dc_status dc_client_gateway_run(dc_client *client);
void dc_client_gateway_stop(dc_client *client);

#ifdef __cplusplus
}
#endif

#endif
