#pragma once
#include <stdint.h>
#include "driver/spi_master.h"

#define LCD_WIDTH  160
#define LCD_HEIGHT 128

extern spi_device_handle_t spi;

void lcd_init(void);
void lcd_clear_screen(uint16_t color);
void lcd_draw_text_fast(const char *text, int x, int y, uint16_t fg_color, uint16_t bg_color);
void lcd_draw_square_fast(int center_x, int center_y, int size_param, uint16_t color);
void lcd_draw_point_in_square(int x, int y, uint16_t color);
void lcd_draw_2x2(int x, int y, uint16_t color);
void lcd_draw_hline_fast(int x_start, int x_end, int y, uint16_t color);
void lcd_draw_vline_fast(int y_start, int y_end, int x, uint16_t color);