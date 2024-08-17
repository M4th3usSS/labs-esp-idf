/**
 * Lib C
 */
#include <stdio.h>
#include <string.h>
/**
 * FreeRTOS
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
/**
 * WiFi
 */
#include "esp_wifi.h"
/**
 * Callback do WiFi
 */
#include "esp_event.h"
/**
 * Logs
 */
#include "esp_system.h"
#include "esp_log.h"
#include <errno.h>

/**
 * NVS
 */
#include "nvs_flash.h"

/**
 *  LWIP;
 */
#include "esp_netif.h"
#include "netdb.h"
#include "lwip/dns.h"

/**
 * LWIP
 */
#include "lwip/api.h"
#include "lwip/err.h"
#include <sys/socket.h>
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"

/**
 *  mDNS lib;
 */
#include "mdns.h"

/**
 * GPIOs;
 */
#include "driver/gpio.h"

/**
 * Lib socket server;
 */
#include "config.h" 


/**
 * Variáveis
 */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "CLIENT";

/**
 * Protótipos
 */
static void socket_client( char * buffer );
static struct timeval delay_us( unsigned long usec );
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
static void wifi_init_sta( void );

/**
 * Delay_us;
 */
static struct timeval delay_us( unsigned long usec )
{
    struct timeval tv;
    tv.tv_sec  = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    return tv;
}

/**
 * Função responsável pelo envio da mensagem via socket;
 */
static void socket_client( char * buffer ) 
{
    int rc; 
	
	if( DEBUG )
		ESP_LOGI(TAG, "socket_client...\n");

	/**
	 * Cria o socket TCP;
	 */
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 

	if( DEBUG )
		ESP_LOGI( TAG, "socket: rc: %d", sock );

	/**
	 * Configura/Vincula o endereço IP e Port do Servidor (Bind);
	 */
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(SERVER_PORT);

	/**
	 * Tenta estabelecer a conexão socket TCP entre ESP32 e Servidor;
	 */
	rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));

	/**
	 * A função recv da LWIP é bloqueante. Portanto, convém definir o valor de TIMEOUT;
	 * Neste caso, configura TIMEOUT de 300 ms.
	 */
	struct timeval tv = delay_us(300000); //300ms
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
	
	/**
	 * error? (-1)
	 */
	if( DEBUG )
		ESP_LOGI(TAG, "connect rc: %d", rc);

	/**
	 * no error;
	 */
	if( rc == 0 )
	{
		/**
		 * Converte o valor de count para string e envia a string 
		 * ao servidor via socket tcp;
		 */
		rc = send(sock, buffer, strlen(buffer), 0 );
		
		/**
		 * error durante o envio? (-1)
		 */
		if( DEBUG )
			ESP_LOGI(TAG, "send: rc: %d", rc);	

		/**
		 * error durante o envio? (-1)
		 */
		int index = 0, ret = 0;
		char buffer[64];	
		while(1)
		{
			ret = recv(sock, buffer + index,sizeof(buffer) - index, 0);
			/// Edit: recv does not return EAGAIN else, it return -1 on error.
			/// and in case of timeout, errno is set to EAGAIN or EWOULDBLOCK
			if (ret <= 0)
			{
				if(index < (sizeof(buffer) - 1)) {
					ESP_LOGI(TAG, "%.*s", index, buffer);
				}				
				break;
			}
			
			index += ret;
		}
	}

	rc = close( sock );
	/**
	 * error no fechamento do socket? (-1)
	 */
	if( DEBUG )
		ESP_LOGI(TAG, "close: rc: %d", rc);
	
}


/**
 * Task responsável pelo controle do botão;
 */
void task_button( void *pvParameter )
{
    char str[20];
    int aux = 0;
  	int counter = 0;
  		
      /**
       * Configuração da GPIO BUTTON;
       */
  	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
  	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 

    for(;;) 
	{

		/**
		 * Enquanto o ESP32 estiver conectado a rede WiFi será realizada a leitura
		 * de button;
		 */
	    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
		
		/**
		 * Botaõ pressionado?
		 */
		if( gpio_get_level( BUTTON ) == 0 && aux == 0 )
		{ 
		  	/**
		  	 * Aguarda 80 ms devido o bounce;
		  	 */
			vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
			/**
			 * Verifica se realmente button está pressionado;
			 */
			if( gpio_get_level( BUTTON ) == 0 && aux == 0 ) 
			{		

				if( DEBUG )
					ESP_LOGI( TAG, "Button %d Pressionado .\r\n", BUTTON );

				sprintf( str, "%d", counter++ ); 
				
				/**
				 * Chama função responsável pelo envio da mensagem para o servidor;
				 */
				socket_client(str); 
				
				aux = 1; 
			}
		}

		/**
		 * Botão solto?
		 */
		if( gpio_get_level( BUTTON ) == 1 && aux == 1 )
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			
  			if(gpio_get_level( BUTTON ) == 1 && aux == 1 )
  			{
  				aux = 0;
  			}
		}	

  		/**
  		 * Contribui para as demais tasks de menor prioridade sejam escalonadas
  		 * pelo scheduler;
  		 */
  		vTaskDelay( 100/portTICK_PERIOD_MS );	
    }
}

/**
 * Função de callback chamada quando é alterado o estado da
 * conexão WiFi;
 */
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
	{
        esp_wifi_connect();
    } else 
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
	{
        esp_wifi_connect();
        xEventGroupClearBits( wifi_event_group, WIFI_CONNECTED_BIT );
    }else 
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
	{
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));   
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta( void )
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *netif_sta = esp_netif_create_default_wifi_sta();
    /**
     * Deseja trabalhar com o endereço IP fixo na rede? 
     */		
#if IP_FIXO
    esp_netif_dhcpc_stop(netif_sta);
    esp_netif_ip_info_t ip_info;

	IP4_ADDR(&ip_info.ip,      192,168,0,223 );
	IP4_ADDR(&ip_info.gw,      192,168,0,1   );
	IP4_ADDR(&ip_info.netmask, 255,255,255,0 );
                                    
    esp_netif_set_ip_info(netif_sta, &ip_info);

	/**
		I referred from here.
		https://www.esp32.com/viewtopic.php?t=5380
		if we should not be using DHCP (for example we are using static IP addresses),
		then we need to instruct the ESP32 of the locations of the DNS servers manually.
		Google publicly makes available two name servers with the addresses of 8.8.8.8 and 8.8.4.4.
	*/
	
	ip_addr_t d;
	d.type = IPADDR_TYPE_V4;
	d.u_addr.ip4.addr = 0x08080808; //8.8.8.8 dns
	dns_setserver(0, &d);
	d.u_addr.ip4.addr = 0x08080404; //8.8.4.4 dns
	dns_setserver(1, &d);

#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}


void app_main(void)
{
    /**
		Inicialização da memória não volátil para armazenamento de dados (Non-volatile storage (NVS)).
		Necessário para realização da calibração do PHY. 
	*/
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
	/**
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação TCP quando a rede WiFi 
	   estiver configurada, ou seja, somente após o aceite de conexão e a 
	   liberação do IP pelo roteador da rede (nos casos de IPs dinâmicos).
	*/
	wifi_event_group = xEventGroupCreate();
	
	/**
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	/**
	 * Aguarda a conexão do ESP32 com a rede WiFi local;
	 */
	xEventGroupWaitBits( wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY );	
	
	/**
	   Task responsável em imprimir o valor do IP recebido pelo roteador da rede. 
	*/
	if( xTaskCreate(  task_button, "task_button", 1024*5, NULL, 2, NULL ) != pdTRUE )
	{
		if( DEBUG )
			ESP_LOGI( TAG, "error - nao foi possivel alocar task_button.\n" );	
		return;		
	}

}