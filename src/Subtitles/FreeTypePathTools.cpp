#include "stdafx.h"
#include "FreeTypePathTools.h"
#include "freetype/ftsnames.h"

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

std::wstring UTF16BE2LE(BYTE* in, int len) {
    std::wstring ret;
    ret.resize(len);
    BYTE* raw = (BYTE*)&ret[0];
    for (int i = 0; i < len; i += 2) {
        raw[i] = in[i + 1];
        raw[i + 1] = in[i];
    }
    ret[len] = 0;
    size_t nlen = ret.find_first_of(L'\0');
    if (std::wstring::npos != nlen) {
        ret.resize(nlen);
    }
    return ret;
}

bool FTLibraryData::CheckValidFamilyName(HDC hdc, std::wstring fontNameK, std::wstring checkFamily) {
    FT_Face face;
    FT_Error error;

    if (IsInitialized() && familyCache.count(fontNameK) == 0) {

        familyCache[fontNameK] = std::unordered_set<std::wstring>(1);
        std::unordered_set<std::wstring>& names = familyCache[fontNameK];

        auto ftLibrary = GetLibrary();

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
            FT_UInt nc = FT_Get_Sfnt_Name_Count(face);
            for (FT_UInt i = 0; i < nc; i++) {
                FT_SfntName fn;
                error = FT_Get_Sfnt_Name(face, i, &fn);
                if (!error && fn.name_id == 1) { //Font Family
                    if (fn.platform_id == 3) { //Microsoft encoding
                        std::wstring familyName = UTF16BE2LE(fn.string, fn.string_len);
                        names.insert(familyName);
                    } else if (fn.platform_id == 1) { //Macintosh encoding
                        int nBufLen = MultiByteToWideChar(CP_MACCP, 0, (LPCCH)fn.string, fn.string_len, NULL, 0);
                        if (nBufLen > 0) {
                            std::wstring familyName;
                            familyName.resize(nBufLen);
                            nBufLen = MultiByteToWideChar(CP_MACCP, 0, (LPCCH)fn.string, fn.string_len, &familyName[0], nBufLen);
                            if (nBufLen > 0) {
                                names.insert(familyName);
                            }
                        }
                    }
                }
            }
        }
        FT_Done_Face(face);

        delete[] fontData;
    }
    if (familyCache.count(fontNameK) > 0) {
        if (familyCache[fontNameK].count(checkFamily) > 0) {
            return true;
        }
    }

    return false;
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
