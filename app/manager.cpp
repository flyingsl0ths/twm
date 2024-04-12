#include <cassert>
#include <iostream>
#include <span>

#include "common.hpp"
#include "manager.hpp"

namespace window_manager
{

static bool                     s_wm_detected {};

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

void make_frame(const Window      window,
                window_manager_t& man,
                const bool        existed = false)
{
    constexpr u32     BORDER_WIDTH {3};
    constexpr u32     BORDER_COLOR {0xff0000};
    constexpr u32     BG_COLOR {0x0000ff};

    XWindowAttributes window_attrs {};
    XGetWindowAttributes(man.display, window, &window_attrs);

    // 2. If window was created before window manager started, we should frame
    // it only if it is visible and doesn't set override_redirect.
    if (existed)
    {
        // If the window has structure redirection enabled or is unmapped or
        // invisible (un-viewable)
        if ((window_attrs.override_redirect != 0) ||
            window_attrs.map_state != IsViewable)
        {
            return;
        }
    }

    // 3. Create frame
    const Window frame = XCreateSimpleWindow(man.display,
                                             man.root,
                                             window_attrs.x,
                                             window_attrs.y,
                                             window_attrs.width,
                                             window_attrs.height,
                                             BORDER_WIDTH,
                                             BORDER_COLOR,
                                             BG_COLOR);

    XSelectInput(
        man.display, frame, SubstructureRedirectMask | SubstructureNotifyMask);

    // 4. Add client to save set, so that it will be restored and kept alive if
    // we crash.
    XAddToSaveSet(man.display, window);

    // 5. Reparent client window.
    XReparentWindow(man.display, window, frame, 0, 0);

    // 6. Map frame.
    XMapWindow(man.display, frame);

    /* Used as a failsafe in case of window manager crash
     * to reparent any windows to their closest living ancestor
     * if they were reparented and mapped -- if unmapped --  so as
     * to map an icon to them [the window]
     *
     * Regardless of the cause the actions of the saveset will be performed
     * even if the window manager exits normally
     *
     * It is almost always the case that windows that are reparented/"iconfied"
     * are placed in the wm's saveset
     *
     * These windows are removeed when destroyed, however.
     * */
    man.clients[window] = frame;
    /* TODO:
        // 7. Save frame handle.
        // 8. Grab events for window management actions on client window.
        //   a. Move windows with alt + left button.
        XGrabButton(...);
        //   b. Resize windows with alt + right button.
        XGrabButton(...);
        //   c. Kill windows with alt + f4.
        XGrabKey(...);
        //   d. Switch windows with alt + tab.
        XGrabKey(...);
    */
}

void frame_existing_windows(window_manager_t& man)
{
    //   c. Grab X server to prevent windows from changing under
    //   us while we frame them.
    XGrabServer(man.display);

    //   d. Frame existing top-level windows.
    //     i. Query existing top-level windows.
    Window  returned_root {};
    Window  returned_parent {};
    Window* top_level_windows {};
    u32     total_top_level_windows {};

    XQueryTree(man.display,
               man.root,
               &returned_root,
               &returned_parent,
               &top_level_windows,
               &total_top_level_windows);

    assert(returned_root == man.root);

    const std::span<Window> windows(top_level_windows, total_top_level_windows);

    for (u32 i {}; i < total_top_level_windows; ++i)
    {
        make_frame(windows[i], man, true);
    }

    XFree(top_level_windows);

    XUngrabServer(man.display);
}

void init(window_manager_t& man)
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

    frame_existing_windows(man);
}

void on_configure_request(const XConfigureRequestEvent& event,
                          window_manager_t&             man)
{
    XWindowChanges changes {};

    changes.x            = event.x;
    changes.y            = event.y;
    changes.width        = event.width;
    changes.height       = event.height;
    changes.border_width = event.border_width;
    changes.sibling      = event.above;
    changes.stack_mode   = event.detail;

    const Window window  = event.window;

    if (man.clients.contains(window))
    {
        const Window frame = man.clients[window];

        XConfigureWindow(man.display, frame, event.value_mask, &changes);

        std::cout << "Resize [" << frame << "] to " << '(' << event.width
                  << ", " << event.height << ")\n";
    }

    XConfigureWindow(man.display, window, event.value_mask, &changes);

    std::cout << "Resize " << window << " to " << '(' << event.width << ','
              << event.height << ")\n";
}

void on_create_notify(const XCreateWindowEvent /*unused*/) {}

void on_reparent_notify(const XReparentEvent /*unused*/) {}

void on_map_notify(const XMapEvent /*unused*/) {}

void on_configure_notify(const XConfigureEvent /*unused*/) {}

void on_map_request(const XMapRequestEvent& event, window_manager_t& man)
{
    make_frame(event.window, man);

    XMapWindow(man.display, event.window);
}

void unframe(const Window window, window_manager_t& man)
{
    const Window frame = man.clients[window];

    XUnmapWindow(man.display, frame);

    XReparentWindow(man.display, window, man.root, 0, 0);

    XRemoveFromSaveSet(man.display, window);

    XDestroyWindow(man.display, frame);

    man.clients.erase(window);

    std::cout << "Unframed window " << window << ": [" << frame << "]\n";
}

void on_unmap_notify(const XUnmapEvent& event, window_manager_t& man)
{
    if (!man.clients.contains(event.window))
    {
        std::cout << "UnmapNotify for non-client window " << event.window
                  << '\n';
        return;
    }

    unframe(event.window, man);
}

void on_destroy_notify(XDestroyWindowEvent /* unused */) {}

void event_loop(window_manager_t& man)
{
    while (true)
    {
        XEvent event {};

        XNextEvent(man.display, &event);

        switch (event.type)
        {
            case CreateNotify: on_create_notify(event.xcreatewindow); break;
            case ReparentNotify: on_reparent_notify(event.xreparent); break;
            case MapNotify: on_map_notify(event.xmap); break;
            case ConfigureNotify: on_configure_notify(event.xconfigure); break;
            case UnmapNotify: on_unmap_notify(event.xunmap, man); break;
            case DestroyNotify: on_destroy_notify(event.xdestroywindow); break;
            case ConfigureRequest:
                on_configure_request(event.xconfigurerequest, man);
                break;
            case MapRequest: on_map_request(event.xmaprequest, man); break;
            default: std::cerr << "Ignored event\n"; break;
        }
    }
}

void run(window_manager_t& man) noexcept(false)
{
    init(man);

    event_loop(man);
}

} // namespace window_manager
