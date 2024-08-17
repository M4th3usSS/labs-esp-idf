// component header
#include "request_http.h"

// mbed TLS headers
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

// standard libraries
#include <string.h>
#include <stdlib.h>

#define M2MQTT_CA_CRT_RSA \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"\
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"\
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"\
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"\
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"\
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"\
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"\
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"\
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"\
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"\
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"\
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"\
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"\
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"\
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"\
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"\
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"\
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"\
"-----END CERTIFICATE-----\n"

const char mbedtls_m2mqtt_srv_crt[] = M2MQTT_CA_CRT_RSA;
const size_t mbedtls_m2mqtt_srv_crt_len = sizeof(mbedtls_m2mqtt_srv_crt);


// mbed TLS debug function
void mbedtls_debug(void *ctx, int level, const char *file, int line, const char *st) {
	 printf("mbedtls: %s\n", st);
}

int http_send(http_config_t *config, char * response, int response_size) 
{
    #ifdef DEBUG_ON
		printf("Config: IP= %s; PORT: %d; URL: %s", (char*)config->ip, config->port, config->url);
	#endif
	
	int _ret;

	// mbed TLS variables
	mbedtls_ssl_config conf;
	mbedtls_net_context server_fd;
	mbedtls_ssl_context ssl;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
	
	// initialize mbed TLS
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);	
	mbedtls_x509_crt_init( &cacert );

	#ifdef DEBUG_ON 
		printf("mbedtls initialized\n");
	#endif
	
	// seed the random number generator
	if((_ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0)
		return MBEDTLS_ERR_DRBG_SEED;	
	#ifdef DEBUG_ON 
		printf("mbedtls_ctr_drbg_seed\n");
	#endif
	
	//Necessário para converter o parâmetro PORTA (Int) para String
	char port[10]; 
	sprintf(port,"%d",config->port);
	
	if((_ret = mbedtls_net_connect( &server_fd, (char*)config->ip, port, MBEDTLS_NET_PROTO_TCP)) != 0)
		return MBEDTLS_ERR_CONNECTION;
	#ifdef DEBUG_ON 
		printf("mbedtls_net_connect\n");
	#endif
	
	// configure the SSL/TLS layer (client mode with TLS)
	if((_ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
		return MBEDTLS_ERR_SSL_DEFAULTS;	
	#ifdef DEBUG_ON 
		printf("mbedtls_ssl_config_defaults\n");
	#endif
	
	/*
	// do not verify the CA certificate
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
	#ifdef DEBUG_ON 
		printf("mbedtls_ssl_conf_authmode\n");
	#endif
	*/
		_ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)mbedtls_m2mqtt_srv_crt, mbedtls_m2mqtt_srv_crt_len );
	if(_ret != 0) {
		printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -_ret );
	}
	mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	
	//Leitura do certificado TLS (x509_crt)
    char buf3[1024];
	mbedtls_x509_crt_info(buf3, sizeof(buf3) - 1, "", &cacert);
	#ifdef DEBUG_ON
		printf("INFO: %s", buf3);
	#endif
	
	
	// configure the random engine and the debug function
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, mbedtls_debug, NULL);
	#ifdef DEBUG_ON 
		printf("mbedtls_ssl_conf_dbg\n");
	#endif
	
	mbedtls_ssl_setup(&ssl, &conf);
	mbedtls_ssl_handshake(&ssl);
	
	// configure the input and output functions
	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
	#ifdef DEBUG_ON 
		printf("mbedtls_ssl_set_bio\n");
	#endif
	
	#ifdef DEBUG_ON
		printf("sending %s\n", config->url);
	#endif
	while((_ret = mbedtls_ssl_write(&ssl, (const unsigned char *)config->url, strlen(config->url))) <= 0) 
		{
            if(_ret != MBEDTLS_ERR_SSL_WANT_READ && _ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
				#ifdef DEBUG_ON
					printf("mbedtls_ssl_write returned -0x%x\n", -_ret);
				#endif				  
				mbedtls_net_free(&server_fd);
				mbedtls_ssl_free(&ssl);
				mbedtls_ssl_config_free(&conf);
				mbedtls_entropy_free(&entropy);
				mbedtls_ctr_drbg_free(&ctr_drbg);	
				mbedtls_x509_crt_free( &cacert );
				
                return MBEDTLS_ERR_SEND;
            }
        }
	#ifdef DEBUG_ON
		printf("mbedtls_ssl_write\n");
	#endif

	int return_code = MBEDTLS_ERR_UNKNOWN;
	int resp_offset = 0;
	memset(response, 0, sizeof(response_size));

	do 
	{
		/**
		 * Os dados recebidos via TCP podem chegar fragmentados; Portanto, é 
		 * preciso unir as pequenas partes recebidas no buffer de recepção;
		 */
		_ret = mbedtls_ssl_read(&ssl, (unsigned char *)response+resp_offset, response_size-resp_offset-1);
						
		if(_ret > 0)
		{
			resp_offset += _ret; 
			response[resp_offset] = '\0';
			
			/**
			 * Verifica se foi retornado o código 200 do servidor web;
			 * caso sim, isso significa que a requisição foi processada com 
			 * sucesso e o servidor web retornou o json com sucesso.
			 */
			if( strstr( response, HTTP_CODE_200 ) != NULL ) 
			{
				return_code = MBEDTLS_OK;	
				break;		
			} else 

			/**
			 * Mensagem de erro notificada no buffer pela mbedtls;
			 */
			if( strstr( response, "You sent an invalid key." ) != NULL) 
			{
				return_code = MBEDTLS_ERR_KEY;	
				break;
			}
		}

	/**
	 * Permanece em loop enquanto os bytes são recebidos;
	 */
	} while(_ret > 0 ||(_ret == MBEDTLS_ERR_SSL_WANT_READ || MBEDTLS_ERR_SSL_WANT_WRITE)); 

	mbedtls_net_free(&server_fd);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_ssl_close_notify(&ssl);
	mbedtls_x509_crt_free( &cacert );
	
    return return_code;
}


