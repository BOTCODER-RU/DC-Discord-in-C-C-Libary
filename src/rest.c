#include "discordc/rest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

typedef struct dc_buffer {
    char *data;
    size_t len;
} dc_buffer;

static size_t dc_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    dc_buffer *buffer = (dc_buffer *)userp;

    char *new_data = (char *)realloc(buffer->data, buffer->len + real_size + 1);
    if (!new_data) {
        return 0;
    }

    buffer->data = new_data;
    memcpy(buffer->data + buffer->len, contents, real_size);
    buffer->len += real_size;
    buffer->data[buffer->len] = '\0';

    return real_size;
}

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

dc_status dc_rest_init(dc_rest *rest, const char *bot_token) {
    if (!rest || !bot_token) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    memset(rest, 0, sizeof(*rest));
    rest->token = dc_strdup(bot_token);
    rest->base_url = dc_strdup("https://discord.com/api/v10");
    if (!rest->token || !rest->base_url) {
        dc_rest_cleanup(rest);
        return DC_ERR_ALLOC;
    }

    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (code != CURLE_OK) {
        dc_rest_cleanup(rest);
        return DC_ERR_HTTP;
    }

    return DC_OK;
}

void dc_rest_cleanup(dc_rest *rest) {
    if (!rest) {
        return;
    }
    free(rest->token);
    free(rest->base_url);
    rest->token = NULL;
    rest->base_url = NULL;
    curl_global_cleanup();
}

dc_status dc_rest_request(
    dc_rest *rest,
    const char *method,
    const char *route,
    const char *json_body,
    dc_http_response *out
) {
    if (!rest || !method || !route || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    memset(out, 0, sizeof(*out));

    CURL *curl = curl_easy_init();
    if (!curl) {
        return DC_ERR_HTTP;
    }

    size_t url_len = strlen(rest->base_url) + strlen(route) + 1;
    char *url = (char *)malloc(url_len);
    if (!url) {
        curl_easy_cleanup(curl);
        return DC_ERR_ALLOC;
    }
    snprintf(url, url_len, "%s%s", rest->base_url, route);

    size_t auth_len = strlen("Authorization: Bot ") + strlen(rest->token) + 1;
    char *auth_header = (char *)malloc(auth_len);
    if (!auth_header) {
        free(url);
        curl_easy_cleanup(curl);
        return DC_ERR_ALLOC;
    }
    snprintf(auth_header, auth_len, "Authorization: Bot %s", rest->token);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: discordc (https://example.local, 0.1.0)");

    dc_buffer buffer = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dc_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (json_body && strlen(json_body) > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    }

    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        free(buffer.data);
        curl_slist_free_all(headers);
        free(auth_header);
        free(url);
        curl_easy_cleanup(curl);
        return DC_ERR_HTTP;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->status_code);
    out->body = buffer.data;
    out->body_len = buffer.len;

    curl_slist_free_all(headers);
    free(auth_header);
    free(url);
    curl_easy_cleanup(curl);
    return DC_OK;
}

void dc_http_response_cleanup(dc_http_response *response) {
    if (!response) {
        return;
    }
    free(response->body);
    response->body = NULL;
    response->body_len = 0;
    response->status_code = 0;
}

dc_status dc_rest_get_current_user(dc_rest *rest, dc_http_response *out) {
    return dc_rest_request(rest, "GET", "/users/@me", NULL, out);
}

dc_status dc_rest_get_channel(dc_rest *rest, const char *channel_id, dc_http_response *out) {
    if (!channel_id) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s", channel_id);

    dc_status status = dc_rest_request(rest, "GET", route, NULL, out);
    free(route);
    return status;
}

dc_status dc_rest_modify_channel(dc_rest *rest, const char *channel_id, const char *json_body, dc_http_response *out) {
    if (!channel_id || !json_body) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s", channel_id);

    dc_status status = dc_rest_request(rest, "PATCH", route, json_body, out);
    free(route);
    return status;
}

dc_status dc_rest_create_message(
    dc_rest *rest,
    const char *channel_id,
    const char *content,
    dc_http_response *out
) {
    if (!channel_id || !content) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    dc_message_create message;
    dc_message_create_init(&message);
    message.content = content;
    return dc_rest_create_message_ex(rest, channel_id, &message, out);
}

dc_status dc_rest_create_message_ex(
    dc_rest *rest,
    const char *channel_id,
    const dc_message_create *message,
    dc_http_response *out
) {
    if (!rest || !channel_id || !message || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    char *json_body = NULL;
    dc_status status = dc_message_create_to_json(message, &json_body);
    if (status != DC_OK) {
        return status;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/messages") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        free(json_body);
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/messages", channel_id);

    status = dc_rest_request(rest, "POST", route, json_body, out);

    free(route);
    free(json_body);
    return status;
}

dc_status dc_rest_delete_message(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    dc_http_response *out
) {
    if (!channel_id || !message_id) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/messages/") + strlen(message_id) + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/messages/%s", channel_id, message_id);

    dc_status status = dc_rest_request(rest, "DELETE", route, NULL, out);
    free(route);
    return status;
}

dc_status dc_rest_add_reaction(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *emoji_encoded,
    dc_http_response *out
) {
    if (!channel_id || !message_id || !emoji_encoded || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/messages/") +
        strlen(message_id) + strlen("/reactions/") + strlen(emoji_encoded) + strlen("/@me") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/messages/%s/reactions/%s/@me", channel_id, message_id, emoji_encoded);

    dc_status status = dc_rest_request(rest, "PUT", route, NULL, out);
    free(route);
    return status;
}

dc_status dc_rest_delete_own_reaction(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *emoji_encoded,
    dc_http_response *out
) {
    if (!channel_id || !message_id || !emoji_encoded || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/messages/") +
        strlen(message_id) + strlen("/reactions/") + strlen(emoji_encoded) + strlen("/@me") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/messages/%s/reactions/%s/@me", channel_id, message_id, emoji_encoded);

    dc_status status = dc_rest_request(rest, "DELETE", route, NULL, out);
    free(route);
    return status;
}

dc_status dc_rest_edit_channel_permissions(
    dc_rest *rest,
    const char *channel_id,
    const char *overwrite_id,
    const char *allow,
    const char *deny,
    int overwrite_type,
    dc_http_response *out
) {
    if (!rest || !channel_id || !overwrite_id || !allow || !deny || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    cJSON *payload = cJSON_CreateObject();
    if (!payload) {
        return DC_ERR_ALLOC;
    }
    cJSON_AddStringToObject(payload, "allow", allow);
    cJSON_AddStringToObject(payload, "deny", deny);
    cJSON_AddNumberToObject(payload, "type", overwrite_type);
    char *json_body = cJSON_PrintUnformatted(payload);
    cJSON_Delete(payload);
    if (!json_body) {
        return DC_ERR_JSON;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/permissions/") + strlen(overwrite_id) + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        free(json_body);
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/permissions/%s", channel_id, overwrite_id);

    dc_status status = dc_rest_request(rest, "PUT", route, json_body, out);
    free(route);
    free(json_body);
    return status;
}

dc_status dc_rest_create_global_command(
    dc_rest *rest,
    const char *application_id,
    const dc_application_command *command,
    dc_http_response *out
) {
    if (!rest || !application_id || !command || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    char *json_body = NULL;
    dc_status status = dc_application_command_to_json(command, &json_body);
    if (status != DC_OK) {
        return status;
    }

    size_t route_len = strlen("/applications/") + strlen(application_id) + strlen("/commands") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        free(json_body);
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/applications/%s/commands", application_id);

    status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    free(json_body);
    return status;
}

dc_status dc_rest_bulk_overwrite_global_commands(
    dc_rest *rest,
    const char *application_id,
    const char *commands_json_array,
    dc_http_response *out
) {
    if (!rest || !application_id || !commands_json_array || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/applications/") + strlen(application_id) + strlen("/commands") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/applications/%s/commands", application_id);

    dc_status status = dc_rest_request(rest, "PUT", route, commands_json_array, out);
    free(route);
    return status;
}

dc_status dc_rest_create_guild_command(
    dc_rest *rest,
    const char *application_id,
    const char *guild_id,
    const dc_application_command *command,
    dc_http_response *out
) {
    if (!rest || !application_id || !guild_id || !command || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    char *json_body = NULL;
    dc_status status = dc_application_command_to_json(command, &json_body);
    if (status != DC_OK) {
        return status;
    }

    size_t route_len = strlen("/applications/") + strlen(application_id) + strlen("/guilds/") +
        strlen(guild_id) + strlen("/commands") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        free(json_body);
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/applications/%s/guilds/%s/commands", application_id, guild_id);

    status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    free(json_body);
    return status;
}

dc_status dc_rest_bulk_overwrite_guild_commands(
    dc_rest *rest,
    const char *application_id,
    const char *guild_id,
    const char *commands_json_array,
    dc_http_response *out
) {
    if (!rest || !application_id || !guild_id || !commands_json_array || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/applications/") + strlen(application_id) + strlen("/guilds/") +
        strlen(guild_id) + strlen("/commands") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/applications/%s/guilds/%s/commands", application_id, guild_id);

    dc_status status = dc_rest_request(rest, "PUT", route, commands_json_array, out);
    free(route);
    return status;
}

dc_status dc_rest_create_interaction_response(
    dc_rest *rest,
    const char *interaction_id,
    const char *interaction_token,
    const dc_interaction_response *response,
    dc_http_response *out
) {
    if (!rest || !interaction_id || !interaction_token || !response || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    char *json_body = NULL;
    dc_status status = dc_interaction_response_to_json(response, &json_body);
    if (status != DC_OK) {
        return status;
    }

    size_t route_len = strlen("/interactions/") + strlen(interaction_id) + strlen("/") +
        strlen(interaction_token) + strlen("/callback") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        free(json_body);
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/interactions/%s/%s/callback", interaction_id, interaction_token);

    status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    free(json_body);
    return status;
}

dc_status dc_rest_edit_original_interaction_response(
    dc_rest *rest,
    const char *application_id,
    const char *interaction_token,
    const char *json_body,
    dc_http_response *out
) {
    if (!rest || !application_id || !interaction_token || !json_body || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/webhooks/") + strlen(application_id) + strlen("/") +
        strlen(interaction_token) + strlen("/messages/@original") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/webhooks/%s/%s/messages/@original", application_id, interaction_token);

    dc_status status = dc_rest_request(rest, "PATCH", route, json_body, out);
    free(route);
    return status;
}

dc_status dc_rest_delete_original_interaction_response(
    dc_rest *rest,
    const char *application_id,
    const char *interaction_token,
    dc_http_response *out
) {
    if (!rest || !application_id || !interaction_token || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/webhooks/") + strlen(application_id) + strlen("/") +
        strlen(interaction_token) + strlen("/messages/@original") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/webhooks/%s/%s/messages/@original", application_id, interaction_token);

    dc_status status = dc_rest_request(rest, "DELETE", route, NULL, out);
    free(route);
    return status;
}

dc_status dc_rest_start_thread_from_message(
    dc_rest *rest,
    const char *channel_id,
    const char *message_id,
    const char *json_body,
    dc_http_response *out
) {
    if (!rest || !channel_id || !message_id || !json_body || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/messages/") +
        strlen(message_id) + strlen("/threads") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/messages/%s/threads", channel_id, message_id);

    dc_status status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    return status;
}

dc_status dc_rest_start_thread_without_message(
    dc_rest *rest,
    const char *channel_id,
    const char *json_body,
    dc_http_response *out
) {
    if (!rest || !channel_id || !json_body || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/threads") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/threads", channel_id);

    dc_status status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    return status;
}

dc_status dc_rest_create_forum_post(
    dc_rest *rest,
    const char *channel_id,
    const char *json_body,
    dc_http_response *out
) {
    if (!rest || !channel_id || !json_body || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    size_t route_len = strlen("/channels/") + strlen(channel_id) + strlen("/threads") + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/channels/%s/threads", channel_id);

    dc_status status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    return status;
}

dc_status dc_rest_execute_webhook(
    dc_rest *rest,
    const char *webhook_id,
    const char *webhook_token,
    const char *json_body,
    int wait,
    dc_http_response *out
) {
    if (!rest || !webhook_id || !webhook_token || !json_body || !out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    const char *wait_value = wait ? "true" : "false";
    size_t route_len = strlen("/webhooks/") + strlen(webhook_id) + strlen("/") +
        strlen(webhook_token) + strlen("?wait=") + strlen(wait_value) + 1;
    char *route = (char *)malloc(route_len);
    if (!route) {
        return DC_ERR_ALLOC;
    }
    snprintf(route, route_len, "/webhooks/%s/%s?wait=%s", webhook_id, webhook_token, wait_value);

    dc_status status = dc_rest_request(rest, "POST", route, json_body, out);
    free(route);
    return status;
}
