#pragma once

//see here: https://docs.microsoft.com/en-us/typography/opentype/spec/languagetags


class OpenTypeLang {
public:
    constexpr static int OTLangHintLen = 3; //see harfbuzz hb-ot-tag-table.hh
    typedef char HintStr[OTLangHintLen + 1];
    typedef struct OpenTypeLangTag {
        HintStr lang;
        const wchar_t* langDescription;
    } T;
    static OpenTypeLangTag OpenTypeLangTags[763];
    static void CStringAtoHintStr(HintStr &langHint, CStringA src) {
        src = src.Left(OpenTypeLang::OTLangHintLen);
        int otlLen = src.GetLength();
        if (otlLen > 0) {
            strncpy_s(langHint, _countof(langHint), src.GetBuffer(), std::min(OpenTypeLang::OTLangHintLen + 1, otlLen + 1));
            src.ReleaseBuffer();
        } else {
            langHint[0] = 0;
        }
    }
};
