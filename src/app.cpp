#include "app.hpp"
#include "nvg_util.hpp"
#include "nanovg/deko3d/nanovg_dk.h"
#include <algorithm>
#include <ranges>
#include <cassert>

#ifndef NDEBUG
#include <cstdio>
#define LOG(...) std::printf(__VA_ARGS__)
#else // NDEBUG
#define LOG(...)
#endif // NDEBUG

// thank you Shchmue ^^
struct ApplicationOccupiedSizeEntry {
    std::uint8_t storageId;
    std::uint64_t sizeApplication;
    std::uint64_t sizePatch;
    std::uint64_t sizeAddOnContent;
};

struct ApplicationOccupiedSize {
    ApplicationOccupiedSizeEntry entry[4];
};

namespace tj {

static constexpr float SCREEN_WIDTH = 1280.f;
static constexpr float SCREEN_HEIGHT = 720.f;

void NsDeleteAppsAsync(const NsDeleteData& data) {
    for (const auto&p : data.entries) {
        const auto result = nsDeleteApplicationCompletely(p);
        if (R_FAILED(result)) {
            data.del_cb(true);
        } else {
            data.del_cb(false);
        }
    }
    data.done_cb();
}

void App::Loop() {
    while (!this->quit && appletMainLoop()) {
        this->Poll();
        this->Update();
        this->Draw();
    }
}

void Controller::UpdateButtonHeld(bool& down, bool held) {
    if (down) {
        this->step = 50;
        this->counter = 0;
    } else if (held) {
        this->counter += this->step;

        if (this->counter >= this->MAX)
        {
            down = true;
            this->counter = 0;
            this->step = std::min(this->step + 50, this->MAX_STEP);
        }
    }
}

void App::Poll() {
    padUpdate(&this->pad);

    const auto down = padGetButtonsDown(&this->pad);
    const auto held = padGetButtons(&this->pad);

    this->controller.A = down & HidNpadButton_A;
    this->controller.B = down & HidNpadButton_B;
    this->controller.X = down & HidNpadButton_X;
    this->controller.Y = down & HidNpadButton_Y;
    this->controller.L = down & HidNpadButton_L;
    this->controller.R = down & HidNpadButton_R;
    this->controller.L2 = down & HidNpadButton_ZL;
    this->controller.R2 = down & HidNpadButton_ZR;
    this->controller.START = down & HidNpadButton_Plus;
    this->controller.SELECT = down & HidNpadButton_Minus;
    // keep directional keys pressed.
    this->controller.DOWN = (down & HidNpadButton_AnyDown);
    this->controller.UP = (down & HidNpadButton_AnyUp);
    this->controller.LEFT = (down & HidNpadButton_AnyLeft);
    this->controller.RIGHT = (down & HidNpadButton_AnyRight);

    this->controller.UpdateButtonHeld(this->controller.DOWN, held & HidNpadButton_AnyDown);
    this->controller.UpdateButtonHeld(this->controller.UP, held & HidNpadButton_AnyUp);

#ifndef NDEBUG
    auto display = [](const char* str, bool key) {
        if (key) {
            LOG("Key %s is Pressed\n", str);
        }
    };

    display("A", this->controller.A);
    display("B", this->controller.B);
    display("X", this->controller.X);
    display("Y", this->controller.Y);
    display("L", this->controller.L);
    display("R", this->controller.R);
    display("L2", this->controller.L2);
    display("R2", this->controller.R2);
#endif
}

void App::Update() {
    switch (this->menu_mode) {
        case MenuMode::LIST:
            this->UpdateList();
            break;
        case MenuMode::CONFIRM:
            this->UpdateConfirm();
            break;
        case MenuMode::PROGRESS:
            this->UpdateProgress();
            break;
    }
}

void App::Draw() {
    const int slot = this->queue.acquireImage(this->swapchain);
    this->queue.submitCommands(this->framebuffer_cmdlists[slot]);
    this->queue.submitCommands(this->render_cmdlist);
    nvgBeginFrame(this->vg, SCREEN_WIDTH, SCREEN_HEIGHT, 1.f);

    this->DrawBackground();

    switch (this->menu_mode) {
        case MenuMode::LIST:
            this->DrawList();
            break;
        case MenuMode::CONFIRM:
            this->DrawConfirm();
            break;
        case MenuMode::PROGRESS:
            this->DrawProgress();
            break;
    }

    nvgEndFrame(this->vg);
    this->queue.presentImage(this->swapchain, slot);
}

void App::DrawBackground() {
    gfx::drawRect(this->vg, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, gfx::Colour::BLACK);
    gfx::drawRect(vg, 30.f, 86.0f, 1220.f, 1.f, gfx::Colour::WHITE);
    gfx::drawRect(vg, 30.f, 646.0f, 1220.f, 1.f, gfx::Colour::WHITE);
}

void App::DrawList() {
    static constexpr float box_height = 120.f;
    static constexpr float box_width = 715.f;
    static constexpr float icon_spacing = 12.f;
    static constexpr float title_spacing_left = 116.f;
    static constexpr float title_spacing_top = 30.f;
    static constexpr float text_spacing_left = title_spacing_left;
    static constexpr float text_spacing_top = 67.f;
    static constexpr float sidebox_x = 870.f;
    static constexpr float sidebox_y = 87.f;
    static constexpr float sidebox_w = 380.f;
    static constexpr float sidebox_h = 558.f;

// uses the APP_VERSION define in makefile for string version.
// source: https://stackoverflow.com/a/2411008
#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
    gfx::drawText(this->vg, 70.f, 40.f, 28.f, "Software", nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
    gfx::drawText(this->vg, 1224.f, 45.f, 22.f, STRINGIZE_VALUE_OF(UNTITLED_VERSION_STRING), nullptr, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP, gfx::Colour::SILVER);
#undef STRINGIZE
#undef STRINGIZE_VALUE_OF

    auto draw_size = [&](const char* str, float x, float y, std::size_t storage_size, std::size_t storage_free, std::size_t storage_used, std::size_t app_size) {
        gfx::drawText(this->vg, x, y, 22.f, str, nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
        gfx::drawRect(this->vg, x - 5.f, y + 28.f, 326.f, 16.f, gfx::Colour::WHITE);
        gfx::drawRect(this->vg, x - 4.f, y + 29.f, 326.f - 2.f, 16.f - 2.f, gfx::Colour::LIGHT_BLACK);
        const float bar_width = (static_cast<float>(storage_used) / static_cast<float>(storage_size)) * (326.f - 4.f);
        const float used_bar_width = (static_cast<float>(app_size) / static_cast<float>(storage_size)) * (326.f - 4.f);
        gfx::drawRect(this->vg, x - 3.f, y + 30.f, bar_width, 16.f - 4.f, gfx::Colour::WHITE);
        gfx::drawRect(this->vg, x - 3.f + bar_width - used_bar_width, y + 30.f, used_bar_width, 16.f - 4.f, gfx::Colour::CYAN);
        gfx::drawText(this->vg, x, y + 60.f, 18.f, "Space available", nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
        gfx::drawTextArgs(this->vg, x + 315.f, y + 54.f, 24.f, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP, gfx::Colour::WHITE, "%.1f GB", static_cast<float>(storage_free) / static_cast<float>(0x40000000));
    };

    // this is not accurate, i need to linear blend the grey to the back
    // nvg does support this (lerp) but im not smart enough to figure it out
    // auto paint = nvgLinearGradient(this->vg, sidebox_x, sidebox_y, sidebox_w, sidebox_h, gfx::getColour(gfx::Colour::LIGHT_BLACK), gfx::getColour(gfx::Colour::BLACK));
    // gfx::drawRect(this->vg, sidebox_x, sidebox_y, sidebox_w, sidebox_h, paint);
    gfx::drawRect(this->vg, sidebox_x, sidebox_y, sidebox_w, sidebox_h, gfx::Colour::LIGHT_BLACK);
    draw_size("System memory", sidebox_x + 30.f, sidebox_y + 56.f, this->nand_storage_size_total, this->nand_storage_size_free, this->nand_storage_size_used, this->entries[this->index].size_nand);
    draw_size("microSD card", sidebox_x + 30.f, sidebox_y + 235.f, this->sdcard_storage_size_total, this->sdcard_storage_size_free, this->sdcard_storage_size_used, this->entries[this->index].size_sd);

    nvgSave(this->vg);
    nvgScissor(this->vg, 30.f, 86.0f, 1220.f, 646.0f); // clip

    static constexpr float x = 90.f;
    float y = this->yoff;

    for (size_t i = this->start; i < this->entries.size(); ++i) {
        if (i == this->index) {
            // idk how to draw an outline, so i draw a colour rect then draw black rect ontop
            gfx::drawRect(this->vg, x - 5.f, y - 5.f, box_width + 10.f, box_height + 10.f, gfx::Colour::CYAN);
            gfx::drawRect(this->vg, x, y, box_width, box_height, gfx::Colour::BLACK);
        }
        if (this->entries[i].selected) {
            gfx::drawRect(this->vg, x - 60.f, y + (box_height / 2.f) - (48.f / 2), 48.f, 48.f, gfx::Colour::RED);
        }

        gfx::drawRect(this->vg, x, y, box_width, 1.f, gfx::Colour::DARK_GREY);
        gfx::drawRect(this->vg, x, y + box_height, box_width, 1.f, gfx::Colour::DARK_GREY);

        auto icon_paint = nvgImagePattern(this->vg, x + icon_spacing, y + icon_spacing, 90.f, 90.f, 0.f, this->entries[i].image, 1.f);
        gfx::drawRect(this->vg, x + icon_spacing, y + icon_spacing, 90.f, 90.f, icon_paint);

        nvgSave(this->vg);
        nvgScissor(this->vg, x + title_spacing_left, y, 585.f, box_height); // clip
        gfx::drawText(this->vg, x + title_spacing_left, y + title_spacing_top, 24.f, this->entries[i].name.c_str(), nullptr, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE);
        nvgRestore(this->vg);

        gfx::drawTextArgs(this->vg, x + text_spacing_left + 25.f, y + text_spacing_top + 9.f, 22.f, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::SILVER, "Nand: %.1f GB", static_cast<float>(this->entries[i].size_nand) / static_cast<float>(0x40000000));
        gfx::drawTextArgs(this->vg, x + text_spacing_left + 180.f + 25.f, y + text_spacing_top + 9.f, 22.f, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::SILVER, "Sd: %.1f GB", static_cast<float>(this->entries[i].size_sd) / static_cast<float>(0x40000000));
        gfx::drawTextArgs(this->vg, x + 708.f, y + 78.f, 32.f, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP, gfx::Colour::CYAN, "%.1f GB", static_cast<float>(this->entries[i].size_total) / static_cast<float>(0x40000000));
        y += box_height;

        // out of bounds (clip)
        if ((y + box_height) > 646.f) {
            break;
        }
    }

    nvgRestore(this->vg);

    gfx::drawTextArgs(this->vg, 55.f, 670.f, 24.f, NVG_ALIGN_LEFT | NVG_ALIGN_TOP, gfx::Colour::WHITE, "Selected %lu / %lu", this->delete_count, this->entries.size());
    gfx::drawButtons(this->vg, gfx::pair{gfx::Button::A, "Select"}, gfx::pair{gfx::Button::B, "Exit"}, gfx::pair{gfx::Button::PLUS, "Delete Selected"}, gfx::pair{gfx::Button::R, this->GetSortStr()});

}

void App::DrawConfirm() {
    gfx::drawButtons(this->vg, gfx::pair{gfx::Button::A, "OK"}, gfx::pair{gfx::Button::B, "Back"});
    gfx::drawText(this->vg, SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f, 36.f, "Are you sure you want to delete the selected games?", nullptr, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, gfx::Colour::RED);
}

void App::DrawProgress() {
    this->mutex.lock();
    const auto count = this->delete_index;
    this->mutex.unlock();
    gfx::drawTextArgs(this->vg, SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f, 36.f, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, gfx::Colour::YELLOW, "Deleted %lu / %lu", count, this->delete_entries.size());
}

void App::Sort()
{
    switch (static_cast<SortType>(this->sort_type))
    {
        case SortType::Alpha_AZ: std::ranges::sort(this->entries, std::ranges::less{}, &AppEntry::name); break;
        case SortType::Alpha_ZA: std::ranges::sort(this->entries, std::ranges::greater{}, &AppEntry::name); break;
        case SortType::Size_BigSmall: std::ranges::sort(this->entries, std::ranges::greater{}, &AppEntry::size_total); break;
        case SortType::Size_SmallBig: std::ranges::sort(this->entries, std::ranges::less{}, &AppEntry::size_total); break;
    }
}

const char* App::GetSortStr() {
    switch (static_cast<SortType>(this->sort_type))
    {
        case SortType::Alpha_AZ: return "Sort Alpha: A-Z";
        case SortType::Alpha_ZA: return "Sort Alpha: Z-A";
        case SortType::Size_BigSmall: return "Sort Size: 9-0";
        case SortType::Size_SmallBig: return "Sort Size: 0-9";
    }

    return "NULL";
}

void App::UpdateList() {
    if (this->controller.B) {
        this->quit = true;
    } else if (this->controller.A) {
        if (this->entries[this->index].selected) {
            this->entries[this->index].selected = false;
            --this->delete_count;
        } else {
            this->entries[this->index].selected = true;
            ++this->delete_count;
        }
        // add to / remove from delete list
    } else if (this->controller.START) { // start delete
        for (const auto&p : this->entries) {
            if (p.selected) {
                this->delete_entries.push_back(p.id);
            }
        }
        if (this->delete_entries.size()) {
            this->menu_mode = MenuMode::CONFIRM;
        }
    } else if (this->controller.DOWN) { // move down
        if (this->index < (this->entries.size() - 1)) {
            ++this->index;
            this->ypos += this->BOX_HEIGHT;
            if ((this->ypos + this->BOX_HEIGHT) > 646.f) {
                LOG("moved down\n");
                this->ypos -= this->BOX_HEIGHT;
                this->yoff = this->ypos - ((this->index - this->start - 1) * this->BOX_HEIGHT);
                ++this->start;
            }
        }
    } else if (this->controller.UP) { // move up
        if (this->index != 0 && this->entries.size()) {
            --this->index;
            this->ypos -= this->BOX_HEIGHT;
            if (this->ypos < 86.f) {
                LOG("moved up\n");
                this->ypos += this->BOX_HEIGHT;
                this->yoff = this->ypos;
                --this->start;
            }
        }
    } else if (this->controller.R)
    {
        this->sort_type++;

        if (this->sort_type == static_cast<uint8_t>(SortType::MAX))
        {
            this->sort_type = 0;
        }

        this->Sort();
    }
    // handle direction keys
}

void App::UpdateConfirm() {
    if (this->controller.A) {
        this->finished_deleting = false;
        this->delete_index = 0;
        NsDeleteData data{
            .entries = this->delete_entries,
            .del_cb = [this](bool error){
                    std::scoped_lock lock{this->mutex};
                    if (error) {
                        LOG("error whilst deleting AppID %lX\n", this->delete_entries[this->delete_index]);
                    }
                    ++this->delete_index;
                },
            .done_cb = [this](){
                std::scoped_lock lock{this->mutex};
                LOG("finished deleting entries...\n");
                this->finished_deleting = true;
            }
        };
        this->menu_mode = MenuMode::PROGRESS;
        this->async_thread = std::async(std::launch::async, NsDeleteAppsAsync, data);
    } else if (this->controller.B) {
        this->delete_entries.clear();
        this->menu_mode = MenuMode::LIST;
    }
}

void App::UpdateProgress() {
    std::scoped_lock lock{this->mutex};
    if (this->finished_deleting) {
        // thread is already over, but we still have to call get.
        this->async_thread.get();
        // remove deleted entries.
        // this looks like it would be slow but it really isn't.
        for (const auto&p : this->delete_entries) {
            for (size_t i = 0; i < this->entries.size(); ++i) {
                if (this->entries[i].id == p) {
                    nvgDeleteImage(this->vg, this->entries[i].image);
                    this->entries.erase(this->entries.begin() + i);
                    break;
                }
            }
        }
        // reset pos to 0
        this->delete_count = 0;
        this->ypos = this->yoff = 130.f;
        this->index = 0;
        this->start = 0;
        this->delete_entries.clear();
        this->menu_mode = MenuMode::LIST;
    }
    // probably do nothing here.
    // could add cancel option but, maybe not.
}

bool App::Scan() {
    Result result{};
    s32 offset{};
    u64 jpeg_size{};
    size_t count{};
    NacpLanguageEntry* language_entry{};
    auto control_data = std::make_unique<NsApplicationControlData>();
    std::array<NsApplicationRecord, 30> record_list;

    for (;;) {
        s32 record_count{};
        result = nsListApplicationRecord(record_list.data(), static_cast<s32>(record_list.size()), offset, &record_count);
        if (R_FAILED(result)) {
            LOG("failed to get record count\n");
            return false;
        }
        // either we have ran out of games or we have no games installed.
        if (record_count == 0) {
            LOG("record count is 0\n");
            return false;
        }

        for (auto i = 0; i < record_count; ++i) {
        AppEntry entry;
            result = nsGetApplicationControlData(NsApplicationControlSource_Storage, record_list[i].application_id, control_data.get(), sizeof(NsApplicationControlData), &jpeg_size);
            // can fail with very messed up piracy installs, it would fail in ofw as well.
            if (R_FAILED(result)) {
                LOG("failed to get control data for %lX\n", record_list[i].application_id);

                // this is a corrupted entry!
                entry.name = "Corrupted";
                entry.author = "NA";
                entry.display_version = "NA";
                entry.id = record_list[i].application_id;
                entry.image = this->default_icon_image;
                entry.own_image = false; // we don't own it
                this->entries.emplace_back(std::move(entry));
                ++count;
                // return false;
            } else {
                result = nsGetApplicationDesiredLanguage(&control_data->nacp, &language_entry);
                if (R_FAILED(result)) {
                    LOG("failed to get lang data\n");
                    return false;
                } else {
                    ApplicationOccupiedSize size{};
                    auto result = nsCalculateApplicationOccupiedSize(record_list[i].application_id, (NsApplicationOccupiedSize*)&size);
                    if (R_FAILED(result)) {
                        LOG("failed to get application occupied size for ID %lX\n", record_list[i].application_id);
                        entry.size_total = entry.size_nand = entry.size_sd = 0;
                    } else {
                        auto fill_size = [&](const ApplicationOccupiedSizeEntry& e) {
                            switch (e.storageId) {
                                case NcmStorageId_BuiltInUser:
                                    entry.size_nand = e.sizeApplication + e.sizeAddOnContent + e.sizePatch;
                                    break;
                                case NcmStorageId_SdCard:
                                    entry.size_sd = e.sizeApplication + e.sizeAddOnContent + e.sizePatch;
                                    break;
                                default:
                                    assert(0 && "unk ncm storageID when getting size!");
                                    break;
                            }
                        };
                        // unsure if the order of the storageID will always be nand then sd.
                        // because of this, i manually check using a switch (for now).
                        fill_size(size.entry[0]);
                        fill_size(size.entry[1]);
                        entry.size_total = entry.size_nand + entry.size_sd;
                    }

                    entry.name = language_entry->name;
                    entry.author = language_entry->author;
                    entry.display_version = control_data->nacp.display_version;
                    entry.id = record_list[i].application_id;
                    assert((jpeg_size - sizeof(NacpStruct)) > 0 && "jpeg size is smaller than the size of NacpStruct");
                    entry.image = nvgCreateImageMem(this->vg, 0, control_data->icon, jpeg_size - sizeof(NacpStruct));
                    entry.own_image = true; // we own it
                    LOG("added %s\n", entry.name.c_str());
                    this->entries.emplace_back(std::move(entry));
                    ++count;
                }
            }
        }

        // if we have less than count, then we are done!
        if (static_cast<size_t>(record_count) < record_list.size()) {
            return true;
        }

        offset += record_count;
    }

    return true;
}


App::App() {
    nsGetTotalSpaceSize(NcmStorageId_SdCard, (s64*)&this->sdcard_storage_size_total);
    nsGetFreeSpaceSize(NcmStorageId_SdCard, (s64*)&this->sdcard_storage_size_free);
    nsGetTotalSpaceSize(NcmStorageId_BuiltInUser, (s64*)&this->nand_storage_size_total);
    nsGetFreeSpaceSize(NcmStorageId_BuiltInUser, (s64*)&this->nand_storage_size_free);
    this->nand_storage_size_used = this->nand_storage_size_total - this->nand_storage_size_free;
    this->sdcard_storage_size_used = this->sdcard_storage_size_total - this->sdcard_storage_size_free;

    LOG("nand total: %lu free: %lu used: %lu\n", this->nand_storage_size_total, this->nand_storage_size_free, this->nand_storage_size_used);
    LOG("sdcard total: %lu free: %lu used: %lu\n", this->sdcard_storage_size_total, this->sdcard_storage_size_free, this->sdcard_storage_size_used);

    PlFontData font_standard, font_extended;
    plGetSharedFontByType(&font_standard, PlSharedFontType_Standard);
    plGetSharedFontByType(&font_extended, PlSharedFontType_NintendoExt);

    // Create the deko3d device
    this->device = dk::DeviceMaker{}.create();

    // Create the main queue
    this->queue = dk::QueueMaker{this->device}.setFlags(DkQueueFlags_Graphics).create();

    // Create the memory pools
    this->pool_images.emplace(device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 16*1024*1024);
    this->pool_code.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 128*1024);
    this->pool_data.emplace(device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

    // Create the static command buffer and feed it freshly allocated memory
    this->cmdbuf = dk::CmdBufMaker{this->device}.create();
    const CMemPool::Handle cmdmem = this->pool_data->allocate(this->StaticCmdSize);
    this->cmdbuf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

    // Create the framebuffer resources
    this->createFramebufferResources();

    this->renderer.emplace(1280, 720, this->device, this->queue, *this->pool_images, *this->pool_code, *this->pool_data);
    this->vg = nvgCreateDk(&*this->renderer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);

    // not sure if these are meant to be deleted or not...
    int standard_font = nvgCreateFontMem(this->vg, "Standard", (unsigned char*)font_standard.address, font_standard.size, 0);
    int extended_font = nvgCreateFontMem(this->vg, "Extended", (unsigned char*)font_extended.address, font_extended.size, 0);

    if (standard_font < 0) {
        LOG("failed to load Standard font\n");
    }
    if (extended_font < 0) {
        LOG("failed to load extended font\n");
    }

    nvgAddFallbackFontId(this->vg, standard_font, extended_font);
    this->default_icon_image = nvgCreateImage(this->vg, "romfs:/default_icon.jpg", NVG_IMAGE_NEAREST);

    // todo: handle errors
    this->Scan();
    this->Sort();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&this->pad);
}

App::~App() {
    for (auto&p : this->entries) {
        if (p.own_image) {
            nvgDeleteImage(this->vg, p.image);
        }
    }
    nvgDeleteImage(this->vg, default_icon_image);
    this->destroyFramebufferResources();
    nvgDeleteDk(this->vg);
    this->renderer.reset();
}

void App::createFramebufferResources() {
    // Create layout for the depth buffer
    dk::ImageLayout layout_depthbuffer;
    dk::ImageLayoutMaker{device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_S8)
        .setDimensions(1280, 720)
        .initialize(layout_depthbuffer);

    // Create the depth buffer
    this->depthBuffer_mem = this->pool_images->allocate(layout_depthbuffer.getSize(), layout_depthbuffer.getAlignment());
    this->depthBuffer.initialize(layout_depthbuffer, this->depthBuffer_mem.getMemBlock(), this->depthBuffer_mem.getOffset());

    // Create layout for the framebuffers
    dk::ImageLayout layout_framebuffer;
    dk::ImageLayoutMaker{device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(1280, 720)
        .initialize(layout_framebuffer);

    // Create the framebuffers
    std::array<DkImage const*, NumFramebuffers> fb_array;
    const uint64_t fb_size  = layout_framebuffer.getSize();
    const uint32_t fb_align = layout_framebuffer.getAlignment();
    for (unsigned i = 0; i < fb_array.size(); ++i) {
        // Allocate a framebuffer
        this->framebuffers_mem[i] = pool_images->allocate(fb_size, fb_align);
        this->framebuffers[i].initialize(layout_framebuffer, framebuffers_mem[i].getMemBlock(), framebuffers_mem[i].getOffset());

        // Generate a command list that binds it
        dk::ImageView colorTarget{ framebuffers[i] }, depthTarget{ depthBuffer };
        this->cmdbuf.bindRenderTargets(&colorTarget, &depthTarget);
        this->framebuffer_cmdlists[i] = cmdbuf.finishList();

        // Fill in the array for use later by the swapchain creation code
        fb_array[i] = &framebuffers[i];
    }

    // Create the swapchain using the framebuffers
    this->swapchain = dk::SwapchainMaker{device, nwindowGetDefault(), fb_array}.create();

    // Generate the main rendering cmdlist
    this->recordStaticCommands();
}

void App::destroyFramebufferResources() {
    // Return early if we have nothing to destroy
    if (!this->swapchain) {
        return;
    }

    this->queue.waitIdle();
    this->cmdbuf.clear();
    swapchain.destroy();

    // Destroy the framebuffers
    for (unsigned i = 0; i < NumFramebuffers; ++i) {
        framebuffers_mem[i].destroy();
    }

    // Destroy the depth buffer
    this->depthBuffer_mem.destroy();
}

void App::recordStaticCommands() {
    // Initialize state structs with deko3d defaults
    dk::RasterizerState rasterizerState;
    dk::ColorState colorState;
    dk::ColorWriteState colorWriteState;
    dk::BlendState blendState;

    // Configure the viewport and scissor
    this->cmdbuf.setViewports(0, { { 0.0f, 0.0f, 1280, 720, 0.0f, 1.0f } });
    this->cmdbuf.setScissors(0, { { 0, 0, 1280, 720 } });

    // Clear the color and depth buffers
    this->cmdbuf.clearColor(0, DkColorMask_RGBA, 0.2f, 0.3f, 0.3f, 1.0f);
    this->cmdbuf.clearDepthStencil(true, 1.0f, 0xFF, 0);

    // Bind required state
    this->cmdbuf.bindRasterizerState(rasterizerState);
    this->cmdbuf.bindColorState(colorState);
    this->cmdbuf.bindColorWriteState(colorWriteState);

    this->render_cmdlist = this->cmdbuf.finishList();
}

} // namespace tj
