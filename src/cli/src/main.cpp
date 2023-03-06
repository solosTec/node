#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory> // for shared_ptr, __shared_ptr_access
#include <string> // for to_string, allocator

#include <analyze_log_v7.h>
#include <send_tcp_ip.h>

#include <ftxui/component/captured_mouse.hpp>     // for ftxui
#include <ftxui/component/component.hpp>          // for MenuEntryAnimated, Renderer, Vertical
#include <ftxui/component/component_base.hpp>     // for ComponentBase
#include <ftxui/component/component_options.hpp>  // for MenuEntryAnimated
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/elements.hpp> // for operator|, separator, Element, Decorator, color, text, hbox, size, bold, frame, inverted, vbox, HEIGHT, LESS_THAN, border
#include <ftxui/screen/color.hpp> // for Color, Color::Blue, Color::Cyan, Color::Green, Color::Red, Color::Yellow
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

using namespace ftxui;

// Define a special style for some menu entry.
MenuEntryOption Colored(ftxui::Color c) {
    MenuEntryOption option;
    option.animated_colors.foreground.enabled = true;
    option.animated_colors.background.enabled = true;
    option.animated_colors.background.active = c;
    option.animated_colors.background.inactive = Color::Black;
    option.animated_colors.foreground.active = Color::White;
    option.animated_colors.foreground.inactive = c;
    return option;
}

int main(void) {

    // auto screen = ScreenInteractive::TerminalOutput();

    // int selected = 0;
    // auto menu = Container::Vertical(
    //     {
    //         MenuEntry(" 1. rear", Colored(Color::Red)),
    //         MenuEntry(" 2. drown", Colored(Color::Yellow)),
    //         MenuEntry(" 3. nail", Colored(Color::Green)),
    //         MenuEntry(" 4. quit", Colored(Color::Cyan)),
    //         MenuEntry(" 5. decorative", Colored(Color::Blue)),
    //         MenuEntry(" 7. costume"),
    //         MenuEntry(" 8. pick"),
    //         MenuEntry(" 9. oral"),
    //         MenuEntry("11. minister"),
    //         MenuEntry("12. football"),
    //         MenuEntry("13. welcome"),
    //         MenuEntry("14. copper"),
    //         MenuEntry("15. inhabitant"),
    //     },
    //     &selected);

    //// Display together the menu with a border
    // auto renderer = Renderer(menu, [&] {
    //     return vbox({
    //                hbox(text("selected = "), text(std::to_string(selected))),
    //                separator(),
    //                menu->Render() | frame,
    //            }) |
    //            border | bgcolor(Color::Black);
    // });

    // screen.Loop(renderer);

    // std::cout << "Selected element = " << selected << std::endl;

    //  F:\backup\customer\sgsw\logs\2023-03-05
    // analyse_log_v7("F:\\backup\\customer\\sgsw\\logs\\2023-03-05");

    send_tcp_ip("localhost", "5200", "F:\\backup\\customer\\sgsw\\logs\\cu_000eef47-0a50-429e-8c25-02b2c58ef229.bin");

    return EXIT_SUCCESS;
}
