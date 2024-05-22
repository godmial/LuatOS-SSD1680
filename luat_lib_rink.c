#include <stdlib.h>

#include "luat_base.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#include "luat_log.h"

#include "luat_timer.h"
#include "luat_mem.h"
#include "luat_spi.h"
#include "luat_gpio.h"

#include "ssd1680.h"

#ifndef LUAT_LOG_TAG
#define LUAT_LOG_TAG "rink"
#endif

static int l_ssd1680_init(lua_State *L)
{
    int x = 250;
    int y = 122;
    ssd1680_orientation_t orientation = SSD1680_270_DEG;

    ssd1680_pinmap_t pinmap = {
        .busy = luaL_checkinteger(L, 1),
        .reset = luaL_checkinteger(L, 2),
        .dc = luaL_checkinteger(L, 3),
        .cs = luaL_checkinteger(L, 4),
        .mosi = luaL_checkinteger(L, 5),
        .sck = luaL_checkinteger(L, 6),
    };

    ssd1680_init(NULL, pinmap, x, y, orientation);

    return 0;
}

static int l_ssd1680_deinit(lua_State *L)
{
    ssd1680_deinit();

    return 0;
}

static int l_ssd1680_fill(lua_State *L)
{
    ssd1680_color_t color = luaL_checkinteger(L, 1);
    ssd1680_fill(color);
    return 0;
}

static int l_ssd1680_send_framebuffer(lua_State *L)
{
    ssd1680_send_framebuffer();
    return 0;
}

static int l_ssd1680_set_pixel(lua_State *L)
{
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    ssd1680_color_t color = luaL_checkinteger(L, 3);
    ssd1680_set_pixel((uint16_t)x, (uint16_t)y, color);
    return 0;
}

static int l_ssd1680_refresh(lua_State *L)
{
    ssd1680_refresh_t refresh = luaL_checkinteger(L, 1);
    ssd1680_refresh(refresh);
    return 0;
}

static int l_ssd1680_show_number(lua_State *L)
{
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int num = luaL_checkinteger(L, 3);
    int size1 = luaL_checkinteger(L, 4);
    ssd1680_color_t color = luaL_checkinteger(L, 5);
    ssd1680_show_number((uint16_t)x, (uint16_t)y, (uint32_t)num, (uint16_t)size1, color);
}

static int l_ssd1680_show_string(lua_State *L)
{
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    char *chr = luaL_checkstring(L, 3);
    int size1 = luaL_checkinteger(L, 4);
    ssd1680_color_t color = luaL_checkinteger(L, 5);
    ssd1680_show_string((uint16_t)x, (uint16_t)y, (uint8_t *)chr, (uint16_t)size1, color);
}

static int l_ssd1680_show_chinese(lua_State *L)
{
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int num = luaL_checkinteger(L, 3);

    int size1 = luaL_checkinteger(L, 4);
    ssd1680_color_t color = luaL_checkinteger(L, 5);
    ssd1680_show_chinese((uint16_t)x, (uint16_t)y, (uint16_t)num, (uint16_t)size1, color);
}

static int l_ssd1680_show_picture(lua_State *L)
{
    size_t len = 0;
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int sizex = luaL_checkinteger(L, 3);
    int sizey = luaL_checkinteger(L, 4);
    const char *data = luaL_checklstring(L, 5, &len);
    ssd1680_color_t color = luaL_checkinteger(L, 6);

    ssd1680_show_picture((uint16_t)x, (uint16_t)y, (uint16_t)sizex, (uint16_t)sizey, data, color);
}

#include "rotable2.h"
static const rotable_Reg_t reg_rink[] =
    {
        {"init", ROREG_FUNC(l_ssd1680_init)},
        {"deinit", ROREG_FUNC(l_ssd1680_deinit)},
        {"clean", ROREG_FUNC(l_ssd1680_fill)},
        {"send", ROREG_FUNC(l_ssd1680_send_framebuffer)},
        {"set_pixel", ROREG_FUNC(l_ssd1680_set_pixel)},
        {"refresh", ROREG_FUNC(l_ssd1680_refresh)},
        {"show_number", ROREG_FUNC(l_ssd1680_show_number)},
        {"show_string", ROREG_FUNC(l_ssd1680_show_string)},
        {"show_chinese", ROREG_FUNC(l_ssd1680_show_chinese)},
        {"show_picture", ROREG_FUNC(l_ssd1680_show_picture)},

        {NULL, ROREG_INT(0)},

};

LUAMOD_API int luaopen_rink(lua_State *L)
{
    luat_newlib2(L, reg_rink);
    return 1;
}
