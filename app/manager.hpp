#pragma once

#include <optional>
#include <type_traits>

extern "C"
{
#include <X11/Xlib.h>
}

namespace window_manager
{

struct window_manager_t final
{
    Display* display;
    Window   root;
};

inline bool was_initialized(std::optional<window_manager_t> man) noexcept(true)
{
    return man && (*man).display != nullptr;
}

std::optional<window_manager_t> create() noexcept(
    std::is_nothrow_constructible_v<std::optional<window_manager_t>>);

void run(window_manager_t /*unused*/) noexcept(true);

inline void teardown(window_manager_t man) noexcept(true)
{
    XCloseDisplay(man.display);
    man.display = nullptr;
}

} // namespace window_manager
