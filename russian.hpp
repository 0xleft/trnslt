#include <unordered_map>

static inline transliteration::TransliterationLang* getRussian() {
	std::wstring mappingSource = L"абвгдезийклмнопрстуфхцЦъыьАБВГДЕЗИЙКЛМНОПРСТУФХЪЫЬ";
	std::wstring mappingTarget = L"abvgdezijklmnoprstufhcC'y'ABVGDEZIJKLMNOPRSTUFH'Y'";

	std::unordered_map<std::wstring, std::wstring> specificMapping;
	std::unordered_map<std::wstring, std::wstring> preProcessorMapping = {
		{ L"ё", L"e"},
		{ L"э", L"e"},
		{ L"Ё", L"E"},
		{ L"Э", L"E"},
		{ L"ъ", L"'"},
		{ L"ь", L"'"},
		{ L"Ъ", L"'"},
		{ L"Ь", L"'"}
	};
	std::unordered_map<std::wstring, std::wstring> specificPreProcessorMapping;

	return new transliteration::TransliterationLang("ru", mappingSource, mappingTarget, specificMapping, preProcessorMapping, specificPreProcessorMapping);
}