#include <iostream>

#include "manager.hpp"

namespace window_manager
{

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

void run(window_manager_t /*unused*/) noexcept(true) {}

} // namespace window_manager
