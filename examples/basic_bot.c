#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "discordc/discord.h"

typedef struct app_ctx {
    dc_client *client;
    dc_router router;
    int button_clicks;
    int modal_submits;
} app_ctx;

static const int DC_INTERACTION_RESPONSE_CHANNEL_MESSAGE_WITH_SOURCE = 4;
static const int DC_INTERACTION_RESPONSE_MODAL = 9;
static const int DC_MESSAGE_FLAG_EPHEMERAL = 1 << 6;

static const char *find_modal_value(cJSON *components, const char *target_custom_id) {
    if (!cJSON_IsArray(components) || !target_custom_id) {
        return NULL;
    }

    cJSON *row = NULL;
    cJSON_ArrayForEach(row, components) {
        cJSON *row_components = cJSON_GetObjectItemCaseSensitive(row, "components");
        if (!cJSON_IsArray(row_components)) {
            continue;
        }

        cJSON *component = NULL;
        cJSON_ArrayForEach(component, row_components) {
            cJSON *custom_id = cJSON_GetObjectItemCaseSensitive(component, "custom_id");
            cJSON *value = cJSON_GetObjectItemCaseSensitive(component, "value");
            if (cJSON_IsString(custom_id) && cJSON_IsString(value) &&
                strcmp(custom_id->valuestring, target_custom_id) == 0) {
                return value->valuestring;
            }
        }
    }

    return NULL;
}

static void handle_command_ping(const dc_interaction_context *ictx) {
    dc_component button = {0};
    button.kind = DC_COMPONENT_BUTTON;
    button.as.button.style = 1;
    button.as.button.label = "Open Modal";
    button.as.button.custom_id = "open_modal";

    dc_action_row row = {0};
    row.components = &button;
    row.component_count = 1;

    dc_interaction_response response = {0};
    response.type = DC_INTERACTION_RESPONSE_CHANNEL_MESSAGE_WITH_SOURCE;
    response.data.content = "Pong from slash command. Нажми кнопку, чтобы открыть modal.";
    response.data.components = &row;
    response.data.component_count = 1;
    response.data.flags = DC_MESSAGE_FLAG_EPHEMERAL;
    response.data.has_flags = 1;

    long http_status = 0;
    dc_status status = dc_router_respond(ictx, &response, &http_status);
    printf("Interaction response status=%d http=%ld\n", status, http_status);
}

static void handle_component_open_modal(const dc_interaction_context *ictx) {
    app_ctx *ctx = (app_ctx *)ictx->user_data;
    ctx->button_clicks++;

    dc_component input_component = {0};
    input_component.kind = DC_COMPONENT_TEXT_INPUT;
    input_component.as.text_input.style = 1;
    input_component.as.text_input.custom_id = "feedback_input";
    input_component.as.text_input.label = "Your feedback";
    input_component.as.text_input.placeholder = "Type something";
    input_component.as.text_input.required = 1;
    input_component.as.text_input.has_required = 1;
    input_component.as.text_input.min_length = 1;
    input_component.as.text_input.has_min_length = 1;
    input_component.as.text_input.max_length = 200;
    input_component.as.text_input.has_max_length = 1;

    dc_action_row modal_row = {0};
    modal_row.components = &input_component;
    modal_row.component_count = 1;

    dc_interaction_response response = {0};
    response.type = DC_INTERACTION_RESPONSE_MODAL;
    response.modal_custom_id = "feedback_modal";
    response.modal_title = "Feedback";
    response.modal_rows = &modal_row;
    response.modal_row_count = 1;

    long http_status = 0;
    dc_status status = dc_router_respond(ictx, &response, &http_status);
    printf("Interaction response status=%d http=%ld\n", status, http_status);
}

static void handle_modal_feedback(const dc_interaction_context *ictx) {
    app_ctx *ctx = (app_ctx *)ictx->user_data;
    ctx->modal_submits++;

    cJSON *components = ictx->data ? cJSON_GetObjectItemCaseSensitive((cJSON *)ictx->data, "components") : NULL;
    const char *feedback = find_modal_value(components, "feedback_input");

    char content_buf[320];
    if (feedback && feedback[0] != '\0') {
        snprintf(
            content_buf,
            sizeof(content_buf),
            "Received modal input: %s (clicks=%d, submits=%d)",
            feedback,
            ctx->button_clicks,
            ctx->modal_submits
        );
    } else {
        snprintf(
            content_buf,
            sizeof(content_buf),
            "Modal submitted with empty value (clicks=%d, submits=%d)",
            ctx->button_clicks,
            ctx->modal_submits
        );
    }

    dc_interaction_response response = {0};
    response.type = DC_INTERACTION_RESPONSE_CHANNEL_MESSAGE_WITH_SOURCE;
    response.data.content = content_buf;
    response.data.flags = DC_MESSAGE_FLAG_EPHEMERAL;
    response.data.has_flags = 1;

    long http_status = 0;
    dc_status status = dc_router_respond(ictx, &response, &http_status);
    printf("Interaction response status=%d http=%ld\n", status, http_status);
}

static void on_event(const char *event_name, const cJSON *payload, void *user_data) {
    app_ctx *ctx = (app_ctx *)user_data;
    dc_client *client = ctx->client;

    if (!event_name) {
        return;
    }

    if (strcmp(event_name, "READY") == 0) {
        printf("Gateway READY\n");
        const char *app_id = getenv("DISCORD_APP_ID");
        const char *guild_id = getenv("DISCORD_GUILD_ID");
        if (app_id && guild_id) {
            dc_application_command command = {0};
            command.name = "ping";
            command.description = "Ping command from discordc";
            command.type = 1;

            dc_http_response cmd_resp = {0};
            dc_status cmd_status = dc_rest_create_guild_command(&client->rest, app_id, guild_id, &command, &cmd_resp);
            printf("Register /ping status=%d http=%ld\n", cmd_status, cmd_resp.status_code);
            dc_http_response_cleanup(&cmd_resp);
        }
        return;
    }

    if (strcmp(event_name, "INTERACTION_CREATE") == 0) {
        dc_status status = dc_router_dispatch(&ctx->router, client, payload);
        if (status != DC_OK) {
            printf("Router dispatch failed: %d\n", status);
        }
        return;
    }

    if (strcmp(event_name, "MESSAGE_CREATE") == 0) {
        cJSON *content = cJSON_GetObjectItemCaseSensitive((cJSON *)payload, "content");
        cJSON *channel_id = cJSON_GetObjectItemCaseSensitive((cJSON *)payload, "channel_id");
        cJSON *author = cJSON_GetObjectItemCaseSensitive((cJSON *)payload, "author");
        cJSON *bot = author ? cJSON_GetObjectItemCaseSensitive(author, "bot") : NULL;

        if (cJSON_IsTrue(bot)) {
            return;
        }

        if (cJSON_IsString(content) && cJSON_IsString(channel_id)) {
            if (strcmp(content->valuestring, "!ping") == 0) {
                dc_http_response response = {0};
                dc_status status = dc_rest_create_message(&client->rest, channel_id->valuestring, "pong", &response);
                if (status == DC_OK) {
                    printf("Sent message, status=%ld\n", response.status_code);
                } else {
                    printf("Failed to send message, code=%d\n", status);
                }
                dc_http_response_cleanup(&response);
                return;
            }

            if (strcmp(content->valuestring, "!embed") == 0) {
                dc_embed_field fields[] = {
                    { "Library", "discordc", 1 },
                    { "API", "Discord v10", 1 },
                    { "Mode", "Embed payload", 1 }
                };

                dc_embed embed = {0};
                embed.title = "Embed from discordc";
                embed.description = "Example of modern message payload with embeds.";
                embed.url = "https://discord.com/developers/docs/resources/channel#embed-object";
                embed.color = 0x2F3136;
                embed.has_color = 1;
                embed.footer.text = "Generated by C library";
                embed.author.name = "discordc bot";
                embed.fields = fields;
                embed.field_count = sizeof(fields) / sizeof(fields[0]);

                dc_message_create message;
                dc_message_create_init(&message);
                message.content = "Embed demo";
                message.embeds = &embed;
                message.embed_count = 1;

                dc_http_response response = {0};
                dc_status status = dc_rest_create_message_ex(&client->rest, channel_id->valuestring, &message, &response);
                if (status == DC_OK) {
                    printf("Sent embed message, status=%ld\n", response.status_code);
                } else {
                    printf("Failed to send embed, code=%d\n", status);
                }
                dc_http_response_cleanup(&response);
                return;
            }

            if (strcmp(content->valuestring, "!components") == 0) {
                dc_component button = {0};
                button.kind = DC_COMPONENT_BUTTON;
                button.as.button.style = 3;
                button.as.button.label = "Click";
                button.as.button.custom_id = "message_btn_click";

                dc_action_row row = {0};
                row.components = &button;
                row.component_count = 1;

                cJSON *components_json = NULL;
                if (dc_components_to_json_array(&row, 1, &components_json) != DC_OK) {
                    return;
                }

                dc_message_create message;
                dc_message_create_init(&message);
                message.content = "Message with components";
                message.components = components_json;

                dc_http_response response = {0};
                dc_status status = dc_rest_create_message_ex(&client->rest, channel_id->valuestring, &message, &response);
                if (status == DC_OK) {
                    printf("Sent components message, status=%ld\n", response.status_code);
                } else {
                    printf("Failed to send components message, code=%d\n", status);
                }
                dc_http_response_cleanup(&response);
                cJSON_Delete(components_json);
            }
        }
    }
}

int main(void) {
    const char *token = getenv("DISCORD_BOT_TOKEN");
    if (!token) {
        fprintf(stderr, "Set DISCORD_BOT_TOKEN\n");
        return 1;
    }

    dc_client client = {0};
    app_ctx ctx = {0};
    ctx.client = &client;

    static const dc_route_entry command_routes[] = {
        { "ping", handle_command_ping }
    };
    static const dc_route_entry component_routes[] = {
        { "open_modal", handle_component_open_modal }
    };
    static const dc_route_entry modal_routes[] = {
        { "feedback_modal", handle_modal_feedback }
    };

    dc_router_init(&ctx.router);
    ctx.router.command_routes = command_routes;
    ctx.router.command_route_count = sizeof(command_routes) / sizeof(command_routes[0]);
    ctx.router.component_routes = component_routes;
    ctx.router.component_route_count = sizeof(component_routes) / sizeof(component_routes[0]);
    ctx.router.modal_routes = modal_routes;
    ctx.router.modal_route_count = sizeof(modal_routes) / sizeof(modal_routes[0]);
    ctx.router.user_data = &ctx;

    dc_client_config config = {
        .bot_token = token,
        .intents = (1ULL << 0) | (1ULL << 9) | (1ULL << 15),
        .on_event = on_event,
        .user_data = &ctx,
    };

    dc_status status = dc_client_init(&client, &config);
    if (status != DC_OK) {
        fprintf(stderr, "dc_client_init failed: %d\n", status);
        return 1;
    }

    dc_http_response me = {0};
    status = dc_rest_get_current_user(&client.rest, &me);
    if (status == DC_OK) {
        printf("REST /users/@me status=%ld body=%s\n", me.status_code, me.body ? me.body : "");
    } else {
        printf("REST call failed: %d\n", status);
    }
    dc_http_response_cleanup(&me);

    printf("Starting gateway loop... Ctrl+C to stop\n");
    status = dc_client_gateway_run(&client);
    if (status != DC_OK) {
        fprintf(stderr, "gateway failed: %d\n", status);
    }

    dc_client_cleanup(&client);
    return status == DC_OK ? 0 : 1;
}