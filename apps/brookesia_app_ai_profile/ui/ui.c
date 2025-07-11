// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.5.0
// LVGL version: 9.1.0
// Project name: SquareLine_Project

#include "ui.h"

///////////////////// VARIABLES ////////////////////


// SCREEN: ui_ScreenScreenAIProfile
void ui_ScreenScreenAIProfile_screen_init(void);
lv_obj_t *ui_ScreenScreenAIProfile;
lv_obj_t *ui_ScreenAIProfileTabviewTabView;
lv_obj_t *ui_ScreenAIProfileTabpageTabPageRole1;
lv_obj_t *ui_ScreenAIProfileLabelLabelRole1Name;
lv_obj_t *ui_ScreenAIProfileImageImageRole1;
lv_obj_t *ui_ScreenAIProfileLabelLabelRole1Description;
lv_obj_t *ui_ScreenAIProfileButtonButtonRole1Select;
lv_obj_t *ui_ScreenAIProfileLabelLabelButtonRole1Select;
lv_obj_t *ui_ScreenAIProfileImageImageRole1Select;
lv_obj_t *ui_ScreenAIProfileTabpageTabPageRole2;
lv_obj_t *ui_ScreenAIProfileImageImageRole2Select;
lv_obj_t *ui_ScreenAIProfileLabelLabelRole2Name;
lv_obj_t *ui_ScreenAIProfileImageImageRole2;
lv_obj_t *ui_ScreenAIProfileLabelLabelRole2Description;
lv_obj_t *ui_ScreenAIProfileButtonButtonRole2Select;
lv_obj_t *ui_ScreenAIProfileLabelLabelButtonRole2Select;
lv_obj_t *ui_ScreenAIProfileContainerContainerIndicator;
lv_obj_t *ui_ScreenAIProfilePanelPanelIndicator1;
lv_obj_t *ui_ScreenAIProfilePanelPanelIndicator2;
// CUSTOM VARIABLES

// EVENTS
lv_obj_t *ui_Startevents___initial_actions0;

// IMAGES AND IMAGE SETS
const lv_image_dsc_t *ui_imgset_esp_brookesia_app_icon_doubao__[1] = {&ui_img_esp_brookesia_app_icon_doubao_112_112_png};
const lv_image_dsc_t *ui_imgset_esp_brookesia_app_icon_dashu__[1] = {&ui_img_esp_brookesia_app_icon_dashu_112_112_png};

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREENS ////////////////////

void speaker_ai_profile_ui_init(void)
{
    lv_disp_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_ScreenScreenAIProfile_screen_init();
    lv_disp_load_scr(ui_ScreenScreenAIProfile);
}
