#include "Utils/FileUtils.hpp"

#include <dirent.h>
#include <fstream>

#include "System/Convert.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Sprite.hpp"

#include "CustomTypes/Toast.hpp"
#include "CustomTypes/Logging.hpp"
#include "Utils/Utils.hpp"

namespace PlaylistEditor
{

const std::string BMBFPlaylistPostfix = "_BMBF.json";

static std::string rapidjsonToString(rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return std::string(buffer.GetString());
}

std::string GetBackupFilePath(const std::string &path) {
    const auto filename = std::filesystem::path(path).filename().string();
    return Utils::ModPackPath + filename;
}

void FileUtils::BackupFile(const std::string &path) {
    std::filesystem::copy_file(path, GetBackupFilePath(path), std::filesystem::copy_options::overwrite_existing);
}

bool FileUtils::WriteFile(const std::string &path, rapidjson::Document &document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    document.Accept(writer);
    if (!writefile(path, buffer.GetString())) {
        ERROR("Failed to write file %s", path.data());
        return false;
    }
    BackupFile(path);
    return true;
}

bool FileUtils::LoadFile(const std::string &path, rapidjson::Document &document) {
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

bool FileUtils::FindSongIdx(const rapidjson::Document &document,
                 const int selectedLevelIdx, const std::string &selectedLevelID, int &idx) {
    const auto selectedHash = selectedLevelID.substr(Utils::CustomLevelPrefixID.length());
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

bool FileUtils::FindAllSongIdx(const rapidjson::Document &document,
                    const std::string &selectedLevelID, std::vector<int> &idxs) {
    try {
        const auto selectedHash = selectedLevelID.substr(Utils::CustomLevelPrefixID.length());
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

namespace {
bool EndsWith(const std::string &path, const std::string &postfix) {
    return (path.length() >= postfix.length()) &&
           (postfix == path.substr(path.length() - postfix.length()));
}
// shrink untill single postfix
std::string ShrinkBMBFPath(const std::string &path) {
    if (!EndsWith(path, BMBFPlaylistPostfix))
        return path + BMBFPlaylistPostfix;
    return ShrinkBMBFPath(path.substr(0, path.length() - 10));
}

}

bool FileUtils::ShrinkPlaylistPath() { // for multiple _BMBF.json
    bool hasShrink = false;
    if(!std::filesystem::is_directory(Utils::CustomLevelPackPath)) {
        INFO("Don't have playlist dir %s", Utils::CustomLevelPackPath.c_str());
        return false;
    }
    for (const auto &entry : std::filesystem::directory_iterator(Utils::CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;

        const auto path = entry.path().string();
        const auto BMBFPath = ShrinkBMBFPath(path);

        if (path == BMBFPath)
            continue;
        std::filesystem::rename(path, BMBFPath);
        hasShrink = true;
        INFO("Shrink playlist from %s to %s", path.c_str(), BMBFPath.c_str());
    }
    return hasShrink;
}

// TODO: handle error cond
// TODO: better append logic
bool FileUtils::AppendData(rapidjson::Document &document, const rapidjson::Document &bkpDocument) {
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    const auto &songs = document.GetObject()["songs"].GetArray();
    const auto &bkpSongs = bkpDocument.GetObject()["songs"].GetArray();

    if (songs.Size() != bkpSongs.Size()) {
        INFO("AppendData with diff size, songs %u <> bkpSongs %u", songs.Size(), bkpSongs.Size());
    }
    for (rapidjson::SizeType i = 0, j = 0; i < songs.Size() && j < bkpSongs.Size(); i++) {
        if (0 != strcasecmp(songs[i]["hash"].GetString(), bkpSongs[j]["hash"].GetString())) {
            if (songs.Size() < bkpSongs.Size()) { // guess user had deleted some songs by BMBF
                INFO("Skip backup hash %s", bkpSongs[j]["hash"].GetString());
                i--;
                j++;
            } else if (songs.Size() > bkpSongs.Size()) { // guess user had added some songs by BMBF
                INFO("Skip hash %s", songs[i]["hash"].GetString());
            } else { // guess user had added and delete some songs or just change order by BMBF
                INFO("Todo loop to j end to find match song %s", songs[i]["hash"].GetString());
            }
            continue;
        }
        songs[i].CopyFrom(bkpSongs[j++], allocator);
    }
    return true;
}

bool FileUtils::AppendPlaylistData() { // for appending extra data that may delete by BMBF
    bool hasAppend = false;
    if(!std::filesystem::is_directory(Utils::CustomLevelPackPath)) {
        INFO("Don't have playlist dir %s", Utils::CustomLevelPackPath.c_str());
        return false;
    }

    if(!std::filesystem::is_directory(Utils::ModPackPath))
        std::filesystem::create_directory(Utils::ModPackPath);

    for (const auto &entry : std::filesystem::directory_iterator(Utils::CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;

        rapidjson::Document document;
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
        const auto path = entry.path().string();
        const auto filename = entry.path().filename().string();
        const auto bkpPath = Utils::ModPackPath + filename;

        if (!LoadFile(path, document))
            continue;

        if (!document.GetObject().HasMember("appendPlaylistEditorData")) {
            hasAppend = true;
            document.AddMember("appendPlaylistEditorData", true, allocator);
            if (std::filesystem::is_regular_file(bkpPath)) {
                rapidjson::Document bkpDocument;
                if (LoadFile(bkpPath, bkpDocument)) {
                    AppendData(document, bkpDocument);
                }
            }
            WriteFile(path, document); // will backup in function
            INFO("Append extra data for playlist %s", path.c_str());
        }
    }
    return hasAppend;
}

void FileUtils::ReloadPlaylistPath() {
    playlists.clear();
    if(!std::filesystem::is_directory(Utils::CustomLevelPackPath)) {
        INFO("Don't have playlist dir %s", Utils::CustomLevelPackPath.c_str());
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(Utils::CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;
        rapidjson::Document document;
        auto path = entry.path().string();
        if (!LoadFile(path, document))
            continue;
        const std::string playlistTitle = document.GetObject()["playlistTitle"].GetString();
        const std::string playlistID = Utils::CustomLevelPackPrefixID + playlistTitle;
        playlists.push_back(make_tuple(playlistID, path));
        // INFO("LoadPlaylists #%d %s : %s", playlists.size()+1, playlistID.c_str(), path.c_str());
    }
}

std::string FileUtils::GetPlaylistPath(const int listIdx, const std::string &listID, const bool canRefresh) {
    if (0 == listIdx || Utils::CustomLevelID == listID)
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
        INFO("Don't have playlist %s, reload all", listID.c_str());
        ReloadPlaylistPath();
        return GetPlaylistPath(listIdx, listID, false);
    }
    INFO("Failed to find playlist of %s", listID.c_str());
    return "";
}

bool FileUtils::CreateFile(const std::string &name) {
    try {
        rapidjson::Document document;
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

        document.SetObject();
        document.AddMember("playlistTitle", name, allocator);   // require
        document.AddMember("playlistAuthor", ID, allocator);
        document.AddMember("playlistDescription", "Created by " ID, allocator);
        document.AddMember("songs", rapidjson::Value(rapidjson::kArrayType), allocator);             // require
        document.AddMember("image", rapidjson::Value(), allocator);
        document.AddMember("appendPlaylistEditorData", true, allocator);
        if (!WriteFile(Utils::CustomLevelPackPath + name + BMBFPlaylistPostfix, document)) // postfix will avoid BMBF clone list
            throw std::invalid_argument("failed to write file");
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error create playlist %s: %s", name.c_str(), e.what());
    }
    return false;
}

bool FileUtils::DeleteFile(const std::string &path) {
    if(!fileexists(path))
        return true;
    if (!deletefile(path))
        return false;
    deletefile(GetBackupFilePath(path));
    return true;
}

static std::string GetCoverImageBase64String(GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel)
{
    const ::ArrayW<uint8_t> byteArray = UnityEngine::ImageConversion::EncodeToPNG(selectedLevel->dyn__coverImage()->get_texture());
    return "data:image/png;base64," + System::Convert::ToBase64String(byteArray);
}

bool FileUtils::UpdateFile(const int selectedLevelIdx, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path,
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
                    insertSong.AddMember("hash", selectedLevelID.substr(Utils::CustomLevelPrefixID.length()), allocator2);
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