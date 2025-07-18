/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/**
 * @brief This file contains utility functions for internal use only and should not be included by other files
 */

#include "esp_brookesia_services_internal.h"

#if !ESP_BROOKESIA_SERVICES_ENABLE_STORAGE_NVS
#   error "Storage NVS is not enabled, please enable it in the menuconfig"
#endif

#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:StorageNVS"
#include "esp_lib_utils.h"

#if !ESP_BROOKESIA_STORAGE_NVS_ENABLE_DEBUG_LOG || defined(ESP_BROOKESIA_UTILS_DISABLE_DEBUG_LOG)
#   undef ESP_UTILS_LOGD_IMPL_FUNC
#   define ESP_UTILS_LOGD_IMPL_FUNC(fmt, ...)
#endif
