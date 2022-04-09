#include "Utils/FileUtils.hpp"

#include <dirent.h>
#include <fstream>

#include "System/Convert.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Sprite.hpp"

#include "CustomTypes/Toast.hpp"
#include "CustomTypes/Logging.hpp"
#include "Utils/Utils.hpp"

namespace PlaylistEditor::Utils
{

static std::string rapidjsonToString(rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return std::string(buffer.GetString());
}

bool WriteFile(const std::string &path, rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);
    if (!writefile(path, buffer.GetString())) {
        ERROR("Failed to write file %s", path.data());
        return false;
    }
    return true;
}

bool LoadFile(const std::string &path, rapidjson::Document &document) {
    bool success = false;
    try {
        if(!fileexists(path))
            return false;
        auto json = readfile(path);
        document.Parse(json);
        if (document.HasParseError())
            throw std::invalid_argument("parsing error");
        if (!document.IsObject())
            throw std::invalid_argument("root isn't object");
        if (document.GetObject()["playlistTitle"].GetType() != rapidjson::kStringType)
            throw std::invalid_argument("root/playlistTitle not string");
        if (document.GetObject()["songs"].GetType() != rapidjson::kArrayType)
            throw std::invalid_argument("root/songs not array");
        success = true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error loading playlist %s: %s", path.data(), e.what());
    }
    return success;
}

bool FindSongIdx(const rapidjson::Document &document,
                 const int selectedLevelIdx, const std::string &selectedLevelID, int &idx) {
    const auto selectedHash = selectedLevelID.substr(CustomLevelPrefixID.length());
    try {
        const auto &songs = document.GetObject()["songs"].GetArray();
        // selectedLevelIdx may greater then real index in list, cause list can contain not found song
        for (rapidjson::SizeType i = selectedLevelIdx; i < songs.Size(); i++) {
            if (!songs[i].IsObject())
                throw std::invalid_argument("invalid song object");
            if (!songs[i].HasMember("hash") || !songs[i]["hash"].IsString())
                throw std::invalid_argument("invalid hash in song object");
            if (0 != strcasecmp(songs[i]["hash"].GetString(), selectedHash.c_str()))
                continue;
            idx = i;
            return true;
        }
        // find again upper if not found
        for (rapidjson::SizeType i = std::min(selectedLevelIdx-1, static_cast<int>(songs.Size())-1); i >= 0; i--) {
            if (!songs[i].IsObject())
                throw std::invalid_argument("invalid song object");
            if (!songs[i].HasMember("hash") || !songs[i]["hash"].IsString())
                throw std::invalid_argument("invalid hash in song object");
            if (0 != strcasecmp(songs[i]["hash"].GetString(), selectedHash.c_str()))
                continue;
            idx = i;
            return true;
        }
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error parsing playlist: %s", e.what());
    }
    return false;
}

bool FindAllSongIdx(const rapidjson::Document &document,
                    const std::string &selectedLevelID, std::vector<int> &idxs) {
    try {
        const auto selectedHash = selectedLevelID.substr(CustomLevelPrefixID.length());
        const auto &songs = document.GetObject()["songs"].GetArray();

        for (rapidjson::SizeType i = 0; i < songs.Size(); i++) {
            if (!songs[i].IsObject())
                throw std::invalid_argument("invalid song object");
            if (!songs[i].HasMember("hash") || !songs[i]["hash"].IsString())
                throw std::invalid_argument("invalid hash in song object");
            if (0 != strcasecmp(songs[i]["hash"].GetString(), selectedHash.c_str()))
                continue;
            idxs.push_back(i);
        }
        return idxs.size() > 0;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error parsing playlist: %s", e.what());
    }
    return false;
}

static std::vector<std::tuple<StringW, std::string>> playlists;
void ReloadPlaylistPath() {
    playlists.clear();
    if(!std::filesystem::is_directory(CustomLevelPackPath)) {
        INFO("Don't have playlist dir %s", CustomLevelPackPath.c_str());
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;
        rapidjson::Document document;
        auto path = entry.path().string();
        if (!LoadFile(path, document))
            continue;
        const std::string playlistTitle = document.GetObject()["playlistTitle"].GetString();
        const std::string playlistID = CustomLevelPackPrefixID + playlistTitle;
        playlists.push_back(make_tuple(playlistID, path));
        // INFO("LoadPlaylists #%d %s : %s", playlists.size()+1, playlistID.c_str(), path.c_str());
    }
}

std::string GetPlaylistPath(const int listIdx, const StringW &listID, const bool canRefresh) {
    if (0 == listIdx || CustomLevelID == listID)
        return "";
    for (int i = listIdx - 1; i < playlists.size(); i++) { // listIdx contain "Custom Level"
        if (listID != std::get<0>(playlists[i]))
            continue;
        return std::get<1>(playlists[i]);
    }
    // find again upper if not found
    for (int i = std::min(listIdx, static_cast<int>(playlists.size())-1); i >= 0; i--) {
        if (listID != std::get<0>(playlists[i]))
            continue;
        return std::get<1>(playlists[i]);
    }
    if (canRefresh) {
        INFO("Don't have playlist %s, reload all", std::string(listID).c_str());
        ReloadPlaylistPath();
        return GetPlaylistPath(listIdx, listID, false);
    }
    INFO("Failed to find playlist of %s", std::string(listID).c_str());
    return "";
}

bool CreateFile(const std::string &name) {
    try {
        rapidjson::Document document;
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

        document.SetObject();
        document.AddMember("playlistTitle", name, allocator);   // require
        document.AddMember("playlistAuthor", ID, allocator);
        document.AddMember("playlistDescription", "Created by " ID, allocator);
        document.AddMember("songs", rapidjson::Value(rapidjson::kArrayType), allocator);             // require
        document.AddMember("image", rapidjson::Value(), allocator);
        if (!WriteFile(CustomLevelPackPath + name + "_BMBF.json", document)) // postfix will avoid BMBF clone list
            throw std::invalid_argument("failed to write file");
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error create playlist %s: %s", name.c_str(), e.what());
    }
    return false;
}

bool DeleteFile(const std::string &path) {
    if(!fileexists(path))
        return true;
    return deletefile(path);
}

static std::string GetCoverImageBase64String(GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel)
{
    const ::ArrayW<uint8_t> byteArray = UnityEngine::ImageConversion::EncodeToPNG(selectedLevel->dyn__coverImage()->get_texture());
    return "data:image/png;base64," + System::Convert::ToBase64String(byteArray);
}

bool UpdateFile(const int selectedLevelIdx, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path,
                const FILE_ACTION act, const std::string &insertPath) {
    try {
        if (!selectedLevel)
            throw std::invalid_argument("null selected level");

        rapidjson::Document document;
        if(!path.empty() && !LoadFile(path, document))
            throw std::invalid_argument("failed to load file");

        int idx = 0;
        const std::string selectedLevelID = selectedLevel->get_levelID();
        const auto &songs = document.GetObject()["songs"].GetArray();
        switch (act) {
            case ITEM_INSERT:
            {
                rapidjson::Document document2;
                rapidjson::Document::AllocatorType &allocator2 = document2.GetAllocator();
                rapidjson::Value insertSong(rapidjson::kObjectType);

                if (!path.empty() && FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx))
                    insertSong = songs[idx];
                else {
                    insertSong.SetObject();
                    insertSong.AddMember("songName", std::string(selectedLevel->get_songName()), allocator2);
                    insertSong.AddMember("levelAuthorName", std::string(selectedLevel->get_levelAuthorName()), allocator2);
                    insertSong.AddMember("hash", selectedLevelID.substr(CustomLevelPrefixID.length()), allocator2);
                    insertSong.AddMember("levelid", selectedLevelID, allocator2);
                    insertSong.AddMember("uploader", std::string(selectedLevel->get_levelAuthorName()), allocator2);
                }

                if (!LoadFile(insertPath, document2))
                    throw std::invalid_argument("failed to load file which want to insert to");

                document2.GetObject()["songs"].GetArray().PushBack(insertSong, allocator2);
                if (document2.GetObject()["image"].IsNull())
                    document2.GetObject()["image"].SetString(GetCoverImageBase64String(selectedLevel), allocator2);

                if (!WriteFile(insertPath, document2))
                    throw std::invalid_argument("failed to write file");
            }
            break;
            case ITEM_REMOVE:
            case ITEM_REMOVE_ALL:
                if (path.empty()) {
                    bool has_remove = false;
                    for (auto it : playlists)
                        has_remove |= UpdateFile(selectedLevelIdx, selectedLevel, std::get<1>(it), ITEM_REMOVE_ALL);
                    if (!has_remove)
                        return false;
                } else {
                    if (ITEM_REMOVE == act) {
                        if (!FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx)) {
                            ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
                            return false;
                        }
                        songs.Erase(songs.Begin() + idx);
                        if (!WriteFile(path, document))
                            throw std::invalid_argument("failed to write file");
                    } else if (ITEM_REMOVE_ALL == act) {
                        std::vector<int> idxs;
                        if (!FindAllSongIdx(document, selectedLevelID, idxs))
                            return false;
                        for (int i = idxs.size()-1; i >= 0; i--)
                            songs.Erase(songs.Begin() + idxs[i]);
                        if (!WriteFile(path, document))
                            throw std::invalid_argument("failed to write file");
                    }
                }
                break;
            case ITEM_MOVE_DOWN:
                if (path.empty() || !FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx)) {
                    ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
                    return false;
                }
                if (idx >= songs.Size() - 1) // already at bottom
                    return false;
                songs[idx].Swap(songs[idx+1]);
                if (!WriteFile(path, document))
                    throw std::invalid_argument("failed to write file");
                break;
            case ITEM_MOVE_UP:
                if (path.empty() || !FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx)) {
                    ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
                    return false;
                }
                if (idx <= 0) // already at top
                    return false;
                songs[idx].Swap(songs[idx-1]);
                if (!WriteFile(path, document))
                    throw std::invalid_argument("failed to write file");
                break;
        }
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error loading playlist %s: %s", path.data(), e.what());
    }
    return false;
}

}