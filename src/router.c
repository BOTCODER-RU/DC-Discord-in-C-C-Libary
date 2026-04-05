#include "discordc/router.h"

#include <string.h>

static const dc_route_entry *dc_find_route(const dc_route_entry *routes, size_t count, const char *key) {
    if (!routes || !key) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (routes[i].key && routes[i].handler && strcmp(routes[i].key, key) == 0) {
            return &routes[i];
        }
    }

    return NULL;
}

void dc_router_init(dc_router *router) {
    if (!router) {
        return;
    }
    memset(router, 0, sizeof(*router));
}

dc_status dc_router_dispatch(dc_router *router, dc_client *client, const cJSON *interaction_payload) {
    if (!router || !client || !interaction_payload) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    cJSON *id = cJSON_GetObjectItemCaseSensitive((cJSON *)interaction_payload, "id");
    cJSON *token = cJSON_GetObjectItemCaseSensitive((cJSON *)interaction_payload, "token");
    cJSON *type = cJSON_GetObjectItemCaseSensitive((cJSON *)interaction_payload, "type");
    cJSON *data = cJSON_GetObjectItemCaseSensitive((cJSON *)interaction_payload, "data");
    if (!cJSON_IsString(id) || !cJSON_IsString(token) || !cJSON_IsNumber(type)) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    dc_interaction_context ctx = {0};
    ctx.client = client;
    ctx.interaction_id = id->valuestring;
    ctx.interaction_token = token->valuestring;
    ctx.interaction_type = (dc_interaction_type)type->valueint;
    ctx.payload = interaction_payload;
    ctx.data = data;
    ctx.user_data = router->user_data;

    if (ctx.interaction_type == DC_INTERACTION_TYPE_APPLICATION_COMMAND) {
        cJSON *name = data ? cJSON_GetObjectItemCaseSensitive(data, "name") : NULL;
        const dc_route_entry *route = cJSON_IsString(name) ?
            dc_find_route(router->command_routes, router->command_route_count, name->valuestring) : NULL;
        if (route) {
            route->handler(&ctx);
            return DC_OK;
        }
    }

    if (ctx.interaction_type == DC_INTERACTION_TYPE_MESSAGE_COMPONENT) {
        cJSON *custom_id = data ? cJSON_GetObjectItemCaseSensitive(data, "custom_id") : NULL;
        const dc_route_entry *route = cJSON_IsString(custom_id) ?
            dc_find_route(router->component_routes, router->component_route_count, custom_id->valuestring) : NULL;
        if (route) {
            route->handler(&ctx);
            return DC_OK;
        }
    }

    if (ctx.interaction_type == DC_INTERACTION_TYPE_MODAL_SUBMIT) {
        cJSON *custom_id = data ? cJSON_GetObjectItemCaseSensitive(data, "custom_id") : NULL;
        const dc_route_entry *route = cJSON_IsString(custom_id) ?
            dc_find_route(router->modal_routes, router->modal_route_count, custom_id->valuestring) : NULL;
        if (route) {
            route->handler(&ctx);
            return DC_OK;
        }
    }

    if (router->default_handler) {
        router->default_handler(&ctx);
    }

    return DC_OK;
}

dc_status dc_router_respond(const dc_interaction_context *ctx, const dc_interaction_response *response, long *http_status_out) {
    if (!ctx || !ctx->client || !response) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    dc_http_response out = {0};
    dc_status status = dc_rest_create_interaction_response(
        &ctx->client->rest,
        ctx->interaction_id,
        ctx->interaction_token,
        response,
        &out
    );

    if (http_status_out) {
        *http_status_out = out.status_code;
    }
    dc_http_response_cleanup(&out);
    return status;
}
