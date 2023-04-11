/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Application
 */

#include <signal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <Kube/Core/String.hpp>

#include "App.hpp"
#include "EventSystem.hpp"
#include "PresentSystem.hpp"
#include "UISystem.hpp"

using namespace kF;

KF_DECLARE_RESOURCE_ENVIRONMENT(UI);

UI::App *UI::App::_Instance { nullptr };

static SDL_WindowFlags ToWindowFlags(const UI::App::WindowFlags flags) noexcept
{
    using enum UI::App::WindowFlags;

    constexpr auto ApplyFlags = [](SDL_WindowFlags &out, const UI::App::WindowFlags flags, const UI::App::WindowFlags from, const SDL_WindowFlags to) {
        if (Core::HasFlags(flags, from))
            out = Core::MakeFlags(out, to);
    };

    SDL_WindowFlags out {};
    if (Core::HasFlags(flags, Fullscreen, Borderless)) {
        out = Core::MakeFlags(out, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        ApplyFlags(out, flags, Fullscreen, SDL_WINDOW_FULLSCREEN);
        ApplyFlags(out, flags, Borderless, SDL_WINDOW_BORDERLESS);
    }
    ApplyFlags(out, flags, Shown, SDL_WINDOW_SHOWN);
    ApplyFlags(out, flags, Hidden, SDL_WINDOW_HIDDEN);
    ApplyFlags(out, flags, Resizable, SDL_WINDOW_RESIZABLE);
    ApplyFlags(out, flags, Minimized, SDL_WINDOW_MINIMIZED);
    ApplyFlags(out, flags, Maximized, SDL_WINDOW_MAXIMIZED);
    ApplyFlags(out, flags, MouseGrabbed, SDL_WINDOW_MOUSE_GRABBED);
    ApplyFlags(out, flags, InputFocus, SDL_WINDOW_INPUT_FOCUS);
    ApplyFlags(out, flags, MouseFocus, SDL_WINDOW_MOUSE_FOCUS);
    ApplyFlags(out, flags, Foreign, SDL_WINDOW_FOREIGN);
    ApplyFlags(out, flags, AllowHighdpi, SDL_WINDOW_ALLOW_HIGHDPI);
    ApplyFlags(out, flags, MouseCapture, SDL_WINDOW_MOUSE_CAPTURE);
    ApplyFlags(out, flags, AlwaysOnTop, SDL_WINDOW_ALWAYS_ON_TOP);
    ApplyFlags(out, flags, SkipTaskbar, SDL_WINDOW_SKIP_TASKBAR);
    ApplyFlags(out, flags, Utility, SDL_WINDOW_UTILITY);
    ApplyFlags(out, flags, Tooltip, SDL_WINDOW_TOOLTIP);
    ApplyFlags(out, flags, PopupMenu, SDL_WINDOW_POPUP_MENU);
    ApplyFlags(out, flags, KeyboardGrabbed, SDL_WINDOW_KEYBOARD_GRABBED);
    return out;
}

GPU::FrameImageModels UI::App::MakeFrameImageModels(void) noexcept
{
    using namespace GPU;

    return FrameImageModels({});
}

GPU::RenderPass UI::App::MakeRenderPass(void) noexcept
{
    using namespace GPU;

    const auto &gpu = GPUObject::Parent();
    const AttachmentReference colorAttachmentRefs[] {
        AttachmentReference(0, ImageLayout::ColorAttachmentOptimal),
    };
    return RenderPass::Make(
        {
            AttachmentDescription(
                AttachmentDescriptionFlags::None,
                static_cast<Format>(gpu.swapchain().surfaceFormat().format),
                SampleCountFlags::X1,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store,
                AttachmentLoadOp::DontCare,
                AttachmentStoreOp::DontCare,
                ImageLayout::Undefined,
                ImageLayout::PresentSrcKhr
            )
        },
        {
            SubpassDescription(
                PipelineBindPoint::Graphics,
                std::begin(colorAttachmentRefs), std::end(colorAttachmentRefs),
                nullptr, nullptr,
                nullptr
            )
        },
        {
            SubpassDependency(
                ExternalSubpassIndex,
                GraphicSubpassIndex,
                PipelineStageFlags::ColorAttachmentOutput,
                PipelineStageFlags::ColorAttachmentOutput,
                AccessFlags::None,
                AccessFlags::ColorAttachmentWrite,
                DependencyFlags::None
            )
        }
    );
}

UI::App::BackendInstance::~BackendInstance(void) noexcept
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

UI::App::BackendInstance::BackendInstance(const std::string_view windowTitle,
        const Point windowPos, const Size windowSize, const WindowFlags windowFlags) noexcept
{
    kFEnsure(!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS),
        "UI::App: Couldn't initialize SDL2 video & events subsystems");

    // Check window position special values
    Point pos = windowPos;
    if (pos.x == UndefinedWindowPos.x)
        pos.x = SDL_WINDOWPOS_UNDEFINED;
    else if (pos.x == CenteredWindowPos.x)
        pos.x = SDL_WINDOWPOS_CENTERED;
    if (pos.y == UndefinedWindowPos.y)
        pos.y = SDL_WINDOWPOS_UNDEFINED;
    else if (pos.y == CenteredWindowPos.y)
        pos.y = SDL_WINDOWPOS_CENTERED;

    // Check window size special values
    Size size = windowSize;
    SDL_Rect rect;
    SDL_GetDisplayUsableBounds(0, &rect);
    if (size.width == FillWindowSize.width)
        size.width = static_cast<Pixel>(rect.w);
    if (size.height == FillWindowSize.height)
        size.height = static_cast<Pixel>(rect.h);

    // Create the backend window
    Core::String str(windowTitle);
    window = SDL_CreateWindow(
        str.c_str(),
        static_cast<int>(pos.x), static_cast<int>(pos.y),
        static_cast<int>(size.width), static_cast<int>(size.height),
        ToWindowFlags(windowFlags) | SDL_WINDOW_VULKAN
    );
    kFEnsure(window, "UI::App::CreateBackendWindow: Couldn't create window '", SDL_GetError(), '\'');

    if (!Core::HasFlags(windowFlags, WindowFlags::Borderless) && windowSize.height == FillWindowSize.height) {
        int top, left, bottom, right;
        if (SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right) < 0)
            return;
        SDL_SetWindowSize(window, static_cast<int>(size.width), static_cast<int>(size.height) - top);
        SDL_SetWindowPosition(window, 0, top);
    }
}

UI::App::~App(void) noexcept
{
    kFEnsure(_Instance,
        "UI::App: App already destroyed");
    _Instance = nullptr;

    // Remove signal handler
    ::signal(SIGINT, nullptr);

    // Wait GPU to stop
    gpu().logicalDevice().waitIdle();
}

UI::App::App(
    const std::string_view windowTitle,
    const Point windowPos,
    const Size windowSize,
    const WindowFlags windowFlags,
    const Core::Version version,
    const std::size_t workerCount,
    const std::size_t taskQueueSize,
    const std::size_t eventQueueSize
) noexcept
    : _backendInstance(windowTitle, windowPos, windowSize, windowFlags)
    , _gpu(_backendInstance.window, MakeFrameImageModels(), { &MakeRenderPass }, version)
    , _executor(workerCount, taskQueueSize, eventQueueSize)
{
    kFEnsure(!_Instance,
        "UI::App: App already initialized");
    _Instance = this;

    ::signal(SIGINT, [](const auto) {
        kFInfo("UI::App: Application interrupted");
        App::Get().stop();
    });

    // Event pipeline
    _executor.addPipelineInline<EventPipeline>(
        DefaultEventRate,
        [](void) -> bool {
            ::SDL_PumpEvents();
            return true;
        }
    ); // @todo make pipeline without events with assert !
    _executor.addSystem<EventSystem>();

    // Present pipeline
    _executor.addPipeline<PresentPipeline>(
        DefaultFrameRate,
        [](void) -> bool {
            return GPU::GPUObject::Parent().commandDispatcher().tryAcquireNextFrame();
        }
    );
    _executor.addSystem<PresentSystem>();
    _uiSystem = &_executor.addSystem<UISystem, ECS::RunBefore<PresentSystem>>(_backendInstance.window);
}

UI::Size UI::App::windowSize(void) const noexcept
{
    return _uiSystem->windowSize();
}

void UI::App::setWindowSize(const Size size) noexcept
{
    SDL_SetWindowSize(_backendInstance.window, static_cast<int>(size.width), static_cast<int>(size.height));
    _gpu->dispatchViewSizeChanged();
}

void UI::App::run(void) noexcept
{
    _executor.run();
}

void UI::App::stop(void) noexcept
{
    _executor.stop();
}
