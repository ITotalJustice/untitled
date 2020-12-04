#include "nvg_util.hpp"
#include <cstddef>
#include <cstdio>
#include <array>
#include <utility>
#include <algorithm>

namespace tj::gfx {

static constexpr std::array<std::pair<Button, const char*>, 31> buttons = {{
    {Button::POWER, "\uE0B8"},
    {Button::A, "\uE0E0"},
    {Button::B, "\uE0E1"},
    {Button::X, "\uE0E2"},
    {Button::Y, "\uE0E3"},
    {Button::L, "\uE0E4"},
    {Button::R, "\uE0E5"},
    {Button::ZL, "\uE0E6"},
    {Button::ZR, "\uE0E7"},
    {Button::SL, "\uE0E8"},
    {Button::SR, "\uE0E9"},
    {Button::UP, "\uE0EB"},
    {Button::DOWN, "\uE0EC"},
    {Button::LEFT, "\uE0ED"},
    {Button::RIGHT, "\uE0EE"},
    {Button::PLUS, "\uE0EF"},
    {Button::MINUS, "\uE0F0"},
    {Button::HOME, "\uE0F4"},
    {Button::SCREENSHOT, "\uE0F5"},
    {Button::LS, "\uE101"},
    {Button::RS, "\uE102"},
    {Button::L3, "\uE104"},
    {Button::R3, "\uE105"},
    {Button::SWITCH, "\uE121"},
    {Button::DUALCONS, "\uE122"},
    {Button::SETTINGS, "\uE130"},
    {Button::NEWS, "\uE132"},
    {Button::ESHOP, "\uE133"},
    {Button::ALBUM, "\uE134"},
    {Button::ALLAPPS, "\uE135"},
    {Button::CONTROLLER, "\uE136"},
}};

#define F(a) (a/255.f) // turn range 0-255 to 0.f-1.f range
static constexpr std::array<std::pair<Colour, NVGcolor>, 12> COLOURS = {{
    {Colour::BLACK, { F(45.f), F(45.f), F(45.f), F(255.f) }},
    {Colour::LIGHT_BLACK, { F(50.f), F(50.f), F(50.f), F(255.f) }},
    {Colour::SILVER, { F(128.f), F(128.f), F(128.f), F(255.f) }},
    {Colour::DARK_GREY, { F(70.f), F(70.f), F(70.f), F(255.f) }},
    {Colour::GREY, { F(77.f), F(77.f), F(77.f), F(255.f) }},
    {Colour::WHITE, { F(251.f), F(251.f), F(251.f), F(255.f) }},
    {Colour::CYAN, { F(0.f), F(255.f), F(200.f), F(255.f) }},
    {Colour::TEAL, { F(143.f), F(253.f), F(252.f), F(255.f) }},
    {Colour::BLUE, { F(36.f), F(141.f), F(199.f), F(255.f) }},
    {Colour::LIGHT_BLUE, { F(26.f), F(188.f), F(252.f), F(255.f) }},
    {Colour::YELLOW, { F(255.f), F(177.f), F(66.f), F(255.f) }},
    {Colour::RED, { F(250.f), F(90.f), F(58.f), F(255.f) }}
}};
#undef F

// https://www.youtube.com/watch?v=INn3xa4pMfg&t
template <typename Key, typename Value, std::size_t Size>
struct Map {
    std::array<std::pair<Key, Value>, Size> data;

    [[nodiscard]] constexpr Value at(const Key& key) const {
        const auto itr = std::find_if(std::begin(data), std::end(data),
            [&key](const auto& v) { return v.first == key; });
        if (itr != std::end(data)) {
            return itr->second;
        }
        __builtin_unreachable(); // it wont happen
    }
};

const char* getButton(const Button button) {
    static constexpr auto map =
        Map<Button, const char*, buttons.size()>{{buttons}};
    return map.at(button);
}

NVGcolor getColour(Colour c) {
    static constexpr auto map =
        Map<Colour, NVGcolor, COLOURS.size()>{{COLOURS}};
    return map.at(c);
}

void drawRect(NVGcontext* vg, float x, float y, float w, float h, Colour c) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, getColour(c));
    nvgFill(vg);
}

void drawRect(NVGcontext* vg, float x, float y, float w, float h, const NVGcolor& c) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, c);
    nvgFill(vg);
}

void drawRect(NVGcontext* vg, float x, float y, float w, float h, const NVGcolor&& c) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, c);
    nvgFill(vg);
}

void drawRect(NVGcontext* vg, float x, float y, float w, float h, const NVGpaint& p) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillPaint(vg, p);
    nvgFill(vg);
}

void drawRect(NVGcontext* vg, float x, float y, float w, float h, const NVGpaint&& p) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillPaint(vg, p);
    nvgFill(vg);
}

void drawText(NVGcontext* vg, float x, float y, float size, const char* str, const char* end, int align, Colour c) {
    nvgBeginPath(vg);
    nvgFontSize(vg, size);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, getColour(c));
    nvgText(vg, x, y, str, end);
}

void drawText(NVGcontext* vg, float x, float y, float size, const char* str, const char* end, int align, const NVGcolor& c) {
    nvgBeginPath(vg);
    nvgFontSize(vg, size);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, c);
    nvgText(vg, x, y, str, end);
}

void drawText(NVGcontext* vg, float x, float y, float size, const char* str, const char* end, int align, const NVGcolor&& c) {
    nvgBeginPath(vg);
    nvgFontSize(vg, size);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, c);
    nvgText(vg, x, y, str, end);
}

void drawTextArgs(NVGcontext* vg, float x, float y, float size, int align, Colour c, const char* str, ...) {
    std::va_list v;
    va_start(v, str);
    char buffer[0x100];
    vsnprintf(buffer, sizeof(buffer), str, v);
    drawText(vg, x, y, size, buffer, nullptr, align, c);
    va_end(v);
}

void drawButton(NVGcontext* vg, float x, float y, float size, Button button) {
    drawText(vg, x, y, size, getButton(button), nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, getColour(Colour::WHITE));
}

} // namespace tj::gfx
