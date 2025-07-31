#include "args.h"

#include "resources.h"
#include "utils.h"

#include <argparse/argparse.hpp>
#include <windows.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace app::args {

argparse::ArgumentParser app_args(APP_NAME);

void init()
{
	int argc;
	auto argvw = ::CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector<std::string> args;
	args.push_back(APP_NAME);

	for (int i = 1; i < argc; ++i)
	{
		auto wstr = std::wstring(argvw[i]);
		auto str = utils::strings::to_string(wstr);
		args.push_back(str);
	}

	app_args.add_argument("-m", "--mode").nargs(1).default_value(std::string(""));
	app_args.add_argument("-e", "--edit-profile").flag();
	app_args.add_argument("-p", "--toggle-psr").flag();
	app_args.add_argument("-p", "--toggle-psr-restart").flag().hidden();
	app_args.add_argument("-a", "--toggle-auto-start").flag();

	app_args.parse_known_args(args);
}

std::optional<api::power::mode> get_mode()
{
	static std::unordered_map<std::string, api::power::mode> modes{
		{"best", api::power::mode::MODE_BEST_PERFORMANCE},
		{"balance", api::power::mode::MODE_BALANCE},
		{"battery", api::power::mode::MODE_BETTER_BATTERY},
	};

	auto arg = app_args.get<std::string>("--mode");
	if (arg == "")
		return std::nullopt;
	return modes[arg];
}

bool has_open_edit_profile()
{
	return app_args.get<bool>("--edit-profile");
}

bool has_toggle_psr()
{
	return app_args.get<bool>("--toggle-psr");
}

bool has_toggle_psr_restart()
{
	return app_args.get<bool>("--toggle-psr-restart");
}

bool has_toggle_auto_start()
{
	return app_args.get<bool>("--toggle-auto-start");
}

} // namespace app::args
