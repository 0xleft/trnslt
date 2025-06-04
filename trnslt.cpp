#include "pch.h"
#include "trnslt.h"
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include "Languages.h"
#include "Settings.h"
#include <regex>
#include "Translate.h"
#include <iostream>
#include <regex>
#include <string>

BAKKESMOD_PLUGIN(trnslt, "trnslt", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void trnslt::onLoad() {
    _globalCvarManager = cvarManager;

    Settings::Transliterate = cvarManager->registerCvar("trnslt_should_transliterate", "0").getBoolValue();

    Settings::ShowMatchLog = cvarManager->registerCvar("trnslt_should_show_match_log", "0").getBoolValue();

    Settings::RemoveMessage = cvarManager->registerCvar("trnslt_remove_message", "1").getBoolValue();
    Settings::TranslateLocalPlayer = cvarManager->registerCvar("trnslt_translate_own", "0").getBoolValue();

    Settings::TranslatePublicChat = cvarManager->registerCvar("trnslt_translate_0", "1").getBoolValue();
    Settings::TranslateTeamChat = cvarManager->registerCvar("trnslt_translate_1", "1").getBoolValue();
    Settings::TranslatePartyChat = cvarManager->registerCvar("trnslt_translate_2", "1").getBoolValue();

    Settings::TranslateApiIndex = cvarManager->registerCvar("trnslt_translate_api", "0", "transltae api index", false, true, 0, true, translateApis.size() - 1, true).getIntValue();

    Settings::TranslateToLanguage = cvarManager->registerCvar("trnslt_language_to", "en", "language to translate to", true, false, 0, false, 0, true).getStringValue();

    cvarManager->registerNotifier("trnslt_set_language_to", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            LOG("usage: trnslt_set_language_to <language_code>");
            return;
        }

        std::string langCode = params[1];
        auto it = std::find_if(languageCodes.begin(), languageCodes.end(), [&langCode](const auto& pair) { return pair.second == langCode; });

        if (it != languageCodes.end()) {
            Settings::TranslateToLanguage = langCode;
        } else {
            Settings::TranslateToLanguage = "en";
            LOG("Language code: \"{}\" not found", langCode);
        }

    }, "set transltate to language", PERMISSION_ALL);

    this->HookChat();
    this->HookGameStart();
    this->alterMsg();
}

void trnslt::HookGameStart() {
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", [this](std::string eventName) {
        this->toCancelQueue.clear();
        this->logMessages.clear();
        this->toFixQueue.clear();
    });
}

void trnslt::UnhookGameStart() {
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
}

void trnslt::onUnload() {
    // Set CVars
    cvarManager->getCvar("trnslt_should_transliterate").setValue(Settings::Transliterate);

    cvarManager->getCvar("trnslt_should_show_match_log").setValue(Settings::ShowMatchLog);

    cvarManager->getCvar("trnslt_remove_message").setValue(Settings::RemoveMessage);
    cvarManager->getCvar("trnslt_translate_own").setValue(Settings::TranslateLocalPlayer);

    cvarManager->getCvar("trnslt_translate_0").setValue(Settings::TranslatePublicChat);
    cvarManager->getCvar("trnslt_translate_1").setValue(Settings::TranslateTeamChat);
    cvarManager->getCvar("trnslt_translate_2").setValue(Settings::TranslatePartyChat);

    cvarManager->getCvar("trnslt_translate_api").setValue(Settings::TranslateApiIndex);

    cvarManager->getCvar("trnslt_language_to").setValue(Settings::TranslateToLanguage);

    this->UnHookChat();
    this->UnhookGameStart();
}

void trnslt::HookChat() {
    // https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L509
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.HUDBase_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        if (params) {
            ChatMessage1* message = (ChatMessage1*)params;
            if (message->PlayerName == nullptr) return;
            if (message->PlayerName == gameWrapper->GetPlayerName().ToWideString() && !Settings::TranslateLocalPlayer) { return; }

            if (message->ChatChannel == 0 && !Settings::TranslatePublicChat) { return; }
            if (message->ChatChannel == 1 && !Settings::TranslateTeamChat) { return; }
            if (message->ChatChannel == 2 && !Settings::TranslatePartyChat) { return; }

            if (message->Message == nullptr) return;
            if (message->bPreset) return;

			if (Settings::RemoveMessage) {
				this->toCancelQueue.push_back({ wToString(message->Message), "", message->ChatChannel, wToString(message->PlayerName) });
			}

            logTranslation(message);
        }
    });
}

// https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L495
void trnslt::alterMsg () {
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_Chat_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        FGFxChatMessage* message = (FGFxChatMessage*)params;
        if (!message) return;

        if (Settings::RemoveMessage) {
            // check for message queue todo
            auto it = std::find_if(this->toCancelQueue.begin(), this->toCancelQueue.end(), [message](LogMessage& logMessage) {
				return logMessage.originalMessage == message->Message.ToString() && logMessage.playerName == message->PlayerName.ToString();
			});

            if (it != this->toCancelQueue.end()) {
                message->Message = FS("");
                message->PlayerName = FS("");
                message->TimeStamp = FS("");
                message->ChatChannel = 0;

				this->toCancelQueue.erase(it);
                return;
            }
        }

        auto it = std::find_if(this->toFixQueue.begin(), this->toFixQueue.end(), [message](LogMessage& logMessage) {
            return logMessage.translatedMessage == message->Message.ToString() && logMessage.playerName == message->PlayerName.ToString();
        });

		if (it != this->toFixQueue.end()) {
            message->ChatChannel = it->chatChannel;
            message->Team = it->team;

			this->toFixQueue.erase(it);
		}
    });
}

void trnslt::UnHookChat() {
	gameWrapper->UnhookEvent("Function TAGame.HUDBase_TA.OnChatMessage");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_Chat_TA.OnChatMessage");
}

void trnslt::RenderSettings() {
    ImGui::BeginChild("Info", ImVec2(250, 85), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Info").x / 2);
        ImGui::Text("Info");

        ImGui::Separator();

        ImGui::Text("Language: %s", GetLanguageFromCode(Settings::TranslateToLanguage));
        ImGui::Text("Translation API: %s", translateApis[Settings::TranslateApiIndex].c_str());

        ImGui::Text("Transliterate: %s", Settings::Transliterate ? "Enabled" : "Disabled");
    }
    ImGui::EndChild();

    float baseHeight = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("Language", ImVec2(250, baseHeight), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Language").x / 2);
        ImGui::Text("Language");

        ImGui::Separator();

        if (ImGui::BeginCombo("API", translateApis[Settings::TranslateApiIndex].c_str()))
        {
            for (int i = 0; i < translateApis.size(); i++)
            {
                bool isSelected = (Settings::TranslateApiIndex == i);

                if (ImGui::Selectable(translateApis[i].c_str(), isSelected))
                {
                    Settings::TranslateApiIndex = i;
                    this->SearchBuffer = "";
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImGui::InputTextWithHint("##xxSearch", "Search", &SearchBuffer);

        ImGui::BeginChild("##xxLangChild");
        {
            for (const auto& lang : languageCodes)
            {
                if (SearchBuffer.empty() || toLower(std::string(lang.first)).find(toLower(SearchBuffer)) != std::string::npos)
                {
                    bool isSelected = (Settings::TranslateToLanguage == lang.second);

                    if (ImGui::Selectable(std::string(lang.first).c_str(), isSelected))
                    {
                        Settings::TranslateToLanguage = lang.second;
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
            }
        }ImGui::EndChild();
    }ImGui::EndChild();

    ImGui::SameLine();

    ImGui::SetCursorPosY(21);
    ImGui::BeginChild("Options", ImVec2(250, 89 + baseHeight), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Options").x / 2);
        ImGui::Text("Options");

        ImGui::Separator();

        ImGui::Checkbox("Transliterate", &Settings::Transliterate);
        ImGui::Checkbox("Remove Messages", &Settings::RemoveMessage);
        ImGui::Checkbox("Translate My Messages", &Settings::TranslateLocalPlayer);

        ImGui::Separator();

        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Translate Chats").x / 2);
        ImGui::Text("Translate Chats");

        ImGui::Separator();

        ImGui::Checkbox("Public", &Settings::TranslatePublicChat);
        ImGui::Checkbox("Team", &Settings::TranslateTeamChat);
        ImGui::Checkbox("Party", &Settings::TranslatePartyChat);

        ImGui::Separator();

        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Extra").x / 2);
        ImGui::Text("Extra");

        ImGui::Separator();

        if (ImGui::Button("Reset settings")) {
            Settings::TranslatePublicChat = true;
            Settings::TranslateTeamChat = true;
            Settings::TranslatePartyChat = true;
            Settings::TranslateApiIndex = 0;
            Settings::TranslateToLanguage = "en";
            Settings::RemoveMessage = true;
            Settings::TranslateLocalPlayer = false;
        }
    }ImGui::EndChild();
}

void trnslt::drawMessageLog() {
    ImGui::Begin("Match log", &Settings::ShowMatchLog, ImGuiWindowFlags_AlwaysAutoResize);
    for (auto& msg : this->logMessages) {
		ImGui::Text(std::format("[{}] {}", msg.playerName, msg.originalMessage).c_str());
	}
    ImGui::End();
}

void trnslt::logTranslation(ChatMessage1* message) {

    switch (Settings::TranslateApiIndex) {
    case 0:
        // google
        googleTranslate(message);
        break;
    default:
        break;
    }
}