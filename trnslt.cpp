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
#include <fstream>

using namespace nlohmann;

BAKKESMOD_PLUGIN(trnslt, "trnslt", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void trnslt::onLoad() {
    _globalCvarManager = cvarManager;

    this->LoadSettings();

    cvarManager->registerNotifier("trnslt_set_language_to", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            LOG("usage: trnslt_set_language_to <language_code>");
            return;
        }

        std::string langCode = params[1];
        auto it = std::find_if(LanguageCodes.begin(), LanguageCodes.end(), [&langCode](const auto& pair) { return pair.second == langCode; });

        if (it != LanguageCodes.end()) {
            Settings::TranslateToLanguage = langCode;
        } else {
            Settings::TranslateToLanguage = "en";
            LOG("Language code: \"{}\" not found", langCode);
        }

    }, "set transltate to language", PERMISSION_ALL);

    this->HookChat();
    this->HookGameStart();
    this->AlterMsg();
}

void trnslt::HookGameStart() {
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", [this](std::string eventName) {
        this->CancelQueue.clear();
        this->LogMessages.clear();
        this->FixQueue.clear();
    });
}

void trnslt::onUnload() {
    // Set CVars
    this->SaveSettings();
    this->ReleaseHooks();
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
				this->CancelQueue.push_back({ wToString(message->Message), "", message->ChatChannel, wToString(message->PlayerName) });
			}

            logTranslation(message);
        }
    });
}

// https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L495
void trnslt::AlterMsg () {
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_Chat_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        FGFxChatMessage* message = (FGFxChatMessage*)params;
        if (!message) return;

        if (Settings::RemoveMessage) {
            // check for message queue todo
            auto it = std::find_if(this->CancelQueue.begin(), this->CancelQueue.end(), [message](LogMessage& logMessage) {
				return logMessage.OriginalMessage == message->Message.ToString() && logMessage.PlayerName == message->PlayerName.ToString();
			});

            if (it != this->CancelQueue.end()) {
                message->Message = FS("");
                message->PlayerName = FS("");
                message->TimeStamp = FS("");
                message->ChatChannel = 0;

				this->CancelQueue.erase(it);
                return;
            }
        }

        auto it = std::find_if(this->FixQueue.begin(), this->FixQueue.end(), [message](LogMessage& logMessage) {
            return logMessage.TranslatedMessage == message->Message.ToString() && logMessage.PlayerName == message->PlayerName.ToString();
        });

		if (it != this->FixQueue.end()) {
            message->ChatChannel = it->ChatChannel;
            message->Team = it->Team;

			this->FixQueue.erase(it);
		}
    });
}

void trnslt::ReleaseHooks() {
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
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
    float baseWidth = ImGui::GetContentRegionAvail().x;

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
            for (const auto& lang : LanguageCodes)
            {
                if (SearchBuffer.empty() || toLower(std::string(lang.first)).find(toLower(SearchBuffer)) != std::string::npos)
                {
                    bool isSelected = (Settings::TranslateToLanguage == lang.second);

                    if (ImGui::Selectable(std::string(lang.first).c_str(), isSelected))
                    {
                        Settings::TranslateToLanguage = lang.second;
                        this->SearchBuffer = "";
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

        if (ImGui::Button("Clear Message Log")) 
        {
            this->LogMessages.clear();
        }

        if (ImGui::Button("Reset settings")) 
        {
            Settings::TranslatePublicChat = true;
            Settings::TranslateTeamChat = true;
            Settings::TranslatePartyChat = true;
            Settings::TranslateApiIndex = 0;
            Settings::TranslateToLanguage = "en";
            Settings::RemoveMessage = true;
            Settings::TranslateLocalPlayer = false;
        }
    }ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Message Log", ImVec2(baseWidth - ImGui::GetCursorPosX(), 89 + baseHeight), true);
    {
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() / 2 - (ImGui::CalcTextSize("Chat Message Log").x / 2));
        ImGui::Text("Chat Message Log");

        ImGui::Separator();

        for (auto& msg : this->LogMessages) {
            ImGui::Text(std::format("[{}] {}", msg.PlayerName, msg.OriginalMessage).c_str());
        }
    }ImGui::EndChild();
}

void trnslt::SaveSettings()
{
    std::filesystem::path latestSavePath = gameWrapper->GetDataFolder() / "Translate" / "LatestSave.json";

    if (!std::filesystem::exists(latestSavePath.parent_path()))
    {
        std::filesystem::create_directories(latestSavePath.parent_path());
    }

    json saveData;

    saveData["LanguageCode"] = Settings::TranslateToLanguage;
    saveData["TranlateApiIndex"] = Settings::TranslateApiIndex;
    saveData["TranslateLocalPlayer"] = Settings::TranslateLocalPlayer;
    saveData["Transliterate"] = Settings::Transliterate;

    saveData["Chat"]["RemoveMessage"] = Settings::RemoveMessage;
    saveData["Chat"]["PublicChat"] = Settings::TranslatePublicChat;
    saveData["Chat"]["TeamChat"] = Settings::TranslateTeamChat;
    saveData["Chat"]["PartyChat"] = Settings::TranslatePartyChat;

    std::ofstream outFile(latestSavePath);

    if (outFile.is_open())
    {
        outFile << saveData.dump(4);
        outFile.close();
        LOG("Saved Settings!");
    }
    else
    {
        LOG("Failed to open file for saving: {}", latestSavePath.string());
    }
}

void trnslt::LoadSettings()
{
    std::filesystem::path latestSavePath = gameWrapper->GetDataFolder() / "Translate" / "LatestSave.json";

    json saveData;

    if (!std::filesystem::exists(latestSavePath))
    {
        LOG("No latest save file found, using default settings.");
        return;
    }

    std::ifstream inFile(latestSavePath);

    json data = json::parse(inFile, nullptr, true);

    if (data.is_null())
    {
        LOG("Failed to parse latest save file.");
        return;
    }

    if (data.contains("LanguageCode"))
        Settings::TranslateToLanguage = data["LanguageCode"];

    if (data.contains("TranlateApiIndex"))
        Settings::TranslateApiIndex = data["TranlateApiIndex"];

    if (data.contains("TranslateLocalPlayer"))
        Settings::TranslateLocalPlayer = data["TranslateLocalPlayer"];

    if (data.contains("Transliterate"))
        Settings::Transliterate = data["Transliterate"];

    if (data.contains("Chat"))
    {
        if (data["Chat"].contains("RemoveMessage"))
            Settings::RemoveMessage = data["Chat"]["RemoveMessage"];
        if (data["Chat"].contains("PublicChat"))
            Settings::TranslatePublicChat = data["Chat"]["PublicChat"];
        if (data["Chat"].contains("TeamChat"))
            Settings::TranslateTeamChat = data["Chat"]["TeamChat"];
        if (data["Chat"].contains("PartyChat"))
            Settings::TranslatePartyChat = data["Chat"]["PartyChat"];
    }

    inFile.close();
    LOG("Loaded Latest Save.");
}

void trnslt::logTranslation(ChatMessage1* message) {

    switch (Settings::TranslateApiIndex) {
    case 0:
        GoogleTranslate(message);
        break;
    default:
        break;
    }
}