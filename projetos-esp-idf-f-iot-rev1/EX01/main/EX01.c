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
#include "driver/gpio.h"
/*
 Pisca um LED conectado no pino GPIO_16 do Kit ESP32-DevKitC.
 Esquema Oficial: https://dl.espressif.com/dl/schematics/ESP32-Core-Board-V2_sch.pdf
*/
void blink_task(void *pvParameter)
{
    gpio_pad_select_gpio(GPIO_NUM_16); 
    gpio_set_direction(GPIO_NUM_16,GPIO_MODE_OUTPUT);
    printf("Blinking LED on GPIO 16\n");
    int cnt=0;
    while(1) {
		gpio_set_level(GPIO_NUM_16,cnt%2); // cnt % 2 (ou seja, se cnt for par, retorna 0, caso contrário, retorna 1)
        cnt++;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
 
void app_main()
{	
    xTaskCreate(blink_task,"blink_task",1024,NULL,1,NULL);
    printf("blink task  started\n");
}