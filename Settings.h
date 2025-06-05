#pragma once

#include <string>

namespace Settings
{
	inline bool TranslateLocalPlayer = false;
	inline bool Transliterate = false;

	inline bool ShowMatchLog = false;

	inline bool RemoveMessage = true;

	inline bool ShowTimeStamp = true;

	inline bool TranslatePublicChat = true;
	inline bool TranslateTeamChat = true;
	inline bool TranslatePartyChat = false;

	inline std::string TranslateToLanguage = "en";

	inline int TranslateApiIndex = 0;
}