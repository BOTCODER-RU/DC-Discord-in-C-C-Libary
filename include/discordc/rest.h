#ifndef DISCORDC_REST_H
#define DISCORDC_REST_H

#include "discordc/types.h"
#include "discordc/models.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dc_rest {
    char *token;
    char *base_url;
} dc_rest;

dc_status dc_rest_init(dc_rest *rest, const char *bot_token);
void dc_rest_cleanup(dc_rest *rest);

dc_status dc_rest_request(
    dc_rest *rest,
    const char *method,
    const char *route,
    const char *json_body,
    dc_http_response *out
);

void dc_http_response_cleanup(dc_http_response *response);

dc_status dc_rest_get_current_user(dc_rest *rest, dc_http_response *out);
dc_status dc_rest_get_channel(dc_rest *rest, const char *channel_id, dc_http_response *out);
dc_status dc_rest_modify_channel(dc_rest *rest, const char *channel_id, const char *json_body, dc_http_response *out);
dc_status dc_rest_create_message(
    dc_rest *rest,
    const char *channel_id,
    const char *content,
    dc_http_response *out
);
dc_status dc_rest_create_message_ex(
    dc_rest *rest,
    const char *channel_id,
    const dc_message_create *message,
    dc_http_response *out
);
dc_status dc_rest_delete_message(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    dc_http_response *out
);
dc_status dc_rest_add_reaction(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *emoji_encoded,
    dc_http_response *out
);
dc_status dc_rest_delete_own_reaction(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *emoji_encoded,
    dc_http_response *out
);
dc_status dc_rest_edit_channel_permissions(
    dc_rest *rest,
    const char *channel_id,
    const char *overwrite_id,
    const char *allow,
    const char *deny,
    int overwrite_type,
    dc_http_response *out
);

dc_status dc_rest_create_global_command(
    dc_rest *rest,
    const char *application_id,
    const dc_application_command *command,
    dc_http_response *out
);
dc_status dc_rest_bulk_overwrite_global_commands(
    dc_rest *rest,
    const char *application_id,
    const char *commands_json_array,
    dc_http_response *out
);
dc_status dc_rest_create_guild_command(
    dc_rest *rest,
    const char *application_id,
    const char *guild_id,
    const dc_application_command *command,
    dc_http_response *out
);
dc_status dc_rest_bulk_overwrite_guild_commands(
    dc_rest *rest,
    const char *application_id,
    const char *guild_id,
    const char *commands_json_array,
    dc_http_response *out
);

dc_status dc_rest_create_interaction_response(
    dc_rest *rest,
    const char *interaction_id,
    const char *interaction_token,
    const dc_interaction_response *response,
    dc_http_response *out
);
dc_status dc_rest_edit_original_interaction_response(
    dc_rest *rest,
    const char *application_id,
    const char *interaction_token,
    const char *json_body,
    dc_http_response *out
);
dc_status dc_rest_delete_original_interaction_response(
    dc_rest *rest,
    const char *application_id,
    const char *interaction_token,
    dc_http_response *out
);

dc_status dc_rest_start_thread_from_message(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *json_body,
    dc_http_response *out
);
dc_status dc_rest_start_thread_without_message(
    dc_rest *rest,
    const char *channel_id,
    const char *json_body,
    dc_http_response *out
);
dc_status dc_rest_create_forum_post(
    dc_rest *rest,
    const char *channel_id,
    const char *json_body,
    dc_http_response *out
);
dc_status dc_rest_execute_webhook(
    dc_rest *rest,
    const char *webhook_id,
    const char *webhook_token,
    const char *json_body,
    int wait,
    dc_http_response *out
);

#ifdef __cplusplus
}
#endif

#endif
