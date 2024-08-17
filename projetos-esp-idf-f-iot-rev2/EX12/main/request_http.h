#ifndef _REQUEST_HTTP_H_
#define _REQUEST_HTTP_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
 
#define MBEDTLS_OK		 			0
#define MBEDTLS_ERR_DRBG_SEED		1
#define MBEDTLS_ERR_CONNECTION		2
#define MBEDTLS_ERR_SSL_DEFAULTS	3
#define MBEDTLS_ERR_SEND			4
#define MBEDTLS_ERR_KEY				5
#define MBEDTLS_ERR_UNKNOWN			10

#define DEBUG_ON 
#define HTTP_BUFFER_SIZE 1024 //bytes
#define HTTP_CODE_200	"200"

struct chttp
{
 int port;
 const char * ip;
 char * url; 
};

typedef enum mhttp http_metodo_t; 
typedef struct chttp http_config_t;

int http_send(http_config_t *config, char * response, int response_size);
void mbedtls_debug(void *ctx, int level, const char *file, int line, const char *st);

#endif 