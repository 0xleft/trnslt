#pragma once

#include <vector>
#include <string>
#include "trnslt.h"

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

void trnslt::googleTranslate(ChatMessage1* message) {
    CurlRequest req;
    req.url = std::format("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl={}&dt=t&dt=bd&dj=1&q={}", cvarManager->getCvar("trnslt_language_to").getStringValue(), urlEncode(wToString(message->Message)));
    std::string playerName = wToString(message->PlayerName);

    HttpWrapper::SendCurlRequest(req, [this, playerName](int code, std::string result) {
        try {
            nlohmann::json data = nlohmann::json::parse(result);
            nlohmann::json sentences = data["sentences"];
            for (int i = 0; i < sentences.size(); i++) {
                nlohmann::json sentence = sentences.at(i);
                std::string trans(sentence["trans"]);
                std::string orig(sentence["orig"]);
                std::string src(data["src"]);

                gameWrapper->Execute([this, orig, src, trans, playerName](GameWrapper* gw) {
                    if (toLower(trans) == toLower(orig) && !cvarManager->getCvar("trnslt_remove_message").getBoolValue()) { return; }
                    
                    gameWrapper->LogToChatbox(trans, std::format("[{}] {}", src, playerName, trans));
                });
            }
        }
        catch (nlohmann::json::exception e) {
            LOG("{}", e.what());
        }
    });
}