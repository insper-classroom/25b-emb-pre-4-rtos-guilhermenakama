#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueLedR;
SemaphoreHandle_t xSemaphore_r;
QueueHandle_t xQueueLedY;
SemaphoreHandle_t xSemaphore_y;

// UM ÚNICO CALLBACK para ambos os botões
void btn_callback_unified(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if (gpio == BTN_PIN_R) {
            xSemaphoreGiveFromISR(xSemaphore_r, 0);
        } else if (gpio == BTN_PIN_Y) {
            xSemaphoreGiveFromISR(xSemaphore_y, 0);
        }
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int toggle_mode = 0;

    while (true) {
        if (xQueueReceive(xQueueLedR, &toggle_mode, 0)) {
            // Mensagem recebida
        }

        if (toggle_mode > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    // REGISTRA O CALLBACK UNIFICADO
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback_unified);

    int led_state = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE) {
            led_state = (led_state == 0) ? 1 : 0;
            xQueueSend(xQueueLedR, &led_state, 0);
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int toggle_mode = 0;

    while (true) {
        if (xQueueReceive(xQueueLedY, &toggle_mode, 0)) {
            // Mensagem recebida
        }

        if (toggle_mode > 0) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void btn_2_task(void *p) {
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    // SÓ HABILITA IRQ (callback já registrado)
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int led_state = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_y, pdMS_TO_TICKS(500)) == pdTRUE) {
            led_state = (led_state == 0) ? 1 : 0;
            xQueueSend(xQueueLedY, &led_state, 0);
        }
    }
}

int main() {
    stdio_init_all();

    xQueueLedR = xQueueCreate(32, sizeof(int));
    xSemaphore_r = xSemaphoreCreateBinary();
    
    xQueueLedY = xQueueCreate(32, sizeof(int));
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    
    xTaskCreate(btn_2_task, "BTN_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1) {}

    return 0;
}