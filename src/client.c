#include "discordc/client.h"

#include <stdlib.h>
#include <string.h>

dc_status dc_client_init(dc_client *client, const dc_client_config *config) {
    if (!client || !config || !config->bot_token) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    memset(client, 0, sizeof(*client));
    client->intents = config->intents;

    dc_status status = dc_rest_init(&client->rest, config->bot_token);
    if (status != DC_OK) {
        return status;
    }

    dc_gateway_config gw_config = {
        .bot_token = config->bot_token,
        .intents = config->intents,
        .on_event = config->on_event,
        .user_data = config->user_data,
    };

    status = dc_gateway_init(&client->gateway, &gw_config);
    if (status != DC_OK) {
        dc_rest_cleanup(&client->rest);
        return status;
    }

    return DC_OK;
}

void dc_client_cleanup(dc_client *client) {
    if (!client) {
        return;
    }

    dc_gateway_cleanup(client->gateway);
    client->gateway = NULL;

    dc_rest_cleanup(&client->rest);
}

dc_status dc_client_gateway_run(dc_client *client) {
    if (!client || !client->gateway) {
        return DC_ERR_INVALID_ARGUMENT;
    }
    return dc_gateway_run(client->gateway);
}

void dc_client_gateway_stop(dc_client *client) {
    if (!client || !client->gateway) {
        return;
    }
    dc_gateway_stop(client->gateway);
}
