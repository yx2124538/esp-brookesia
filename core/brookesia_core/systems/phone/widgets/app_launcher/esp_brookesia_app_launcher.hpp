/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/core/esp_brookesia_core.hpp"
#include "lvgl/esp_brookesia_lv_helper.hpp"
#include "esp_brookesia_app_launcher_icon.hpp"

// *INDENT-OFF*

typedef struct {
    struct {
        int y_start;
        ESP_Brookesia_StyleSize_t size;
    } main;
    struct {
        uint8_t default_num;
        ESP_Brookesia_StyleSize_t size;
    } table;
    struct {
        ESP_Brookesia_StyleSize_t main_size;
        uint8_t main_layout_column_pad;
        int main_layout_bottom_offset;
        ESP_Brookesia_StyleSize_t spot_inactive_size;
        ESP_Brookesia_StyleSize_t spot_active_size;
        ESP_Brookesia_StyleColor_t spot_inactive_background_color;
        ESP_Brookesia_StyleColor_t spot_active_background_color;
    } indicator;
    ESP_Brookesia_AppLauncherIconData_t icon;
    struct {
        uint8_t enable_table_scroll_anim: 1;
    } flags;
} ESP_Brookesia_AppLauncherData_t;

class ESP_Brookesia_AppLauncher {
public:
    ESP_Brookesia_AppLauncher(ESP_Brookesia_Core &core, const ESP_Brookesia_AppLauncherData_t &data);
    ~ESP_Brookesia_AppLauncher();

    bool begin(lv_obj_t *parent);
    bool del(void);

    bool addIcon(uint8_t page_index, const ESP_Brookesia_AppLauncherIconInfo_t &info);
    bool removeIcon(int id);
    bool changeIconTable(int id, uint8_t new_table_index);
    bool scrollToPage(uint8_t index);
    bool scrollToRightPage(void);
    bool scrollToLeftPage(void);

    bool checkInitialized(void) const        { return (_main_obj != nullptr); }
    bool checkTableFull(uint8_t page_index) const;
    bool checkVisible(void) const;
    bool checkPointInsideMain(lv_point_t &point) const;
    uint8_t getActiveScreenIndex(void) const { return _table_current_page_index; }

    static bool calibrateData(const ESP_Brookesia_StyleSize_t &screen_size, const ESP_Brookesia_CoreHome &home,
                              ESP_Brookesia_AppLauncherData_t &data);

private:
    typedef struct {
        uint8_t page_icon_count;
        ESP_Brookesia_LvObj_t page_main_obj;
        ESP_Brookesia_LvObj_t page_obj;
        ESP_Brookesia_LvObj_t spot_obj;
    } ESP_Brookesia_AppLauncherMixObject_t;
    typedef struct {
        uint8_t current_page_index;
        uint8_t target_page_index;
        std::shared_ptr<ESP_Brookesia_AppLauncherIcon> icon;
    } ESP_Brookesia_AppLauncherMixIcon_t;

    bool createMixObject(ESP_Brookesia_LvObj_t &table_obj, ESP_Brookesia_LvObj_t &indicator_obj,
                         std::vector<ESP_Brookesia_AppLauncherMixObject_t> &mix_objs);
    bool destoryMixObject(uint8_t index, std::vector<ESP_Brookesia_AppLauncherMixObject_t> &mix_objs);
    bool updateMixByNewData(uint8_t index, std::vector<ESP_Brookesia_AppLauncherMixObject_t> &mix_objs);
    bool togglePageIconClickable(uint8_t page_index, bool clickable);
    bool toggleCurrentPageIconClickable(bool clickable);
    bool updateActiveSpot(void);
    bool updateByNewData(void);

    static void onDataUpdateEventCallback(lv_event_t *event);
    static void onPageTouchEventCallback(lv_event_t *event);

    // Core
    ESP_Brookesia_Core &_core;
    const ESP_Brookesia_AppLauncherData_t &_data;

    int _table_current_page_index;
    uint8_t _table_page_icon_count_max;
    int _table_page_pad_row;
    int _table_page_pad_column;
    ESP_Brookesia_LvObj_t _main_obj;
    ESP_Brookesia_LvObj_t _table_obj;
    ESP_Brookesia_LvObj_t _indicator_obj;
    std::vector <ESP_Brookesia_AppLauncherMixObject_t> _mix_objs;
    std::map <int, ESP_Brookesia_AppLauncherMixIcon_t> _id_mix_icon_map;
};

// *INDENT-ON*
