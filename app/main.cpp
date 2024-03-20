#include <iostream>

#include "common.hpp"
#include "manager.hpp"

s32 main()
{
    auto wman = window_manager::create();

    if (!(wman && window_manager::was_initialized(*wman)))
    {
        std::cerr << "Failed to initialize window manager\n";
        return -1;
    }

    auto man = *wman;

    window_manager::run(man);

    window_manager::teardown(man);

    return 0;
}
