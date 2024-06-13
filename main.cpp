// Window Includes
#include <Windows.h>
#include <ntstatus.h>
#include <tchar.h>
#include <Shlobj.h>
#include <Psapi.h>
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <utility>

// Custom Includes
#include "overlay/render.h"
#include "game/settings.h"
#include "Vendor/Protection/xorst.h"
#include "Vendor/grdv/global.h"
#include "Vendor/grdv/binary/dropper.h"
#include "Kernal/driver.hpp"
#include "game/aimbot/razer/rzctl.h"
#pragma comment(lib, "ntdll.lib")

#include "Vendor/ImGui/imgui.h"
#include "Vendor/ImGui/imgui_impl_dx11.h"
#include "Vendor/ImGui/imgui_impl_win32.h"

#include "console/console.h"

auto main() -> int
{
    AllocConsole();
    mouse_interface();

    HWND consoleWindow = GetConsoleWindow();
    int opacity = 225;
    SetLayeredWindowAttributes(consoleWindow, 0, opacity, LWA_ALPHA);

    console::SetConsoleSize(80, 20);

    if (!rzctl::init)
    {
        printf("Error Code 1!");
    }

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);
    console::writeAddress("Screen Width  -> ", screen_width);
    console::writeAddress("Screen Height -> ", screen_height);

    if (!io::find_driver())
    {
        console::write("Driver Not Loaded Retard!");
        console::sleep(3000);
        ExitProcess(0);
    }

    if (!globals::developer)
    {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    io::find_process("FortniteClient-Win64-Shipping.exe");
    base_address = io::get_base_address();

    console::writeAddress("Entry Point -> ", base_address);
    console::writeHandle("Handle -> ", io::driver_handle);
    console::writeInt32("ProcID -> ", io::process_id);

    std::thread([&]()
        {
        while (true)
        {
            g_main->cache_entities();
            g_main->gun_loop();
            //g_main->carfly();
        }
        }).detach();

        render->hijack();
        render->imgui();
        render->render();

        return 0;
}