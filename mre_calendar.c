#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include <time.h>
#include "vmgraph.h"

#define CELL_W  30 //40
#define CELL_H  20 //30
#define COLS    7
#define ROWS    6

VMINT layer_hdl[1];

VMINT layer_width = 0;
VMINT layer_height = 0;

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
static void fill_white(void);
static void draw_calendar(void);

void vm_main(void) {

    layer_hdl[0] = -1;
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    vm_font_set_font_size(VM_SMALL_FONT);
    vm_font_set_font_style(TRUE, FALSE, FALSE);
    g_ym = current_year_month();
    layer_width = vm_graphic_get_screen_width();
    layer_height = vm_graphic_get_screen_height();
}

void handle_sysevt(VMINT message, VMINT param) {

    switch (message) {
        case VM_MSG_CREATE:
        case VM_MSG_ACTIVE:
            layer_hdl[0] = vm_graphic_create_layer(0, 0, layer_width, layer_height, -1);
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
        g_ym.month++;
        if (g_ym.month > 12) { g_ym.month = 1; g_ym.year++; }
        fill_white();
        draw_calendar();
    }

    if (event == VM_KEY_EVENT_UP && keycode == VM_KEY_LEFT) {
        g_ym.month--;
        if (g_ym.month < 1) { g_ym.month = 12; g_ym.year--; }
        fill_white();
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
    //d = (d + 6) % 7;         // 0 = Monday
    //return d;
    return (d == 0 ? 6 : d - 1); // MON=0 ... SUN=6
}

static void fill_white(void) {

    vm_graphic_color color;
    //color.vm_color_565 = VM_COLOR_WHITE;
    color.vm_color_565 = VM_COLOR_BLUE;
    vm_graphic_setcolor(&color);

    if (layer_hdl[0] != -1) {
        vm_graphic_fill_rect_ex(layer_hdl[0], 0, 0, layer_width, layer_height);
        vm_graphic_flush_layer(layer_hdl, 1);
    }
}

static void draw_calendar(void) {

    vm_graphic_color col;
    VMWCHAR text[16];
    VMCHAR ascii[16];
    
    VMINT title_y = 20;
    VMINT cal_w = COLS * CELL_W;
    VMINT cal_h = ROWS * CELL_H + title_y;
    VMINT start_x = (layer_width - cal_w) / 2; // 240 - kalendroriaus_plotis
    //VMINT start_y = (layer_height - cal_h) / 2; //3title_y - kalendroriaus_aukstis
    VMINT start_y = title_y * 3;

    vm_sprintf(ascii, "%04d-%02d", g_ym.year, g_ym.month);
    vm_ascii_to_ucs2(text, sizeof(text), ascii);
    col.vm_color_565 = VM_COLOR_WHITE;
    vm_graphic_setcolor(&col);
    vm_graphic_textout_to_layer(layer_hdl[0], start_x, start_y - title_y, text, vm_graphic_get_character_height());

    VMCHAR *wd[7] = {"Mo","Tu","We","Th","Fr","Sa","Su"};
    int i;
    for (i=0; i<7; i++) {
        vm_ascii_to_ucs2(text, sizeof(text), wd[i]);
        if (i >= 5) { col.vm_color_565 = VM_COLOR_RED; }
        else { col.vm_color_565 = VM_COLOR_WHITE; }
        vm_graphic_setcolor(&col);
        vm_graphic_textout_to_layer(layer_hdl[0], start_x + i*CELL_W + 5, start_y, text, vm_graphic_get_character_height());
    }

    VMINT dim = days_in_month(g_ym.year, g_ym.month);
    VMINT firstW = weekday_of_first(g_ym.year, g_ym.month);
    VMINT day = 1;

    int r;
    int c;

    for (r=0; r<ROWS && day<=dim; r++) {
        for (c=0; c<COLS && day<=dim; c++) {
            if (r == 0 && c < firstW) continue;
            VMINT x = start_x + c*CELL_W;
            VMINT y = start_y + title_y + r*CELL_H;

            col.vm_color_565 = VM_COLOR_WHITE;
            vm_graphic_setcolor(&col);
            //vm_graphic_rect_ex(layer_hdl[0], x, y, x+CELL_W-1, y+CELL_H-1);

            vm_sprintf(ascii, "%d", day);
            vm_ascii_to_ucs2(text, sizeof(text), ascii);

            if (c >= 5) col.vm_color_565 = VM_COLOR_RED;
            else col.vm_color_565 = VM_COLOR_WHITE;
            vm_graphic_setcolor(&col);
            vm_graphic_textout_to_layer(layer_hdl[0], x + 5, y + 3, text, vm_graphic_get_character_height());
            day++;
        }
    }
    vm_graphic_flush_layer(layer_hdl, 1);
}