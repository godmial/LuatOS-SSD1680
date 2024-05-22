
#include "ssd1680.h"
#include "luat_mem.h"

static ssd1680_t default_disp = {0};

static inline uint8_t getNumberCount(uint32_t number)
{
    if (0 == number)
        return 1;

    uint8_t count = 0;
    for (uint32_t i = number; i != 0; i /= 10, count++)
    {
    }

    return count;
}

typedef struct
{
    uint16_t mux : 9;
    uint16_t : 7;
    uint8_t gd : 1;
    uint8_t sm : 1;
    uint8_t tb : 1;
    uint8_t : 5;
} ssd1680_gate_t;

typedef struct
{
    uint8_t start;
    uint8_t stop;
} ssd1680_x_window_t;

typedef struct
{
    uint16_t start;
    uint16_t stop;
} ssd1680_y_window_t;

static void ssd1680_wait_busy()
{
    luat_rtos_task_sleep(1);
    // LOW: idle, HIGH: busy
    for (uint32_t count = 0; luat_gpio_get(default_disp.pinmap.busy) == LUAT_GPIO_HIGH && count < 2000; count++)
    {
        luat_rtos_task_sleep(1);
    }
}

static void ssd1680_bus_wr(uint8_t dat)
{
    uint8_t i;

#define ssd1680_scl_clr() luat_gpio_set(default_disp.pinmap.sck, LUAT_GPIO_LOW) // sck
#define ssd1680_scl_set() luat_gpio_set(default_disp.pinmap.sck, LUAT_GPIO_HIGH)

#define ssd1680_sda_clr() luat_gpio_set(default_disp.pinmap.mosi, LUAT_GPIO_LOW) // mosi
#define ssd1680_sda_set() luat_gpio_set(default_disp.pinmap.mosi, LUAT_GPIO_HIGH)

    for (i = 0; i < 8; i++)
    {
        ssd1680_scl_clr();
        if (dat & 0x80)
            ssd1680_sda_set();

        else
            ssd1680_sda_clr();

        ssd1680_scl_set();
        dat <<= 1;
    }
}

static void ssd1680_write(ssd1360_regmap_t cmd, void *data, size_t data_size)
{

    luat_gpio_set(default_disp.pinmap.dc, LUAT_GPIO_LOW);
    luat_rtos_task_sleep(1);
    if (1 != luat_spi_send(0, &cmd, 1))
    {
        LUAT_DEBUG_PRINT("spi cmd写入失败");
    }
    if (data == NULL || data_size == 0)
        return;

    luat_gpio_set(default_disp.pinmap.dc, LUAT_GPIO_HIGH);
    luat_rtos_task_sleep(1);

    if (data_size != luat_spi_send(0, data, data_size))
    {
        LUAT_DEBUG_PRINT("spi data写入失败");
    }
}

static void ssd1680_hw_reset()
{
    luat_gpio_set(default_disp.pinmap.reset, LUAT_GPIO_LOW);
    luat_rtos_task_sleep(20);
    luat_gpio_set(default_disp.pinmap.reset, LUAT_GPIO_HIGH);
    luat_rtos_task_sleep(20);
    ssd1680_wait_busy(default_disp);
}

static void ssd1680_sw_reset()
{
    ssd1680_write(SSD1680_SW_RESET, NULL, 0);
    luat_rtos_task_sleep(10);
    ssd1680_wait_busy();
}

static void ssd1680_set_x_window(uint16_t x_start, uint16_t x_stop)
{
    uint8_t wnd[2] = {
        x_start & 0x1F,
        x_stop & 0x1F,
    };
    ssd1680_write(SSD1680_SET_RAM_X_ADDR, &wnd, sizeof(ssd1680_x_window_t));
    ssd1680_wait_busy();
}

static void ssd1680_set_y_window(uint16_t y_start, uint16_t y_stop)
{
    uint8_t wnd[4] = {
        y_start & 0xFF,
        (y_start >> 8) & 0x01,
        y_stop & 0xFF,
        (y_stop >> 8) & 0x01,
    };
    ssd1680_write(SSD1680_SET_RAM_Y_ADDR, &wnd, sizeof(wnd));
    ssd1680_wait_busy();
}

static void ssd1680_set_ram_pos(uint16_t x_pos, uint16_t y_pos)
{
    default_disp.pos_x = x_pos & 0x1F;
    default_disp.pos_y = y_pos & 0x1FF;
    ssd1680_write(SSD1680_SET_RAM_X_ADDR_CNT, &default_disp.pos_x, sizeof(uint8_t));
    ssd1680_wait_busy();
    ssd1680_write(SSD1680_SET_RAM_Y_ADDR_CNT, &default_disp.pos_y, sizeof(uint16_t));
    ssd1680_wait_busy();
}

static void ssd1680_setup_gate_driver()
{
    ssd1680_gate_t gate = {
        .mux = default_disp.rows_cnt - 1,
        .gd = 0,
        .sm = 0,
        .tb = 0,
    };
    ssd1680_write(SSD1680_DRIVER_OUTPUT_CTRL, &gate, sizeof(ssd1680_gate_t));
    ssd1680_wait_busy();
}

static void ssd1680_setup_border()
{
    uint8_t b = 0x5;
    ssd1680_write(SSD1680_BORDER_WAVEFORM_CTRL, &b, sizeof(uint8_t));
    ssd1680_wait_busy();
}

static void ssd1680_setup_booster()
{
    const uint8_t driver_strength = 0x2;
    const uint8_t min_off_time = 0x4;

    uint8_t booster[4] = {
        (1 << 7) | (driver_strength << 4) | (min_off_time),
        (1 << 7) | (driver_strength << 4) | (min_off_time),
        (1 << 7) | (driver_strength << 4) | (min_off_time),
        0x5,
    };
    ssd1680_write(SSD1680_BOOSTER_SOFT_START_CTRL, &booster, sizeof(booster));
    ssd1680_wait_busy();
}

static void ssd1680_setup_ram()
{
    // Set data entry mode
    uint8_t b;
    uint16_t xwnd_start, xwnd_stop;
    uint16_t ywnd_start, ywnd_stop;
    b = 0x5;
    xwnd_start = 0;
    xwnd_stop = default_disp.clmn_cnt - 1;
    ywnd_start = default_disp.rows_cnt - 1;
    ywnd_stop = 0;

    ssd1680_write(SSD1680_DATA_ENTRY_MODE, &b, sizeof(uint8_t));
    ssd1680_wait_busy();

    // Set draw window
    ssd1680_set_x_window(xwnd_start, xwnd_stop);
    ssd1680_set_y_window(ywnd_start, ywnd_stop);

    // Setup border waveform
    ssd1680_setup_border();

    // Setup update control
    uint8_t ctrl_1[2] = {0, 0x80};
    ssd1680_write(SSD1680_DISP_UPDATE_CTRL_1, &ctrl_1, sizeof(ctrl_1));
    ssd1680_wait_busy();
    uint8_t ctrl_2 = 0xF7;
    ssd1680_write(SSD1680_DISP_UPDATE_CTRL_2, &ctrl_2, sizeof(ctrl_2));
    ssd1680_wait_busy();
}

static void ssd1680_init_sequence()
{
    ssd1680_setup_gate_driver();
    ssd1680_setup_booster();
    ssd1680_setup_ram();
}

int ssd1680_init(void *spi_host, ssd1680_pinmap_t pinmap, uint16_t res_x, uint16_t res_y, ssd1680_orientation_t orientation)
{
    uint32_t result;

    memset(&default_disp, 0, sizeof(ssd1680_t)); // 填充0,保证无脏数据

    default_disp.spi_host = spi_host;
    default_disp.pinmap = pinmap;

    default_disp.orientation = orientation;
    default_disp.res_x = res_x;
    default_disp.res_y = res_y;

    default_disp.clmn_cnt = (res_y + 7) / 8;
    default_disp.rows_cnt = res_x;

    LUAT_DEBUG_PRINT("clmns: %d, rows: %d\r\n", default_disp.clmn_cnt, default_disp.rows_cnt);

    default_disp.framebuffer_size = default_disp.clmn_cnt * default_disp.rows_cnt;

    luat_gpio_cfg_t gpio_busy_cfg = {
        .pin = default_disp.pinmap.busy,
        .mode = LUAT_GPIO_INPUT,
        .output_level = LUAT_GPIO_HIGH,
        .pull = LUAT_GPIO_PULLUP};

    luat_gpio_cfg_t gpio_reset_cfg = {
        .pin = default_disp.pinmap.reset,
        .mode = LUAT_GPIO_OUTPUT,
        .pull = LUAT_GPIO_PULLUP,
        .output_level = LUAT_GPIO_HIGH};

    luat_gpio_cfg_t gpio_dc_cfg = {
        .pin = default_disp.pinmap.dc,
        .mode = LUAT_GPIO_OUTPUT,
        .pull = LUAT_GPIO_PULLUP,
        .output_level = LUAT_GPIO_HIGH};

    luat_gpio_cfg_t gpio_cs_cfg = {
        .pin = default_disp.pinmap.cs,
        .mode = LUAT_GPIO_OUTPUT,
        .pull = LUAT_GPIO_DEFAULT,
        .output_level = LUAT_GPIO_HIGH};

    luat_gpio_cfg_t gpio_scl_cfg = {
        .pin = 11,
        .mode = LUAT_GPIO_OUTPUT,
        .pull = LUAT_GPIO_PULLUP,
        .output_level = LUAT_GPIO_HIGH};

    luat_gpio_cfg_t gpio_sda_cfg = {
        .pin = 9,
        .mode = LUAT_GPIO_OUTPUT,
        .pull = LUAT_GPIO_PULLUP,
        .output_level = LUAT_GPIO_HIGH};

    luat_spi_t eink_spi_t = {
        .id = 0,
        .mode = 1,
        .CPHA = 0,
        .CPOL = 0,
        .dataw = 8,
        .bit_dict = 0,
        .master = 1,
        .bandrate = 40000,
        .cs = default_disp.pinmap.cs};

    result = luat_spi_setup(&eink_spi_t);
    if (0 != result)
    {
        LUAT_DEBUG_PRINT("初始化spi错误 %d", result);
    }

    luat_gpio_open(&gpio_dc_cfg);
    luat_gpio_open(&gpio_busy_cfg);
    luat_gpio_open(&gpio_reset_cfg);

    luat_gpio_set(default_disp.pinmap.dc, 1);
    luat_gpio_set(default_disp.pinmap.reset, 1);

    ssd1680_hw_reset();
    ssd1680_sw_reset();

    ssd1680_init_sequence();

    return 0;

error:
    memset(&default_disp, 0, sizeof(ssd1680_t)); // 填充0,保证无脏数据
    return NULL;
}

void ssd1680_deinit()
{
    memset(&default_disp, 0, sizeof(ssd1680_t)); // 填充0,保证无脏数据
    luat_spi_close(0);
}

void ssd1680_sleep()
{
    uint8_t mode = 0x01;
    ssd1680_write(SSD1680_DEEP_SLEEP_MODE, &mode, 1);
}

void ssd1680_wakeup()
{
    ssd1680_hw_reset();
    ssd1680_init_sequence();
    ssd1680_send_framebuffer();
}

void ssd1680_set_pixel(uint16_t x, uint16_t y, ssd1680_color_t color)
{

    int idx, offset;
    x -= 1;
    y -= 1;

    idx = (y >> 3);
    offset = 7 - (y - (idx << 3));
    idx = idx * default_disp.rows_cnt + x;

    default_disp.framebuffer_bw[idx] &= ~(1 << offset);
    default_disp.framebuffer_bw[idx] |= color << offset;
}

void ssd1680_fill(ssd1680_color_t color)
{
    memset(default_disp.framebuffer_bw, color, default_disp.framebuffer_size);
}

void ssd1680_send_framebuffer()
{

    ssd1680_set_ram_pos(0, default_disp.rows_cnt - 1);

    ssd1680_write(SSD1680_WRITE_RAM_BW, default_disp.framebuffer_bw, default_disp.framebuffer_size);
    ssd1680_wait_busy();
}

void ssd1680_set_refresh_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    ssd1680_set_x_window(y1 >> 3, y2 >> 3);
    ssd1680_set_y_window(x2, x1);
}

void ssd1680_refresh(ssd1680_refresh_t refresh)
{
    uint8_t mode = SSD1680_Global_Refresh == refresh ? 0xF7 : 0xFF;
    // F7 Mode1
    // FF Mode2 不频闪 直接显示  刷新很快 咨询客服之后 说是局部刷新 但是测试之后显示效果不好，有些许残影，建议用算法操作一下  参考裸屏规格书P17
    // 91 Mode1 不显示
    // C7 Mode1 不显示
    // B1 Mode1 不显示
    // CF Mode2 不显示
    // B9 Mode2 不显示
    // B1 Mode2 不显示
    // 99	   不显示
    // 04	   不显示
    // 03	   不显示
    // C0	   不显示
    ssd1680_write(SSD1680_DISP_UPDATE_CTRL_2, &mode, 1);
    ssd1680_write(SSD1680_MASTER_ACTIVATION, NULL, 0);
    ssd1680_wait_busy();
}

/**
 * @brief              指数计算
 *
 * @param m            原数字
 * @param n            幂次
 * @return uint32_t    结果
 */
uint32_t ssd1680_pow(uint16_t m, uint16_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result;
}

/**
 * @brief        墨水屏显示字符
 *
 * @param x      X轴起始位置
 * @param y      Y轴起始位置
 * @param chr    显示的字符
 * @param size1  字体大小
 * @param color  颜色
 */
void ssd1680_show_char(uint16_t x, uint16_t y, uint16_t chr, uint16_t size1, ssd1680_color_t color)
{

    uint16_t i, m, temp, size2, chr1;
    uint16_t x0, y0;
    x += 1, y += 1, x0 = x, y0 = y;
    if (size1 == 8)
        size2 = 6;
    else
        size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); // 得到字体一个字符对应点阵集所占的字节数

    chr1 = chr - ' '; // 计算偏移后的值
    for (i = 0; i < size2; i++)
    {
        if (size1 == 8)
        {
            temp = asc2_0806[chr1][i];
        } // 调用0806字体
        else if (size1 == 12)
        {
            temp = asc2_1206[chr1][i];
        } // 调用1206字体
        else if (size1 == 16)
        {
            temp = asc2_1608[chr1][i];
        } // 调用1608字体
        else if (size1 == 24)
        {
            temp = asc2_2412[chr1][i];
        } // 调用2412字体
        else
            return;
        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                ssd1680_set_pixel(x, y, color);
            else
                ssd1680_set_pixel(x, y, (ssd1680_color_t)!color);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2))
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}

/**
 * @brief       墨水屏显示字符串
 *
 * @param x     X轴起始位置
 * @param y     Y轴起始位置
 * @param chr   字符串起始地址
 * @param size1 字体大小
 * @param color 0,反色显示;1,正常显示
 */
void ssd1680_show_string(uint16_t x, uint16_t y, uint8_t *chr, uint16_t size1, ssd1680_color_t color)
{
    while (*chr != '\0') // 判断是不是非法字符!
    {
        ssd1680_show_char(x, y, *chr, size1, color);
        chr++;
        x += size1 / 2;
    }
}

/**
 * @brief       墨水屏显示数字
 *
 * @param x     X轴起始位置
 * @param y     Y轴起始位置
 * @param num   显示的数字
 * @param len   数字的位数
 * @param size1 字体大小
 * @param color 字体颜色
 */
void ssd1680_show_number(uint16_t x, uint16_t y, uint32_t num, uint16_t size1, ssd1680_color_t color)
{
    uint8_t len = getNumberCount(num);
    uint8_t t, temp, m = 0;
    if (size1 == 8)
        m = 2;

    for (t = 0; t < len; t++)
    {
        temp = (num / ssd1680_pow(10, len - t - 1)) % 10;
        if (temp == 0)
            ssd1680_show_char(x + (size1 / 2 + m) * t, y, '0', size1, color);
        else
            ssd1680_show_char(x + (size1 / 2 + m) * t, y, temp + '0', size1, color);
    }
}

/**
 * @brief       墨水屏显示汉字
 *
 * @param x     X轴起始位置
 * @param y     Y轴起始位置
 * @param num   显示的汉字位于字库中的下标
 * @param size1 字体大小
 * @param color 字体颜色
 */
void ssd1680_show_chinese(uint16_t x, uint16_t y, uint16_t num, uint16_t size1, ssd1680_color_t color)
{
    uint16_t m, temp;
    uint16_t x0, y0;
    uint16_t i, size3;
    x += 1, y += 1, x0 = x, y0 = y;

    switch (size1)
    {
    case font_size_12:
        size3 = 24;
        break;

    default:
        return;
    }

    for (i = 0; i < size3; i++)
    {
        switch (size1)
        {
        case font_size_12:
            temp = headerFont[num][i];
            break;

        default:
            return;
        }

        for (m = 0; m < 8; m++)
        {
            if (temp & 0x01)
                ssd1680_set_pixel(x, y, color);
            else
                ssd1680_set_pixel(x, y, (ssd1680_color_t)!color);
            temp >>= 1;
            y++;
        }
        x++;
        if ((x - x0) == size1)
        {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}

/**
 * @brief       墨水屏显示图片
 *
 * @param x     X轴起始位置
 * @param y     Y轴起始位置
 * @param sizex 图片宽度
 * @param sizey 图片高度
 * @param BMP   图片数组
 * @param Color 显示颜色
 */
void ssd1680_show_picture(uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, const uint8_t BMP[], ssd1680_color_t Color)
{
    uint16_t j = 0;
    uint16_t i, n, temp, m;
    uint16_t x0, y0;
    x += 1, y += 1, x0 = x, y0 = y;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
    for (n = 0; n < sizey; n++)
    {
        for (i = 0; i < sizex; i++)
        {
            temp = BMP[j];
            j++;
            for (m = 0; m < 8; m++)
            {
                if (temp & 0x01)
                {
                    // ssd1680_set_pixel(  x, y, !Color);
                }
                else
                    ssd1680_set_pixel(x, y, Color);

                temp >>= 1;
                y++;
            }
            x++;
            if ((x - x0) == sizex)
            {
                x = x0;
                y0 = y0 + 8;
            }
            y = y0;
        }
    }
}
