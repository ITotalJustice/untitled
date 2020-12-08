#pragma once

#include "nanovg/nanovg.h"
#include "nanovg/deko3d/dk_renderer.hpp"
#include <switch.h>
#include <cstdint>
#include <vector>
#include <string>
#include <future>
#include <mutex>
#include <functional>

namespace tj {

using AppID = std::uint64_t;

enum class MenuMode { LIST, CONFIRM, PROGRESS };

struct Controller final {
    // these are tap only
    bool A;
    bool B;
    bool X;
    bool Y;
    bool L;
    bool R;
    bool L2;
    bool R2;
    bool START;
    bool SELECT;
    // these can be held
    bool LEFT;
    bool RIGHT;
    bool UP;
    bool DOWN;
};

struct AppEntry final {
    std::string name;
    std::string author;
    std::string display_version;
    std::size_t size_nand;
    std::size_t size_sd;
    std::size_t size_total;
    AppID id;
    int image;
    bool selected{false};
};

struct NsDeleteData final {
    std::vector<AppID> entries;
    std::function<void(bool)> del_cb; // called when deleted an entry
    std::function<void(void)> done_cb; // called when finished
};

void NsDeleteAppsAsync(const NsDeleteData& data);

class App final {
public:
    App();
    ~App();
    void Loop();

private:
    NVGcontext* vg{nullptr};
    std::vector<AppEntry> entries;
    std::vector<AppID> delete_entries;
    PadState pad{};
    Controller controller{};
    Controller previous_controller{};

    std::size_t nand_storage_size_total{};
    std::size_t nand_storage_size_used{};
    std::size_t nand_storage_size_free{};
    std::size_t sdcard_storage_size_total{};
    std::size_t sdcard_storage_size_used{};
    std::size_t sdcard_storage_size_free{};

    std::future<void> async_thread;
    std::mutex mutex{};
    std::size_t delete_index{}; // mutex locked
    bool finished_deleting{false}; // mutex locked

    // this is just bad code, ignore it
    static constexpr float BOX_HEIGHT{120.f};
    float yoff{130.f};
    float ypos{130.f};
    std::size_t start{0};
    std::size_t delete_count{0};
    std::size_t index{}; // where i am in the array
    MenuMode menu_mode{MenuMode::LIST};
    bool quit{false};

    void Draw();
    void Update();
    void Poll();
    bool Scan(); // called on init

    void UpdateList();
    void UpdateConfirm();
    void UpdateProgress();

    void DrawBackground();
    void DrawList();
    void DrawConfirm();
    void DrawProgress();

private: // from nanovg decko3d example by adubbz
    static constexpr unsigned NumFramebuffers = 2;
    static constexpr unsigned StaticCmdSize = 0x1000;
    dk::UniqueDevice device;
    dk::UniqueQueue queue;
    std::optional<CMemPool> pool_images;
    std::optional<CMemPool> pool_code;
    std::optional<CMemPool> pool_data;
    dk::UniqueCmdBuf cmdbuf;
    CMemPool::Handle depthBuffer_mem;
    CMemPool::Handle framebuffers_mem[NumFramebuffers];
    dk::Image depthBuffer;
    dk::Image framebuffers[NumFramebuffers];
    DkCmdList framebuffer_cmdlists[NumFramebuffers];
    dk::UniqueSwapchain swapchain;
    DkCmdList render_cmdlist;
    std::optional<nvg::DkRenderer> renderer;
    void createFramebufferResources();
    void destroyFramebufferResources();
    void recordStaticCommands();
};

} // namespace tj
