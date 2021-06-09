#include <math.h>
#include "go_home.h"
#include "main_screen.h"
#include "stack.h"

float PI = 3.14159265359f;

void dispatch_main_screen(struct gesture_* gest)
{
    if (gest->gesture == GESTURE_SWIPE_UP)
        appdata->screen = SCREEN_STATE;
}

float to_radian(float degree)
{
    return degree * PI / 180.0f;
}

int to_degree(float angle)
{
    return (int)(angle * 180.0f / PI);
}

int get_pixel(int x, int y)
{
    if (x >= VIDEO_X || y >= VIDEO_Y)
        return -1;
    void* screen = get_ptr_screen_memory();
    byte* address = (byte*)screen + ((y * VIDEO_X + x) >> 1);
    return ((x & 1) ? *address >> 4 : *address) & 0x0f;
}

void draw_pixel(int x, int y, int color)
{
    if (x >= VIDEO_X || y >= VIDEO_Y) return;

    void* screen = get_ptr_screen_memory();
    byte* address = (byte*)screen + ((y * VIDEO_X + x) >> 1);

    if (x & 1) {
       *address &= 0xf;
       *address |= (color & 0xf) << 4;
    }
    else {
       *address &= 0xf0;
       *address |= color & 0xf;
    }
}

void swap(int* a, int* b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void draw_line(int x0, int y0, int x1, int y1, int color)
{
    int steep = abssub(y1, y0) > abssub(x1, x0);
    if (steep)
    {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1)
    {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    int dx = x1 - x0;
    int dy = abssub(y1, y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;
    for (int x = x0; x <= x1; x++)
    {
        draw_pixel(steep ? y : x, steep ? x : y, color);
        error -= dy;
        if (error < 0)
        {
            y += ystep;
            error += dx;
        }
    }
}

byte fill_line(byte x, byte y, int color_src, int color_dst) {
   byte xl = x;
   for (; xl > 0; xl--) {
      if (get_pixel(xl, y) != color_src) break;
      draw_pixel(xl, y, color_dst);
   }
   byte xr = x+1;
   for (; xr < VIDEO_X; xr++) {
      if (get_pixel(xr, y) != color_src) break;
      draw_pixel(xr, y, color_dst);
   }
   return (xr - xl < 3) ? 0xff : ((xl + xr) >> 1);
}

void fill_simple(byte x, byte y, int color) {
   int color_src = get_pixel(x, y);
   byte xt = x;
   for (byte yt = y; yt > 0; yt--) {
      xt = fill_line(xt, yt, color_src, color);
      if (xt == 0xff) break;
   }
   byte xb = x;
   for (byte yb = y+1; yb < VIDEO_Y; yb++) {
      xb = fill_line(xb, yb, color_src, color);
      if (xb == 0xff) break;
   }
}

void fill(byte x, byte y, int color)
{
    struct stack_t stack;
    init(&stack);

    int old_color = get_pixel(x, y);
    if (color == old_color)
        return;

    struct point_t pt = { x, y };
    push(&stack, pt);

    while (stack.head)
    {
        pt = pop(&stack);
        word y1 = pt.y;
        while (y1 >= 1 && get_pixel(pt.x, y1) == old_color)
            y1--;
        y1++;
        int span_left = 0;
        int span_right = 0;

        while (y1 < VIDEO_Y && get_pixel(pt.x, y1) == old_color)
        {
            draw_pixel(pt.x, y1, color);
            if (!span_left && pt.x > 0 && get_pixel(pt.x - 1, y1) == old_color)
            {
                struct point_t new_pt = { pt.x - 1, y1 };
                push(&stack, new_pt);
                span_left = 1;
            }
            else if (span_left && pt.x > 0 && get_pixel(pt.x - 1, y1) != old_color)
            {
                span_left = 0;
            }

            if (!span_right && pt.x < VIDEO_X && get_pixel(pt.x + 1, y1) == old_color)
            {
                struct point_t new_pt = { pt.x + 1, y1 };
                push(&stack, new_pt);
                span_right = 1;
            }
            else if (span_right && pt.x < VIDEO_X && get_pixel(pt.x + 1, y1) != old_color)
            {
                span_right = 0;
            }
            y1++;
        }
    }
}

struct point_t rotate(float angle, struct point_t pt)
{
    struct point_t res;
    res.x = pt.x * cosf(angle) - pt.y * sinf(angle);
    res.y = pt.y * cosf(angle) + pt.x * sinf(angle);
    return res;
}

const int ARROW_SIZE = 10;

void draw_arrow(int x, int y, int color, int direction_angle)
{
    struct point_t points[] = { {-2.6 * ARROW_SIZE, 2 * ARROW_SIZE}, {0, -4 * ARROW_SIZE}, {2.6 * ARROW_SIZE, 2 * ARROW_SIZE}, {0, 1 * ARROW_SIZE} };
    float angle = to_radian(direction_angle);

    for (int i = 0; i < 4; i++)
    {
        struct point_t pt0 = rotate(angle, points[i]);
        struct point_t pt1 = rotate(angle, points[(i + 1) % 4]);
        draw_line(x + pt0.x, y + pt0.y, x + pt1.x, y + pt1.y, color);
    }
}

void draw_circle_compass(int x, int y, int color) {
	// Circle around the compass
    struct point_t pt_pixel = { 0, ARROW_SIZE * 6 };
    for (int i = 0; i < 12; i++) {
        struct point_t pt = rotate(PI / 6 * i , pt_pixel);
        if (i%3)
            draw_pixel(x + pt.x, y + pt.y, color);
        else {
            draw_line(x + pt.x - 1, y + pt.y, x + pt.x + 1, y + pt.y, color);
            draw_line(x + pt.x, y + pt.y - 1, x + pt.x, y + pt.y + 1, color);
        }
    }
}

#define COMPASS_ARROW_LENGTH	50
#define COMPASS_ARROW_HWIDTH	10

void draw_arrow_compass(int x, int y, int color, int direction_angle) {
    struct point_t points[] = { {-COMPASS_ARROW_HWIDTH, 0}, {0, -COMPASS_ARROW_LENGTH}, {COMPASS_ARROW_HWIDTH, 0} };
	struct point_t pt[3];
    float angle = to_radian(direction_angle);
    for (int i = 0; i < 3; i++) {
		pt[i] = rotate(angle, points[i]);
		pt[i].x += x;
		pt[i].y += y;
	}
	for (int i = 0; i < 3; i++) {
        struct point_t* pt0 = &pt[i];
        struct point_t* pt1 = &pt[(i + 1) % 3];
        draw_line(pt0->x, pt0->y, pt1->x, pt1->y, color);
    }
	struct point_t pt_mid = {(pt[1].x+x)>>1, (pt[1].y+y)>>1};
	fill_simple(pt_mid.x, pt_mid.y, color);
}


float get_distance(struct location_t a, struct location_t b)
{
    float a_lat = to_radian(a.latitude);
    float a_lon = to_radian(a.longitude);
    float b_lat = to_radian(b.latitude);
    float b_lon = to_radian(b.longitude);

    const int RAD = 6372795;

    float cl1 = cosf(a_lat);
    float cl2 = cosf(b_lat);
    float sl1 = sinf(a_lat);
    float sl2 = sinf(b_lat);
    float delta = b_lon - a_lon;
    float cdelta = cosf(delta);
    float sdelta = sinf(delta);
    float y = sqrtf(powf(cl2 * sdelta, 2) + powf(cl1 * sl2 - sl1 * cl2 * cdelta, 2));
    float x = sl1 * sl2 + cl1 * cl2 * cdelta;
    float ad = atan2f(y, x);
    return ad * RAD;
}

void get_distance_string_to_waypoint(char* buffer)
{
	if (appdata->target_location.longitude == 0.0 && appdata->target_location.latitude == 0.0) { 
		_sprintf(buffer, "-");
		return;
	}
    float distance = get_distance(appdata->target_location, appdata->current_location);
    if (distance > 1e6)
        _sprintf(buffer, "%d км", (int)(distance / 1000));
    else if (distance > 1000)
        _sprintf(buffer, "%.2f км", distance / 1000.0);
    else
        _sprintf(buffer, "%d м", (int)distance);
}

void show_time(int x, int y)
{
    struct datetime_ datetime;
    struct res_params_ res_params;

    get_current_date_time(&datetime);
    int nres = datetime.hour / 10;
    get_res_params(ELF_INDEX_SELF, nres, &res_params);
    show_elf_res_by_id(ELF_INDEX_SELF, nres, x, y);
    x += res_params.width + 2;
    nres = datetime.hour % 10;
    get_res_params(ELF_INDEX_SELF, nres, &res_params);
    show_elf_res_by_id(ELF_INDEX_SELF, nres, x, y);
    x += res_params.width + 2;
    show_elf_res_by_id(ELF_INDEX_SELF, 10, x, y+2);
    x += 4;
    nres = datetime.min / 10;
    get_res_params(ELF_INDEX_SELF, nres, &res_params);
    show_elf_res_by_id(ELF_INDEX_SELF, nres, x, y);
    x += res_params.width + 2;
    nres = datetime.min % 10;
    show_elf_res_by_id(ELF_INDEX_SELF, nres, x, y);
}

int get_decimal_max(int value)
{
    if (value == 0)
        return 1;

    int max = 100000;

    while (max)
    {
        if (value / max)
            break;
        max = max / 10;
    }
    return max;
}

void show_speed(int x, int y)
{
    struct res_params_ res_params;

    // kmh
    show_elf_res_by_id(ELF_INDEX_SELF, 27, x, y);
    // speed int part
    int value = (int)appdata->speed;
    int max = get_decimal_max(value);
    while (max)
    {
        int d = value / max;
        get_res_params(ELF_INDEX_SELF, 11 + d, &res_params);
        show_elf_res_by_id(ELF_INDEX_SELF, 11 + d, x, y + 12);
        x += res_params.width + 3;
        value = value % max;
        max = max / 10;
    }
    show_elf_res_by_id(ELF_INDEX_SELF, 21, x, y + 12 + 18);
    x += 5;
    int d = ((int)(appdata->speed * 10)) % 10;
    show_elf_res_by_id(ELF_INDEX_SELF, 11 + d, x, y + 12);
}

void show_distance(int x, int y)
{
    struct res_params_ res_params;

    float distance = get_distance(appdata->target_location, appdata->current_location);
    int nres = distance > 1e3 ? 26 : 28;
    get_res_params(ELF_INDEX_SELF, nres, &res_params);
    show_elf_res_by_id(ELF_INDEX_SELF, nres, x - res_params.width, y);

    if (distance > 1e3 && distance < 1e6)
    {
        int d = ((int)((distance + 50) / 100)) % 10;
        get_res_params(ELF_INDEX_SELF, 11+d, &res_params);
        x -= res_params.width;
        show_elf_res_by_id(ELF_INDEX_SELF, 11 + d, x, y + 12);
        x -= 5;
        show_elf_res_by_id(ELF_INDEX_SELF, 21, x, y + 12 + 18);
        x -= 2;
    }

    int value = (int)(distance >= 1e3 ? distance / 1000 : distance);

    do
    { 
        int d = value % 10;
        get_res_params(ELF_INDEX_SELF, 11 + d, &res_params);
        x -= res_params.width;
        show_elf_res_by_id(ELF_INDEX_SELF, 11 + d, x, y + 12);
        x -= 3;
        value = value / 10;
    } while (value);

}

void show_battery(int x, int y)
{
    struct res_params_ res_params;
    get_res_params(ELF_INDEX_SELF, 29, &res_params);
    x -= res_params.width;
    show_elf_res_by_id(ELF_INDEX_SELF, 29, x, y);

#ifdef BIPEMULATOR
	word battery_percentage = 80;
#else
	int battery_percentage = get_battery_charge();
#endif

	int r_count = battery_percentage / 20;
	r_count = r_count > 4 ? 4 : r_count < 1 ? 1 : r_count;
	set_bg_color(battery_percentage < 20 ? COLOR_RED : COLOR_GREEN);
	for (int i = 0; i < r_count; i++) {
		draw_filled_rect_bg(x + 2 + i * 5, y + 2, x + 5 + i * 5, y + 9);
	}

	x -= 3;

	do {
		int d = battery_percentage % 10;
		get_res_params(ELF_INDEX_SELF, d, &res_params);
		x -= res_params.width;
		show_elf_res_by_id(ELF_INDEX_SELF, d, x, y + 1);
		x -= 2;
		battery_percentage = battery_percentage / 10;
	} while (battery_percentage);
}

int normalize_degree(int angle)
{
    while(angle < 0)
        angle += 360;
    angle = angle % 360;
    return angle;
}

const int SQUARE_HALF_SIZE = 4;

void draw_main_screen()
{
    char buffer[32];
    int point_dir = 0, delta_dir = 0;

    // Время
    show_time(5, 9);

    // Батарея
    show_battery(170, 8);

	bool is_show_distance = appdata->target_valid && appdata->gps_valid;

	if (!is_show_distance) {
		// Рисуем в нижнем правом углу стрелку "вниз"
		struct res_params_ res_params;
		get_res_params(ELF_INDEX_SELF, RES_ARROW_DOWN, &res_params);
		show_elf_res_by_id(ELF_INDEX_SELF, RES_ARROW_DOWN, VIDEO_X - 5 - res_params.width, VIDEO_Y - 5 - res_params.height);
	}

	// Необходимо компас откалибровать
	if (!appdata->compass_calibrated) {
		struct res_params_ res_params;
		get_res_params(INDEX_MAIN_RES, 286, &res_params);
		int x = (VIDEO_X - res_params.width) >> 1;
		int y = ((VIDEO_Y - res_params.height) >> 1) - 20;
		show_elf_res_by_id(INDEX_MAIN_RES, 286, x, y);
		y += res_params.height;
		set_bg_color(COLOR_BLACK);
		set_fg_color(COLOR_YELLOW);
		text_out_center("Нужна калибровка", VIDEO_X >> 1, y + ((VIDEO_Y - y - get_text_height()) >> 1));
		return;
	}

	draw_circle_compass(88, 88, COLOR_SH_YELLOW);

    if (appdata->target_valid)
    {
        point_dir = normalize_degree(270 - get_direction(appdata->target_location, appdata->current_location));
        delta_dir = normalize_degree(point_dir - appdata->azimuth);
        // Указываем на точку возврата
        draw_arrow(88, 88, COLOR_SH_WHITE, delta_dir);
		fill(88, 80, COLOR_SH_WHITE);
		
		// Направление на Север
		set_fg_color(COLOR_AQUA);
		const int len = COMPASS_ARROW_LENGTH + 5;
		float ncv = to_radian(normalize_degree(-appdata->azimuth));
		int x = 88 + len * sinf(ncv);
		int y = 88 - len * cosf(ncv);		
		draw_filled_rect(x - SQUARE_HALF_SIZE, y - SQUARE_HALF_SIZE, x + SQUARE_HALF_SIZE, y + SQUARE_HALF_SIZE);
    }
    else
    {
        // если точка не выбрана, то работаем как компас
		draw_arrow_compass(88, 88, COLOR_SH_BLUE, normalize_degree(-appdata->azimuth));
		draw_arrow_compass(88, 88, COLOR_SH_RED, normalize_degree(180-appdata->azimuth));
    }

    // Скорость
    if (appdata->gps_valid)
        show_speed(5, 135);

    // Расстояние
    if (is_show_distance)
        show_distance(170, 135);
	
	// Значок готовности GPS
	show_elf_res_by_id(ELF_INDEX_SELF, appdata->gps_valid ? RES_GPS_ON : RES_GPS_OFF, 5, 25);
}

