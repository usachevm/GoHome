#include <math.h>
#include "go_home.h"
#include "state_screen.h"

#define OFS_BTN_Y	135

void dispatch_state_screen(struct gesture_* gest)
{
    if (gest->gesture == GESTURE_SWIPE_DOWN)
        appdata->screen = SCREEN_MAIN;
    if (gest->gesture == GESTURE_CLICK) {
		if (gest->touch_pos_y > OFS_BTN_Y) {
			// Попали на кнопку
			if (appdata->target_valid) {
				appdata->target_location.latitude = 0.0f;
				appdata->target_location.longitude = 0.0f;
				appdata->target_valid = 0;
			} else {
				if (!appdata->gps_valid) return;
				// Устанавливаем новую цель
				appdata->target_valid = 1;
				appdata->target_location = appdata->current_location;
			}
			// Сохраняем настройки
			write_settings();
			vibrate(1, 300, 0);
		}
	}
}

void draw_state_screen()
{
    char buffer[32];
    int point_dir = 0, delta_dir = 0;

    if (appdata->target_valid) {
        point_dir = normalize_degree(270 - get_direction(appdata->target_location, appdata->current_location));
        delta_dir = normalize_degree(point_dir - appdata->azimuth);
	}

    set_bg_color(COLOR_BLACK);

	// Рисуем в верхнем правом углу стрелку "вверх"
	struct res_params_ res_params;
	get_res_params(ELF_INDEX_SELF, RES_ARROW_UP, &res_params);
	show_elf_res_by_id(ELF_INDEX_SELF, RES_ARROW_UP, VIDEO_X - 5 - res_params.width, 5);

    set_fg_color(COLOR_GREEN);
	if (appdata->compass_calibrated) {
		_sprintf(buffer, "Север  : %d", appdata->azimuth);
		text_out(buffer, 5, 8);
	} else
		text_out("Компас некалиброван", 5, 8);

	if (appdata->gps_valid) {
		set_fg_color(COLOR_AQUA);
		_sprintf(buffer, "ТочкаШ: %f", appdata->current_location.latitude);
		text_out(buffer, 5, 38);
		_sprintf(buffer, "ТочкаД: %f", appdata->current_location.longitude);
		text_out(buffer, 5, 58);
	}

	if (appdata->target_valid) {
		set_fg_color(COLOR_YELLOW);
		_sprintf(buffer, "ЦельШ: %f", appdata->target_location.latitude);
		text_out(buffer, 5, 88);
		_sprintf(buffer, "ЦельД: %f", appdata->target_location.longitude);
		text_out(buffer, 5, 108);
	}

	if (appdata->target_valid || appdata->gps_valid) {
		// Кнопка "Установить цель / Снять цель"
		set_bg_color(appdata->target_valid ? COLOR_RED : COLOR_GREEN);
		draw_filled_rect_bg(0, OFS_BTN_Y, VIDEO_X, VIDEO_Y);
		int resid = appdata->target_valid ? RES_DEL : RES_ADD;
		struct res_params_ res_params;
		get_res_params(ELF_INDEX_SELF, resid, &res_params);
		show_elf_res_by_id(ELF_INDEX_SELF, resid, (VIDEO_X - res_params.width) >> 1, OFS_BTN_Y + ((VIDEO_Y - res_params.height - OFS_BTN_Y) >> 1));
	}
}

