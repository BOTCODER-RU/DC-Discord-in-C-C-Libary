#ifndef DISCORDC_ROUTER_H
#define DISCORDC_ROUTER_H

#include <stddef.h>

#include <cjson/cJSON.h>

#include "discordc/client.h"
#include "discordc/models.h"
#include "discordc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dc_interaction_type {
    DC_INTERACTION_TYPE_PING = 1,
    DC_INTERACTION_TYPE_APPLICATION_COMMAND = 2,
    DC_INTERACTION_TYPE_MESSAGE_COMPONENT = 3,
    DC_INTERACTION_TYPE_APPLICATION_COMMAND_AUTOCOMPLETE = 4,
    DC_INTERACTION_TYPE_MODAL_SUBMIT = 5
} dc_interaction_type;

typedef struct dc_interaction_context {
    dc_client *client;
    const char *interaction_id;
    const char *interaction_token;
    dc_interaction_type interaction_type;
    const cJSON *payload;
    const cJSON *data;
    void *user_data;
} dc_interaction_context;

typedef void (*dc_interaction_handler)(const dc_interaction_context *ctx);

typedef struct dc_route_entry {
    const char *key;
    dc_interaction_handler handler;
} dc_route_entry;

typedef struct dc_router {
    const dc_route_entry *command_routes;
    size_t command_route_count;
    const dc_route_entry *component_routes;
    size_t component_route_count;
    const dc_route_entry *modal_routes;
    size_t modal_route_count;
    dc_interaction_handler default_handler;
    void *user_data;
} dc_router;

void dc_router_init(dc_router *router);
dc_status dc_router_dispatch(dc_router *router, dc_client *client, const cJSON *interaction_payload);
dc_status dc_router_respond(const dc_interaction_context *ctx, const dc_interaction_response *response, long *http_status_out);

#ifdef __cplusplus
}
#endif

#endif
