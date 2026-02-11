#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <nlohmann/json.hpp>
#include <utils.h> 
#include "pack.hpp"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

extern std::string toLower(std::string str);

struct LogMessage {
	std::string OriginalMessage;
	std::string TranslatedMessage;
	uint8_t ChatChannel;
	std::string PlayerName;
	uint8_t Team = 0;
	std::string TimeStamp = "0:00";
	std::string LangCode;

	bool MatchingMessages() const {
		return toLower(TranslatedMessage) == toLower(OriginalMessage);
	}
};

class trnslt: public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
	transliteration::TransliterationPack pack;

	std::vector<LogMessage> LogMessages;
	void onLoad() override;
	void onUnload() override;

public:
	void LogTranslation(FGFxChatMessage* message);

	void HookGameStart();
	void ReleaseHooks();

	void RenderSettings() override;

	void GoogleTranslate(FGFxChatMessage* message);
	void AlterMsg();

	void RegisterCvars();
private:
	std::string SearchBuffer = "";
};