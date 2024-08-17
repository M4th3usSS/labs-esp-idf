/*
  Autor: Prof° Fernando Simplicio de Sousa
  Hardware: NodeMCU ESP32
  SDK-IDF: v3.1
  Curso Online: Formação em Internet das Coisas (IoT) com ESP32
  Link: https://www.microgenios.com.br/formacao-iot-esp32/
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/uart.h"
// Error library
#include "esp_err.h"
/*
	Importante:
	Habilite o Bluetooth via menuconfig 
*/
#if defined(CONFIG_BT_ENABLED)

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#endif 

#define BUTTON	GPIO_NUM_17 
//----------------------------------------
QueueHandle_t Queue_Button;
static EventGroupHandle_t ble_event_group;
const static int CONNECTED_BIT = BIT0;
//----------------------------------------
#define TXD  (GPIO_NUM_4)
#define RXD  (GPIO_NUM_16)
#define RTS  (UART_PIN_NO_CHANGE)
#define CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (512)
QueueHandle_t Queue_Uart;
//----------------------------------------
#define SPP_TAG "BLUE"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXAMPLE_DEVICE_NAME "BLUETOOTH SPP"

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

int socket_blue;


struct AMessage
 {
    int ucMessageID;
    char ucData[ 512 ];
 } xMessage;
struct AMessage pxMessage;
 
/* Esta é a função de Callback do Bluetooth (registrada no início do programa) */

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    
    switch (event) {
	
	/* Chamado quando é inicializado o evento do SPP */
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
	/* Chamado após o SDP discovery 
	https://www.bluetooth.com/specifications/assigned-numbers/service-discovery*/
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
	/* Este evento é chamado quando o central BLUE (ESP32) se conectar a uma periferico Bluetooth, tal como o Android */
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
	/* Chamado quando o cliente ou o servidor se desconecta da rede */
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
		xEventGroupClearBits(ble_event_group, CONNECTED_BIT);
        break;
	/* Evento chamado logo após "ESP_SPP_INIT_EVT" */
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
	/* Chamado quando é inicializado a conexão do cliente ESP32 */	
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
	/* Chamado quando um dado for recebido pelo Bluetooth */
    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
	    
		pxMessage.ucMessageID  = param->data_ind.len;
		sprintf(pxMessage.ucData,"%.*s",param->data_ind.len, param->data_ind.data);
		xQueueSend(Queue_Uart,(void *)&pxMessage,(TickType_t )0); 
		
		//Debug:
		//printf("Data %.*s", pxMessage.ucMessageID,pxMessage.ucData);
		
        break;
	/* Chamado quando o bit de congestionamento de dados for sinalizado */
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
	/* Chamado quando um dado for enviado com sucesso */
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
	/* Chamado quando o Bluetooth server (ex: Smartphone) se conectar ao bluetooth cliente, no caso o ESP32 */
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
		
		xEventGroupSetBits(ble_event_group, CONNECTED_BIT);
		socket_blue = param->srv_open.handle; 
        break;
    default:
        break;
    }
}


void Task_Button ( void *pvParameter )
{
    char res[10]; 
    volatile int aux=0;
	uint32_t counter = 0;
		
    /* Configura a GPIO 17 como saída */ 
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 
	
    printf("Leitura da GPIO BUTTON\n");
    
    for(;;) 
	{
	    xEventGroupWaitBits(ble_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
		if(gpio_get_level( BUTTON ) == 0 && aux == 0)
		{ 
		  /* Delay para tratamento do bounce da tecla*/
		  vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
		  if(gpio_get_level( BUTTON ) == 0) 
		  {		
			
			sprintf(res, "%d\r\n", counter);
			printf("\r\nsocket n: %d.\r\n", socket_blue);	
			if(esp_spp_write(socket_blue, (int)strlen(res), (uint8_t*)res) == ESP_OK)
			{
			    counter++;
			} else {
			    printf("\r\nFalha no envio.\r\n");
			}
			
			aux = 1; 
		  }
		}

		if(gpio_get_level( BUTTON ) == 1 && aux == 1)
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			if(gpio_get_level( BUTTON ) == 1 )
			{
				aux = 0;
			}
		}	

		vTaskDelay( 10/portTICK_PERIOD_MS );	
    }
}



static void Task_Uart( void *pvParameter )
{
    struct AMessage pxMessage;
	
    /* Configuração do descritor da UART */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
	/* Define qual a UART a ser configurada. Neste caso, a UART_NUM_1 */ 
	/* No ESP32 há três UART, sendo a UART_NUM_0 utilizada para o debug, enquanto a UART_NUM_1 e UART_NUM_2 
	   estão livres para uso geral. 
	*/
    uart_param_config(UART_NUM_1, &uart_config);
	
	/* Definimos quais os pinos que serão utilizados para controle da UART_NUN_1 */
    uart_set_pin(UART_NUM_1, TXD, RXD, RTS, CTS);
	
	/* Define o buffer de recepção (RX) com 1024 bytes, e mantem o buffer de transmissão em zero. Neste caso, 
	   a task permanecerá bloqueada até que o conteúdo (charactere) seja transmitido. O três últimos parametros
	   estão relacionados a interrupção (evento) da UART.*/
	   
    uart_driver_install(UART_NUM_1, BUF_SIZE * 1, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int len;
	
    for(;;) 
	{      
	   
	   /* Leitura da UART */
       len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
	   if(len>0)
	   {
	        if(esp_spp_write(socket_blue, len, (uint8_t*)data) == ESP_OK)
			{
			   printf("\r\nMCU->MSG: %.*s\r\n", len, (uint8_t*)data );
			} else {
			    printf("\r\nFalha no envio.\r\n");
			}
	   }
	   
        if( Queue_Uart != NULL ) 
		{
		   if(xQueueReceive(Queue_Uart, &(pxMessage), 0))
		   {
		        printf("BLE->MSG: %.*s", pxMessage.ucMessageID,pxMessage.ucData);
				uart_write_bytes(UART_NUM_1, (char *) pxMessage.ucData, pxMessage.ucMessageID);
		   }
        }		         
		
		vTaskDelay(500/portTICK_PERIOD_MS); 
    }
}


void app_main()
{
    /* Inicializa o setor de flash não volátil */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

	/*
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação quando a rede BLUE estiver configurada e pareada com o master BLUE.
	*/
	ble_event_group = xEventGroupCreate();
	Queue_Uart = xQueueCreate( 10, sizeof( struct AMessage ) );
	if( Queue_Uart == NULL )
    {
        printf("Failed to create the queue"); 
		return;
    }
	
    /* Inicializa o controlador do Bluetooth */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e Verifica se o controlador do Bluetooth foi inicializado corretamente */
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Bluedroid é o nome do Stack BLUE da Espressif. Inicializa o Stack Bluetooth*/
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e verifica se o stack bluetooth foi inicializado corretamente */
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Registra a função de Callback responsável em receber e tratar todos os eventos do rádio Bluetooth */
    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* O bluetooth possui vários perfis. Dentre eles o SPP (Serial Port Profile). Este perfil pertence ao Bluetooth clássico
	e é por meio dele que fazemos uma comunicação serial transparente (ponto a ponto). 
	Habilita e verifica se o perfil SPP do Bluetooth.*/
    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
	
	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	xTaskCreate( Task_Button, "TaskButton", 10000, NULL, 2, NULL );
	xTaskCreate( Task_Uart, "TaskUart", 10000, NULL, 3, NULL );
}

