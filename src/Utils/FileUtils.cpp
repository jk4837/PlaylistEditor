#include "Utils/FileUtils.hpp"

#include <dirent.h>
#include <fstream>

#include "CustomTypes/Toast.hpp"
#include "logging.hpp"
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

bool FindSongIdx(const rapidjson::Document &document, const std::string &selectedLevelID, int &idx) {
    const auto selectedHash = selectedLevelID.substr(CustomLevelPrefixID.length());
    try {
        const auto &songs = document.GetObject()["songs"].GetArray();
        for (rapidjson::SizeType i = 0; i < songs.Size(); i++) {
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

static std::map<StringW, std::string> playlists;
std::string GetPlaylistPath(const StringW &listID, const bool fullRefresh) {
    if (CustomLevelID == listID)
        return "";
    if (fullRefresh)
        playlists.clear();
    if (!playlists.contains(listID)) {
        INFO("Don't have playlist %s, reload all", std::string(listID).c_str());
        playlists.clear();
        const std::string playlistPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/PlaylistManager/Playlists";

        if(!std::filesystem::is_directory(playlistPath)) {
            INFO("Don't have playlist dir %s", playlistPath.c_str());
            return "";
        }
        for (const auto& entry : std::filesystem::directory_iterator(playlistPath)) {
            if(!entry.is_directory()) {
                rapidjson::Document document;
                auto path = entry.path().string();
                if (!LoadFile(path, document))
                    continue;
                const std::string playlistTitle = document.GetObject()["playlistTitle"].GetString();
                playlists[CustomLevelPackPrefixID + playlistTitle] = path;
                INFO("LoadPlaylists %s : %s", playlistTitle.c_str(), path.c_str());
            }
        }
    }
    if (std::string(listID).empty() || !playlists.contains(listID)) {
        INFO("Failed to find playlist of %s", std::string(listID).c_str());
        return "";
    }
    return playlists[listID];
}

bool CreateFile(const std::string &name) {
    try {
        rapidjson::Document document;
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        document.SetObject();
        document.AddMember("playlistTitle", name, allocator);   // require
        document.AddMember("playlistAuthor", ID, allocator);
        document.AddMember("playlistDescription", "Created by " ID, allocator);
        document.AddMember("songs", rapidjson::Value(rapidjson::kArrayType), allocator);             // require
        document.AddMember("image", rapidjson::Value(), allocator);
        if (!WriteFile(CustomLevelPackPath + name, document))
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

bool UpdateFile(GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path, const FILE_ACTION act, const std::string &insertPath) {
    try {
        if (!selectedLevel)
            throw std::invalid_argument("null selected level");

        rapidjson::Document document;
        if(!path.empty() && !LoadFile(path, document))
            throw std::invalid_argument("failed to load file");

        int idx = 0;
        const std::string selectedLevelID = selectedLevel->get_levelID();
        const bool found = !path.empty() && FindSongIdx(document, selectedLevelID, idx);

        const auto &songs = document.GetObject()["songs"].GetArray();
        switch (act) {
            case ITEM_INSERT:
            {
                rapidjson::Document document2;
                rapidjson::Document::AllocatorType& allocator2 = document2.GetAllocator();
                rapidjson::Value insertSong(rapidjson::kObjectType);

                if (!found) {
                    insertSong.SetObject();
                    insertSong.AddMember("songName", std::string(selectedLevel->get_songName()), allocator2);
                    insertSong.AddMember("levelAuthorName", std::string(selectedLevel->get_levelAuthorName()), allocator2);
                    insertSong.AddMember("hash", selectedLevelID.substr(CustomLevelPrefixID.length()), allocator2);
                    insertSong.AddMember("levelid", selectedLevelID, allocator2);
                    insertSong.AddMember("uploader", std::string(selectedLevel->get_levelAuthorName()), allocator2);
                } else
                    insertSong = songs[idx];

                if (!LoadFile(insertPath, document2))
                    throw std::invalid_argument("failed to load file which want to insert to");
                document2.GetObject()["songs"].GetArray().PushBack(insertSong, allocator2);
                if (!WriteFile(insertPath, document2))
                    throw std::invalid_argument("failed to write file");
            }
            break;
            case ITEM_REMOVE:
                if (path.empty()) {
                    bool has_remove = false;
                    if (playlists.empty())
                        GetPlaylistPath();
                    for (auto it : playlists)
                        has_remove |= UpdateFile(selectedLevel, it.second, ITEM_REMOVE);
                    if (!has_remove)
                        return false;
                } else if (!found) {
                    ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
                    return false;
                } else {
                    songs.Erase(songs.Begin() + idx);
                    if (!WriteFile(path, document))
                        throw std::invalid_argument("failed to write file");
                }
                break;
            case ITEM_MOVE_DOWN:
                if (!found) {
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
                if (!found) {
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