#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK 5
#define PIN_NUM_CS 6
#define PIN_NUM_DC 7
#define PIN_NUM_RST 8

spi_device_handle_t spi;

extern const uint8_t font8x8_basic[128][8] = {
    ['H'] = {0b10000010, 0b10000010, 0b10000010, 0b11111110, 0b10000010, 0b10000010, 0b10000010, 0b00000000},
    ['E'] = {0b11111110, 0b10000000, 0b10000000, 0b11111110, 0b10000000, 0b10000000, 0b11111110, 0b00000000},
    ['L'] = {0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b11111110, 0b00000000},
    ['O'] = {0b01111100, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    [' '] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
    ['W'] = {0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10010010, 0b10101010, 0b11000110, 0b00000000},
    ['R'] = {0b11111100, 0b10000010, 0b10000010, 0b11111100, 0b10001000, 0b10000100, 0b10000010, 0b00000000},
    ['D'] = {0b11111100, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b11111100, 0b00000000},
    ['*'] = {0b00001000, 0b00000100, 0b11100010, 0b00000010, 0b11100010, 0b00000100, 0b00001000, 0b00000000},
    ['-'] = {0b00000000, 0b00000000, 0b00000000, 0b01111100, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
    ['0'] = {0b01111100, 0b10000110, 0b10001010, 0b10010010, 0b10100010, 0b11000010, 0b01111100, 0b00000000},
    ['1'] = {0b00010000, 0b00110000, 0b01010000, 0b00010000, 0b00010000, 0b00010000, 0b01111100, 0b00000000},
    ['2'] = {0b01111100, 0b10000010, 0b00000010, 0b00011100, 0b01100000, 0b10000000, 0b11111110, 0b00000000},
    ['3'] = {0b11111110, 0b00000010, 0b00000100, 0b00011100, 0b00000010, 0b10000010, 0b01111100, 0b00000000},
    ['4'] = {0b00001100, 0b00110100, 0b01000100, 0b10000100, 0b11111110, 0b00000100, 0b00000100, 0b00000000},
    ['5'] = {0b11111110, 0b10000000, 0b11111100, 0b00000010, 0b00000010, 0b10000010, 0b01111100, 0b00000000},
    ['6'] = {0b00111100, 0b01000000, 0b10000000, 0b11111100, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    ['7'] = {0b11111110, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b00000000},
    ['8'] = {0b01111100, 0b10000010, 0b10000010, 0b01111100, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    ['9'] = {0b01111100, 0b10000010, 0b10000010, 0b01111110, 0b00000010, 0b00000100, 0b01111000, 0b00000000},
    ['X'] = {0b10000010, 0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100, 0b10000010, 0b00000000},
    ['Y'] = {0b10000010, 0b01000100, 0b00101000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00000000},
    ['+'] = {0b00010000, 0b00010000, 0b00010000, 0b11111110, 0b00010000, 0b00010000, 0b00010000, 0b00000000},
    [':'] = {0b00000000, 0b00000000, 0b00010000, 0b00000000, 0b00010000, 0b00000000, 0b00000000, 0b00000000},
    ['S'] = {0b00111100, 0b01000010, 0b10000000, 0b01111100, 0b00000010, 0b10000010, 0b01111100, 0b00000000}};
static void lcd_send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd};
    spi_device_polling_transmit(spi, &t);
}

static void lcd_send_data(uint8_t data)
{
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data};
    spi_device_polling_transmit(spi, &t);
}

static void lcd_reset()
{
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_send_cmd(0x2A);
    lcd_send_data(0x00);
    lcd_send_data(x0);
    lcd_send_data(0x00);
    lcd_send_data(x1);

    lcd_send_cmd(0x2B);
    lcd_send_data(0x00);
    lcd_send_data(y0);
    lcd_send_data(0x00);
    lcd_send_data(y1);

    lcd_send_cmd(0x2C);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;

    lcd_set_window(x, y, x, y);
    gpio_set_level(PIN_NUM_DC, 1);

    uint8_t data[] = {color >> 8, color & 0xFF};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data};
    spi_device_polling_transmit(spi, &t);
}

void lcd_clear_screen(uint16_t color)
{
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    gpio_set_level(PIN_NUM_DC, 1);
    uint8_t line[LCD_WIDTH * 2];
    for (int i = 0; i < LCD_WIDTH; i++)
    {
        line[i * 2] = color >> 8;
        line[i * 2 + 1] = color & 0xFF;
    }
    for (int y = 0; y < LCD_HEIGHT; y++)
    {
        spi_transaction_t t = {
            .length = LCD_WIDTH * 2 * 8,
            .tx_buffer = line};
        spi_device_polling_transmit(spi, &t);
    }
}

void lcd_init()
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST),
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .max_transfer_sz = 4096,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 15 * 1000 * 1000, // Up to 15 МГц
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    lcd_reset();
    lcd_send_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_send_cmd(0x3A);
    lcd_send_data(0x05); // 16-bit color
    lcd_send_cmd(0x36);
    lcd_send_data(0x60);
    lcd_send_cmd(0x29); // Display ON
}
static void lcd_draw_char_fast(uint8_t ch, int x, int y, uint16_t fg_color, uint16_t bg_color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    if (x + 7 >= LCD_WIDTH || y + 7 >= LCD_HEIGHT)
        return;
    const uint8_t *glyph = font8x8_basic[ch];
    uint8_t buffer[64 * 2];
    int index = 0;
    for (int row = 0; row < 8; row++)
    {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++)
        {
            uint16_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;
            buffer[index++] = color >> 8;
            buffer[index++] = color & 0xFF;
        }
    }
    lcd_set_window(x, y, x + 7, y + 7);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 128 * 8,
        .tx_buffer = buffer};
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_text_fast(const char *text, int x, int y, uint16_t fg_color, uint16_t bg_color)
{
    int current_x = x;
    while (*text && current_x < LCD_WIDTH)
    {
        lcd_draw_char_fast(*text, current_x, y, fg_color, bg_color);
        current_x += 9;
        text++;
    }
}

void lcd_draw_2x2(int x, int y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
        return;
    if (x + 1 >= LCD_WIDTH || y + 1 >= LCD_HEIGHT)
        return;
    uint8_t buffer[4 * 2];
    for (int i = 0; i < 4; i++)
    {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(x, y, x + 1, y + 1);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 8 * 8,
        .tx_buffer = buffer};
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_hline_fast(int x_start, int x_end, int y, uint16_t color)
{
    if (y < 0 || y >= LCD_HEIGHT)
        return;
    int draw_x_start = x_start < 0 ? 0 : x_start;
    int draw_x_end = x_end > LCD_WIDTH - 1 ? LCD_WIDTH - 1 : x_end;
    if (draw_x_start > draw_x_end)
        return;
    int width = draw_x_end - draw_x_start + 1;
    uint8_t buffer[width * 2];
    for (int i = 0; i < width; i++)
    {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(draw_x_start, y, draw_x_end, y);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = width * 16,
        .tx_buffer = buffer};
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_vline_fast(int y_start, int y_end, int x, uint16_t color)
{
    if (x < 0 || x >= LCD_WIDTH)
        return;
    int draw_y_start = y_start < 0 ? 0 : y_start;
    int draw_y_end = y_end > LCD_HEIGHT - 1 ? LCD_HEIGHT - 1 : y_end;
    if (draw_y_start > draw_y_end)
        return;
    int height = draw_y_end - draw_y_start + 1;
    uint8_t buffer[height * 2];
    for (int i = 0; i < height; i++)
    {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(x, draw_y_start, x, draw_y_end);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = height * 16,
        .tx_buffer = buffer};
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_square_fast(int center_x, int center_y, int size_param, uint16_t color)
{
    int size = size_param + 4;
    int half_size = size / 2;
    int top_y = center_y - half_size;
    int bottom_y = center_y + half_size;
    int left_x = center_x - half_size;
    int right_x = center_x + half_size;
    lcd_draw_hline_fast(left_x, right_x - 1, top_y, color);
    lcd_draw_hline_fast(left_x, right_x - 1, bottom_y, color);
    lcd_draw_vline_fast(top_y + 1, bottom_y - 1, left_x, color);
    lcd_draw_vline_fast(top_y + 1, bottom_y - 1, right_x, color);
}

void lcd_draw_point_in_square(int x, int y, uint16_t color)
{
    lcd_draw_2x2(x, y, color);
}
