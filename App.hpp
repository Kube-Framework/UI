/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Application
 */

#pragma once

#include <Kube/ECS/Executor.hpp>
#include <Kube/GPU/GPU.hpp>
#include <Kube/IO/ResourceManager.hpp>

#include "Base.hpp"

namespace kF::UI
{
    class App;
    class UISystem;
}

class kF::UI::App
{
public:
    /** @brief Flags of window */
    enum class WindowFlags
    {
        None            = 0b0,
        Fullscreen      = 0b1,                     // Fullscreen window
        Shown           = 0b10,                    // Window is visible
        Hidden          = 0b100,                   // Window is not visible
        Borderless      = 0b1000,                  // No window decoration
        Resizable       = 0b10000,                 // Window can be resized
        Minimized       = 0b100000,                // Window is minimized
        Maximized       = 0b1000000,               // Window is maximized
        MouseGrabbed    = 0b10000000,              // Window has grabbed mouse input
        InputFocus      = 0b100000000,             // Window has input focus
        MouseFocus      = 0b1000000000,            // Window has mouse focus
        Foreign         = 0b10000000000,           // Window not created by SDL
        AllowHighdpi    = 0b100000000000,          // Window should be created in high-DPI mode if supported.
        MouseCapture    = 0b1000000000000,         // Mouse is captured outside window
        AlwaysOnTop     = 0b10000000000000,        // Window should always be above others
        SkipTaskbar     = 0b100000000000000,       // Window should not be added to the taskbar
        Utility         = 0b1000000000000000,      // Window should be treated as a utility window
        Tooltip         = 0b10000000000000000,     // Window should be treated as a tooltip
        PopupMenu       = 0b100000000000000000,    // Window should be treated as a popup menu
        KeyboardGrabbed = 0b1000000000000000000,   // Window has grabbed keyboard input
    };

    /** @brief Undefined window position */
    static constexpr Point UndefinedWindowPos { -1.0f, -1.0f };

    /** @brief Centered window position */
    static constexpr Point CenteredWindowPos { PixelInfinity, PixelInfinity };

    /** @brief Centered window position */
    static constexpr Size FillWindowSize { PixelInfinity, PixelInfinity };

    /** @brief Default target frame rate */
    static constexpr int DefaultEventRate { 60 };

    /** @brief Default target frame rate */
    static constexpr int DefaultFrameRate { 60 };

    /** @brief Default target frame rate */
    static constexpr int DefaultFrameTickRate { 1'000'000'000 / DefaultFrameRate };


    /** @brief Get app global instance */
    [[nodiscard]] static inline App &Get(void) noexcept { return *_Instance; }


    /** @brief Destructor */
    ~App(void) noexcept;

    /** @brief App initializer */
    App(const std::string_view windowTitle,
            const Point windowPos = UndefinedWindowPos,
            const Size windowSize = FillWindowSize,
            const WindowFlags windowFlags = WindowFlags::None,
            const Core::Version version = Core::Version(0, 1, 0),
            const std::size_t workerCount = Flow::Scheduler::AutoWorkerCount,
            const std::size_t taskQueueSize = Flow::Scheduler::DefaultTaskQueueSize,
            const std::size_t eventQueueSize = ECS::Executor::DefaultExecutorEventQueueSize) noexcept;


    /** @brief Get GPU instance */
    [[nodiscard]] inline GPU::GPU &gpu(void) noexcept { return *_gpu; }

    /** @brief Get const GPU instance */
    [[nodiscard]] inline const GPU::GPU &gpu(void) const noexcept { return *_gpu; }


    /** @brief Get ResourceManager instance */
    [[nodiscard]] inline IO::ResourceManager &resourceManager(void) noexcept { return _resourceManager; }

    /** @brief Get const ResourceManager instance */
    [[nodiscard]] inline const IO::ResourceManager &resourceManager(void) const noexcept { return _resourceManager; }


    /** @brief Get system executor */
    [[nodiscard]] inline ECS::Executor &executor(void) noexcept { return _executor; }

    /** @brief Get const system executor */
    [[nodiscard]] inline const ECS::Executor &executor(void) const noexcept { return _executor; }


    /** @brief Get reference over the UI system of the application */
    [[nodiscard]] inline UISystem &uiSystem(void) const noexcept { return *_uiSystem; }


    /** @brief Get current window size */
    [[nodiscard]] Size windowSize(void) const noexcept;

    /** @brief Set current window size */
    void setWindowSize(const Size size) noexcept;


    /** @brief Get relative mouse mode state */
    [[nodiscard]] bool relativeMouseMode(void) const noexcept;

    /** @brief Set relative mouse mode state */
    void setRelativeMouseMode(const bool state) noexcept;


    /** @brief Get mouse grab state */
    [[nodiscard]] bool mouseGrab(void) const noexcept;

    /** @brief Set current mouse grab state */
    void setMouseGrab(const bool state) noexcept;


    /** @brief Get keyboard grab state */
    [[nodiscard]] bool keyboardGrab(void) const noexcept;

    /** @brief Set current keyboard grab state */
    void setKeyboardGrab(const bool state) noexcept;


    /** @brief Run app in blocking mode */
    void run(void) noexcept;

    /** @brief Stop app */
    void stop(void) noexcept;

private:
    /** @brief Backend Instance */
    struct BackendInstance
    {
        GPU::BackendWindow *window {};

        /** @brief Destructor */
        ~BackendInstance(void) noexcept;

        /** @brief Constructor */
        BackendInstance(const std::string_view windowTitle,
                const Point windowPos, const Size windowSize, const WindowFlags windowFlags) noexcept;
    };


    /** @brief UI FramebufferManager model */
    [[nodiscard]] static GPU::FrameImageModels MakeFrameImageModels(void) noexcept;

    /** @brief UI RenderPass factory */
    [[nodiscard]] static GPU::RenderPass MakeRenderPass(void) noexcept;


    static App *_Instance;

    // Cacheline 0 & 1
    BackendInstance _backendInstance;
    GPU::GPU::GlobalInstance _gpu;
    std::int64_t _frameTickRate { DefaultFrameTickRate };
    UISystem *_uiSystem { nullptr };
    IO::ResourceManager _resourceManager {};
    // Cacheline 2 -> ...
    ECS::Executor _executor;
};
static_assert_alignof_double_cacheline(kF::UI::App);