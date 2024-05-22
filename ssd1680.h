
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_spi.h"
#include "luat_gpio.h"

#include "ssd1680_regmap.h"
#include "font.h"


typedef enum
{
    font_size_8 = 8,
    font_size_12 = 12,
    font_size_16 = 16,
    font_size_24 = 24,
} ssd1680_fontSize_t;

typedef enum
{
    SSD1680_BLACK = 0x00,
    SSD1680_WHITE = 0xFF,
    SSD1680_RED = 0x01,
} ssd1680_color_t;

typedef enum
{
    TYPE_TEMP_HUMI = 0,
    TYPE_LIGHT = 1,
    TYPE_TEMP_HUMI_LIGHT = 2,
    TYPE_DS18B20 = 3,
    TYPE_CO2 = 4,
    TYPE_SOIL_TEMP_HUMI = 5,
    TYPE_PM_MODE = 6,
    TYPE_LOGO = 7,

} background_type_t;

typedef enum
{
    SSD1680_Global_Refresh = 0x00,
    SSD1680_Local_Refresh,
} ssd1680_refresh_t;

typedef enum
{
    SSD1680_NORMAL,
    SSD1680_90_DEG,
    SSD1680_180_DEG,
    SSD1680_270_DEG,
} ssd1680_orientation_t;

typedef struct
{
    int busy;
    int reset;
    int dc;
    int cs;
    int mosi;
    int sck;
} ssd1680_pinmap_t;

typedef struct
{
    void *spi_host;

    ssd1680_pinmap_t pinmap;
    ssd1680_orientation_t orientation;

    uint8_t framebuffer_bw[5220];
    // uint8_t* framebuffer_bw;
    // uint8_t *framebuffer_red;
    // uint8_t framebuffer_red[2048];
    uint32_t framebuffer_size;
    uint16_t pos_x, pos_y;
    uint16_t res_x, res_y;
    uint16_t rows_cnt, clmn_cnt;
} ssd1680_t;

int ssd1680_init(void *spi_host, ssd1680_pinmap_t pinmap, uint16_t res_x, uint16_t res_y, ssd1680_orientation_t orientation);
void ssd1680_deinit();

void ssd1680_sleep();
void ssd1680_wakeup();

void ssd1680_fill(ssd1680_color_t color);
void ssd1680_set_pixel(uint16_t x, uint16_t y, ssd1680_color_t color);
void ssd1680_send_framebuffer();

void ssd1680_set_refresh_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ssd1680_refresh(ssd1680_refresh_t refresh);

void ssd1680_show_char(uint16_t x, uint16_t y, uint16_t chr, uint16_t size1, ssd1680_color_t color);
void ssd1680_show_string(uint16_t x, uint16_t y, uint8_t *chr, uint16_t size1, ssd1680_color_t color);
void ssd1680_show_number(uint16_t x, uint16_t y, uint32_t num, uint16_t size1, ssd1680_color_t color);
void ssd1680_show_chinese(uint16_t x, uint16_t y, uint16_t num, uint16_t size1, ssd1680_color_t color);
void ssd1680_show_picture(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t BMP[], ssd1680_color_t color);
