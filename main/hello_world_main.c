/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "sdkconfig.h"

// 433 MHz input port
#define RF_IN 14

// 433 MHz Output port
#define RF_OUT 12

#define RMT_RX_CHANNEL  0                               /*!< RMT channel for receiver */
#define RMT_RX_GPIO_NUM RF_IN                           /*!< GPIO number for receiver */
#define RMT_CLK_DIV     (234)                            /*!< RMT counter clock divider */
#define RMT_TICK_10_US  (80000000/RMT_CLK_DIV/100000)
#define RMT_TIMEOUT_US  9500                            /*!< RMT receiver timeout value(us) */

void receive_init() {
    rmt_config_t rmt_rx;
    rmt_rx.channel = RMT_RX_CHANNEL;
    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = false;
    rmt_rx.rx_config.filter_ticks_thresh = 100;
    rmt_rx.rx_config.idle_threshold = RMT_TIMEOUT_US / 10 * (RMT_TICK_10_US);
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
}

void receive_task(void *parameter) {
    int channel = RMT_RX_CHANNEL;
    receive_init();
    RingbufHandle_t rb = NULL;
    //get RMT RX ringbuffer
    rmt_get_ringbuf_handler(channel, &rb);
    rmt_rx_start(channel, 1);
    
    while(rb) {
        size_t rx_size = 0;
        //try to receive data from ringbuffer.
        //RMT driver will push all the data it receives to its ringbuffer.
        //We just need to parse the value and return the spaces of ringbuffer.
        rmt_item16_t *items = (rmt_item16_t *)xRingbufferReceive(rb, &rx_size, 1000);
        rmt_item16_t *item = items;
        uint32_t length = 0;
        unsigned char idx = 0;

        while (item < items + rx_size/2){               
            
            if (item->duration > 100) {
                if (item->level != idx) {
                    printf("%d|%d\n", idx, length * 3);
                    fflush(stdout);
                    length = 0;
                    idx = item->level;
                }
                length = item->duration;
            }
            
            item++;
        }
        
        vRingbufferReturnItem(rb, (void*)items);
    }
    
}

void app_main() {
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    xTaskCreate(&receive_task, "receive_task", 2048, NULL, 5, NULL);
}
