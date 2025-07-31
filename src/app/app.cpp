#include "app.h"

#include "resources.h"
#include "utils.h"
#include "api/mediakey.h"
#include "api/power.h"
#include "api/windows.h"
#include "args.h"
#include "settings/app.h"

#include <windows.h>

#include <format>
#include <vector>
#include <iostream>

namespace app {

constexpr int WM_SYSTEM_TRAY = WM_APP + 0;

enum app_menu : UINT
{
	MODE_BEGIN = 1,

	PSR = 5,
	AUTO_START,
	MEDIAKEY_CONTROL,

	EXIT,

	PROFILE_EDIT,
	PROFILE_ITEM_BEGIN,
};

} // namespace app

namespace app {

HWND handle = nullptr;
HMENU main_menu = nullptr, profile_menu = nullptr;

std::vector<api::power::profile> recent_profiles;

void run()
{
	// 시작 옵션 처리
	{
		bool has_action = false;

		if (auto mode = app::args::get_mode())
		{
			api::power::apply_power_mode(*mode);
			has_action = true;
		}

		if (app::args::has_open_edit_profile())
		{
			open_edit_profile();
			has_action = true;
		}

		if (app::args::has_toggle_psr())
		{
			toggle_psr(false);
			has_action = true;
		}

		if (app::args::has_toggle_auto_start())
		{
			toggle_auto_start();
			has_action = true;
		}

		if (app::args::has_toggle_psr_restart())
			toggle_psr(false);

		if (app::args::has_cycle_mode()){
			api::power::cycle_power_mode();
			has_action = true;
		}

		if (has_action)
			return;
	}

	WNDCLASS wc{};
	wc.lpfnWndProc = wndproc;
	wc.hInstance = ::GetModuleHandle(nullptr);

	auto app_name = utils::strings::to_wstring(APP_NAME);
	auto class_name = std::format(L"{}Class", app_name);
	wc.lpszClassName = class_name.c_str();

	::RegisterClass(&wc);

	handle = ::CreateWindowEx(0, wc.lpszClassName, app_name.c_str(), 0, 0, 0, 0, 0, 0, 0, 0, wc.hInstance);

	NOTIFYICONDATA nid{};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = handle;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage = WM_SYSTEM_TRAY;
	nid.hIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	auto tip = utils::strings::to_wstring(APP_DESCRIPTION);
	utils::strings::wstring_copy(nid.szTip, tip);

	::Shell_NotifyIcon(NIM_ADD, &nid);
	
	auto media = api::mediakey();
	if (settings::app::is_mediakey_control_enabled())
		media.hook();
	on_menu_create();
	mainloop();

	::Shell_NotifyIcon(NIM_DELETE, &nid);
}

void mainloop()
{
	MSG msg{};
	while (::GetMessage(&msg, nullptr, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_SYSTEM_TRAY && (lparam == WM_RBUTTONUP || lparam == WM_LBUTTONUP))
	{
		on_menu_update();
		on_menu_show();
	}
	else if (msg == WM_DESTROY)
	{
		on_app_exit();
	}

	return ::DefWindowProc(hwnd, msg, wparam, lparam);
}

void on_app_exit()
{
	if (main_menu)
		::DestroyMenu(main_menu);
	if (profile_menu)
		::DestroyMenu(profile_menu);

	::PostQuitMessage(0);
}

void on_menu_create()
{
	main_menu = ::CreatePopupMenu();
	profile_menu = ::CreatePopupMenu();

	for (size_t i = 0; i < api::power::mode::MODES.size(); i++)
	{
		const auto &mode = api::power::mode::MODES[i];
		::AppendMenu(main_menu, MF_STRING, app_menu::MODE_BEGIN + i, mode.name.c_str());
	}

	// ::AppendMenu(main_menu, MF_SEPARATOR, 0, nullptr);
	// ::AppendMenu(main_menu, MF_POPUP, (UINT_PTR)profile_menu, L"전원 프로필");
	// ::AppendMenu(profile_menu, MF_STRING, app_menu::PROFILE_EDIT, L"편집...");
	// ::AppendMenu(profile_menu, MF_SEPARATOR, 0, nullptr);

	// ::AppendMenu(main_menu, MF_SEPARATOR, 0, nullptr);
	// ::AppendMenu(main_menu, MF_STRING, app_menu::PSR, L"PSR 활성화");

	::AppendMenu(main_menu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(main_menu, MF_STRING, app_menu::AUTO_START, L"Windows 시작 시 자동 실행");

	::AppendMenu(main_menu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(main_menu, MF_STRING, app_menu::MEDIAKEY_CONTROL, L"정지/재생 버튼 컨트롤");

	::AppendMenu(main_menu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(main_menu, MF_STRING, app_menu::EXIT, L"끝내기");
}

void on_menu_update()
{
	auto &current_mode = api::power::get_power_mode();
	for (size_t i = 0; i < api::power::mode::MODES.size(); i++)
	{
		const auto &mode = api::power::mode::MODES[i];
		const auto current = current_mode.guid == mode.guid;
		::CheckMenuItem(main_menu, app_menu::MODE_BEGIN + i, current ? MF_CHECKED : MF_UNCHECKED);
	}

	// ::CheckMenuItem(main_menu, app_menu::PSR, settings::app::is_psr_enabled() ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(main_menu, app_menu::AUTO_START, settings::app::is_auto_start() ? MF_CHECKED : MF_UNCHECKED);

	// for (size_t i = 0; i < recent_profiles.size(); i++)
	// {
	// 	const auto &profile = recent_profiles[i];
	// 	::DeleteMenu(profile_menu, app_menu::PROFILE_ITEM_BEGIN + i, MF_BYCOMMAND);
	// }

	// recent_profiles = api::power::get_power_profiles();

	// for (size_t i = 0; i < recent_profiles.size(); i++)
	// {
	// 	const auto &profile = recent_profiles[i];
	// 	auto name = profile.name.c_str();
	// 	::AppendMenu(
	// 		profile_menu,
	// 		MF_STRING | (profile.enabled ? MF_CHECKED : MF_UNCHECKED),
	// 		app_menu::PROFILE_ITEM_BEGIN + i,
	// 		name);
	// }
	// auto media_instance = api::mediakey::instance;
	// if (media_instance)
	// 	::CheckMenuItem(main_menu, app_menu::MEDIAKEY_CONTROL, media_instance->active() ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(main_menu, app_menu::MEDIAKEY_CONTROL, settings::app::is_mediakey_control_enabled() ? MF_CHECKED : MF_UNCHECKED);
}

void on_menu_show()
{
	POINT pt;
	::GetCursorPos(&pt);

	::SetForegroundWindow(handle);

	UINT cmd = ::TrackPopupMenu(main_menu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, handle, nullptr);

	if (cmd == app_menu::PSR)
	{
		toggle_psr(true);
	}
	else if (cmd == app_menu::AUTO_START)
	{
		toggle_auto_start();
	}
	else if (cmd == app_menu::EXIT)
	{
		on_app_exit();
	}
	else if (cmd == app_menu::MEDIAKEY_CONTROL)
	{
		toggle_mediakey_control();
	}
	// else if (cmd == app_menu::PROFILE_EDIT)
	// {
	// 	open_edit_profile();
	// }
	// else if (cmd >= app_menu::MODE_BEGIN && cmd < app_menu::MODE_BEGIN + static_cast<UINT>(api::power::mode::MODES.size()))
	// {
	// 	int index = cmd - app_menu::MODE_BEGIN;
	// 	auto &mode = api::power::mode::MODES[index];
	// 	api::power::apply_power_mode(mode);
	// }
	// else if (cmd >= app_menu::PROFILE_ITEM_BEGIN)
	// {
	// 	const auto &profile = recent_profiles[cmd - app_menu::PROFILE_ITEM_BEGIN];
	// 	api::power::apply_power_profile(profile);
	// }
}

void toggle_mediakey_control(){
	auto media_control = api::mediakey::instance;
	auto bval = !settings::app::is_mediakey_control_enabled();
	settings::app::set_mediakey_control(bval);
	if (bval)
		media_control->hook();
	else
		media_control->unhook();
}

void open_edit_profile()
{
	api::windows::run_process("control powercfg.cpl");
}

void toggle_psr(bool retain_app)
{
	if (api::windows::is_user_administrator())
	{
		auto enabled = !settings::app::is_psr_enabled();
		settings::app::set_psr_enabled(enabled);
	}
	else
	{
		api::windows::restart_as_administrator(retain_app);
	}
}

void toggle_auto_start()
{
	auto enabled = !settings::app::is_auto_start();
	settings::app::set_auto_start(enabled);
}

} // namespace app
