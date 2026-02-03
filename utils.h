#pragma once

#include <string>
#include <vector>
#include <Windows.h>

// hippity hoppity i copyti this from https://github.com/JulienML/BetterChat/commit/dedb44e8e543eb4b40e404257b4ef123404617f1?diff=split#diff-c2a4ceabc49cd85b048435a7850490d7fab95a64ffc33a813b02b6eebf8b14b4
// gracie mille

struct FString
{
public:
    using ElementType = const wchar_t;
    using ElementPointer = ElementType*;

private:
    ElementPointer ArrayData;
    int32_t ArrayCount;
    int32_t ArrayMax;

public:
    FString()
    {
        ArrayData = nullptr;
        ArrayCount = 0;
        ArrayMax = 0;
    }

    FString(ElementPointer other)
    {
        ArrayData = nullptr;
        ArrayCount = 0;
        ArrayMax = 0;

        ArrayMax = ArrayCount = *other ? (wcslen(other) + 1) : 0;

        if (ArrayCount > 0)
        {
            ArrayData = other;
        }
    }

    ~FString() {}

public:
	std::wstring ToWideString() const
	{
		if (!IsValid())
		{
			return std::wstring(ArrayData);
		}
		return std::wstring();
	}

    std::string ToString() const
    {
        if (!IsValid())
        {
            std::wstring wideStr(ArrayData);
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), (int)wideStr.length(), nullptr, 0, nullptr, nullptr);
			std::string str(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), (int)wideStr.length(), &str[0], size_needed, nullptr, nullptr);
            return str;
        }

		return std::string("null");
    }

    bool IsValid() const
    {
        return !ArrayData;
    }

    FString operator=(ElementPointer other)
    {
        if (ArrayData != other)
        {
            ArrayMax = ArrayCount = *other ? (wcslen(other) + 1) : 0;

            if (ArrayCount > 0)
            {
                ArrayData = other;
            }
        }

        return *this;
    }

    bool operator==(const FString& other)
    {
        return (!wcscmp(ArrayData, other.ArrayData));
    }
};

FString FS(const std::string& s) {
    wchar_t* p = new wchar_t[s.size() + 1];
    for (std::string::size_type i = 0; i < s.size(); ++i)
        p[i] = s[i];

    p[s.size()] = '\0';
    return FString(p);
}

struct FSceNpOnlineId
{
    uint64_t                                           Data[0x2];                                        		// 0x0000 (0x0010) [0x0000000000000000]               
    uint8_t                                            Term;                                             		// 0x0010 (0x0001) [0x0000000000000000]               
    uint8_t                                            Dummy[0x3];                                       		// 0x0011 (0x0003) [0x0000000000000000]               
};
struct FSceNpId
{
    struct FSceNpOnlineId                              Handle;                                           		// 0x0000 (0x0018) [0x0000000000000002] (CPF_Const)   
    uint64_t                                           Opt;                                              		// 0x0018 (0x0008) [0x0000000000000002] (CPF_Const)   
    uint64_t                                           Reserved;                                         		// 0x0020 (0x0008) [0x0000000000000002] (CPF_Const)   
};
struct FUniqueNetId
{
    uint64_t                                           Uid;                                              		// 0x0000 (0x0008) [0x0000000000000000]               
    struct FSceNpId                                    NpId;                                             		// 0x0008 (0x0028) [0x0000000000000000]               
    struct FString                                     EpicAccountId;                                    		// 0x0030 (0x0010) [0x0000000000400000] (CPF_NeedCtorLink)
    uint8_t                                            Platform;                                         		// 0x0040 (0x0001) [0x0000000000000000]               
    uint8_t                                            SplitscreenID;                                    		// 0x0041 (0x0001) [0x0000000000000000]               
};
struct FGFxChatMessage {
    int32_t Team;
	struct FString PlayerName;
	struct FString Message;
    uint8_t ChatChannel;
    bool bLocalPlayer : 1;
    struct FUniqueNetId SenderID;
    uint8_t MessageType;
	struct FString TimeStamp;
};