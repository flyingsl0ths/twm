#include <iostream>

#include "common.hpp"
#include "manager.hpp"

namespace window_manager
{

static bool s_wm_detected {};

std::optional<window_manager_t> create() noexcept(
    std::is_nothrow_constructible_v<std::optional<window_manager_t>>)
{
    Display* const display = XOpenDisplay(nullptr);

    if (display == nullptr)
    {
        std::cerr << "Failed to open X display " << XDisplayName(nullptr)
                  << '\n';
        return {};
    }

    return std::make_optional<window_manager_t>(
        {.display = display, .root = DefaultRootWindow(display)});
}

s32 on_xerror(Display* const /*unused*/, XErrorEvent* const /*unused*/)
{
    return 0;
}

s32 on_wm_detected(Display* const /*unused*/, XErrorEvent* const event)
{
    if (event->error_code == BadAccess) { s_wm_detected = true; }

    return 0;
}

void init(window_manager_t man)
{
    // 1. Init.
    //   a. Select events on root window. Use an error handler so we can
    //   exit gracefully if another wm is already running
    s_wm_detected = false;

    XSetErrorHandler(&on_wm_detected);

    XSelectInput(man.display,
                 man.root,
                 SubstructureRedirectMask | SubstructureNotifyMask);

    XSync(man.display, 0);

    if (s_wm_detected)
    {
        std::cerr << "Detected another window manager on display "
                  << XDisplayString(man.display) << '\n';
        return;
    }

    //  b. Setup another error handler
    XSetErrorHandler(&on_xerror);
}

void event_loop(window_manager_t man)
{
    while (true)
    {
        XEvent event {};

        XNextEvent(man.display, &event);

        switch (event.type)
        {
            case CreateNotify: on_create_notify(event.xcreatewindow); break;
            case DestroyNotify: on_destroy_notify(event.xdestroywindow); break;
            case ReparentNotify: on_reparent_notify(event.xreparent); break;
            default: std::cerr << "Ignored event\n";
        }
    }
}

void run(window_manager_t man)
{
    init(man);

    event_loop(man);
}

} // namespace window_manager
