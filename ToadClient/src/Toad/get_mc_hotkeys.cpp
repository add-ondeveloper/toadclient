#include "pch.h"
#include "get_mc_hotkeys.h"

#include "Logger/logger.h"

namespace toadll
{
	namespace fs = std::filesystem;

	static std::unordered_map<std::string, uint8_t> mc_keys;

	static const std::unordered_map<uint8_t, uint8_t> mc_as_vkc
	{
		{ 0, 0 },	// None
		{ 1, 27 },	// Escape
		{ 2, 49 },	// 1
		{ 3, 50 },	// 2
		{ 4, 51 },	// 3
		{ 5, 52 },	// 4
		{ 6, 53 },	// 5
		{ 7, 54 },	// 6
		{ 8, 55 },	// 7
		{ 9, 56 },	// 8
		{ 10, 57 },	// 9
		{ 11, 48 },	// 0
		{ 12, 48 },	// -
		{ 13, 48 },	// =
		{ 14, 8 },	// Backspace
		{ 15, 9 },  // Tab
		{ 16, 81 },	// Q
		{ 17, 87 },	// W
		{ 18, 69 },	// E
		{ 19, 82 },	// R
		{ 20, 84 },	// T
		{ 21, 89 },	// Y
		{ 22, 85 },	// U
		{ 23, 73 },	// I
		{ 24, 79 },	// O
		{ 25, 80 },	// P
		{ 29, 17 },	// LControl
		{ 30, 65 },	// A
		{ 31, 83 },	// S
		{ 32, 68 },	// D
		{ 33, 70 },	// F
		{ 34, 71 },	// G
		{ 35, 72 },	// H
		{ 36, 74 },	// J
		{ 37, 75 },	// K 
		{ 38, 76 },	// L
		{ 42, 16 },	// Left shift
		{ 44, 90 },	// Z
		{ 45, 88 },	// X
		{ 46, 67 },	// C
		{ 47, 86 },	// V
		{ 48, 66 },	// B
		{ 49, 78 },	// N
		{ 50, 77 },	// M
		{ 57, 32 },	// Space
		{ 58, 85 },	// Caps lock

		// other keys & mouse missing 
	};

	static uint8_t get_mc_keycode(const std::string& key_str)
	{
		try
		{
			int i = std::stoi(key_str);

			try
			{
				return mc_as_vkc.at(i);
			}
			catch (std::out_of_range& e)
			{
				LOGERROR("[GetKey] Can't find mapped value for key: {}, key_str:{}", i, key_str);
				return 0;
			}
		}
		catch (std::invalid_argument& e)
		{
			LOGERROR("[GetKey] No conversion possible with: key_str:{}. {}", key_str, e.what());
			return 0;
		}
		catch (std::out_of_range& e)
		{
			LOGERROR("[GetKey] Conversion result out of range for int: key_str:{}. {}", key_str, e.what());
			return 0;
		}
	}

	std::unordered_map<std::string, uint8_t> get_mc_hotkeys()
	{
		mc_keys.clear();

		char* appdata_env = getenv("APPDATA");
		if (!appdata_env)
		{
			LOGERROR("[GetMCHotKeys] Failed to get path to appdata");
			return mc_keys;
		}

		fs::path appdata_path(appdata_env);
		appdata_path = appdata_path / ".minecraft" / "options.txt";

		if (!fs::exists(appdata_path))
		{
			LOGERROR("[GetMCHotKeys] File doesn't exist {}", appdata_path.string());
			return mc_keys;
		}

		std::ifstream mc_options(appdata_path);
		if (!mc_options)
		{
			LOGERROR("[GetMCHotKeys] Failed to open {}", appdata_path.string());
			return mc_keys;
		}

		LOGDEBUG("[GetMCHotkeys] Reading MC's options.txt");
		
		std::string option_line;
		while (std::getline(mc_options, option_line))
		{
			if (option_line.find("key_key") != std::string::npos)
			{
				std::string key;
				std::string key_name;

				size_t splitter = option_line.find('.');
				if (splitter != std::string::npos)
				{
					key_name = option_line.substr(0, splitter); 
					key = option_line.substr(splitter);

					mc_keys[key_name] = get_mc_keycode(key);
				}
			}
		}

		mc_options.close();

		return mc_keys;
	}

}