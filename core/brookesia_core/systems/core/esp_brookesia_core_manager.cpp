/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cstring>
#include <cmath>
#include "esp_brookesia_systems_internal.h"
#if !ESP_BROOKESIA_CORE_MANAGER_ENABLE_DEBUG_LOG
#   define ESP_BROOKESIA_UTILS_DISABLE_DEBUG_LOG
#endif
#include "private/esp_brookesia_core_utils.hpp"
#include "lvgl/esp_brookesia_lv.hpp"
#include "esp_brookesia_core_manager.hpp"
#include "esp_brookesia_core.hpp"

using namespace std;
using namespace esp_brookesia::gui;

ESP_Brookesia_CoreManager::ESP_Brookesia_CoreManager(ESP_Brookesia_Core &core, const ESP_Brookesia_CoreManagerData_t &data):
    _core(core),
    _core_data(data)
{
}

ESP_Brookesia_CoreManager::~ESP_Brookesia_CoreManager()
{
    ESP_UTILS_LOGD("Destroy(@0x%p)", this);
    if (!delCore()) {
        ESP_UTILS_LOGE("Delete failed");
    }
}

int ESP_Brookesia_CoreManager::installApp(ESP_Brookesia_CoreApp *app)
{
    bool app_installed = false;
    bool home_process_app_installed = false;
    lv_area_t app_visual_area = {};
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, -1, "Invalid app");

    ESP_UTILS_LOGD("Install App(%p)", app);

    // Check if the app is already installed
    for (auto it = _id_installed_app_map.begin(); it != _id_installed_app_map.end(); it++) {
        ESP_UTILS_CHECK_FALSE_RETURN(it->second != app, -1, "Already installed");
    }

    // Initialize app
    ESP_UTILS_CHECK_FALSE_GOTO(app_installed = app->processInstall(&_core, _app_free_id), err, "App install failed");
    // Insert app to installed_app_map
    ESP_UTILS_CHECK_FALSE_GOTO(_id_installed_app_map.insert(pair <int, ESP_Brookesia_CoreApp *>(app->_id, app)).second, err,
                               "Insert app failed");

    ESP_UTILS_CHECK_FALSE_GOTO(home.getAppVisualArea(app, app_visual_area), err, "Home get app visual area failed");
    ESP_UTILS_CHECK_FALSE_GOTO(app->setVisualArea(app_visual_area), err, "App set visual area failed");
    ESP_UTILS_CHECK_FALSE_GOTO(app->calibrateVisualArea(), err, "App calibrate visual area failed");

    // Process home
    ESP_UTILS_CHECK_FALSE_GOTO(home_process_app_installed = home.processAppInstall(app), err,
                               "Home process app install failed");

    // Update free app id
    _app_free_id++;

    return app->getId();

err:
    if (home_process_app_installed && !home.processAppUninstall(app)) {
        ESP_UTILS_LOGE("Home process app uninstall failed");
    }
    if (app_installed && !app->processUninstall()) {
        ESP_UTILS_LOGE("App uninstall failed");
    }
    _id_installed_app_map.erase(app->_id);

    return -1;
}

int ESP_Brookesia_CoreManager::installApp(ESP_Brookesia_CoreApp &app)
{
    return installApp(&app);
}

int ESP_Brookesia_CoreManager::uninstallApp(ESP_Brookesia_CoreApp *app)
{
    bool ret = true;
    int app_id = -1;
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    app_id = app->_id;

    ESP_UTILS_LOGD("Uninstall App(%d)", app_id);

    // Check if the app is already installed
    auto it = _id_installed_app_map.begin();
    for (; it != _id_installed_app_map.end(); it++) {
        if (it->second == app) {
            break;
        }
    }
    ESP_UTILS_CHECK_FALSE_RETURN(it->second == app, false, "App(%d) is not installed", app_id);

    // Process home
    ESP_UTILS_CHECK_FALSE_RETURN(home.processAppUninstall(app), false, "Home process app uninstall failed");

    // Deinit app
    ret = app->processUninstall();
    if (!ret) {
        ESP_UTILS_LOGE("App uninstall failed");
    }

    // Remove app from installed_app_map
    ESP_UTILS_CHECK_FALSE_RETURN(_id_installed_app_map.erase(app_id) > 0, false, "Remove app failed");

    return ret;
}

int ESP_Brookesia_CoreManager::uninstallApp(ESP_Brookesia_CoreApp &app)
{
    return uninstallApp(&app);
}

bool ESP_Brookesia_CoreManager::uninstallApp(int id)
{
    ESP_Brookesia_CoreApp *app = nullptr;

    ESP_UTILS_LOGD("Uninstall App(%d)", id);

    app = getInstalledApp(id);
    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Get installed app failed");

    ESP_UTILS_CHECK_FALSE_RETURN(uninstallApp(app), false, "Uninstall app failed");

    return true;
}

bool ESP_Brookesia_CoreManager::startApp(int id)
{
    ESP_Brookesia_CoreApp *app = NULL;
    ESP_Brookesia_CoreApp *app_old = NULL;

    // Check if the app is already running
    auto find_ret = _id_running_app_map.find(id);
    if (find_ret != _id_running_app_map.end()) {
        app = find_ret->second;
        ESP_UTILS_LOGD("App(%d) is already running, just resume it", app->_id);
        // If so, resume app
        ESP_UTILS_CHECK_FALSE_RETURN(processAppResume(app), false, "Resume app failed");

        return true;
    }

    // If not, then find the target app from installed app map
    find_ret = _id_installed_app_map.find(id);
    ESP_UTILS_CHECK_FALSE_RETURN(find_ret != _id_installed_app_map.end(), false, "Can't find app in installed app map");
    app = find_ret->second;

    // Check if the running app num is at the limit
    if ((_core_data.app.max_running_num != 0) && (int)_id_running_app_map.size() >= _core_data.app.max_running_num) {
        for (auto it = _id_running_app_map.begin(); it != _id_running_app_map.end(); it++) {
            app_old = it->second;
        }
        ESP_UTILS_CHECK_NULL_RETURN(app_old, false, "Get old app failed");

        ESP_UTILS_LOGW("Running app num(%d) is already at the limit, will close the oldest app(%d)",
                       (int)_id_running_app_map.size(), app_old->_id);

        ESP_UTILS_CHECK_FALSE_RETURN(processAppClose(app_old), false, "Close app failed");
    }

    // Start app
    ESP_UTILS_CHECK_FALSE_RETURN(processAppRun(app), false, "Start app failed");

    // Add app to running_app_map
    ESP_UTILS_CHECK_FALSE_GOTO(_id_running_app_map.insert(pair <int, ESP_Brookesia_CoreApp *>(id, app)).second, err,
                               "Insert app to running map failed");

    return true;

err:
    ESP_UTILS_CHECK_FALSE_RETURN(processAppClose(app), false, "Close app failed");

    return false;
}

bool ESP_Brookesia_CoreManager::processAppRun(ESP_Brookesia_CoreApp *app)
{
    bool is_home_run = false;
    bool is_app_run = false;
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Process app(%d) run", app->_id);

    // Process home, and get the visual area of the app
    ESP_UTILS_CHECK_FALSE_RETURN(is_home_run = home.processAppRun(app), false, "Process home before app run failed");

    // Process app
    ESP_UTILS_CHECK_FALSE_GOTO(is_app_run = app->processRun(), err, "Process app run failed");

    // Process extra
    ESP_UTILS_CHECK_FALSE_GOTO(processAppRunExtra(app), err, "Process app run extra failed");

    // Update active app
    _active_app = app;

    return true;

err:
    if (is_home_run && !home.processAppClose(app)) {
        ESP_UTILS_LOGE("Home process close failed");
    }
    if (is_app_run && !app->processClose(true)) {
        ESP_UTILS_LOGE("App process close failed");
    }
    ESP_UTILS_CHECK_FALSE_RETURN(home.processMainScreenLoad(), false, "Home load main screen failed");

    return false;
}

bool ESP_Brookesia_CoreManager::processAppResume(ESP_Brookesia_CoreApp *app)
{
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Process app(%d) resume", app->_id);

    // Check if the screen is showing app and the app is not the active one
    if ((_active_app != nullptr) && (_active_app != app)) {
        // if so, pause the active app
        ESP_UTILS_CHECK_FALSE_RETURN(processAppPause(_active_app), false, "App process pause failed");
    }

    // Process home
    ESP_UTILS_CHECK_FALSE_RETURN(home.processAppResume(app), false, "Home process resume failed");

    // Process app, only load active screen if the app is not shown
    ESP_UTILS_CHECK_FALSE_RETURN(app->processResume(), false, "App process resume failed");

    // Process extra
    ESP_UTILS_CHECK_FALSE_RETURN(processAppResumeExtra(app), false, "Process app resume extra failed");

    // Update active app
    _active_app = app;

    return true;
}

bool ESP_Brookesia_CoreManager::processAppPause(ESP_Brookesia_CoreApp *app)
{
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Process app(%d) pause", app->_id);

    // Process app
    ESP_UTILS_CHECK_FALSE_RETURN(app->processPause(), false, "App process pause failed");
    if (_core_data.flags.enable_app_save_snapshot) {
        if (!saveAppSnapshot(app)) {
            ESP_UTILS_LOGE("Save app snapshot failed");
        }
    }

    // Process home,
    ESP_UTILS_CHECK_FALSE_GOTO(home.processAppPause(app), err, "Home process load failed");

    // Process extra
    ESP_UTILS_CHECK_FALSE_GOTO(processAppPauseExtra(app), err, "Process app pause extra failed");

    return true;

err:
    ESP_UTILS_CHECK_FALSE_RETURN(processAppClose(app), false, "Close app failed");

    return false;
}

bool ESP_Brookesia_CoreManager::processAppClose(ESP_Brookesia_CoreApp *app)
{
    ESP_Brookesia_CoreHome &home = _core._core_display;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Process app(%d) close", app->_id);

    // Process app, enable auto clean when the app is showing
    ESP_UTILS_CHECK_FALSE_RETURN(app->processClose(_active_app == app), false, "App process close failed");
    if (_core_data.flags.enable_app_save_snapshot) {
        if (!releaseAppSnapshot(app)) {
            ESP_UTILS_LOGE("Release app snapshot failed");
        }
    }

    // Process home, load main screen if the app is showing
    ESP_UTILS_CHECK_FALSE_RETURN(home.processAppClose(app), false, "Home process close failed");

    // Process extra
    ESP_UTILS_CHECK_FALSE_RETURN(processAppCloseExtra(app), false, "Process app pause extra failed");

    // Remove app from running map and update active app
    ESP_UTILS_CHECK_FALSE_RETURN(_id_running_app_map.erase(app->_id) > 0, false, "Remove app from running map failed");
    if (_active_app == app) {
        _active_app = nullptr;
    }

    return true;
}

bool ESP_Brookesia_CoreManager::saveAppSnapshot(ESP_Brookesia_CoreApp *app)
{
#if !LV_USE_SNAPSHOT
    ESP_UTILS_CHECK_FALSE_RETURN(false, false, "`LV_USE_SNAPSHOT` is not enabled");
#else
    bool resize_app_screen = false;
    lv_draw_buf_t *snapshot_buffer = nullptr;
    lv_res_t ret = LV_RES_INV;
    lv_area_t app_screen_area = {};
    shared_ptr<ESP_Brookesia_AppSnapshot_t> snapshot = nullptr;

    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Save app(%d) snapshot", app->_id);

    ESP_UTILS_CHECK_FALSE_RETURN(app->_active_screen != nullptr, false, "Invalid active screen");
    app_screen_area = app->_active_screen->coords;
    if ((lv_area_get_width(&app_screen_area) != _core.getCoreData().screen_size.width) ||
            (lv_area_get_height(&app_screen_area) != _core.getCoreData().screen_size.height)) {
        ESP_UTILS_LOGD("Active screen size is not match screen size, resize it");
        app->_active_screen->coords = (lv_area_t) {
            .x1 = 0,
            .y1 = 0,
            .x2 = (lv_coord_t)(_core.getCoreData().screen_size.width - 1),
            .y2 = (lv_coord_t)(_core.getCoreData().screen_size.height - 1),
        };
        resize_app_screen = true;
    }

    auto it = _id_app_snapshot_map.find(app->_id);
    auto color_format = _core.getDisplayDevice()->color_format;
    snapshot = (it != _id_app_snapshot_map.end()) ? it->second : nullptr;

    if ((snapshot != nullptr) &&
            (snapshot->image_resource->header.w == lv_area_get_width(&app_screen_area)) &&
            (snapshot->image_resource->header.h == lv_area_get_height(&app_screen_area))) {
        snapshot_buffer = snapshot->image_resource;
    } else {
        if (snapshot != nullptr) {
            lv_draw_buf_destroy(snapshot->image_resource);
        }
        snapshot_buffer = lv_snapshot_create_draw_buf(app->_active_screen, color_format);
        ESP_UTILS_CHECK_NULL_GOTO(snapshot_buffer, err, "Create snapshot buffer failed");
    }

    if (snapshot == nullptr) {
        snapshot = make_shared<ESP_Brookesia_AppSnapshot_t>();
        ESP_UTILS_CHECK_NULL_GOTO(snapshot, err, "Make snapshot object failed");
    }

    // And take snapshot for recent screen
    ret = lv_snapshot_take_to_draw_buf(app->_active_screen, color_format, snapshot_buffer);
    ESP_UTILS_CHECK_FALSE_GOTO(ret == LV_RESULT_OK, err, "Take snapshot fail");

    snapshot->image_resource = snapshot_buffer;
    _id_app_snapshot_map[app->_id] = snapshot;
    if (resize_app_screen) {
        app->_active_screen->coords = app_screen_area;
    }

    return true;

err:
    if (snapshot_buffer != nullptr) {
        lv_draw_buf_destroy(snapshot_buffer);
    }
    if (resize_app_screen) {
        app->_active_screen->coords = app_screen_area;
    }

    return false;
#endif
}

bool ESP_Brookesia_CoreManager::releaseAppSnapshot(ESP_Brookesia_CoreApp *app)
{
    ESP_UTILS_CHECK_NULL_RETURN(app, false, "Invalid app");
    ESP_UTILS_LOGD("Release app(%d) snapshot", app->_id);

    auto it = _id_app_snapshot_map.find(app->_id);
    if (it == _id_app_snapshot_map.end()) {
        return true;
    }

    ESP_UTILS_CHECK_NULL_RETURN(it->second, false, "Invalid snapshot object");
    auto snapshot_buffer = it->second->image_resource;
    if (snapshot_buffer != nullptr) {
        lv_draw_buf_destroy(snapshot_buffer);
    }
    ESP_UTILS_CHECK_FALSE_RETURN(_id_app_snapshot_map.erase(app->_id) > 0, false, "Free snapshot failed");

    return true;
}

void ESP_Brookesia_CoreManager::resetActiveApp(void)
{
    ESP_UTILS_LOGD("Reset active app");
    _active_app = nullptr;
}

int ESP_Brookesia_CoreManager::getRunningAppIndexByApp(ESP_Brookesia_CoreApp *app)
{
    ESP_UTILS_CHECK_NULL_RETURN(app, -1, "Invalid app");

    for (auto it = _id_running_app_map.begin(); it != _id_running_app_map.end(); ++it) {
        if (it->second == app) {
            return distance(it, _id_running_app_map.end()) - 1;
        }
    }

    return -1;
}

int ESP_Brookesia_CoreManager::getRunningAppIndexById(int id)
{
    auto it = _id_running_app_map.find(id);
    if (it != _id_running_app_map.end()) {
        return distance(it, _id_running_app_map.end()) - 1;
    }

    return -1;
}

ESP_Brookesia_CoreApp *ESP_Brookesia_CoreManager::getInstalledApp(int id)
{
    auto it = _id_installed_app_map.find(id);
    if (it != _id_installed_app_map.end()) {
        return it->second;
    }

    return nullptr;
}

ESP_Brookesia_CoreApp *ESP_Brookesia_CoreManager::getRunningAppByIdenx(uint8_t index)
{
    if (index >= _id_running_app_map.size()) {
        return nullptr;
    }

    auto it = _id_running_app_map.begin();
    advance(it, _id_running_app_map.size() - index - 1);

    return it->second;
}

ESP_Brookesia_CoreApp *ESP_Brookesia_CoreManager::getRunningAppById(int id)
{
    auto it = _id_running_app_map.find(id);
    if (it != _id_running_app_map.end()) {
        return it->second;
    }

    return nullptr;
}

const lv_draw_buf_t *ESP_Brookesia_CoreManager::getAppSnapshot(int id)
{
    auto it = _id_app_snapshot_map.find(id);
    ESP_UTILS_CHECK_FALSE_RETURN(it != _id_app_snapshot_map.end(), nullptr, "App snapshot not found");

    return it->second->image_resource;
}

bool ESP_Brookesia_CoreManager::beginCore(void)
{
    ESP_UTILS_LOGD("Begin(@0x%p)", this);

    ESP_UTILS_CHECK_FALSE_RETURN(_core.registerAppEventCallback(onAppEventCallback, this), false,
                                 "Register app event failed");
    ESP_UTILS_CHECK_FALSE_GOTO(_core.registerNavigateEventCallback(onNavigationEventCallback, this), err,
                               "Register navigation event failed");

    return true;

err:
    ESP_UTILS_CHECK_FALSE_RETURN(delCore(), false, "Delete failed");

    return false;
}

bool ESP_Brookesia_CoreManager::delCore(void)
{
    bool ret = true;
    unordered_map <int, ESP_Brookesia_CoreApp *> id_installed_app_map = _id_installed_app_map;

    ESP_UTILS_LOGD("Delete(@0x%p)", this);

    if (_core.checkCoreInitialized()) {
        if (!_core.unregisterAppEventCallback(onAppEventCallback, this)) {
            ESP_UTILS_LOGE("Unregister app event failed");
            ret = false;
        }
    }

    _app_free_id = 0;
    _active_app = nullptr;
    for (auto app : id_installed_app_map) {
        if (!uninstallApp(app.second)) {
            ESP_UTILS_LOGE("Uninstall app(%d) failed", app.second->_id);
            ret = false;
        }
    }
    _id_installed_app_map.clear();
    _id_running_app_map.clear();
    _id_app_snapshot_map.clear();

    return ret;
}

void ESP_Brookesia_CoreManager::onAppEventCallback(lv_event_t *event)
{
    int id = -1;
    ESP_Brookesia_CoreApp *app = nullptr;
    ESP_Brookesia_CoreManager *manager = nullptr;
    ESP_Brookesia_CoreAppEventData_t *event_data = nullptr;

    ESP_UTILS_LOGD("App start event callback");
    ESP_UTILS_CHECK_NULL_EXIT(event, "Invalid event object");

    manager = (ESP_Brookesia_CoreManager *)lv_event_get_user_data(event);
    event_data = (ESP_Brookesia_CoreAppEventData_t *)lv_event_get_param(event);
    ESP_UTILS_CHECK_NULL_EXIT(manager, "Invalid manager object");
    ESP_UTILS_CHECK_FALSE_EXIT((event_data != nullptr) && (event_data->type < ESP_BROOKESIA_CORE_APP_EVENT_TYPE_MAX),
                               "Invalid event data");

    id = event_data->id;
    switch (event_data->type) {
    case ESP_BROOKESIA_CORE_APP_EVENT_TYPE_START:
        ESP_UTILS_LOGD("Start app(%d)", id);
        ESP_UTILS_CHECK_FALSE_EXIT(manager->startApp(id), "Run app failed");
        break;
    case ESP_BROOKESIA_CORE_APP_EVENT_TYPE_STOP:
        ESP_UTILS_LOGD("Stop app(%d)", id);
        app = manager->getRunningAppById(id);
        ESP_UTILS_CHECK_NULL_EXIT(app, "Invalid app");
        ESP_UTILS_CHECK_FALSE_EXIT(manager->processAppClose(app), "Close app failed");
        break;
    default:
        break;
    }
}

void ESP_Brookesia_CoreManager::onNavigationEventCallback(lv_event_t *event)
{
    void *param = nullptr;
    ESP_Brookesia_CoreManager *manager = nullptr;
    ESP_Brookesia_CoreNavigateType_t navigation_type = ESP_BROOKESIA_CORE_NAVIGATE_TYPE_MAX;

    ESP_UTILS_LOGD("Navigatiton bar event callback");
    ESP_UTILS_CHECK_NULL_EXIT(event, "Invalid event object");

    manager = (ESP_Brookesia_CoreManager *)lv_event_get_user_data(event);
    ESP_UTILS_CHECK_NULL_EXIT(manager, "Invalid manager");

    param = lv_event_get_param(event);
    memcpy(&navigation_type, &param, sizeof(ESP_Brookesia_CoreNavigateType_t));
    ESP_UTILS_CHECK_FALSE_EXIT(navigation_type < ESP_BROOKESIA_CORE_NAVIGATE_TYPE_MAX, "Invalid navigate type");

    ESP_UTILS_CHECK_FALSE_EXIT(manager->processNavigationEvent(navigation_type), "Process navigation bar event failed");
}
