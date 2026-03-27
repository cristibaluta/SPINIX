#include "task_buttons.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "pins.h"

#define BUTTON1 GPIO_NUM_2
#define BUTTON2 GPIO_NUM_3
#define BUTTON3 GPIO_NUM_4

#define DEBOUNCE_TIME_MS 50

static QueueHandle_t button_queue;

typedef struct {
    int pin;
    int64_t time;
} button_event_t;


/* ---------------- INTERRUPT ---------------- */

static void IRAM_ATTR button_isr(void *arg)
{
    int pin = (int)arg;

    button_event_t evt;
    evt.pin = pin;
    evt.time = esp_timer_get_time(); // microseconds

    xQueueSendFromISR(button_queue, &evt, NULL);
}

/* ---------------- BUTTON INIT ---------------- */

void configure_buttons()
{
    button_queue = xQueueCreate(10, sizeof(button_event_t));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask =
            (1ULL << BUTTON1) |
            (1ULL << BUTTON2) |
            (1ULL << BUTTON3),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(BUTTON1, button_isr, (void*)BUTTON1);
    gpio_isr_handler_add(BUTTON2, button_isr, (void*)BUTTON2);
    gpio_isr_handler_add(BUTTON3, button_isr, (void*)BUTTON3);
}

void configure_button() {
	// 1. Configure pin as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}


/* ---------------- BUTTON TASK ---------------- */

void button_task(void *arg)
{
    button_event_t evt;

    static int64_t last_press[GPIO_NUM_MAX] = {0};

    while (1) {
        if (xQueueReceive(button_queue, &evt, portMAX_DELAY)) {
            int64_t now = evt.time;

            if ((now - last_press[evt.pin]) > (DEBOUNCE_TIME_MS * 1000)) {
                last_press[evt.pin] = now;

                switch(evt.pin) {
                    case BUTTON1:
                        printf("Button 1 pressed\n");
                        break;

                    case BUTTON2:
                        printf("Button 2 pressed\n");
                        break;

                    case BUTTON3:
                        printf("Button 3 pressed\n");
                        break;
                }
            }
        }
    }
}

void task_buttons(void *params)
{
    printf("------- init task buttons\n");
	// configure_led();
    while(1) {
        // printf("Free stack 2: %u\n", uxTaskGetStackHighWaterMark(NULL));
		bool button_pressed = gpio_get_level(PIN_BUTTON) == 0;
		if (button_pressed) {
            printf("-------- button press\n");
			gpio_set_level(PIN_LED, 0);
		} else {
			gpio_set_level(PIN_LED, 1);
		}
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
