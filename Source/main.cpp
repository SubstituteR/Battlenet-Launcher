#include <array>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <format>
#include <optional>
#include <shellapi.h>
#include <thread>
#include <tlhelp32.h>
#include <unordered_set>
#include <wil/resource.h>
#include <WinReg.hpp>
#include <CommCtrl.h>
#include <future>
#include <map>
#include <ranges>
#include <set>
#include <span>


#pragma comment(lib,"comctl32.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace win
{
	namespace process
	{
		auto create(const std::wstring_view& process_name, const std::wstring_view& command_line = L"", unsigned long create_flags = 0)-> PROCESS_INFORMATION
		{
			STARTUPINFOW startupInfo{};
			PROCESS_INFORMATION	process_information{};
			startupInfo.cb = sizeof(startupInfo);
			CreateProcess(std::wstring(process_name).c_str(), std::format(L"{} {}", process_name, command_line).data(), nullptr, nullptr, FALSE, create_flags, nullptr, nullptr, &startupInfo, &process_information);
			return process_information;
		}
		auto terminate(const unsigned long process_id, const std::uint32_t exit_code = 0)
		{
			const wil::unique_handle process_handle(OpenProcess(PROCESS_TERMINATE, false, process_id));
			if (!process_handle)
				return;
			TerminateProcess(process_handle.get(), exit_code);
		}
		auto has_thread(const unsigned long process_id, const std::wstring_view& thread_name) -> bool
		{
			wil::unique_handle tlSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, process_id));
			if (!tlSnapshot)
				return false;

			THREADENTRY32 thread_entry;
			thread_entry.dwSize = sizeof(THREADENTRY32);

			if (!Thread32First(tlSnapshot.get(), &thread_entry))
				return false;

			do
			{
				wil::unique_handle thread_handle(OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, thread_entry.th32ThreadID));

				if (!thread_handle)
					continue;

				auto thread_description = wil::make_unique_hlocal<PWCHAR>();
				HRESULT hr = GetThreadDescription(thread_handle.get(), thread_description.get());

				if (*thread_description == thread_name)
					return true;

			} while (Thread32Next(tlSnapshot.get(), &thread_entry));

			return false;
		}
	}
	
	auto find_processes(const std::wstring_view& process_name) -> std::unordered_set<unsigned long>
	{
		std::unordered_set<unsigned long> process_ids;
		wil::unique_handle tlSnapShot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));
		if (!tlSnapShot)
			return process_ids;

		PROCESSENTRY32 process_entry;
		process_entry.dwSize = sizeof(PROCESSENTRY32);

		if (!Process32First(tlSnapShot.get(), &process_entry))
			return process_ids;

		do
		{
			if (process_entry.szExeFile == process_name)
				process_ids.insert(process_entry.th32ProcessID);

		} while (Process32Next(tlSnapShot.get(), &process_entry));

		return process_ids;
	}
}






namespace bnet::launcher
{

	struct launcher_options
	{
		std::wstring name;
		std::wstring bnet_arguments;
		std::wstring game_executable;
	};

	const std::unordered_map<std::wstring, launcher_options> launch_options =
	{
		{(L"rtro"),{ L"Blizzard Arcade Collection", LR"(--exec="launch rto")",L"client.exe" }},
		{(L"codhq"),{L"Call of Duty HQ", LR"(--exec="launch AUKS")",L"cod.exe" }},
		{( L"codmw2"),{L"Call of Duty: MWII | WZ2.0", LR"(--exec="launch AUKS")",L"cod22-cod.exe" }},
		{(L"codbo4"),{L"Call of Duty: Black Ops 4", LR"(--exec="launch VIPR")",L"BlackOps4.exe" }},
		{(L"codbocw"),{ L"Call of Duty: Black Ops Cold War", LR"(--exec="launch ZEUS")",L"BlackOpsColdWar.exe" }},
		{(L"codmw2019"),{ L"Call of Duty: Modern Warfare", LR"(--exec="launch ODIN")",L"ModernWarfare.exe" }},
		{(L"codmw2cr"),{ L"Call of Duty: Modern Warfare 2 Campaign Remastered", LR"(--exec="launch LAZR")",L"MW2CR.exe" }},
		{(L"codvg"),{ L"Call of Duty: Vanguard", LR"(--exec="launch FORE")",L"Vanguard.exe" }},
		{(L"cb4"),{ L"Crash Bandicoot 4: It's About Time", LR"(--exec="launch WLBY")",L"CrashBandicoot4.exe" }},
		{(L"d2r"),{ L"Diablo 2: Resurrected", LR"(--exec="launch OSI")",L"D2R.exe" }},
		{(L"d3"),{ L"Diablo 3", LR"(--exec="launch D3")",L"Diablo III%.exe" }},
		{(L"d3ptr"),{ L"Diablo 3 Public Test Realm", LR"(--exec="launch d3t")",L"Diablo III%.exe" }},
		{(L"d4"),{ L"Diablo 4", LR"(--exec="launch Fen")",L"Diablo IV.exe" }},
		{(L"di"),{ L"Diablo Immortal", LR"(--exec="launch ANBS")",L"DiabloImmortal.exe" }},
		{(L"hs"),{ L"Hearthstone", LR"(--exec="launch WTCG")",L"Hearthstone.exe" }},
		{(L"hots"),{ L"Heroes of the Storm", LR"(--exec="launch Hero")",L"HeroesSwitcher%.exe" }},
		{(L"hotsptr"),{ L"Heroes of the Storm Public Test Realm", LR"(--exec="launch herot")",L"HeroesSwitcher%.exe" }},
		{(L"ow"),{ L"Overwatch", LR"(--exec="launch Pro")",L"Overwatch.exe" }},
		{(L"owptr"),{ L"Overwatch Public Test Realm", LR"(--exec="launch prot")",L"Overwatch.exe" }},
		{(L"scr"),{ L"Starcraft Remastered", LR"(--exec="launch S1")",L"StarCraft.exe" }},
		{(L"sc2"),{ L"Starcraft 2", LR"(--exec="launch S2")",L"SC2Switcher%.exe" }},
		{(L"w1"),{ L"Warcraft 1: Orcs & Humans", LR"(--exec="launch W1")",L"DOSBox.exe" }},
		{(L"w1r"),{ L"Warcraft 1: Remastered", LR"(--exec="launch W1R")",L"Warcraft.exe" }},
		{(L"w2"),{ L"Warcraft 2: Battle.net Edition", LR"(--exec="launch W2")",L"Warcraft II BNE.exe" }},
		{(L"w2r"),{ L"Warcraft 2: Remastered", LR"(--exec="launch W2R")",L"Warcraft II.exe" }},
		{(L"w3"),{ L"Warcraft 3: Reforged", LR"(--exec="launch W3")",L"Warcraft III.exe" }},
		{(L"wow"),{ L"World of Warcraft", LR"(--exec="launch WoW")",L"Wow.exe" }},
		{(L"wowclassic"),{ L"World of Warcraft Classic", LR"(--exec="launch WoWC")",L"WowClassic.exe" }},
		{(L"wowptr"),{ L"World of Warcraft Public Test Realm", LR"(--exec="launch wowt")",L"WowT.exe" }},
		//{L"wowclassicera", L"", LR"(--exec="launch wow_classic_era")", L"WowClassic.exe" },


	};

	auto find_executable() -> std::wstring
	{
		constexpr auto default_value = L"";
		winreg::RegKey bnet_regkey{};
		if (!bnet_regkey.TryOpen(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Battle.net)"))
			return default_value;

		const auto install_directory = bnet_regkey.TryGetStringValue(L"InstallLocation");

		if (!install_directory.IsValid())
			return default_value;

		return std::format(LR"({}\Battle.net.exe)", install_directory.GetValue());
	}

	auto run() -> unsigned long
	{
		return win::process::create(find_executable()).dwProcessId;
	}

	auto run_game(const std::wstring_view& game) -> unsigned long
	{
		if (!launch_options.contains(std::wstring(game)))
		{
			MessageBox(nullptr, L"Please specify a valid parameter: \n\nBlizzard Arcade Collection = rtro\nCall of Duty HQ = codhq\nCall of Duty: Black Ops 4 = codbo4\nCall of Duty: Black Ops Cold War = codbocw\nCall of Duty: Modern Warfare = codmw2019\nCall of Duty: Modern Warfare 2 Campaign Remastered = codmw2cr\nCall of Duty: MWII | WZ2.0 = codmw2\nCall of Duty: Vanguard = codvg\nCrash Bandicoot 4: It's About Time = cb4\nDiablo 2: Resurrected = d2r\nDiablo 3 = d3\nDiablo 3 Public Test Realm = d3ptr\nDiablo 4 = d4\nDiablo Immortal = di\nHearthstone = hs\nHeroes of the Storm = hots\nHeroes of the Storm Public Test Realm = hotsptr\nOverwatch = ow\nOverwatch Public Test Realm = owptr\nStarcraft 2 = sc2\nStarcraft Remastered = scr\nWarcraft 1: Orcs & Humans = w1\nWarcraft 1: Remastered = w1r\nWarcraft 2: Battle.net Edition = w2\nWarcraft 2: Remastered = w2r\nWarcraft 3: Reforged = w3\nWorld of Warcraft = wow\nWorld of Warcraft Classic = wowclassic\nWorld of Warcraft Public Test Realm = wowptr", L"Error", MB_OK);
			return 0;
		}
		return win::process::create(find_executable(), LR"(--exec="launch WoWC")").dwProcessId;
	}

	auto wait(const unsigned long process_id) -> void
	{
		using namespace std::chrono_literals;
		// Generally the launcher is ready to launch a game once this thread is spawned.
		//while (!win::process::has_thread(process_id, L"CompositorTileWorkerBackground"))
		//{
		//	std::this_thread::sleep_for(1s);
		//}

		//std::this_thread::sleep_for(3s);
		std::this_thread::sleep_for(10s);
	}

	auto kill(const unsigned long process_id) -> void
	{
		win::process::terminate(process_id);
	}

	auto kill() -> void
	{
		for (const auto process_id : win::find_processes(L"Battle.net.exe"))
			kill(process_id);
	}
}



namespace ui::dialogs
{
	auto help_dialog() -> void
	{

		std::wstring help;
		std::vector<std::pair<std::wstring, bnet::launcher::launcher_options>> options{bnet::launcher::launch_options.begin(), bnet::launcher::launch_options.end()};
		std::ranges::sort(options, [](auto&& first, auto&& second)
		{
			return second.second.name > first.second.name;
		});

		for(auto& [key, value] : options)
			help = std::format(L"{}{}: {}\n", help, value.name, key);

		const TASKDIALOGCONFIG config =
		{
			.cbSize = sizeof(TASKDIALOGCONFIG),
			.hwndParent = nullptr,
			.dwCommonButtons = TDCBF_OK_BUTTON,
			.pszWindowTitle = L"Help",
			.pszMainIcon = TD_INFORMATION_ICON,
			.pszMainInstruction = L"Run this program with one of the follow game identifiers",
			.pszContent = help.c_str(),
		};

		static_cast<void>(TaskDialogIndirect(&config, nullptr, nullptr, nullptr));
	}
	auto invalid_game_dialog() -> void
	{
		MessageBox(nullptr, L"Invalid Game Option", L"Error", MB_OK | MB_ICONERROR);
	}

	auto no_game_dialog() -> std::wstring {
		constexpr int close_button_id = 2;
		auto wndproc = [](_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ LONG_PTR lpRefData)
		{
			if (msg != TDN_HYPERLINK_CLICKED)
				return S_OK;

			const auto hyperlink = std::wstring(std::bit_cast<const wchar_t*>(lParam));
			if (hyperlink == L"help")
				help_dialog();
			else
				ShellExecute(nullptr, L"open", hyperlink.c_str(), nullptr, nullptr, SW_SHOW);

			return S_OK;
		};
		auto generate_radio_buttons = []()
		{
			int counter = 0;
			std::vector<TASKDIALOG_BUTTON> game_options{};
			std::unordered_map<int, std::wstring> game_ids{};


			game_options.reserve(bnet::launcher::launch_options.size());
			game_ids.reserve(bnet::launcher::launch_options.size());

			for (const auto& [key, value] : bnet::launcher::launch_options)
			{
				game_options.emplace_back(counter, value.name.c_str());
				game_ids[counter] = key;
				++counter;
			}

			std::sort(game_options.begin(), game_options.end(), [](auto&& first, auto&& second)
			{
					return std::wstring_view(second.pszButtonText) > std::wstring_view(first.pszButtonText);
			});

			return std::make_pair(game_options, game_ids);
		};
		auto radio_buttons = generate_radio_buttons();

		TASKDIALOG_BUTTON action_buttons[] =
		{
			{0, L"Launch Game"}
		};


		const TASKDIALOGCONFIG config =
		{
			.cbSize = sizeof(TASKDIALOGCONFIG),
			.hwndParent = nullptr,
			.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION,
			.pszWindowTitle = L"Alert",
			.pszMainIcon = TD_INFORMATION_ICON,
			.pszMainInstruction = L"Specify which game to launch",
			.pszContent = L"Launched without specifying a game.\nPlease choose from the following:",\
			.cButtons = static_cast<std::uint32_t>(std::size(action_buttons)),
			.pButtons = action_buttons,
			.cRadioButtons = static_cast<std::uint32_t>(radio_buttons.first.size()),
			.pRadioButtons = radio_buttons.first.data(),
			.nDefaultRadioButton = radio_buttons.first.begin()->nButtonID,
			.pszFooter = LR"(<a href="help">Help</a> | <a href="https://github.com/ShizCalev/Battlenet-Launcher/issues">Open Github Issue</a>)",
			.pfCallback = wndproc,
		};

		int chosen_action = 0;
		int chosen_game = 0;

		static_cast<void>(TaskDialogIndirect(&config, &chosen_action, &chosen_game, nullptr));

		if (chosen_action == close_button_id)
			return L"";

		return radio_buttons.second[chosen_game];

	}
}

int wmain(int argc, wchar_t **argv)
{
	FreeConsole();
	InitCommonControls();

	auto game = argv[1] ? argv[1] : ui::dialogs::no_game_dialog();

	if (game.length() == 0)
		return 0; // User has cancelled

	if(!bnet::launcher::launch_options.contains(game))
	{
		ui::dialogs::invalid_game_dialog();
		return 0;
	}

	bnet::launcher::kill();
	bnet::launcher::wait(bnet::launcher::run());
	bnet::launcher::run_game(game);

	/*
	LPCTSTR WindowName = L"Battle.net";
	HWND Find = FindWindow(NULL, WindowName);
	//close all the battle.net client windows that open up when you launch a game. gross.
	while (Find) {
		PostMessage(Find, WM_CLOSE, 0, 0);
		Sleep(1);
		Find = FindWindow(NULL, WindowName);
	}
	//wait for game to close
	while (IsRunning(gEX)){
		Sleep(1000);
	}
	//usually we'd sleep here before closing the client to allow for cloudsave syncing, but battle.net's saves are typically handled by the games themselves.
	//Close battlenet
	Kill(L"Battle.net.exe");
	return FALSE;
	*/
}
