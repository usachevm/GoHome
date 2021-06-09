#include <math.h>
#include "go_home.h"
#include "main_screen.h"
#include "state_screen.h"

struct regmenu_ menu_screen = { 55, 1, 0, dispatch_screen, keypress_screen, screen_job, 0, show_screen, 0, 0 };

struct appdata_t** appdata_p;
struct appdata_t* appdata;

int main(int p, char** a)
{
    show_screen((void*)p);
}

void read_settings()
{
	int ofst = 0;
	ElfReadSettings(ELF_INDEX_SELF, &appdata->target_valid, ofst, sizeof(int)); ofst += sizeof(int);
    ElfReadSettings(ELF_INDEX_SELF, &appdata->target_location, ofst, sizeof(struct location_t)); ofst += sizeof(struct location_t);
}

void write_settings()
{
	int ofst = 0;
	ElfWriteSettings(ELF_INDEX_SELF, &appdata->target_valid, ofst, sizeof(int)); ofst += sizeof(int);
    ElfWriteSettings(ELF_INDEX_SELF, &appdata->target_location, ofst, sizeof(struct location_t)); ofst += sizeof(struct location_t);
}

void show_screen(void* p)
{
    appdata_p = (struct appdata_t**)get_ptr_temp_buf_2();

    if ((p == *appdata_p) && get_var_menu_overlay()) {
        appdata = *appdata_p;
        *appdata_p = (struct appdata_t*)NULL;
        reg_menu(&menu_screen, 0);
        *appdata_p = appdata;
    }
    else {
        reg_menu(&menu_screen, 0);
        *appdata_p = (struct appdata_t*)pvPortMalloc(sizeof(struct appdata_t));
        appdata = *appdata_p;
        _memclr(appdata, sizeof(struct appdata_t));
        appdata->proc = (Elf_proc_*)p;

        read_settings();
    }

    if (p && appdata->proc->elf_finish)
        appdata->ret_f = appdata->proc->elf_finish;
    else
        appdata->ret_f = show_watchface;

    appdata->screen = SCREEN_MAIN;

	if (!appdata->sensors_on) {
		switch_gps_pressure_sensors(1);
		compassEnable();
		appdata->sensors_on = 1;
		
		// Состояние компаса
		appdata->compass_calibrated = !isCalibrationRequired();
		appdata->azimuth = getAzimuh();
		// Состояние GPS
		struct datetime_ dt;
		navi_struct_ navi_data;
		get_navi_data(&navi_data);
		appdata->navi_initialized = IS_NAVI_GPS_READY(navi_data.ready);
	}

    draw_screen();

    set_display_state_value(8, 1);
    set_display_state_value(2, 1);

    set_update_period(1, appdata->compass_calibrated ? 500 : 1000); 
}

void keypress_screen()
{
    switch (appdata->screen) {
    case SCREEN_MAIN:
		// Выключаем сенсоры
		appdata->sensors_on = 0;
		compassDisable();
		switch_gps_pressure_sensors(0);
		show_menu_animate(appdata->ret_f, (unsigned int) show_screen, ANIMATE_RIGHT);
        break;
    case SCREEN_STATE:
        appdata->screen = SCREEN_MAIN;
        break;
    }
};

int dispatch_screen(void* p)
{
    struct gesture_* gest = (struct gesture_*)p;

    switch (appdata->screen)
    {
    case SCREEN_MAIN:
        dispatch_main_screen(gest);
        break;
    case SCREEN_STATE:
        dispatch_state_screen(gest);
        break;
    default:
        return 0;
    }

    draw_screen();
    return 0;
}

float normalize_radians(float angle)
{
    float twopi = 2 * PI;

    while (angle < 0)
        angle += twopi;

    while(angle > twopi)
        angle -= twopi;

    return angle;
}

int get_direction(struct location_t from, struct location_t to)
{
    float to_lat = to_radian(to.latitude);
    float to_lon = to_radian(to.longitude);
    float from_lat = to_radian(from.latitude);
    float from_lon = to_radian(from.longitude);
    float angle = atan2f(to_lat - from_lat, to_lon - from_lon);
    return to_degree(normalize_radians(angle));
}

void screen_job()
{
    navi_struct_ navi_data;
    get_navi_data(&navi_data);

    if (IS_NAVI_GPS_READY(navi_data.ready))
    {
		if (!appdata->navi_initialized) {
			vibrate(10, 30, 30);
			appdata->navi_initialized = 1;
		}
			
        appdata->current_location.latitude = navi_data.latitude / 3000000.0f * (navi_data.ns ? 1 : -1);
        appdata->current_location.longitude = navi_data.longitude / 3000000.0f * (navi_data.ew == 2 ? -1 : 1);

		int ticks = get_tick_count();
		int delta = ticks - appdata->ticks;
		if (delta > DELTA_TICKS_FOR_SPEED)
		{
			if (appdata->previous_location.latitude != 0.0f && appdata->previous_location.longitude != 0.0f) {
				// Можно посчитать скорость, т.к. известны предыдущие координаты
				appdata->speed = get_distance(appdata->current_location, appdata->previous_location) * 3.6f / delta * 510.0f;
				appdata->ticks = ticks;
			}
			// Сохраняем актуальные координаты
			appdata->previous_location = appdata->current_location;
		}

		appdata->gps_valid = 1;
    }
	else
	{
		appdata->current_location.latitude = 0.0f;
		appdata->current_location.longitude = 0.0f;
		
		appdata->gps_valid = 0;
	}

	// Получаем текущий азимут
	appdata->azimuth = getAzimuh();
	//if (!appdata->compass_calibrated && appdata->azimuth != 0) {
	if (!appdata->compass_calibrated && !isCalibrationRequired()) {
		// Произошла калибровка компаса - можно получать азимут
		appdata->compass_calibrated = 1;
		vibrate(1, 400, 0);
	}

    draw_screen();
	
    set_update_period(1, appdata->compass_calibrated ? 500 : 1000); 
}

void draw_screen()
{
    set_graph_callback_to_ram_1();
    set_bg_color(COLOR_BLACK);
    fill_screen_bg();
    load_font();

    switch (appdata->screen)
    {
    case SCREEN_MAIN:
        draw_main_screen();
        break;
    case SCREEN_STATE:
        draw_state_screen();
        break;
    }

    repaint_screen_lines(0, 176);
}
