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

struct ChatMessage1
{
	void* PRI;
	void* Team;
	wchar_t* PlayerName;
	uint8_t PlayerNamePadding[0x8];
	wchar_t* Message;
	uint8_t MessagePadding[0x8];
	uint8_t ChatChannel;
	unsigned long bPreset : 1;
};

struct ChatMessage2 {
	int32_t Team;
	class FString PlayerName;
	class FString Message;
	uint8_t ChatChannel;
	bool bLocalPlayer : 1;
	unsigned char IDPadding[0x48];
	uint8_t MessageType;
};

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
	std::vector<LogMessage> CancelQueue;
	std::vector<LogMessage> FixQueue;
	void onLoad() override;
	void onUnload() override;

public:
	void LogTranslation(ChatMessage1* message);

	void HookGameStart();
	void HookChat();
	void ReleaseHooks();

	void RenderSettings() override;

	void GoogleTranslate(ChatMessage1* message);
	void AlterMsg();

	void SaveSettings();
	void LoadSettings();
private:
	std::string SearchBuffer = "";
};
