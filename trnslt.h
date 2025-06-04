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
	std::string originalMessage;
	std::string translatedMessage;
	uint8_t chatChannel;
	std::string playerName;
	uint8_t team = 0;
};

class trnslt: public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
	transliteration::TransliterationPack pack;

	std::vector<LogMessage> logMessages;
	std::vector<LogMessage> toCancelQueue;
	std::vector<LogMessage> toFixQueue;
	void onLoad() override;
	void onUnload() override;

public:
	void logTranslation(ChatMessage1* message);
	void HookChat();
	void UnHookChat();
	void RenderSettings() override;
	void googleTranslate(ChatMessage1* message);
	void alterMsg();
	void HookGameStart();
	void UnhookGameStart();
	void drawMessageLog();
private:
	std::string SearchBuffer = "";
};
