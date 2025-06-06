#pragma once

#include <vector>
#include <string>
#include "trnslt.h"
#include <Windows.h>

std::wstring utf8ToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


std::string wToString(wchar_t* string) {
    std::wstring ws(string);
	std::string str(ws.begin(), ws.end());
	return str;
}

std::string toLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return str;
}

std::vector<std::string> translateApis = {
	"Google translate",
};

std::string urlEncode(std::string str) {
    // https://stackoverflow.com/questions/154536/encode-decode-urls-in-ced by @tormuto
    std::string new_str = "";
    char c;
    int ic;
    const char* chars = str.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for (int i = 0; i < len; i++) {
        c = chars[i];
        ic = c;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
        else {
            sprintf(bufHex, "%X", c);
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
bool stringReplace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

using namespace nlohmann;

void trnslt::GoogleTranslate(ChatMessage1* message) {
    CurlRequest req;
    req.url = std::format("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl={}&dt=t&dt=bd&dj=1&q={}", cvarManager->getCvar("trnslt_language_to").getStringValue(), urlEncode(wToString(message->Message)));

    std::string PlayerName = wToString(message->PlayerName);
    
    uint8_t Chat = message->ChatChannel;
    uint8_t TeamNum = 0;

    ServerWrapper server = gameWrapper->GetCurrentGameState();

    int GameTime = 0;

    if (server)
    {
        GameTime = server.GetSecondsRemaining();
    }

    TeamWrapper team = TeamWrapper(reinterpret_cast<uintptr_t>(message->Team));

    if (!team.IsNull()) {
		TeamNum = team.GetTeamNum();
    }

    HttpWrapper::SendCurlRequest(req, [this, Chat, PlayerName, TeamNum, GameTime](int code, std::string result) {
        try {
            json data = json::parse(result);

            json sentences = data["sentences"];

            LogMessage logMessage;

            int minutes = GameTime / 60;
            int seconds = GameTime % 60;

            logMessage.TimeStamp = std::format("{}:{:02}", minutes, seconds);

            logMessage.ChatChannel = Chat;
            logMessage.PlayerName = PlayerName;
            logMessage.Team = TeamNum;

            for (int i = 0; i < sentences.size(); i++) {

                if (sentences[i]["trans"] != nullptr)
                    logMessage.TranslatedMessage = sentences[i]["trans"];
                else if (sentences[i]["orig"] != nullptr)
                    logMessage.OriginalMessage = sentences[i]["orig"];

                logMessage.LangCode = data["src"];

                if (cvarManager->getCvar("trnslt_should_transliterate").getBoolValue()) {
                    std::wstring trans = this->pack.transliterate(std::wstring(logMessage.TranslatedMessage.begin(), logMessage.TranslatedMessage.end()), cvarManager->getCvar("trnslt_language_to").getStringValue());
                    logMessage.TranslatedMessage = std::string(trans.begin(), trans.end());
                }

                gameWrapper->Execute([this, logMessage](GameWrapper* gw) {
                    if (logMessage.MatchingMessages() && !cvarManager->getCvar("trnslt_remove_message").getBoolValue())
                        return;

                    this->LogMessages.push_back(logMessage);

                    LogMessage message = logMessage;

                    if (cvarManager->getCvar("trnslt_display_timestamp").getBoolValue())
                        message.PlayerName = std::format("[{}] [{}] {}", logMessage.TimeStamp, logMessage.LangCode, logMessage.PlayerName);
                    else
                        message.PlayerName = std::format("[{}] {}", logMessage.LangCode, logMessage.PlayerName);

                    this->FixQueue.push_back(message);
                    gameWrapper->LogToChatbox(message.TranslatedMessage, message.PlayerName);
                });
            }
        }
        catch (json::exception e) {
            LOG("{}", e.what());
        }
    });
}