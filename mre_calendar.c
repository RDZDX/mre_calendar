#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include <time.h>
#include "vmgraph.h"

#define CELL_W  30 //40
#define CELL_H  20 //30
#define START_X 10 //10 tarpas tarp sono ir 2025
#define START_Y 45 //40 zemiau metu (nuo to lygio prasideda Su Mo Tu... nuo virsutinio remelio +40 prasideda tekstas Su Mo Tu...
#define COLS    7
#define ROWS    6

VMINT layer_hdl[1];

VMINT layer_width = 0, layer_height = 0;

typedef struct {
    VMINT year;
    VMINT month; /* 1..12 */
} YearMonth;

static YearMonth g_ym;

void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);
static YearMonth current_year_month(void);
static VMINT days_in_month(VMINT year, VMINT month);
static VMINT weekday_of_first(VMINT year, VMINT month);
static void draw_calendar(void);
static void fill_white(void);

void vm_main(void) {

    layer_hdl[0] = -1;
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    vm_font_set_font_size(VM_SMALL_FONT);
    g_ym = current_year_month();
}

void handle_sysevt(VMINT message, VMINT param) {

    switch (message) {
        case VM_MSG_CREATE:
        case VM_MSG_ACTIVE:
            layer_hdl[0] = vm_graphic_create_layer(0, 0, vm_graphic_get_screen_width(), vm_graphic_get_screen_height(), -1);
            fill_white();
            break;

        case VM_MSG_PAINT:
            vm_switch_power_saving_mode(turn_off_mode);
            draw_calendar();
            break;

        case VM_MSG_INACTIVE:
            vm_switch_power_saving_mode(turn_on_mode);
            if (layer_hdl[0] != -1)
                vm_graphic_delete_layer(layer_hdl[0]);
            break;

        case VM_MSG_QUIT:
            if (layer_hdl[0] != -1)
                vm_graphic_delete_layer(layer_hdl[0]);
            break;
    }
}

void handle_keyevt(VMINT event, VMINT keycode) {

    if (event == VM_KEY_EVENT_UP && keycode == VM_KEY_RIGHT_SOFTKEY) {
        if (layer_hdl[0] != -1) {
            vm_graphic_delete_layer(layer_hdl[0]);
            layer_hdl[0] = -1;
        }
        vm_exit_app();
    }

    if (event == VM_KEY_EVENT_UP && keycode == VM_KEY_RIGHT) {
        fill_white();
        g_ym.month++;
        if (g_ym.month > 12) { g_ym.month = 1; g_ym.year++; }
        draw_calendar();
    }

    if (event == VM_KEY_EVENT_UP && keycode == VM_KEY_LEFT) {
        fill_white();
        g_ym.month--;
        if (g_ym.month < 1) { g_ym.month = 12; g_ym.year--; }
        draw_calendar();
    }
}

static YearMonth current_year_month(void) {

    vm_time_t t;
    vm_get_time(&t);

    YearMonth ym;
    ym.year = t.year;
    ym.month = t.mon;
    return ym;
}

static VMINT days_in_month(VMINT year, VMINT month) {

    static const VMINT mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    VMINT d = mdays[month-1];
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            d = 29;
    }
    return d;
}

static VMINT weekday_of_first(VMINT year, VMINT month) { /* Zellerio formulė (0=Sunday..6=Saturday) */

    if (month < 3) {
        month += 12;
        year--;
    }
    VMINT q = 1;
    VMINT k = year % 100;
    VMINT j = year / 100;
    VMINT h = (q + (13*(month + 1))/5 + k + (k/4) + (j/4) + 5*j) % 7;
    VMINT d = ((h + 6) % 7); // 0 = Sunday
    return d;
}

static void draw_calendar(void) {

    int x;
    int r;
    int c;

    VMWCHAR text1[100] = {0};
    VMWCHAR text2[100] = {0};
    VMCHAR wdays[7][3] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
    VMWCHAR wdays1[7][6]; // 10?  vietos 7 UCS2 stringams
    vm_graphic_color color;

    VMCHAR title[32];
    vm_sprintf(title, "%04d-%02d", g_ym.year, g_ym.month);
    vm_ascii_to_ucs2(text1, sizeof(text1), title);

    color.vm_color_565 = VM_COLOR_WHITE;

    vm_graphic_setcolor(&color);

    vm_graphic_textout_to_layer(layer_hdl[0], 10, 10, text1, vm_graphic_get_character_height());

    for (x=0; x<7; ++x) {
        vm_ascii_to_ucs2(wdays1[x], sizeof(wdays1[x]), wdays[x]);
	vm_graphic_textout_to_layer(layer_hdl[0], START_X + x*CELL_W + 5, START_Y - 15, wdays1[x], vm_graphic_get_character_height());
    }

    int dim = days_in_month(g_ym.year, g_ym.month);
    int firstW = weekday_of_first(g_ym.year, g_ym.month);

    VMINT day = 1;
    for (r=0; r<ROWS && day<=dim; ++r) {
        for (c=0; c<COLS && day<=dim; ++c) {
            if (r == 0 && c < firstW) continue;

            VMINT x = START_X + c*CELL_W;
            VMINT y = START_Y + r*CELL_H;
            //vm_graphic_rect_ex(layer_hdl[0], x, y, x+CELL_W-2, y+CELL_H-2); // piesia iskraipytus remelius !

            VMCHAR buf[8];
            vm_sprintf(buf, "%d", day);
            vm_ascii_to_ucs2(text2, sizeof(text2), buf);
	    vm_graphic_textout_to_layer(layer_hdl[0], x + 5, y + 5, text2, vm_graphic_get_character_height());
            day++;
        }
    }

    vm_graphic_flush_layer(layer_hdl, 1);
}

static void fill_white(void) {

    vm_graphic_color color;
    //color.vm_color_565 = VM_COLOR_WHITE;
    color.vm_color_565 = VM_COLOR_BLUE;
    vm_graphic_setcolor(&color);

    if (layer_hdl[0] != -1) {
        vm_graphic_fill_rect_ex(layer_hdl[0], 0, 0, vm_graphic_get_screen_width(), vm_graphic_get_screen_height());
        vm_graphic_flush_layer(layer_hdl, 1);
    }
}