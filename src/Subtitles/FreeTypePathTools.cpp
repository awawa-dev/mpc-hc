#include "stdafx.h"
#include "FreeTypePathTools.h"

FTLibraryData::FTLibraryData() :
    ftInitialized(false)
    ,ftLibrary(nullptr)
{

}

FTLibraryData::~FTLibraryData() {
    if (ftInitialized) {
        for (auto& it : faceCache) {
            FT_Done_Face(it.second.face);
            delete[] it.second.fontData;
            hb_font_destroy(it.second.hbFont);
        }

        FT_Done_FreeType(ftLibrary);
    }
}


FTFaceCache& FTLibraryData::GetFaceCache() {
    return faceCache;
}

const FT_Library FTLibraryData::GetLibrary() {
    return ftLibrary;
}

bool FTLibraryData::IsInitialized() {
    if (!ftInitialized) {
        ftInitialized = !FT_Init_FreeType(&ftLibrary);
    }
    return ftInitialized;
}

bool FTLibraryData::LoadCodeFaceData(HDC hdc, std::wstring fontNameK) {
    FT_Face face;
    FT_Error error;

    if (IsInitialized()) {
        auto& faceCache = GetFaceCache();
        auto ftLibrary = GetLibrary();
        LONG tmAscent = 0;
        if (faceCache.count(fontNameK) > 0) {
            face = faceCache[fontNameK].face;
            tmAscent = faceCache[fontNameK].ascent;
            error = FT_Set_Pixel_Sizes(face, faceCache[fontNameK].ratio, faceCache[fontNameK].ratio);
        } else {
            DWORD fontSize = GetFontData(hdc, 0, 0, NULL, 0);
            FT_Byte* fontData = nullptr;
            try {
                fontData = DEBUG_NEW FT_Byte[fontSize];
            } catch (...) {
                return false;
            }
            GetFontData(hdc, 0, 0, fontData, fontSize);
            error = FT_New_Memory_Face(ftLibrary, fontData, fontSize, 0, &face);
            if (!error) {
                TEXTMETRIC GDIMetrics;
                GetTextMetricsW(hdc, &GDIMetrics);

                error = FT_Set_Pixel_Sizes(face, 0xffff, 0xffff);
                FT_UInt fRatio;
                //this is a weird hack, but the ratio of the ascent seems a good estimate of the right font size.  Worked perfectly with Arial
                FT_Pos tHeight = face->size->metrics.height;
                fRatio = static_cast<FT_UInt>(float(GDIMetrics.tmAscent) / face->size->metrics.ascender * 0xffff * 64);
                error = !error && FT_Set_Pixel_Sizes(face, fRatio, fRatio);
                //If the ascent ratio didn't work (>3.125%), we will do a basic height ratio.  it works well on other fonts
                if (!error && std::abs(64 - float(face->size->metrics.height) / GDIMetrics.tmHeight) > 2) {
                    fRatio = static_cast<FT_UInt>(float(GDIMetrics.tmHeight) / tHeight * 0xffff * 64);
                    error = FT_Set_Pixel_Sizes(face, fRatio, fRatio);
                }
                if (!error) {
                    tmAscent = GDIMetrics.tmAscent;
                    faceCache[fontNameK] = { fontData, nullptr, face, fRatio, std::unordered_map<wchar_t, FT_UInt>(), tmAscent };
                }
            }
        }
        return true;
    }
    return false;
}

void FTLibraryData::LoadCodePoints(CStringW str, std::wstring fontNameK, const char* langHint) {
    ASSERT(nullptr != langHint && langHint[0] != 0);

    auto& fd = faceCache[fontNameK];
    if (!fd.face) {
        ASSERT(false);
        return;
    }
    if (!fd.hbFont) {
        fd.hbFont = hb_ft_font_create(fd.face, NULL);
    }

    std::wstring wstr(str);
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf16(buf, (uint16_t*)wstr.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(buf);
    hb_segment_properties_t props;
    hb_buffer_get_segment_properties(buf, &props);
    props.language = hb_language_from_string(langHint, -1);
    hb_buffer_set_segment_properties(buf, &props);

    unsigned int glyphCount;
    hb_shape(fd.hbFont, buf, NULL, 0);
    hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buf, &glyphCount);
    if (glyphCount > 0) {
        for (unsigned int i = 0; i < glyphCount; i++) {
            wchar_t ch = wstr.at(glyphInfo[i].cluster);
            if (fd.codePoints.count(ch) == 0) {
                fd.codePoints[ch] = glyphInfo[i].codepoint;
            }
        }
    }
    hb_buffer_destroy(buf);
}
