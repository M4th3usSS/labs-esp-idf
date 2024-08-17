// component header
#include "request_http.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

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

#define M2MQTT_CA_CRT_RSA "-----BEGIN CERTIFICATE-----\n"\
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n"\
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"\
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n"\
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n"\
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"\
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n"\
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n"\
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n"\
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n"\
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n"\
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n"\
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n"\
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n"\
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n"\
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n"\
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n"\
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n"\
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n"\
"-----END CERTIFICATE-----\n"

const char mbedtls_m2mqtt_srv_crt[] = M2MQTT_CA_CRT_RSA;
const size_t mbedtls_m2mqtt_srv_crt_len = sizeof(mbedtls_m2mqtt_srv_crt);


// mbed TLS debug function
void mbedtls_debug(void *ctx, int level, const char *file, int line, const char *st) {
	 printf("mbedtls: %s\n", st);
}

int http_send(http_config_t *config, char * response) {
    char buf[HTTP_JSON_RETURN_SIZE];
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
	memset( buf, 0, sizeof( buf ) );
	int len = sizeof(buf) - 1;
	do 
	{
		
		_ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf+resp_offset, len-resp_offset);

		if( _ret == MBEDTLS_ERR_SSL_WANT_READ || _ret == MBEDTLS_ERR_SSL_WANT_WRITE )
		{
			vTaskDelay( 100/portTICK_PERIOD_MS );
			continue;
		}
		
		resp_offset += _ret; 
		
		if( _ret < 0 )
		{
			#ifdef DEBUG_ON
				printf("http: FAILED mbedtls_ssl_read returned %d\r\n", _ret );
			#endif
			break;
		}
				
	    if( _ret == 0) 
		{
			#ifdef DEBUG_ON
				printf(" Size2: %d\n",  _ret);
			#endif	  
			
			if(return_code == MBEDTLS_OK)
			  strcpy(response, buf);
			  
			break;
		}
		if(_ret > 0)
		{
			#ifdef DEBUG_ON
				printf("response %s Size: %d\n", buf, _ret);
			#endif	
			
			if(strstr(buf, "200") != NULL) {
				return_code = MBEDTLS_OK;		
				//break;
				
			} else if( strstr(buf, "You sent an invalid key." ) != NULL) {
				return_code = MBEDTLS_ERR_KEY;	
				break;
			}
		}
		
	}while(1); 

	mbedtls_net_free(&server_fd);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_ssl_close_notify(&ssl);
	mbedtls_x509_crt_free( &cacert );
	
    return return_code;
}


