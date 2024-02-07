/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2015, 2017 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include "freetype/freetype.h"
#include <harfbuzz/harfbuzz/src/hb-ft.h>
#include <unordered_map>
#include <unordered_set>

struct faceData {
    FT_Byte* fontData;
    hb_font_t* hbFont;
    FT_Face face;
    FT_UInt ratio;
    std::unordered_map<wchar_t, FT_UInt> codePoints;
    long ascent;
};

typedef std::unordered_map<std::wstring, faceData> FTFaceCache;
typedef std::unordered_map<std::wstring, std::unordered_set<std::wstring>> FaceFamilyCache;

class FTLibraryData {
public:
    FTLibraryData();
    ~FTLibraryData();
    FTFaceCache& GetFaceCache();
    const FT_Library GetLibrary();
    bool IsInitialized();
    bool CheckValidFamilyName(HDC hdc, std::wstring fontNameK, std::wstring checkFamily);
    bool LoadCodeFaceData(HDC hdc, std::wstring fontNameK);
    void LoadCodePoints(CStringW str, std::wstring fontNameK, const char* langHint);
private:
    FTFaceCache faceCache;
    FaceFamilyCache familyCache;
    FT_Library ftLibrary;
    bool ftInitialized;
};
