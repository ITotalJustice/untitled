#pragma once

#include "nanovg/nanovg.h"
#include <cstdarg>
#include <array>
#include <cstdio>

namespace tj::gfx {

enum class Colour {
    BLACK,
    LIGHT_BLACK,
    SILVER,
    DARK_GREY,
    GREY,
    WHITE,
    CYAN,
    TEAL,
    BLUE,
    LIGHT_BLUE,
    YELLOW,
    RED,
};

enum class Button {
    POWER = 0xE0B8,
    A = 0xE0E0,
    B = 0xE0E1,
    X = 0xE0E2,
    Y = 0xE0E3,
    L = 0xE0E4,
    R = 0xE0E5,
    ZL = 0xE0E6,
    ZR = 0xE0E7,
    SL = 0xE0E8,
    SR = 0xE0E9,
    UP = 0xE0EB,
    DOWN = 0xE0EC,
    LEFT = 0xE0ED,
    RIGHT = 0xE0EE,
    PLUS = 0xE0EF,
    MINUS = 0xE0F0,
    HOME = 0xE0F4,
    SCREENSHOT = 0xE0F5,
    LS = 0xE101,
    RS = 0xE102,
    L3 = 0xE104,
    R3 = 0xE105,
    SWITCH = 0xE121,
    DUALCONS = 0xE122,
    SETTINGS = 0xE130,
    NEWS = 0xE132,
    ESHOP = 0xE133,
    ALBUM = 0xE134,
    ALLAPPS = 0xE135,
    CONTROLLER = 0xE136,
};

NVGcolor getColour(Colour c);

void drawRect(NVGcontext*, float x, float y, float w, float h, Colour c);
void drawRect(NVGcontext*, float x, float y, float w, float h, const NVGcolor& c);
void drawRect(NVGcontext*, float x, float y, float w, float h, const NVGcolor&& c);
void drawRect(NVGcontext*, float x, float y, float w, float h, const NVGpaint& p);
void drawRect(NVGcontext*, float x, float y, float w, float h, const NVGpaint&& p);

void drawText(NVGcontext*, float x, float y, float size, const char* str, const char* end, int align, Colour c);
void drawText(NVGcontext*, float x, float y, float size, const char* str, const char* end, int align, const NVGcolor& c);
void drawText(NVGcontext*, float x, float y, float size, const char* str, const char* end, int align, const NVGcolor&& c);
void drawTextArgs(NVGcontext*, float x, float y, float size, int align, Colour c, const char* str, ...);

void drawButton(NVGcontext* vg, float x, float y, float size, Button button);

const char* getButton(Button button);

using pair = std::pair<Button, const char*>;
void drawButtons(NVGcontext* vg, std::same_as<pair> auto ...args) {
    const std::array list = {args...};
    nvgBeginPath(vg);
    nvgFontSize(vg, 24.f);
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
    nvgFillColor(vg, getColour(Colour::WHITE));

    float x = 1220.f;
    const float y = 675.f;
    float bounds[4]{};

    for (const auto& [button, text] : list) {
        nvgFontSize(vg, 20.f);
        nvgTextBounds(vg, x, y, text, nullptr, bounds);
        auto len = bounds[2] - bounds[0];
        nvgText(vg, x, y, text, nullptr);

        x -= len + 10.f;
        nvgFontSize(vg, 30.f);
        nvgTextBounds(vg, x, y - 7.f, getButton(button), nullptr, bounds);
        len = bounds[2] - bounds[0];
        nvgText(vg, x, y - 7.f, getButton(button), nullptr);
        x -= len + 34.f;
    }
}

} // namespace tj::gfx
