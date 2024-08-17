/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/sockets.h>
#include <errno.h>
#define DR_REG_RNG_BASE   0x3ff75144

#define MAX_NUMERO_CLIENTES 5
/*Porta que será aberta pelo Servidor TCP*/
#define PORT_NUMBER 9765 

void Task_Socket( void * socket_handle );
/*
 Informe o SSID e Password de sua rede WiFi local.
*/

//#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD

#define EXAMPLE_ESP_WIFI_SSID 	"FSimplicio"
#define EXAMPLE_ESP_WIFI_PASS 	"fsimpliciokzz5"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "WIFI: ";

/*
  Função de callback responsável em receber as notificações durante as etapas de conexão do WiFi.
  Por meio desta função de callback podemos saber o momento em que o WiFi do ESP32 foi inicializado com sucesso
  até quando é recebido o aceite do IP pelo roteador.
  ref: http://esp32.info/docs/esp_idf/html/dc/df5/esp__wifi_8h_source.html
*/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
		case SYSTEM_EVENT_STA_START:
			/*
				O WiFi do ESP32 foi configurado com sucesso. 
				Agora precisamos conectar a rede WiFi local. Portanto, foi chamado a função esp_wifi_connect();
			*/
			esp_wifi_connect();
			break;
		case SYSTEM_EVENT_STA_GOT_IP:
		    /*
				Se chegou aqui é porque a rede WiFi foi aceita pelo roteador e um IP foi 
				recebido e gravado no ESP32.
			*/
			ESP_LOGI(TAG, "got ip:%s",
					 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
			/*
			   O bit WiFi_CONNECTED_BIT é setado pelo EventGroup pois precisamos avisar as demais Tasks
			   que a rede WiFi foi devidamente configurada. 
			*/
			xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
			break;
		case SYSTEM_EVENT_STA_DISCONNECTED:
		    /*
				Se chegou aqui foi devido a falha de conexão com a rede WiFi.
				Por esse motivo, haverá uma nova tentativa de conexão WiFi pelo ESP32.
			*/
			esp_wifi_connect();
			
			/*
				É necessário apagar o bit WIFI_CONNECTED_BIT para avisar as demais Tasks que a conexão WiFi
				está offline no momento. 
			*/
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
			break;
		default:
			break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    tcpip_adapter_init();
		
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}


void socket_server( void * vParam ) 
{
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	/*
	  Cria o socket.
	*/
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGI(TAG, "socket: %d %s", sock, strerror(errno));
		goto END;
	}

	//Vincula o IP do servidor com a PORTA a ser aberta.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(PORT_NUMBER);
	int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGI(TAG, "bind: %d %s", rc, strerror(errno));
		goto END;
	}

	//Define o tamanho máximo da Queue para atendimento das conexões dos clientes.
	rc = listen(sock, MAX_NUMERO_CLIENTES);
	if (rc < 0) {
		ESP_LOGI(TAG, "listen: %d %s", rc, strerror(errno));
		goto END;
	}

	for(;;) 
	{
		socklen_t clientAddressLength = sizeof(clientAddress);
		/*
			Aceita a conexão e retorna o socket do cliente.
		*/
		int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
		if (clientSock < 0) 
		{
			ESP_LOGI(TAG, "accept: %d %s", clientSock, strerror(errno));
			
		} else {
	  
			ESP_LOGI(TAG, "SOCKET N: %d\r\n", clientSock);	
			/*
			  Cria uma nova task para tratamento dos dados deste socket. Se tiver apenas 1 cliente em todo o 
			  projeto, você pode tratar o cliente sem criar uma nova task, porém, para trabalhar com multiplas
			  conexões socket, ou seja, com vários clientes conectados ao ESP32 (Server TCP) tem que criar essa
			  task abaixo.  Essa task é necessária pois precisamos informar o handle do socket para podemos receber
			  ou enviar dados para o cliente. E por isso é necessário rodar em outra task.
			*/
			xTaskCreate( Task_Socket,"Task_Socket", 10000, (void*)clientSock, 5, NULL );
		}
		
	}
	END:
	/*
	  Se chegou aqui é porque houve falha na abertura do socket Server. Ou por falta de memória ou por ter
	  configurado o IP errado do servidor. 
	*/
	printf("FIM");
	vTaskDelete(NULL);
}

/*
  Esta task é responsável em tratar as multiplas conexões sockets dos clientes. 
  Para testarmos envio e recepção de dados, o cliente envia a string "CMD" e recebe um número Randômico
  como resposta. 

*/
void Task_Socket( void * socket_handle )
{
	int clientSock = (int) socket_handle;
	/*
	  Aumente o tamanho do buffer de recepção conforme o tamanho da mensagem a ser recebida ou enviada. 
	  Porém, seja cauteloso, pois será criado um buffer para cada cliente conectado ao servidor.
	*/
	int total =	1*1024;  
	int sizeUsed = 0;
	char *data = malloc(total);
	ESP_LOGI(TAG, "IN Task_Socket\r\n");	
	for(;;)
	{
		ssize_t sizeRead = recv(clientSock, data, total, 0);
		if (sizeRead < 0) {
			ESP_LOGI(TAG, "recv: %d %s", sizeRead, strerror(errno));
			break;
		}
		if (sizeRead == 0) {
			break;
		}
		
		ESP_LOGI(TAG, "Socket: %d - Data read (size: %d) was: %.*s", clientSock, sizeRead, sizeRead, data);
		
		/*
		   Comando "CMD" enviado pelo cliente. 
		   Em resposta a este comando o Servidor irá enviar um número Randômico ao cliente.
		*/
		if(data[0]=='C' && data[1]=='M' && data[2]=='D')
		{
			
			char buffer[20];
			//Gera o número randomico. 
			uint32_t randonN = READ_PERI_REG(DR_REG_RNG_BASE);
			//Converte o número randomico para string e armazena no buffer.
			sprintf(buffer, "%u\r\n", randonN);
			//Envia o buffer populado para o cliente da conexão socket.
			int rc = send(clientSock, buffer, strlen(buffer), 0);
			ESP_LOGI(TAG, "send: rc: %d", rc);
		}
		
		/* Este delay é recomendado pois em algum momento esta task tem que ser bloqueada, caso necessário*/
		vTaskDelay( 300/portTICK_PERIOD_MS );
   
	}
	/*
	   Esta task deve ser deletada toda vez que o cliente da conexão se desconectar do servidor. 
	*/
	free(data);
	close(clientSock);
	ESP_LOGI(TAG, "OUT Task_Socket N: %d\r\n",clientSock );
    vTaskDelete(NULL);
}
void app_main()
{	
    /*
		Inicialização da memória não volátil para armazenamento de dados (Non-volatile storage (NVS)).
		**Necessário para realização da calibração do PHY. 
	*/
	
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
	/*
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação TCP quando a rede WiFi estiver configurada, ou seja, 
	   somente após o aceite de conexão e a liberação do IP pelo roteador da rede (nos casos de IPs dinâmicos).
	*/
	wifi_event_group = xEventGroupCreate();
	
	/*
	  Configura a rede WiFi.
	*/
    wifi_init_sta();
	
	
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, true, true, portMAX_DELAY);	
	/*
	   Task responsável em imprimir o valor do IP recebido pelo roteador da rede. 
	*/
	xTaskCreate( socket_server,"Socket", 10000, NULL, 5, NULL );

}

/*
Links de Referencia sobre TCP Server UNIX 
https://msdn.microsoft.com/pt-br/library/windows/desktop/ms739168(v=vs.85).aspx
http://www.nongnu.org/lwip/2_0_x/group__socket.html
https://en.wikibooks.org/wiki/C_Programming/Networking_in_UNIX
*/
