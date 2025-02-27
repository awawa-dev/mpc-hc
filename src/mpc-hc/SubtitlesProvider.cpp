﻿/*
 * (C) 2016-2017 see Authors.txt
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

#include "stdafx.h"
#include "SubtitlesProvider.h"
#include "SubtitlesProvidersUtils.h"
#include "mplayerc.h"
#include "ISOLang.h"
#include "Logger.h"
#include "base64/base64.h"
#include "tinyxml2/library/tinyxml2.h"
#include "rapidjson/include/rapidjson/pointer.h"
#include <wincrypt.h>

#pragma warning(disable: 4244)

#define LOG SUBTITLES_LOG
#define LOG_NONE    _T("()")
#define LOG_INPUT   _T("(\"%S\")")
#define LOG_OUTPUT  _T("()=%S")
#define LOG_BOTH    _T("(\"%S\")=%S")
#define LOG_ERROR   _T("() ERROR: %S")

#define GUESSED_NAME_POSTFIX " (*)"
#define CheckAbortAndReturn() { if (IsAborting()) return SR_ABORTED; }

using namespace SubtitlesProvidersUtils;

class LanguageDownloadException : public std::exception
{
    using exception::exception;
};

/******************************************************************************
** Register providers
******************************************************************************/
void SubtitlesProviders::RegisterProviders()
{
    //Register<OpenSubtitles>(this);
    Register<OpenSubtitles2>(this);
    Register<podnapisi>(this);
    Register<Napisy24>(this);
#ifdef INCLUDE_SUB_ADDIC7ED
    Register<Addic7ed>(this);
#endif
}

/******************************************************************************
** OpenSubtitles
******************************************************************************/

#if 0
void OpenSubtitles::Initialize()
{
    xmlrpc = std::make_unique<XmlRpcClient>((Url() + "/xml-rpc").c_str());
    xmlrpc->setIgnoreCertificateAuthority();
}

SRESULT OpenSubtitles::Login(const std::string& sUserName, const std::string& sPassword)
{
    // OpenSubtitles currently only works with a user account
    if (sUserName.empty()) {
        return SR_FAILED;
    }

    if (xmlrpc) {
        XmlRpcValue args, result;
        args[0] = sUserName;
        args[1] = sPassword;
        args[2] = "en";
        const auto& strUA = UserAgent();
        args[3] = strUA.c_str(); // Test with "OSTestUserAgent"

        if (!xmlrpc->execute("LogIn", args, result)) {
            return SR_FAILED;
        }

        if (result["status"].getType() == XmlRpcValue::Type::TypeString) {
            if (result["status"] == std::string("200 OK")) {
                token = result["token"];
            } else if (result["status"] == std::string("401 Unauthorized") && !sUserName.empty()) {
                // Notify user that User/Pass provided are invalid.
                CString msg;
                msg.FormatMessage(IDS_SUB_CREDENTIALS_ERROR, static_cast<LPCWSTR>(UTF8To16(Name().c_str())), static_cast<LPCWSTR>(UTF8To16(sUserName.c_str())));
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }

    LOG(LOG_BOTH, sUserName.c_str(), token.valid() ? (LPCSTR)token : "failed");
    return token.valid() ? SR_SUCCEEDED : SR_FAILED;
}

SRESULT OpenSubtitles::LogOut()
{
    if (xmlrpc && token.valid()) {
        XmlRpcValue args, result;
        args[0] = token;
        VERIFY(xmlrpc->execute("LogOut", args, result));
        token.clear();
        LOG(LOG_NONE);
    }
    m_nLoggedIn = SPL_UNDEFINED;

    return SR_SUCCEEDED;
}

SRESULT OpenSubtitles::Hash(SubtitlesInfo& pFileInfo)
{
    pFileInfo.fileHash = StringFormat("%016I64x", GenerateOSHash(pFileInfo));
    LOG(LOG_OUTPUT, pFileInfo.fileHash.c_str());
    return SR_SUCCEEDED;
}

SRESULT OpenSubtitles::Search(const SubtitlesInfo& pFileInfo)
{
    const auto languages = LanguagesISO6392();
    XmlRpcValue args, result;

    args[0] = token;
    auto& movieInfo = args[1][0];
    args[2]["limit"] = 500;
    movieInfo["sublanguageid"] = !languages.empty() ? JoinContainer(languages, ",") : "all";
    if (pFileInfo.manualSearchString.IsEmpty()) {
        movieInfo["moviehash"] = pFileInfo.fileHash;
        movieInfo["moviebytesize"] = std::to_string(pFileInfo.fileSize);
        //args[1][1]["sublanguageid"] = !languages.empty() ? languages : "all";
        //args[1][1]["tag"] = pFileInfo.fileName + "." + pFileInfo.fileExtension;

        LOG(LOG_INPUT,
            StringFormat("{ sublanguageid=\"%s\", moviehash=\"%s\", moviebytesize=\"%s\", limit=%d }",
                (LPCSTR)movieInfo["sublanguageid"],
                (LPCSTR)movieInfo["moviehash"],
                (LPCSTR)movieInfo["moviebytesize"],
                (int)args[2]["limit"]).c_str());
    } else {
        CT2CA pszConvertedAnsiString(pFileInfo.manualSearchString);
        movieInfo["query"] = std::string(pszConvertedAnsiString);
    }

    if (!xmlrpc->execute("SearchSubtitles", args, result)) {
        LOG(_T("search failed"));
        return SR_FAILED;
    }

    if (result["data"].getType() != XmlRpcValue::Type::TypeArray) {
        LOG(_T("search failed (invalid data)"));
        return SR_FAILED;
    }

    int nCount = result["data"].size();
    bool searchedByFileName = false;

    if (nCount == 0 && movieInfo.hasMember("moviehash")) {
        movieInfo.clear();
        //    movieInfo["tag"] = std::string(pFileInfo.fileName); //sadly, tag support has been disabled on opensubtitles.org :-/
        movieInfo["query"] = std::string(pFileInfo.fileName); //search by filename...as a query
        movieInfo["sublanguageid"] = !languages.empty() ? JoinContainer(languages, ",") : "all";
        if (!xmlrpc->execute("SearchSubtitles", args, result)) {
            LOG(_T("search failed"));
            return SR_FAILED;
        }
        if (result["data"].getType() != XmlRpcValue::Type::TypeArray) {
            LOG(_T("search failed (invalid data)"));
            return SR_FAILED;
        }
        nCount = result["data"].size();
        searchedByFileName = true;
    }

    std::string fnameLower = pFileInfo.fileName;
    std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(), [](unsigned char c) { return std::tolower(c); });

    bool matchFound = false;
    int maxPasses = searchedByFileName ? 2 : 1;
    for (int passCount = 0; passCount < maxPasses && !matchFound; passCount++) {
        for (int i = 0; i < nCount; ++i) {
            CheckAbortAndReturn();
            XmlRpcValue& data(result["data"][i]);
            std::string subFileName = (const char*)data["SubFileName"];

            if (searchedByFileName && 0 == passCount) {
                std::string subFilePrefix = subFileName.substr(0, subFileName.find_last_of("."));
                std::transform(subFilePrefix.begin(), subFilePrefix.end(), subFilePrefix.begin(), [](unsigned char c) { return std::tolower(c); });
                if (fnameLower.compare(subFilePrefix) != 0) {
                    continue;
                }
            }
            matchFound = true;

            SubtitlesInfo pSubtitlesInfo;
            pSubtitlesInfo.id = (const char*)data["IDSubtitleFile"];
            pSubtitlesInfo.discNumber = data["SubActualCD"];
            pSubtitlesInfo.discCount = data["SubSumCD"];
            pSubtitlesInfo.fileExtension = (const char*)data["SubFormat"];
            pSubtitlesInfo.languageCode = (const char*)data["ISO639"]; //"SubLanguageID"
            pSubtitlesInfo.languageName = (const char*)data["LanguageName"];
            pSubtitlesInfo.downloadCount = data["SubDownloadsCnt"];

            pSubtitlesInfo.fileName = subFileName;
            regexResult results;
            stringMatch("\"([^\"]+)\" (.+)", (const char*)data["MovieName"], results);
            if (!results.empty()) {
                pSubtitlesInfo.title = results[0];
                pSubtitlesInfo.title2 = results[1];
            } else {
                pSubtitlesInfo.title = (const char*)data["MovieName"];
            }
            pSubtitlesInfo.year = (int)data["MovieYear"] == 0 ? -1 : (int)data["MovieYear"];
            pSubtitlesInfo.seasonNumber = (int)data["SeriesSeason"] == 0 ? -1 : (int)data["SeriesSeason"];
            pSubtitlesInfo.episodeNumber = (int)data["SeriesEpisode"] == 0 ? -1 : (int)data["SeriesEpisode"];
            pSubtitlesInfo.hearingImpaired = data["SubHearingImpaired"];
            pSubtitlesInfo.url = (const char*)data["SubtitlesLink"];
            pSubtitlesInfo.releaseNames.emplace_back((const char*)data["MovieReleaseName"]);
            pSubtitlesInfo.imdbid = (const char*)data["IDMovieImdb"];
            pSubtitlesInfo.corrected = (int)data["SubBad"] ? -1 : 0;
            Set(pSubtitlesInfo);
        }
    }

    LOG(std::to_wstring(nCount).c_str());
    return SR_SUCCEEDED;
}

SRESULT OpenSubtitles::Download(SubtitlesInfo& pSubtitlesInfo)
{
    XmlRpcValue args, result;
    args[0] = token;
    args[1][0] = pSubtitlesInfo.id;
    if (!xmlrpc->execute("DownloadSubtitles", args, result)) {
        return SR_FAILED;
    }

    LOG(LOG_INPUT, pSubtitlesInfo.id.c_str());

    if (result["data"].getType() != XmlRpcValue::Type::TypeArray) {
        LOG(_T("download failed (invalid type)"));
        return SR_FAILED;
    }

    pSubtitlesInfo.fileContents = Base64::decode(std::string(result["data"][0]["data"]));
    return SR_SUCCEEDED;
}

SRESULT OpenSubtitles::Upload(const SubtitlesInfo& pSubtitlesInfo)
{
    XmlRpcValue args, result;
    args[0] = token;

    std::string uFileName = pSubtitlesInfo.fileName;
    //uFileName = "test upload file name";

    //TODO: Ask how to obtain commented values !!!
    args[1]["cd1"]["subhash"] = StringToHash(pSubtitlesInfo.fileContents, CALG_MD5);
    args[1]["cd1"]["subfilename"] = uFileName + ".srt";
    args[1]["cd1"]["moviehash"] = pSubtitlesInfo.fileHash;
    args[1]["cd1"]["moviebytesize"] = std::to_string(pSubtitlesInfo.fileSize);
    //args[1]["cd1"]["movietimems"];
    //args[1]["cd1"]["movieframes"];
    //args[1]["cd1"]["moviefps"];

    args[1]["cd1"]["moviefilename"] = uFileName + "." + pSubtitlesInfo.fileExtension;

    CheckAbortAndReturn();
    if (!xmlrpc->execute("TryUploadSubtitles", args, result)) {
        LOG(_T("TryUploadSubtitles failed"));
        return SR_FAILED;
    }
    CheckAbortAndReturn();

    if ((int)result["alreadyindb"] == 1) {
        LOG(_T("File already in database"));
        return SR_EXISTS;
    } else if ((int)result["alreadyindb"] == 0) {
        LOG(_T("Trying to determine IMDB ID"));
        // We need imdbid to proceed
        if (result["data"].getType() == XmlRpcValue::Type::TypeArray) {
            args[1]["baseinfo"]["idmovieimdb"] = result["data"][0]["IDMovieImdb"];
        } else if (!pSubtitlesInfo.imdbid.empty()) {
            args[1]["baseinfo"]["idmovieimdb"] = pSubtitlesInfo.imdbid;
        } else {
            std::string title(StringReplace(pSubtitlesInfo.title, "and", "&"));
            if (!args[1]["baseinfo"]["idmovieimdb"].valid()) {
                XmlRpcValue _args, _result;
                _args[0] = token;
                _args[1][0] = pSubtitlesInfo.fileHash;
                if (!xmlrpc->execute("CheckMovieHash", _args, _result)) {
                    LOG(_T("CheckMovieHash fail"));
                    return SR_FAILED;
                }

                if (_result["data"].getType() == XmlRpcValue::Type::TypeStruct
                    && _result["data"].hasMember(pSubtitlesInfo.fileHash)
                    && _result["data"][pSubtitlesInfo.fileHash].getType() == XmlRpcValue::Type::TypeStruct
                    && _result["data"][pSubtitlesInfo.fileHash].hasMember("MovieName")
                    && _result["data"][pSubtitlesInfo.fileHash].hasMember("MovieImdbID")
                    ) {
                    //regexResults results;
                    //stringMatch("\"(.+)\" (.+)", (const char*)data["MovieName"], results);
                    //if (!results.empty()) {
                    //    pSubtitlesInfo.title = results[0][0];
                    //    pSubtitlesInfo.title2 = results[0][1];
                    //} else {
                    //    pSubtitlesInfo.title = (const char*)data["MovieName"];
                    //}
                    regexResults results;
                    stringMatch("\"(.+)\" .+|(.+)", StringReplace((const char*)_result["data"][pSubtitlesInfo.fileHash]["MovieName"], "and", "&"), results);
                    if (results.size() > 0) {
                        std::string _title(results[0][0] + results[0][1]);
                        if (_stricmp(title.c_str(), _title.c_str()) == 0 /*&& (pSubtitlesInfo.year == -1 || (pSubtitlesInfo.year != -1 && pSubtitlesInfo.year == atoi(_result["data"][pSubtitlesInfo.fileHash]["MovieYear"])))*/) {
                            args[1]["baseinfo"]["idmovieimdb"] = _result["data"][pSubtitlesInfo.fileHash]["MovieImdbID"]; //imdbid
                        }
                    }
                }
            }

            if (!args[1]["baseinfo"]["idmovieimdb"].valid()) {
                XmlRpcValue _args, _result;
                _args[0] = token;
                _args[1][0] = pSubtitlesInfo.fileHash;
                if (!xmlrpc->execute("CheckMovieHash2", _args, _result)) {
                    return SR_FAILED;
                }

                if (_result["data"].getType() == XmlRpcValue::Type::TypeArray
                    && _result["data"].hasMember(pSubtitlesInfo.fileHash)
                    && _result["data"][pSubtitlesInfo.fileHash].size() > 0
                    ) {
                    int nCount = _result["data"][pSubtitlesInfo.fileHash].size();
                    for (int i = 0; i < nCount; ++i) {
                        regexResults results;
                        if (_result["data"][pSubtitlesInfo.fileHash][i].hasMember("MovieName")
                            && _result["data"][pSubtitlesInfo.fileHash][i].hasMember("MovieImdbID")
                            ) {
                            stringMatch("\"(.+)\" .+|(.+)", StringReplace((const char*)_result["data"][pSubtitlesInfo.fileHash][i]["MovieName"], "and", "&"), results);
                            if (results.size() > 0) {
                                std::string _title(results[0][0] + results[0][1]);

                                if (_stricmp(title.c_str(), _title.c_str()) == 0 /*&& (pSubtitlesInfo.year == -1 || (pSubtitlesInfo.year != -1 && pSubtitlesInfo.year == atoi(_result["data"][pSubtitlesInfo.fileHash][i]["MovieYear"])))*/) {
                                    args[1]["baseinfo"]["idmovieimdb"] = _result["data"][pSubtitlesInfo.fileHash][i]["MovieImdbID"]; //imdbid
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (!args[1]["baseinfo"]["idmovieimdb"].valid()) {
                XmlRpcValue _args, _result;
                _args[0] = token;
                _args[1] = title;
                if (!xmlrpc->execute("SearchMoviesOnIMDB", _args, _result)) {
                    return SR_FAILED;
                }
                if (_result["data"].getType() == XmlRpcValue::Type::TypeArray) {
                    int nCount = _result["data"].size();
                    for (int i = 0; i < nCount; ++i) {
                        regexResults results;
                        if (_result["data"][i].hasMember("title")) {
                            stringMatch("(.+) [(](\\d{4})[)]", StringReplace((const char*)_result["data"][i]["title"], "and", "&"), results);
                            if (results.size() == 1) {
                                std::string _title(results[0][0]);

                                if (_stricmp(title.c_str(), _title.c_str()) == 0 /*&& (pSubtitlesInfo.year == -1 || (pSubtitlesInfo.year != -1 && pSubtitlesInfo.year == atoi(results[0][1].c_str())))*/) {
                                    args[1]["baseinfo"]["idmovieimdb"] = _result["data"][i]["id"]; //imdbid
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (!args[1]["baseinfo"]["idmovieimdb"].valid()) {
                XmlRpcValue _args, _result;
                _args[0] = token;
                _args[1]["moviename"] = title;
                _args[1]["movieyear"] = pSubtitlesInfo.year;
                if (!xmlrpc->execute("InsertMovie", _args, _result)) {
                    return SR_FAILED;
                }
                if (_result.getType() == XmlRpcValue::Type::TypeStruct && _result.hasMember("id")) {
                    args[1]["baseinfo"]["idmovieimdb"] = _result["id"]; //imdbid
                }
            }
        }
        if (args[1]["baseinfo"]["idmovieimdb"].valid()) {
            XmlRpcValue _args, _result;
            _args[0] = token;
            _args[1][0]["moviehash"] = pSubtitlesInfo.fileHash;
            _args[1][0]["moviebytesize"] = (int)pSubtitlesInfo.fileSize;
            _args[1][0]["imdbid"] = args[1]["baseinfo"]["idmovieimdb"];
            //_args[1][0]["movietimems"];
            //_args[1][0]["moviefps"];
            _args[1][0]["moviefilename"] = uFileName + "." + pSubtitlesInfo.fileExtension;
            if (!xmlrpc->execute("InsertMovieHash", _args, _result)) {
                LOG(_T("InsertMovieHash fail"));
                return SR_FAILED;
            }
            // REsult value is irrelevant
            _result["data"]["accepted_moviehashes"];


            //args[1]["baseinfo"]["moviereleasename"];
            //args[1]["baseinfo"]["movieaka"];
            //args[1]["baseinfo"]["sublanguageid"];
            //args[1]["baseinfo"]["subauthorcomment"];
            if (pSubtitlesInfo.hearingImpaired != -1) {
                args[1]["baseinfo"]["hearingimpaired"] = pSubtitlesInfo.hearingImpaired;
            }
            //args[1]["baseinfo"]["highdefinition"];
            //args[1]["baseinfo"]["automatictranslation"];

            args[1]["cd1"]["subcontent"] = Base64::encode(StringGzipCompress(pSubtitlesInfo.fileContents));

            if (!xmlrpc->execute("UploadSubtitles", args, result)) {
                LOG(_T("Upload failed"));
                return SR_FAILED;
            }

            if (_result.getType() == XmlRpcValue::Type::TypeStruct) {
                if (_result.hasMember("data")) {
                    LOG(LOG_OUTPUT, (LPCSTR)result["data"]);
                }
                if (_result.hasMember("status")) {
                    if (result["status"] == std::string("200 OK")) {
                        LOG(_T("Upload succeeded"));
                        return SR_SUCCEEDED;
                    } else {
                        CString t("Upload failed - ");
                        CA2W status(result["status"]);
                        t += status;
                        LOG(t);
                    }
                }
            }
        }
    }
    LOG(_T("Upload failed"));
    return SR_FAILED;
}

const std::set<std::string>& OpenSubtitles::Languages() const
{
    static std::once_flag initialized;
    static std::set<std::string> result;
#if 1
    result = {"af", "an", "ar", "at", "az", "be", "bg", "bn", "br", "bs", "ca", "cs", "da", "de", "el", "en", "eo", "es", "et", "eu", "ex", "fa", "fi", "fr", "ga", "gd", "gl", "he", "hi", "hr", "hu", "hy", "id", "ig", "is", "it", "ja", "ka", "kk", "km", "kn", "ko", "ku", "lb", "lt", "lv", "ma", "me", "mk", "ml", "mn", "ms", "my", "nl", "no", "oc", "pb", "pl", "pm", "pt", "ro", "ru", "sd", "se", "si", "sk", "sl", "so", "sq", "sr", "sv", "sw", "sy", "ta", "te", "th", "tl", "tr", "tt", "uk", "ur", "vi", "ze", "zh", "zt"};
#else

    try {
        std::call_once(initialized, [this]() {
            if (!CheckInternetConnection()) {
                throw LanguageDownloadException("No internet connection.");
            }
            XmlRpcValue args, res;
            args = "en";
            if (!xmlrpc->execute("GetSubLanguages", args, res)) {
                throw LanguageDownloadException("Failed to execute xmlrpc command.");
            }
            if (res["data"].getType() != XmlRpcValue::Type::TypeArray) {
                throw LanguageDownloadException("Response is not an array.");
            }

            auto& data = res["data"];
            int count = data.size();
            for (int i = 0; i < count; ++i) {
#ifdef _DEBUG
                // Validate if language code conversion is in sync with OpenSubtitles database.
                std::string subLanguageID = data[i]["SubLanguageID"];
                std::string ISO6391 = data[i]["ISO639"];
                ASSERT(!ISO6391.empty());
                ASSERT(!subLanguageID.empty());
                ASSERT(ISOLang::ISO6391To6392(ISO6391.c_str()) == subLanguageID.c_str());
                ASSERT(ISOLang::ISO6392To6391(subLanguageID.c_str()) == ISO6391.c_str());
                //std::string languageName = data[i]["LanguageName"];
                //ASSERT(ISO639XToLanguage(ISO6391.c_str()) == languageName.c_str());
                //ASSERT(ISO639XToLanguage(subLanguageID.c_str()) == languageName.c_str());
#endif
                result.emplace(data[i]["ISO639"]);
            }
            });
    } catch (const LanguageDownloadException& e) {
        UNREFERENCED_PARAMETER(e);
        LOG(LOG_ERROR, e.what());
    }
#endif
    return result;
}

bool OpenSubtitles::NeedLogin()
{
    // return true to call Login() or false to skip Login()
    if (!token.valid()) {
        return true;
    }

    XmlRpcValue args, result;
    args[0] = token;
    if (!xmlrpc->execute("NoOperation", args, result)) {
        return false;
    }

    if ((result["status"].getType() == XmlRpcValue::Type::TypeString) && (result["status"] == std::string("200 OK"))) {
        return false;
    }

    return true;
}
#endif

/******************************************************************************
** OpenSubtitles.com
******************************************************************************/

void OpenSubtitles2::Initialize()
{

}

bool OpenSubtitles2::NeedLogin()
{
    if (token.IsEmpty()) {
        return true;
    }

    return false;
}

SRESULT OpenSubtitles2::Login(const std::string& sUserName, const std::string& sPassword)
{
    SRESULT result = SR_FAILED;

    if (sUserName.empty() || sPassword.empty()) {
        return SR_UNDEFINED;
    }

    CString userAgent(UserAgent().c_str());
    CInternetSession session(userAgent);
    CHttpConnection* con = session.GetHttpConnection(_T("api.opensubtitles.com"), (DWORD)INTERNET_FLAG_SECURE);
    CString url(_T("/api/v1/login"));
    CHttpFile* httpFile = con->OpenRequest(CHttpConnection::HTTP_VERB_POST, url, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE);

    //Headers will be converted to UTF-8 but the body will be sent as-is
    //That's why everything uses CString except for the body
    CString headers(_T("Content-Type: application/json\r\n"));
    headers.AppendFormat(_T("Api-Key: %s\r\n"), APIKEY);
    headers.Append(_T("Accept: application/json\r\n"));

    std::string body(R"({ "username": ")");
    body = body + sUserName + R"(", "password": ")" + sPassword + R"(" })";


    Response response;
    if (CallAPI(httpFile, headers, body, response))
    {
        rapidjson::Document doc;
        doc.Parse(response.text.c_str());
        if (doc.IsObject() && doc.HasMember("token") && doc["token"].IsString()) {
            token = doc["token"].GetString();
            result = SR_SUCCEEDED;
        }
    } else if (response.code == 401) {
        CString msg;
        msg.FormatMessage(IDS_SUB_CREDENTIALS_ERROR, static_cast<LPCWSTR>(UTF8To16(Name().c_str())), static_cast<LPCWSTR>(UTF8To16(sUserName.c_str())));
        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
    }

    httpFile->Close();
    delete httpFile;
    con->Close();
    delete con;
    return result;
}

SRESULT OpenSubtitles2::Search(const SubtitlesInfo& pFileInfo)
{
    SRESULT result = SR_FAILED;

    CString userAgent(UserAgent().c_str());
    CInternetSession session(userAgent);
    CHttpConnection* con = session.GetHttpConnection(_T("api.opensubtitles.com"), (DWORD)INTERNET_FLAG_SECURE);

    CString url(_T("/api/v1/subtitles?"));
    const auto languages = LanguagesISO6391();
    if (!languages.empty()) {
        url.AppendFormat(_T("languages=%s&"), JoinContainer(languages, _T(",")).c_str());
    }
    if (!pFileInfo.fileHash.empty()) {
        url.AppendFormat(_T("moviehash=%s&"), (LPCTSTR) CString(pFileInfo.fileHash.c_str()));
    }

    CString query;
    if (pFileInfo.manualSearchString.IsEmpty())
        query = pFileInfo.fileName.c_str();
    else
        query = pFileInfo.manualSearchString;

    url.AppendFormat(_T("query=%s"), (LPCTSTR)query);

    CHttpFile* httpFile = con->OpenRequest(CHttpConnection::HTTP_VERB_GET, url, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE);

    CString headers(_T("Api-Key: "));
    headers.Append(APIKEY);
    headers.Append(_T("\r\n"));
    Response response;
    if (CallAPI(httpFile, headers, response)) {
        rapidjson::Document doc;
        doc.Parse(response.text.c_str());
        if (doc.IsObject() && doc.HasMember("data") && doc["data"].IsArray()) {
            result = SR_SUCCEEDED;
            const auto& data = doc["data"];
            for (const auto& item : data.GetArray()) {

                SubtitlesInfo pSubtitlesInfo;

                if (!GetOptionalValue(item, "/attributes/files/0/file_id", pSubtitlesInfo.id)) {
                    continue;
                }
                GetOptionalValue(item, "/attributes/files/0/file_name", pSubtitlesInfo.fileName);
                GetOptionalValue(item, "/attributes/files/0/cd_number", pSubtitlesInfo.discNumber);
                pSubtitlesInfo.fileExtension = "srt";
                GetOptionalValue(item, "/attributes/language", pSubtitlesInfo.languageCode);
                GetOptionalValue(item, "/attributes/download_count", pSubtitlesInfo.downloadCount);
                GetOptionalValue(item, "/attributes/feature_details/movie_name", pSubtitlesInfo.title);
                GetOptionalValue(item, "/attributes/feature_details/year", pSubtitlesInfo.year);
                GetOptionalValue(item, "/attributes/feature_details/season_number", pSubtitlesInfo.seasonNumber);
                GetOptionalValue(item, "/attributes/feature_details/episode_number", pSubtitlesInfo.episodeNumber);
                GetOptionalValue(item, "/attributes/hearing_impaired", pSubtitlesInfo.hearingImpaired);
                GetOptionalValue(item, "/attributes/feature_details/imdb_id", pSubtitlesInfo.imdbid);
                GetOptionalValue(item, "/attributes/fps", pSubtitlesInfo.frameRate);
                Set(pSubtitlesInfo);
            }
        }
    }
    httpFile->Close();
    delete httpFile;
    con->Close();
    delete con;
    return result;
}

SRESULT OpenSubtitles2::Download(SubtitlesInfo& pSubtitlesInfo)
{
    SRESULT result = SR_FAILED;

    CString userAgent(UserAgent().c_str());
    CInternetSession session(userAgent);
    CHttpConnection* con = session.GetHttpConnection(_T("api.opensubtitles.com"), (DWORD)INTERNET_FLAG_SECURE);
    CString url(_T("/api/v1/download"));
    CHttpFile* httpFile = con->OpenRequest(CHttpConnection::HTTP_VERB_POST, url, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE);

    CString headers(_T("Accept: application/json\r\n")); 
    headers.AppendFormat(_T("Api-Key: %s\r\n"), APIKEY);
	headers.Append(_T("Content-Type: application/json\r\n"));
    if (!token.IsEmpty()) {
        headers.AppendFormat(_T("Authorization: Bearer %s\r\n"), (LPCTSTR)token);
    }

    std::string body(R"({ "file_id": )");
    body += pSubtitlesInfo.id;
    body += " }";

    Response response;
    if (CallAPI(httpFile, headers, body, response)) {
        rapidjson::Document doc;
        doc.Parse(response.text.c_str());
        if (!doc.HasParseError()) {
            if (doc.HasMember("file_name") && doc["file_name"].IsString())
            {
                std::string downloadLink = doc["link"].GetString();
                LOG(LOG_INPUT, downloadLink.c_str());
                result = DownloadInternal(downloadLink, "", pSubtitlesInfo.fileContents);
            }
        }

    }
    httpFile->Close();
    delete httpFile;
    con->Close();
    delete con;
    return result;
}

SRESULT OpenSubtitles2::LogOut()
{
    SRESULT result = SR_FAILED;

    if (!token.IsEmpty()) {
        CString userAgent(UserAgent().c_str());
        CInternetSession session(userAgent);
        CHttpConnection* con = session.GetHttpConnection(_T("api.opensubtitles.com"), (DWORD)INTERNET_FLAG_SECURE);
        CString url(_T("/api/v1/logout"));
        CHttpFile* httpFile = con->OpenRequest(CHttpConnection::HTTP_VERB_DELETE, url, NULL, 1, NULL, NULL, INTERNET_FLAG_SECURE);

        CString headers(_T("Accept: application/json\r\n"));
        headers.AppendFormat(_T("Api-Key: %s\r\n"), (LPCTSTR)APIKEY);
        headers.AppendFormat(_T("Authorization: Bearer %s\r\n"), (LPCTSTR)token);
        Response response;
        if (CallAPI(httpFile, headers, response)) {
            result = SR_SUCCEEDED;
        }
        httpFile->Close();
        delete httpFile;
        con->Close();
        delete con;
    }

    token.Empty();
    
    return result;
}

SRESULT OpenSubtitles2::Hash(SubtitlesInfo& pFileInfo)
{
    pFileInfo.fileHash = StringFormat("%016I64x", GenerateOSHash(pFileInfo));
    LOG(LOG_OUTPUT, pFileInfo.fileHash.c_str());
    return SR_SUCCEEDED;
}

bool OpenSubtitles2::GetOptionalValue(const rapidjson::Value& node, const char* path, std::string& result)
{
    bool success = false;
    const rapidjson::Value* foundNode = rapidjson::Pointer(path).Get(node);
    if (foundNode) {
        if (foundNode->IsString()) {
            result = foundNode->GetString();
            success = true;
        } else if (foundNode->IsInt64()) {
            result = std::to_string(foundNode->GetInt64());
            success = true;
        }
    }
    return success;
}

bool OpenSubtitles2::GetOptionalValue(const rapidjson::Value& node, const char* path, int& result)
{
    bool success = false;
    const rapidjson::Value* foundNode = rapidjson::Pointer(path).Get(node);
    if (foundNode) {
        if (foundNode->IsInt()) {
            result = foundNode->GetInt();
            success = true;
        } else if (foundNode->IsBool()) {
            result = foundNode->GetBool() ? TRUE : FALSE;
            success = true;
        }
    }
    return success;
}

bool OpenSubtitles2::GetOptionalValue(const rapidjson::Value& node, const char* path, double& result)
{
    bool success = false;
    const rapidjson::Value* foundNode = rapidjson::Pointer(path).Get(node);
    if (foundNode) {
        if (foundNode->IsDouble()) {
            result = foundNode->GetDouble();
            success = true;
        }
    }
    return success;
}
bool OpenSubtitles2::CallAPI(CHttpFile* httpFile, CString& headers, Response& response)
{
    httpFile->SendRequest(headers);
    return CallAPIResponse(httpFile, response);
}


bool OpenSubtitles2::CallAPI(CHttpFile* httpFile, CString& headers, std::string& body, Response& response)
{
    httpFile->SendRequest(headers, body.data(), static_cast<DWORD>(body.size()));
    return CallAPIResponse(httpFile, response);
}

bool OpenSubtitles2::CallAPIResponse(CHttpFile* httpFile, Response& response)
{
    httpFile->QueryInfoStatusCode(response.code);

    auto size = httpFile->GetLength();
    while (size > 0)
    {
        std::string temp;
        temp.resize(size);
        httpFile->Read(temp.data(), size);
        response.text += temp;
        size = httpFile->GetLength();
    }
    if (response.code != 200)
    {
        LOG(LOG_ERROR, (std::string("Server returned: ") + std::to_string(response.code) + " with message: " + response.text).c_str());
        return false;
    }
    return true;
}


const std::set<std::string>& OpenSubtitles2::Languages() const
{
    static std::set<std::string> result;
    result = { "af", "an", "ar", "at", "az", "be", "bg", "bn", "br", "bs", "ca", "cs", "da", "de", "el", "en", "eo", "es", "et", "eu", "ex", "fa", "fi", "fr", "ga", "gd", "gl", "he", "hi", "hr", "hu", "hy", "id", "ig", "is", "it", "ja", "ka", "kk", "km", "kn", "ko", "ku", "lb", "lt", "lv", "ma", "me", "mk", "ml", "mn", "ms", "my", "nl", "no", "oc", "pb", "pl", "pm", "pt", "ro", "ru", "sd", "se", "si", "sk", "sl", "so", "sq", "sr", "sv", "sw", "sy", "ta", "te", "th", "tl", "tr", "tt", "uk", "ur", "vi", "ze", "zh", "zt" };
    return result;
}

/******************************************************************************
** podnapisi
******************************************************************************/

SRESULT podnapisi::Login(const std::string& sUserName, const std::string& sPassword)
{
    //TODO: implement
    return SR_UNDEFINED;
}

/*
UPDATED
https://www.podnapisi.net/forum/viewtopic.php?f=62&t=26164#p212652
RESULTS ------------------------------------------------
"/sXML/1/"  //Reply in XML format
"/page//"   //Return nth page of results
SEARCH -------------------------------------------------
"/sT/1/"    //Type: -1=all, 0=movies, 1=series, don't specify for auto detection
"/sAKA/1/"  //Include movie title aliases
"/sM//"     //Movie id from www.omdb.si
"/sK//"     //Title url encoded text
"/sY//"     //Year number
"/sTS//"    //Season number
"/sTE//"    //Episode number
"/sR//"     //Release name url encoded text
"/sJ/0/"    //Languages (old integer IDs), comma delimited, 0=all
"/sL/en/"   //Languages in ISO ISO codes (exception are sr-latn and pt-br), comma delimited
"/sEH//"    //Exact hash match (OSH)
"/sMH//"    //Movie hash (OSH)
SEARCH ADDITIONAL --------------------------------------
"/sFT/0/"   //Subtitles Format: 0=all, 1=MicroDVD, 2=SAMI, 3=SSA, 4=SubRip, 5=SubViewer 2.0, 6=SubViewer, 7=MPSub, 8=Advanced SSA, 9=DVDSubtitle, 10=TMPlayer, 11=MPlayer2
"/sA/0/"    //Search subtitles by user id, 0=all
"/sI//"     //Search subtitles by subtitle id
SORTING ------------------------------------------------
"/sS//"     //Sorting field: movie, year, fps, language, downloads, cds, username, time, rating
"/sO//"     //Soring order: asc, desc
FILTERS ------------------------------------------------
"/sOE/1/"   //Subtitles for extended edition only
"/sOD/1/"   //Subtitles suitable for DVD only
"/sOH/1/"   //Subtitles for high-definition video only
"/sOI/1/"   //Subtitles for hearing impaired only
"/sOT/1/"   //Technically correct only
"/sOL/1/"   //Grammatically correct only
"/sOA/1/"   //Author subtitles only
"/sOCS/1/"  //Only subtitles for a complete season
UNKNOWN ------------------------------------------------
"/sH//"     //Search subtitles by video file hash ??? (not working for me)
*/

SRESULT podnapisi::Search(const SubtitlesInfo& pFileInfo)
{
    SRESULT searchResult = SR_UNDEFINED;
    int page = 1, pages = 1, results = 0;
    do {
        CheckAbortAndReturn();

        std::string url(Url() + "/ppodnapisi/search");
        url += "?sXML=1";
        url += "&sAKA=1";

        if (pFileInfo.manualSearchString.IsEmpty()) {
            std::string search(pFileInfo.title);
            if (!pFileInfo.country.empty()) {
                search += " " + pFileInfo.country;
            }
            search = std::regex_replace(search, std::regex(" and | *[!?&':] *", RegexFlags), " ");

            if (!search.empty()) {
                url += "&sK=" + UrlEncode(search.c_str());
            }
            url += (pFileInfo.year != -1 ? "&sY=" + std::to_string(pFileInfo.year) : "");
            url += (pFileInfo.seasonNumber != -1 ? "&sTS=" + std::to_string(pFileInfo.seasonNumber) : "");
            url += (pFileInfo.episodeNumber != -1 ? "&sTE=" + std::to_string(pFileInfo.episodeNumber) : "");
            url += "&sMH=" + pFileInfo.fileHash;
            //url += "&sR=" + UrlEncode(pFileInfo.fileName.c_str());
        } else {
            CT2CA pszConvertedAnsiString(pFileInfo.manualSearchString);
            std::string search(pszConvertedAnsiString);
            search = std::regex_replace(search, std::regex(" and | *[!?&':] *", RegexFlags), " ");

            if (!search.empty()) {
                url += "&sK=" + UrlEncode(search.c_str());
            }
        }
        const auto languages = LanguagesISO6391();
        if (!languages.empty()) {
            url += "&sL=" + JoinContainer(languages, ",");
            // add alternative language codes used by the provider in case they differ from standard ISO code
            for (auto it = languages.begin(); it != languages.end(); ++it) {
                if ((*it).compare("pb") == 0) { // Portuguese Brazil
                    url += ",pt-br";
                }
            }
        }
        url += "&page=" + std::to_string(page);
        LOG(LOG_INPUT, url.c_str());

        std::string data;
        searchResult = DownloadInternal(url, "", data);

        using namespace tinyxml2;

        tinyxml2::XMLDocument dxml;
        if (dxml.Parse(data.c_str()) == XML_SUCCESS) {

            auto GetChildElementText = [&](XMLElement * pElement, const char* value) -> std::string {
                std::string str;
                XMLElement* pChildElement = pElement->FirstChildElement(value);
                if (pChildElement != nullptr)
                {
                    auto pText = pChildElement->GetText();
                    if (pText != nullptr) {
                        str = pText;
                    }
                }
                return str;
                };

            XMLElement* pRootElmt = dxml.FirstChildElement("results");
            if (pRootElmt) {
                XMLElement* pPaginationElmt = pRootElmt->FirstChildElement("pagination");
                if (pPaginationElmt) {
                    page = atoi(GetChildElementText(pPaginationElmt, "current").c_str());
                    pages = atoi(GetChildElementText(pPaginationElmt, "count").c_str());
                    results = atoi(GetChildElementText(pPaginationElmt, "results").c_str());
                }
                // 30 results per page
                if (page > 1) {
                    return SR_TOOMANY;
                }

                if (results > 0) {
                    XMLElement* pSubtitleElmt = pRootElmt->FirstChildElement("subtitle");

                    while (pSubtitleElmt) {
                        CheckAbortAndReturn();

                        SubtitlesInfo pSubtitlesInfo;

                        pSubtitlesInfo.id = GetChildElementText(pSubtitleElmt, "pid");
                        pSubtitlesInfo.title = HtmlSpecialCharsDecode(GetChildElementText(pSubtitleElmt, "title").c_str());

                        std::string year = GetChildElementText(pSubtitleElmt, "year");
                        pSubtitlesInfo.year = year.empty() ? -1 : atoi(year.c_str());

                        pSubtitlesInfo.url = GetChildElementText(pSubtitleElmt, "url");
                        std::string format = GetChildElementText(pSubtitleElmt, "format");
                        pSubtitlesInfo.fileExtension = (format == "SubRip" || format == "N/A") ? "srt" : format;

                        pSubtitlesInfo.languageCode = podnapisi_languages[atoi(GetChildElementText(pSubtitleElmt, "languageId").c_str())].code;
                        pSubtitlesInfo.languageName = GetChildElementText(pSubtitleElmt, "languageName");
                        pSubtitlesInfo.seasonNumber = atoi(GetChildElementText(pSubtitleElmt, "tvSeason").c_str());
                        pSubtitlesInfo.episodeNumber = atoi(GetChildElementText(pSubtitleElmt, "tvEpisode").c_str());
                        pSubtitlesInfo.discCount = atoi(GetChildElementText(pSubtitleElmt, "cds").c_str());
                        pSubtitlesInfo.discNumber = pSubtitlesInfo.discCount;

                        std::string flags = GetChildElementText(pSubtitleElmt, "flags");
                        pSubtitlesInfo.hearingImpaired = (flags.find("n") != std::string::npos) ? TRUE : FALSE;
                        pSubtitlesInfo.corrected = (flags.find("r") != std::string::npos) ? -1 : 0;
                        pSubtitlesInfo.downloadCount = atoi(GetChildElementText(pSubtitleElmt, "downloads").c_str());
                        pSubtitlesInfo.imdbid = GetChildElementText(pSubtitleElmt, "movieId");
                        pSubtitlesInfo.frameRate = atof(GetChildElementText(pSubtitleElmt, "fps").c_str());

                        XMLElement* pReleasesElem = pSubtitleElmt->FirstChildElement("releases");
                        if (pReleasesElem) {
                            XMLElement* pReleaseElem = pReleasesElem->FirstChildElement("release");

                            while (pReleaseElem) {
                                auto pText = pReleaseElem->GetText();

                                if (!pText) {
                                    continue;
                                }

                                pSubtitlesInfo.releaseNames.emplace_back(pText);

                                if (pSubtitlesInfo.fileName.empty() || pFileInfo.fileName.find(pText) != std::string::npos) {
                                    pSubtitlesInfo.fileName = pText;
                                    pSubtitlesInfo.fileName += "." + pSubtitlesInfo.fileExtension;
                                }
                                pReleaseElem = pReleaseElem->NextSiblingElement();
                            }
                        }

                        if (pSubtitlesInfo.fileName.empty()) {
                            std::string str = pSubtitlesInfo.title;
                            if (!year.empty()) {
                                str += " " + year;
                            }
                            if (pSubtitlesInfo.seasonNumber > 0) {
                                str += StringFormat(" S%02d", pSubtitlesInfo.seasonNumber);
                            }
                            if (pSubtitlesInfo.episodeNumber > 0) {
                                str += StringFormat("%sE%02d", (pSubtitlesInfo.seasonNumber > 0) ? "" : " ", pSubtitlesInfo.episodeNumber);
                            }
                            str += GUESSED_NAME_POSTFIX;
                            pSubtitlesInfo.fileName = str;
                        }

                        Set(pSubtitlesInfo);
                        pSubtitleElmt = pSubtitleElmt->NextSiblingElement();
                    }
                }
            }
        }
    } while (page++ < pages);

    return searchResult;
}

SRESULT podnapisi::Hash(SubtitlesInfo& pFileInfo)
{
    pFileInfo.fileHash = StringFormat("%016I64x", GenerateOSHash(pFileInfo));
    LOG(LOG_OUTPUT, pFileInfo.fileHash.c_str());
    return SR_SUCCEEDED;
}

SRESULT podnapisi::Download(SubtitlesInfo& pSubtitlesInfo)
{
    std::string url = StringFormat("%s/subtitles/%s/download", Url().c_str(), pSubtitlesInfo.id.c_str());
    LOG(LOG_INPUT, url.c_str());
    return DownloadInternal(url, "", pSubtitlesInfo.fileContents);
}

const std::set<std::string>& podnapisi::Languages() const
{
    static std::once_flag initialized;
    static std::set<std::string> result;

    std::call_once(initialized, [this]() {
        for (const auto& iter : podnapisi_languages) {
            if (strlen(iter.code)) {
                result.emplace(iter.code);
            }
        }
        });
    return result;
}

/******************************************************************************
** Napisy24
******************************************************************************/

SRESULT Napisy24::Search(const SubtitlesInfo& pFileInfo)
{
    if (!pFileInfo.manualSearchString.IsEmpty()) {
        return SR_FAILED; //napisys24 does not support manual search
    }
    stringMap headers({
        { "User-Agent", UserAgent() },
        { "Content-Type", "application/x-www-form-urlencoded" }
        });
    std::string data;
    std::string url = Url() + "/run/CheckSubAgent.php";
    std::string content = "postAction=CheckSub";
    content += "&ua=mpc-hc";
    content += "&ap=mpc-hc";
    content += "&fh=" + pFileInfo.fileHash;
    content += "&fs=" + std::to_string(pFileInfo.fileSize);
    content += "&fn=" + pFileInfo.fileName;

    LOG(LOG_INPUT, std::string(url + "?" + content).c_str());
    StringUpload(url, headers, content, data);

    if (data.length() < 4) {
        return SR_FAILED;
    }

    // Get status
    std::string status = data.substr(0, 4);
    if (status != "OK-2" && status != "OK-3") {
        return SR_FAILED;
    }
    data.erase(0, 5);

    size_t infoEnd = data.find("||");
    if (infoEnd == std::string::npos) {
        return SR_FAILED;
    }

    // Search already returns whole file
    SubtitlesInfo subtitleInfo;
    subtitleInfo.fileContents = data.substr(infoEnd + 2);
    subtitleInfo.languageCode = "pl"; // API doesn't support other languages yet.

    // Remove subtitle data
    data.erase(infoEnd);

    std::unordered_map<std::string, std::string> subtitleInfoMap;
    std::istringstream stringStream(data);
    std::string entry;
    while (std::getline(stringStream, entry, '|')) {
        auto delimPos = entry.find(':');
        if (delimPos == std::string::npos) {
            continue;
        }
        std::string key = entry.substr(0, delimPos);
        if (entry.length() <= delimPos + 1) {
            continue;
        }
        std::string value = entry.substr(delimPos + 1);
        subtitleInfoMap[key] = value;
    }

    subtitleInfo.url = Url() + "/komentarze?napisId=" + subtitleInfoMap["napisId"];
    subtitleInfo.title = subtitleInfoMap["ftitle"];
    subtitleInfo.imdbid = subtitleInfoMap["fimdb"];

    auto it = subtitleInfoMap.find("fyear");
    if (it != subtitleInfoMap.end()) {
        subtitleInfo.year = std::stoi(it->second);
    }

    it = subtitleInfoMap.find("fps");
    if (it != subtitleInfoMap.end()) {
        subtitleInfo.frameRate = std::stod(it->second);
    }

    int hour, minute, second;
    if (sscanf_s(subtitleInfoMap["time"].c_str(), "%02d:%02d:%02d", &hour, &minute, &second) == 3) {
        subtitleInfo.lengthMs = ((hour * 60 + minute) * 60 + second) * 1000;
    }

    subtitleInfo.fileName = pFileInfo.fileName + "." + pFileInfo.fileExtension;
    subtitleInfo.discNumber = 1;
    subtitleInfo.discCount = 1;

    Set(subtitleInfo);

    return SR_SUCCEEDED;
}

SRESULT Napisy24::Hash(SubtitlesInfo& pFileInfo)
{
    pFileInfo.fileHash = StringFormat("%016I64x", GenerateOSHash(pFileInfo));
    LOG(LOG_OUTPUT, pFileInfo.fileHash.c_str());
    return SR_SUCCEEDED;
}

SRESULT Napisy24::Download(SubtitlesInfo& subtitlesInfo)
{
    LOG(LOG_INPUT, subtitlesInfo.url.c_str());
    return subtitlesInfo.fileContents.empty() ? SR_FAILED : SR_SUCCEEDED;
}

const std::set<std::string>& Napisy24::Languages() const
{
    static std::set<std::string> result = {"pl"};
    return result;
}

/******************************************************************************
** Addic7ed
******************************************************************************/

#ifdef INCLUDE_SUB_ADDIC7ED

SRESULT Addic7ed::Search(const SubtitlesInfo& pFileInfo)
{
    // validate cookie, it's necessary
    if (cookie.IsEmpty())
        return SR_FAILED;

    // prepare the form (GET)
    stringMap headers({{ "User-Agent", SUB_ADDIC7ED_AGENT }});
    std::string data;
    std::string url = Url() + "search.php";
    std::string content = "search=";
    CT2CA pszConvertedAnsiString(cookie);

    // restore cookie, it's needed for downloading
    headers.insert({ "Set-Cookie", (LPSTR)pszConvertedAnsiString });

    // escape the search term
    CString searchTerm = (!pFileInfo.manualSearchString.IsEmpty()) ? pFileInfo.manualSearchString : CString(pFileInfo.fileName.c_str());
    const ULONG bufsizefilename = 512;
    ULONG convertSizeParam = bufsizefilename;
    wchar_t convertBuffer[bufsizefilename];

    if (InternetCanonicalizeUrl(searchTerm, convertBuffer, &convertSizeParam, 0))
    {
        std::wstring ws(convertBuffer);
        std::string convert(ws.begin(), ws.end());
        content = content + convert;
    }

    // search for subtitles using GET request
    DWORD status = 0;
    url = url + "?" + content;
    LOG(LOG_INPUT, std::string(url).c_str());
    StringDownload(url, headers, data, false, &status);

    if (data.length() < 128 || status != 200)
    {
        LOG(LOG_INPUT, ("Error during downloading the search result"));
        return SR_FAILED;
    }

    /*
    // TESTING CODE ON SAVED RESULTS
    #include <fstream>
    std::ofstream dout1;
    dout1.open("D:/a/"+ pFileInfo.fileName +".txt", std::ios::out | std::ios::binary | std::ios::trunc);
    dout1 << data;
    dout1 << std::flush;
    dout1.flush();
    dout1.close();

    std::string data;
    std::ifstream dout1;
    std::stringstream buffer;
    dout1.open("D:/a/"+ pFileInfo.fileName +".txt", std::ios::in | std::ios::binary);
    buffer << dout1.rdbuf();
    data = buffer.str();
    dout1.close();
    */

   
    // current constant token from their webpages, could change after their updates    
    std::string currentLine;
    std::istringstream lineStream(data);
    const auto languages = LanguagesISO6391();

    const std::string newSubtitleItemToken = "<div id=\"";
    const std::string downloadLinkToken = "<a class=\"buttonDownload\" href=\"/";
    const std::string languageToken = "class=\"language\">";
    const std::string noresultToken = "0 results found";
    const std::string releaseInfoStartToken = "class=\"NewsTitle\">";
    const std::string releaseInfoEndToken = "/>";
    const std::string releaseInfoCloseToken = ", Duration:";
    const std::string downloadsCountToken = u8"Downloads · ";
    const std::string hearingImpairedToken = "Hearing Impaired";

    std::size_t foundIndex,secondIndex,thirdIndex;
    std::string foundLink, foundLanguage, foundReleaseInfo;
    int downloadCount = INT_ERROR;
    bool hearingImpaired = false;

   
    // parse the result
    while (std::getline(lineStream, currentLine, '\n')) {
        if (currentLine.find(noresultToken) != std::string::npos)
        {
            LOG(LOG_INPUT, ("No results"));
            return SR_FAILED;
        }

        // check hearing impaired info
        if (currentLine.find(hearingImpairedToken) != std::string::npos)
            hearingImpaired = true;

        // check download count info
        foundIndex = currentLine.find(downloadsCountToken);
        if (foundIndex != std::string::npos)
        {
            std::string downNum;
            for (size_t i = foundIndex + downloadsCountToken.length(); i < currentLine.length(); ++i)
            {
                char inChar = currentLine.at(i);
                if ((inChar >= '0' && inChar <= '9') || inChar == ' ')
                    downNum.push_back(inChar);
                else
                    break;
            }
            if (!downNum.empty())
            {
                int parseCount = strtol(downNum.c_str(), NULL, 0);
                if (parseCount > 0)
                    downloadCount = parseCount;
            }
            continue;
        }

        // check release info
        foundIndex = currentLine.find(releaseInfoStartToken);
        if (foundIndex != std::string::npos)
        {
            secondIndex = currentLine.find(releaseInfoEndToken, foundIndex + releaseInfoStartToken.length() + 1);
            if (secondIndex != std::string::npos)
            {
                thirdIndex = currentLine.find(releaseInfoCloseToken, secondIndex + releaseInfoEndToken.length() + 1);
                if (thirdIndex != std::string::npos)
                {
                    size_t helper = secondIndex + releaseInfoEndToken.length();
                    foundReleaseInfo = currentLine.substr(helper, thirdIndex - helper);
                }
            }
            continue;
        }
     

        // check download link
        foundIndex = currentLine.find(downloadLinkToken);
        if (foundIndex != std::string::npos)
        {
            secondIndex = currentLine.find("\"", foundIndex + downloadLinkToken .length() + 1);
            if (secondIndex != std::string::npos)
            {
                size_t helper = foundIndex + downloadLinkToken.length();
                foundLink = currentLine.substr(helper, secondIndex - helper);
            }
            continue;
        }

        // check langueage
        foundIndex = currentLine.find(languageToken);
        if (foundIndex != std::string::npos)
        {
            secondIndex = currentLine.find("<", foundIndex + languageToken.length() + 1);
            if (secondIndex != std::string::npos)
            {
                size_t helper = foundIndex + languageToken.length();
                foundLanguage = currentLine.substr(helper, secondIndex - helper);

                bool foundISO = false;
                for (auto lang : Addic7ed_languages)
                {
                    if (lang.name == foundLanguage &&
                        (languages.empty() || (std::find(languages.begin(), languages.end(), lang.code) != languages.end())))
                    {

                        foundISO = true;
                        foundLanguage = lang.code;
                        break;
                    }
                }
                // check if unknown language
                if (!foundISO)
                    foundLanguage.clear();
            }
            continue;
        }

        // check if new item begins, save the old result if found
        foundIndex = currentLine.find(newSubtitleItemToken);
        if (foundIndex != std::string::npos)
        {
            if (!foundLanguage.empty() && !foundLink.empty())
            {
                SubtitlesInfo subtitleInfo;
                subtitleInfo.fileContents = "";
                subtitleInfo.languageCode = foundLanguage;
                subtitleInfo.url = Url() + foundLink;
                subtitleInfo.title = foundReleaseInfo;
                subtitleInfo.fileName = pFileInfo.fileName + "." + pFileInfo.fileExtension;
                subtitleInfo.downloadCount = downloadCount;
                subtitleInfo.hearingImpaired = hearingImpaired;
                subtitleInfo.discNumber = 1;
                subtitleInfo.discCount = 1;

                Set(subtitleInfo);
            }
            
            foundLanguage.clear();
            foundLink.clear();
            foundReleaseInfo.clear();
            hearingImpaired = false;
            downloadCount = INT_ERROR;
        }
    }
    
    return SR_SUCCEEDED;
}

SRESULT Addic7ed::Download(SubtitlesInfo& subtitlesInfo)
{
    // request for subtitles from Addic7ed (send them their cookie first)
    stringMap headers({ { "User-Agent", SUB_ADDIC7ED_AGENT } });
    std::string url = subtitlesInfo.url;
    DWORD dwStatusCode = 0;
    CT2CA pszConvertedAnsiString(cookie);

    // restore cookie, it's needed for downloading
    headers.insert({ "Set-Cookie", (LPSTR)pszConvertedAnsiString });
    headers.insert({ "Referer",  Url() });

    LOG(LOG_INPUT, url.c_str());

    if (StringDownload(url, headers, subtitlesInfo.fileContents, false, &dwStatusCode)!= S_OK || dwStatusCode != 200)
        return SR_FAILED;
    else
        return SR_SUCCEEDED;
}

const std::set<std::string>& Addic7ed::Languages() const
{
    static std::once_flag initialized;
    static std::set<std::string> result;

    std::call_once(initialized, [this]() {
        for (const auto& iter : Addic7ed_languages) {
            if (strlen(iter.code)) {
                result.emplace(iter.code);
            }
        }
    });

    return result;
}

bool Addic7ed::NeedLogin()
{
    return cookie.IsEmpty();
}

SRESULT Addic7ed::Login(const std::string& sUserName, const std::string& sPassword)
{
    // request for a new Addic7ed cookie
    stringMap headers({{ "User-Agent", SUB_ADDIC7ED_AGENT }});
    std::string data;   
    std::string url = Url();
    DWORD status = 0;
    StringDownload(url, headers, data, false, &status, &cookie);

    return (!cookie.IsEmpty())? SR_SUCCEEDED : SR_FAILED;
}
#endif

