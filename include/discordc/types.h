#ifndef DISCORDC_TYPES_H
#define DISCORDC_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dc_status {
    DC_OK = 0,
    DC_ERR_INVALID_ARGUMENT = -1,
    DC_ERR_ALLOC = -2,
    DC_ERR_HTTP = -3,
    DC_ERR_JSON = -4,
    DC_ERR_GATEWAY = -5,
    DC_ERR_INTERNAL = -6
} dc_status;

typedef struct dc_http_response {
    long status_code;
    char *body;
    size_t body_len;
} dc_http_response;

#ifdef __cplusplus
}
#endif

#endif
