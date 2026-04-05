#ifndef DISCORDC_MODELS_H
#define DISCORDC_MODELS_H

#include <stddef.h>

#include <cjson/cJSON.h>

#include "discordc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dc_embed_field {
    const char *name;
    const char *value;
    int is_inline;
} dc_embed_field;

typedef struct dc_embed_footer {
    const char *text;
    const char *icon_url;
} dc_embed_footer;

typedef struct dc_embed_author {
    const char *name;
    const char *url;
    const char *icon_url;
} dc_embed_author;

typedef struct dc_embed_media {
    const char *url;
} dc_embed_media;

typedef struct dc_embed {
    const char *title;
    const char *description;
    const char *url;
    const char *timestamp;
    int color;
    int has_color;

    dc_embed_footer footer;
    dc_embed_author author;
    dc_embed_media image;
    dc_embed_media thumbnail;

    dc_embed_field *fields;
    size_t field_count;
} dc_embed;

typedef struct dc_allowed_mentions {
    int replied_user;
} dc_allowed_mentions;

typedef struct dc_message_create {
    const char *content;
    int tts;
    const char *message_reference_id;

    dc_embed *embeds;
    size_t embed_count;

    dc_allowed_mentions allowed_mentions;
    int has_allowed_mentions;

    cJSON *components;
} dc_message_create;

typedef struct dc_application_command {
    const char *name;
    const char *description;
    int type;
    cJSON *options;
    cJSON *default_member_permissions;
    int dm_permission;
    int has_dm_permission;
} dc_application_command;

typedef struct dc_button_component {
    int style;
    const char *label;
    const char *custom_id;
    const char *url;
    int disabled;
} dc_button_component;

typedef struct dc_select_option {
    const char *label;
    const char *value;
    const char *description;
    const char *emoji_name;
    const char *emoji_id;
    int is_default;
} dc_select_option;

typedef struct dc_string_select_component {
    const char *custom_id;
    const char *placeholder;
    int min_values;
    int has_min_values;
    int max_values;
    int has_max_values;
    int disabled;
    dc_select_option *options;
    size_t option_count;
} dc_string_select_component;

typedef struct dc_text_input_component {
    const char *custom_id;
    int style;
    const char *label;
    const char *value;
    const char *placeholder;
    int required;
    int has_required;
    int min_length;
    int has_min_length;
    int max_length;
    int has_max_length;
} dc_text_input_component;

typedef enum dc_component_kind {
    DC_COMPONENT_BUTTON = 2,
    DC_COMPONENT_STRING_SELECT = 3,
    DC_COMPONENT_TEXT_INPUT = 4
} dc_component_kind;

typedef struct dc_component {
    dc_component_kind kind;
    union {
        dc_button_component button;
        dc_string_select_component string_select;
        dc_text_input_component text_input;
    } as;
} dc_component;

typedef struct dc_action_row {
    dc_component *components;
    size_t component_count;
} dc_action_row;

typedef struct dc_interaction_callback_data {
    const char *content;
    dc_embed *embeds;
    size_t embed_count;
    dc_action_row *components;
    size_t component_count;
    int flags;
    int has_flags;
} dc_interaction_callback_data;

typedef struct dc_interaction_response {
    int type;
    dc_interaction_callback_data data;
    const char *modal_custom_id;
    const char *modal_title;
    dc_action_row *modal_rows;
    size_t modal_row_count;
} dc_interaction_response;

void dc_message_create_init(dc_message_create *message);
dc_status dc_message_create_to_json(const dc_message_create *message, char **json_out);
dc_status dc_application_command_to_json(const dc_application_command *command, char **json_out);
dc_status dc_interaction_response_to_json(const dc_interaction_response *response, char **json_out);

cJSON *dc_embed_to_json(const dc_embed *embed);
cJSON *dc_component_to_json(const dc_component *component);
cJSON *dc_action_row_to_json(const dc_action_row *row);
dc_status dc_components_to_json_array(const dc_action_row *rows, size_t row_count, cJSON **array_out);

#ifdef __cplusplus
}
#endif

#endif
