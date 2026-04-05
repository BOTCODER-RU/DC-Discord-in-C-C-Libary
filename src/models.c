#include "discordc/models.h"

#include <stdlib.h>
#include <string.h>

void dc_message_create_init(dc_message_create *message) {
    if (!message) {
        return;
    }
    memset(message, 0, sizeof(*message));
}

static void dc_add_nonempty_string(cJSON *obj, const char *name, const char *value) {
    if (!obj || !name || !value || value[0] == '\0') {
        return;
    }
    cJSON_AddStringToObject(obj, name, value);
}

cJSON *dc_embed_to_json(const dc_embed *embed) {
    if (!embed) {
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    dc_add_nonempty_string(obj, "title", embed->title);
    dc_add_nonempty_string(obj, "description", embed->description);
    dc_add_nonempty_string(obj, "url", embed->url);
    dc_add_nonempty_string(obj, "timestamp", embed->timestamp);
    if (embed->has_color) {
        cJSON_AddNumberToObject(obj, "color", embed->color);
    }

    if (embed->footer.text && embed->footer.text[0] != '\0') {
        cJSON *footer = cJSON_CreateObject();
        if (!footer) {
            cJSON_Delete(obj);
            return NULL;
        }
        dc_add_nonempty_string(footer, "text", embed->footer.text);
        dc_add_nonempty_string(footer, "icon_url", embed->footer.icon_url);
        cJSON_AddItemToObject(obj, "footer", footer);
    }

    if (embed->author.name && embed->author.name[0] != '\0') {
        cJSON *author = cJSON_CreateObject();
        if (!author) {
            cJSON_Delete(obj);
            return NULL;
        }
        dc_add_nonempty_string(author, "name", embed->author.name);
        dc_add_nonempty_string(author, "url", embed->author.url);
        dc_add_nonempty_string(author, "icon_url", embed->author.icon_url);
        cJSON_AddItemToObject(obj, "author", author);
    }

    if (embed->image.url && embed->image.url[0] != '\0') {
        cJSON *image = cJSON_CreateObject();
        if (!image) {
            cJSON_Delete(obj);
            return NULL;
        }
        dc_add_nonempty_string(image, "url", embed->image.url);
        cJSON_AddItemToObject(obj, "image", image);
    }

    if (embed->thumbnail.url && embed->thumbnail.url[0] != '\0') {
        cJSON *thumbnail = cJSON_CreateObject();
        if (!thumbnail) {
            cJSON_Delete(obj);
            return NULL;
        }
        dc_add_nonempty_string(thumbnail, "url", embed->thumbnail.url);
        cJSON_AddItemToObject(obj, "thumbnail", thumbnail);
    }

    if (embed->fields && embed->field_count > 0) {
        cJSON *fields = cJSON_CreateArray();
        if (!fields) {
            cJSON_Delete(obj);
            return NULL;
        }

        for (size_t i = 0; i < embed->field_count; i++) {
            const dc_embed_field *field = &embed->fields[i];
            if (!field->name || !field->value) {
                continue;
            }

            cJSON *field_obj = cJSON_CreateObject();
            if (!field_obj) {
                cJSON_Delete(fields);
                cJSON_Delete(obj);
                return NULL;
            }
            cJSON_AddStringToObject(field_obj, "name", field->name);
            cJSON_AddStringToObject(field_obj, "value", field->value);
            cJSON_AddBoolToObject(field_obj, "inline", field->is_inline ? 1 : 0);
            cJSON_AddItemToArray(fields, field_obj);
        }

        cJSON_AddItemToObject(obj, "fields", fields);
    }

    return obj;
}

static cJSON *dc_select_option_to_json(const dc_select_option *option) {
    if (!option || !option->label || !option->value) {
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    cJSON_AddStringToObject(obj, "label", option->label);
    cJSON_AddStringToObject(obj, "value", option->value);
    dc_add_nonempty_string(obj, "description", option->description);

    if ((option->emoji_name && option->emoji_name[0] != '\0') || (option->emoji_id && option->emoji_id[0] != '\0')) {
        cJSON *emoji = cJSON_CreateObject();
        if (!emoji) {
            cJSON_Delete(obj);
            return NULL;
        }
        dc_add_nonempty_string(emoji, "name", option->emoji_name);
        dc_add_nonempty_string(emoji, "id", option->emoji_id);
        cJSON_AddItemToObject(obj, "emoji", emoji);
    }

    if (option->is_default) {
        cJSON_AddBoolToObject(obj, "default", 1);
    }

    return obj;
}

cJSON *dc_component_to_json(const dc_component *component) {
    if (!component) {
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    switch (component->kind) {
        case DC_COMPONENT_BUTTON: {
            const dc_button_component *button = &component->as.button;
            cJSON_AddNumberToObject(obj, "type", DC_COMPONENT_BUTTON);
            cJSON_AddNumberToObject(obj, "style", button->style);
            dc_add_nonempty_string(obj, "label", button->label);
            dc_add_nonempty_string(obj, "custom_id", button->custom_id);
            dc_add_nonempty_string(obj, "url", button->url);
            if (button->disabled) {
                cJSON_AddBoolToObject(obj, "disabled", 1);
            }
            break;
        }

        case DC_COMPONENT_STRING_SELECT: {
            const dc_string_select_component *select = &component->as.string_select;
            cJSON_AddNumberToObject(obj, "type", DC_COMPONENT_STRING_SELECT);
            dc_add_nonempty_string(obj, "custom_id", select->custom_id);
            dc_add_nonempty_string(obj, "placeholder", select->placeholder);
            if (select->has_min_values) {
                cJSON_AddNumberToObject(obj, "min_values", select->min_values);
            }
            if (select->has_max_values) {
                cJSON_AddNumberToObject(obj, "max_values", select->max_values);
            }
            if (select->disabled) {
                cJSON_AddBoolToObject(obj, "disabled", 1);
            }

            cJSON *options = cJSON_CreateArray();
            if (!options) {
                cJSON_Delete(obj);
                return NULL;
            }

            for (size_t i = 0; i < select->option_count; i++) {
                cJSON *option_json = dc_select_option_to_json(&select->options[i]);
                if (!option_json) {
                    cJSON_Delete(options);
                    cJSON_Delete(obj);
                    return NULL;
                }
                cJSON_AddItemToArray(options, option_json);
            }

            cJSON_AddItemToObject(obj, "options", options);
            break;
        }

        case DC_COMPONENT_TEXT_INPUT: {
            const dc_text_input_component *input = &component->as.text_input;
            cJSON_AddNumberToObject(obj, "type", DC_COMPONENT_TEXT_INPUT);
            cJSON_AddNumberToObject(obj, "style", input->style);
            dc_add_nonempty_string(obj, "custom_id", input->custom_id);
            dc_add_nonempty_string(obj, "label", input->label);
            dc_add_nonempty_string(obj, "value", input->value);
            dc_add_nonempty_string(obj, "placeholder", input->placeholder);
            if (input->has_required) {
                cJSON_AddBoolToObject(obj, "required", input->required ? 1 : 0);
            }
            if (input->has_min_length) {
                cJSON_AddNumberToObject(obj, "min_length", input->min_length);
            }
            if (input->has_max_length) {
                cJSON_AddNumberToObject(obj, "max_length", input->max_length);
            }
            break;
        }

        default:
            cJSON_Delete(obj);
            return NULL;
    }

    return obj;
}

cJSON *dc_action_row_to_json(const dc_action_row *row) {
    if (!row) {
        return NULL;
    }

    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        return NULL;
    }

    cJSON_AddNumberToObject(obj, "type", 1);

    cJSON *components = cJSON_CreateArray();
    if (!components) {
        cJSON_Delete(obj);
        return NULL;
    }

    for (size_t i = 0; i < row->component_count; i++) {
        cJSON *item = dc_component_to_json(&row->components[i]);
        if (!item) {
            cJSON_Delete(components);
            cJSON_Delete(obj);
            return NULL;
        }
        cJSON_AddItemToArray(components, item);
    }

    cJSON_AddItemToObject(obj, "components", components);
    return obj;
}

dc_status dc_components_to_json_array(const dc_action_row *rows, size_t row_count, cJSON **array_out) {
    if (!array_out) {
        return DC_ERR_INVALID_ARGUMENT;
    }
    *array_out = NULL;

    if (!rows || row_count == 0) {
        return DC_OK;
    }

    cJSON *components = cJSON_CreateArray();
    if (!components) {
        return DC_ERR_ALLOC;
    }

    for (size_t i = 0; i < row_count; i++) {
        cJSON *row = dc_action_row_to_json(&rows[i]);
        if (!row) {
            cJSON_Delete(components);
            return DC_ERR_JSON;
        }
        cJSON_AddItemToArray(components, row);
    }

    *array_out = components;
    return DC_OK;
}

static dc_status dc_append_components(cJSON *root, const dc_action_row *rows, size_t row_count) {
    cJSON *components = NULL;
    dc_status status = dc_components_to_json_array(rows, row_count, &components);
    if (status != DC_OK) {
        return status;
    }
    if (components) {
        cJSON_AddItemToObject(root, "components", components);
    }
    return DC_OK;
}

dc_status dc_message_create_to_json(const dc_message_create *message, char **json_out) {
    if (!message || !json_out) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    *json_out = NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return DC_ERR_ALLOC;
    }

    if (message->content) {
        cJSON_AddStringToObject(root, "content", message->content);
    }
    if (message->tts) {
        cJSON_AddBoolToObject(root, "tts", 1);
    }

    if (message->message_reference_id && message->message_reference_id[0] != '\0') {
        cJSON *reference = cJSON_CreateObject();
        if (!reference) {
            cJSON_Delete(root);
            return DC_ERR_ALLOC;
        }
        cJSON_AddStringToObject(reference, "message_id", message->message_reference_id);
        cJSON_AddItemToObject(root, "message_reference", reference);
    }

    if (message->embeds && message->embed_count > 0) {
        cJSON *embeds = cJSON_CreateArray();
        if (!embeds) {
            cJSON_Delete(root);
            return DC_ERR_ALLOC;
        }

        for (size_t i = 0; i < message->embed_count; i++) {
            cJSON *embed = dc_embed_to_json(&message->embeds[i]);
            if (!embed) {
                cJSON_Delete(embeds);
                cJSON_Delete(root);
                return DC_ERR_JSON;
            }
            cJSON_AddItemToArray(embeds, embed);
        }

        cJSON_AddItemToObject(root, "embeds", embeds);
    }

    if (message->has_allowed_mentions) {
        cJSON *allowed_mentions = cJSON_CreateObject();
        if (!allowed_mentions) {
            cJSON_Delete(root);
            return DC_ERR_ALLOC;
        }
        cJSON_AddBoolToObject(allowed_mentions, "replied_user", message->allowed_mentions.replied_user ? 1 : 0);
        cJSON_AddItemToObject(root, "allowed_mentions", allowed_mentions);
    }

    if (message->components) {
        cJSON_AddItemToObject(root, "components", cJSON_Duplicate(message->components, 1));
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        return DC_ERR_JSON;
    }

    *json_out = json;
    return DC_OK;
}

dc_status dc_application_command_to_json(const dc_application_command *command, char **json_out) {
    if (!command || !json_out || !command->name || !command->description) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    *json_out = NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return DC_ERR_ALLOC;
    }

    cJSON_AddStringToObject(root, "name", command->name);
    cJSON_AddStringToObject(root, "description", command->description);
    if (command->type > 0) {
        cJSON_AddNumberToObject(root, "type", command->type);
    }
    if (command->options) {
        cJSON_AddItemToObject(root, "options", cJSON_Duplicate(command->options, 1));
    }
    if (command->default_member_permissions) {
        cJSON_AddItemToObject(root, "default_member_permissions", cJSON_Duplicate(command->default_member_permissions, 1));
    }
    if (command->has_dm_permission) {
        cJSON_AddBoolToObject(root, "dm_permission", command->dm_permission ? 1 : 0);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        return DC_ERR_JSON;
    }

    *json_out = json;
    return DC_OK;
}

dc_status dc_interaction_response_to_json(const dc_interaction_response *response, char **json_out) {
    if (!response || !json_out || response->type <= 0) {
        return DC_ERR_INVALID_ARGUMENT;
    }

    *json_out = NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return DC_ERR_ALLOC;
    }

    cJSON_AddNumberToObject(root, "type", response->type);

    cJSON *data = cJSON_CreateObject();
    if (!data) {
        cJSON_Delete(root);
        return DC_ERR_ALLOC;
    }

    if (response->data.content) {
        cJSON_AddStringToObject(data, "content", response->data.content);
    }

    if (response->data.embeds && response->data.embed_count > 0) {
        cJSON *embeds = cJSON_CreateArray();
        if (!embeds) {
            cJSON_Delete(data);
            cJSON_Delete(root);
            return DC_ERR_ALLOC;
        }

        for (size_t i = 0; i < response->data.embed_count; i++) {
            cJSON *embed = dc_embed_to_json(&response->data.embeds[i]);
            if (!embed) {
                cJSON_Delete(embeds);
                cJSON_Delete(data);
                cJSON_Delete(root);
                return DC_ERR_JSON;
            }
            cJSON_AddItemToArray(embeds, embed);
        }

        cJSON_AddItemToObject(data, "embeds", embeds);
    }

    if (response->data.has_flags) {
        cJSON_AddNumberToObject(data, "flags", response->data.flags);
    }

    if (response->data.components && response->data.component_count > 0) {
        dc_status components_status = dc_append_components(data, response->data.components, response->data.component_count);
        if (components_status != DC_OK) {
            cJSON_Delete(data);
            cJSON_Delete(root);
            return components_status;
        }
    }

    if (response->modal_custom_id && response->modal_title) {
        cJSON_AddStringToObject(data, "custom_id", response->modal_custom_id);
        cJSON_AddStringToObject(data, "title", response->modal_title);

        if (response->modal_rows && response->modal_row_count > 0) {
            dc_status components_status = dc_append_components(data, response->modal_rows, response->modal_row_count);
            if (components_status != DC_OK) {
                cJSON_Delete(data);
                cJSON_Delete(root);
                return components_status;
            }
        }
    }

    if (cJSON_GetArraySize(data) > 0) {
        cJSON_AddItemToObject(root, "data", data);
    } else {
        cJSON_Delete(data);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        return DC_ERR_JSON;
    }

    *json_out = json;
    return DC_OK;
}
