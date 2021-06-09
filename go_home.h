#ifndef __GO_HOME__
#define __GO_HOME__

//#define DEBUG

#include "libbip.h"
#include "compass.h"

typedef unsigned short word;
typedef unsigned char byte;

struct location_t
{
    float latitude;
    float longitude;
};

struct appdata_t
{
    Elf_proc_* proc;
    void* ret_f;
    int screen;
    struct location_t current_location;
	struct location_t previous_location;
    float speed;
    // in degree
    int azimuth;
    int ticks;

	// persistent information
	int target_valid;
	struct location_t target_location;
	
	// state
	int sensors_on;
	int gps_valid;
	int compass_calibrated;
	int navi_initialized;
};

extern struct appdata_t** appdata_p;
extern struct appdata_t* appdata;

#define DELTA_TICKS_FOR_SPEED 	(510 * 5)

// res

#define RES_ACCEPT 30
#define RES_ADD 31
#define RES_DEL 32
#define RES_GPS_ON 33
#define RES_GPS_OFF 34
#define RES_ARROW_UP 35
#define RES_ARROW_DOWN 36

// screen's

#define SCREEN_MAIN 	0
#define SCREEN_STATE 	1

extern float PI;

// prototypes

void show_screen(void* return_screen);
void keypress_screen();
int dispatch_screen(void* p);
void draw_screen();
void screen_job();
void read_settings();
void write_settings();

extern float get_distance(struct location_t a, struct location_t b);
extern void get_distance_string_to_waypoint(char* buffer);
float normalize_radians(float angle);
int get_direction(struct location_t from, struct location_t to);

#endif
