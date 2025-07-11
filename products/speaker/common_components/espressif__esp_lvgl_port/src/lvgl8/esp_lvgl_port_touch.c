/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"

static const char *TAG = "LVGL";

static SemaphoreHandle_t touch_isr_mux = NULL;

/*******************************************************************************
* Types definitions
*******************************************************************************/

typedef struct {
    esp_lcd_touch_handle_t   handle;     /* LCD touch IO handle */
    lv_indev_drv_t           indev_drv;  /* LVGL input device driver */
    struct {
        float x;
        float y;
    } scale;                            /* Touch scale */
} lvgl_port_touch_ctx_t;

/*******************************************************************************
* Function definitions
*******************************************************************************/

static void lvgl_port_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/*******************************************************************************
* Public API functions
*******************************************************************************/

lv_indev_t *lvgl_port_add_touch(const lvgl_port_touch_cfg_t *touch_cfg)
{
    assert(touch_cfg != NULL);
    assert(touch_cfg->disp != NULL);
    assert(touch_cfg->handle != NULL);

    /* Touch context */
    lvgl_port_touch_ctx_t *touch_ctx = malloc(sizeof(lvgl_port_touch_ctx_t));
    if (touch_ctx == NULL) {
        ESP_LOGE(TAG, "Not enough memory for touch context allocation!");
        return NULL;
    }
    touch_ctx->handle = touch_cfg->handle;
    touch_ctx->scale.x = (touch_cfg->scale.x ? touch_cfg->scale.x : 1);
    touch_ctx->scale.y = (touch_cfg->scale.y ? touch_cfg->scale.y : 1);

    /* Register a touchpad input device */
    lv_indev_drv_init(&touch_ctx->indev_drv);
    touch_ctx->indev_drv.type = LV_INDEV_TYPE_POINTER;
    touch_ctx->indev_drv.disp = touch_cfg->disp;
    touch_ctx->indev_drv.read_cb = lvgl_port_touchpad_read;
    touch_ctx->indev_drv.user_data = touch_ctx;
    return lv_indev_drv_register(&touch_ctx->indev_drv);
}

esp_err_t lvgl_port_remove_touch(lv_indev_t *touch)
{
    assert(touch);
    lv_indev_drv_t *indev_drv = touch->driver;
    assert(indev_drv);
    lvgl_port_touch_ctx_t *touch_ctx = (lvgl_port_touch_ctx_t *)indev_drv->user_data;

    /* Remove input device driver */
    lv_indev_delete(touch);

    if (touch_ctx) {
        free(touch_ctx);
    }

    return ESP_OK;
}

/*******************************************************************************
* Private functions
*******************************************************************************/

static void lvgl_port_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    assert(indev_drv);
    lvgl_port_touch_ctx_t *touch_ctx = (lvgl_port_touch_ctx_t *)indev_drv->user_data;
    assert(touch_ctx->handle);

    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read data from touch controller into memory */
    if (touch_isr_mux == NULL || xSemaphoreTake(touch_isr_mux, 0) == pdTRUE) {
        esp_lcd_touch_read_data(touch_ctx->handle);
    }

    /* Read data from touch controller */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_ctx->handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);
    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touch_ctx->scale.x * touchpad_x[0];
        data->point.y = touch_ctx->scale.y * touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_port_touch_isr_cb(esp_lcd_touch_handle_t tp)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(touch_isr_mux, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

SemaphoreHandle_t init_touch_isr_mux(void)
{
    touch_isr_mux = xSemaphoreCreateBinary();
    assert(touch_isr_mux);

    return touch_isr_mux;
}
