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

void FileUtils::RemoveTmpDir() {
    std::filesystem::remove_all(Utils::ModPackTmpPath);
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

void FileUtils::AppendData(rapidjson::Document &document, const rapidjson::Document &bkpDocument) {
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    const auto &songs = document.GetObject()["songs"].GetArray();
    const auto &bkpSongs = bkpDocument.GetObject()["songs"].GetArray();

    if (songs.Size() != bkpSongs.Size()) {
        askUserRestore = true;
    }

    // logic can test with src/Test/AppendDataTest.cpp
    for (rapidjson::SizeType i = 0, j = 0; i < songs.Size(); i++) {
        if (j < bkpSongs.Size() &&
            0 == strcasecmp(songs[i]["hash"].GetString(), bkpSongs[j]["hash"].GetString())) {
            songs[i].CopyFrom(bkpSongs[j++], allocator);
            continue;
        }
        askUserRestore = true;
        // Looking from j-th to begin and end
        j = std::min(bkpSongs.Size() - 1, j);
        for (int k1 = j, k2 = j + 1; // k1 can't not be unsigned int
             k1 >= 0 || k2 < bkpSongs.Size();
             k1--, k2++) {
            if (k1 >= 0 &&
                0 == strcasecmp(songs[i]["hash"].GetString(), bkpSongs[k1]["hash"].GetString())) {
                songs[i].CopyFrom(bkpSongs[k1], allocator);
                j = k1 + 1;
                break;
            }
            if (k2 < bkpSongs.Size() &&
                0 == strcasecmp(songs[i]["hash"].GetString(), bkpSongs[k2]["hash"].GetString())) {
                songs[i].CopyFrom(bkpSongs[k2], allocator);
                j = k2 + 1;
                break;
            }
        }
    }
}

void FileUtils::AppendPlaylistData() { // for appending extra data that may delete by BMBF
    askUserRestore = false;

    if(!std::filesystem::is_directory(Utils::CustomLevelPackPath)) {
        INFO("Don't have playlist dir %s", Utils::CustomLevelPackPath.c_str());
        return;
    }

    if(std::filesystem::is_directory(Utils::ModPackPath)) {
        std::filesystem::remove_all(Utils::ModPackTmpPath);
        std::filesystem::rename(Utils::ModPackPath, Utils::ModPackTmpPath);
    }

    std::filesystem::create_directory(Utils::ModPackPath);

    std::vector<std::string> playlistFilename;
    for (const auto &entry : std::filesystem::directory_iterator(Utils::CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;

        rapidjson::Document document;
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
        const auto path = entry.path().string();
        const auto filename = entry.path().filename().string();
        const auto bkpPath = Utils::ModPackPath + filename;
        const auto tmpPath = Utils::ModPackTmpPath + filename;

        if (!LoadFile(path, document))
            continue;

        if (!document.GetObject().HasMember("appendPlaylistEditorData")) {
            document.AddMember("appendPlaylistEditorData", true, allocator);
            if (std::filesystem::is_regular_file(tmpPath)) {
                rapidjson::Document bkpDocument;
                if (LoadFile(tmpPath, bkpDocument)) {
                    AppendData(document, bkpDocument);
                }
            }
            WriteFile(path, document); // will backup in function
            INFO("Append extra data for playlist %s", path.c_str());
        } else
            BackupFile(path);

        playlistFilename.push_back(filename);
    }

    if (!askUserRestore && std::filesystem::is_directory(Utils::ModPackTmpPath)) {
        // see if any old playlist being deleted or created
        std::vector<std::string> playlistBkpFilename;
        for (const auto &entry : std::filesystem::directory_iterator(Utils::ModPackTmpPath)) {
            if(entry.is_directory())
                continue;

            playlistBkpFilename.push_back(entry.path().filename().string());
        }
        if (playlistFilename.size() != playlistBkpFilename.size()) {
            askUserRestore = true;
            return;
        }
        std::sort(playlistFilename.begin(), playlistFilename.end());
        std::sort(playlistBkpFilename.begin(), playlistBkpFilename.end());
        for (int i = 0; i < playlistFilename.size(); i++) {
            if (playlistFilename[i] != playlistBkpFilename[i]) {
                askUserRestore = true;
                return;
            }
        }
    }
}

void FileUtils::RestorePlaylistFile() {
    for (const auto &entry : std::filesystem::directory_iterator(Utils::CustomLevelPackPath)) {
        if(entry.is_directory())
            continue;
        std::filesystem::remove(entry.path());
    }

    std::filesystem::copy(Utils::ModPackTmpPath, Utils::CustomLevelPackPath);
    std::filesystem::remove_all(Utils::ModPackPath);
    std::filesystem::rename(Utils::ModPackTmpPath, Utils::ModPackPath);
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
        const std::string playlistTitle = to_utf8(csstrtostr(il2cpp_utils::newcsstr(document.GetObject()["playlistTitle"].GetString()))); // turn utf8 char to '?'
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
    Array<uint8_t> *byteArray = UnityEngine::ImageConversion::EncodeToPNG(selectedLevel->dyn__coverImage()->get_texture());
    return "data:image/png;base64," + to_utf8(csstrtostr(System::Convert::ToBase64String(byteArray)));
}

bool FileUtils::SetCoverImage(const std::string &path, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel)
{
    rapidjson::Document document;

    if(path.empty() | !LoadFile(path, document)) {
        ERROR("Failed to load file %s", path.c_str());
        return false;
    }

    if (selectedLevel) {
        rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
        document.GetObject()["image"].SetString(GetCoverImageBase64String(selectedLevel), allocator);
    } else
        document.GetObject()["image"].SetNull();

    if (!WriteFile(path, document)) {
        ERROR("Failed to write file");
        return false;
    }
    return true;
}

bool FileUtils::UpdateFile(const int selectedLevelIdx, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path,
                const FILE_ACTION act, const std::string &insertPath, const std::string &charStr, const int diff) {
    try {
        if (!selectedLevel)
            throw std::invalid_argument("null selected level");

        rapidjson::Document document;
        if(!path.empty() && !LoadFile(path, document))
            throw std::invalid_argument("failed to load file");

        int idx = 0;
        const std::string selectedLevelID = to_utf8(csstrtostr(selectedLevel->get_levelID()));
        const auto &songs = document.GetObject()["songs"].GetArray();
        switch (act) {
            case ITEM_INSERT:
            {
                rapidjson::Document document2;
                rapidjson::Document::AllocatorType &allocator2 = document2.GetAllocator();
                rapidjson::Value insertSong(rapidjson::kObjectType);

                if (!path.empty() && FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx))
                    insertSong.CopyFrom(songs[idx], allocator2);
                else {
                    insertSong.SetObject();
                    insertSong.AddMember("songName", to_utf8(csstrtostr(selectedLevel->get_songName())), allocator2);
                    insertSong.AddMember("levelAuthorName", to_utf8(csstrtostr(selectedLevel->get_levelAuthorName())), allocator2);
                    insertSong.AddMember("hash", selectedLevelID.substr(Utils::CustomLevelPrefixID.length()), allocator2);
                    insertSong.AddMember("levelid", selectedLevelID, allocator2);
                    insertSong.AddMember("uploader", to_utf8(csstrtostr(selectedLevel->get_levelAuthorName())), allocator2);
                }

                if (!insertSong.HasMember("char"))
                    insertSong.AddMember("char", charStr, allocator2);
                else
                    insertSong["char"].SetString(charStr, allocator2);
                if (!insertSong.HasMember("diff"))
                    insertSong.AddMember("diff", diff, allocator2);
                else
                    insertSong["diff"].SetInt(diff);
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
            default:
                break;
        }
        return true;
    } catch (const std::exception &e) {
        PlaylistEditor::Toast::GetInstance()->ShowMessage(e.what());
        ERROR("Error loading playlist %s: %s", path.data(), e.what());
    }
    return false;
}

bool FileUtils::UpdateSongLock(const int selectedLevelIdx, const std::string &selectedLevelID, const std::string &path,
                               const FILE_ACTION act, const std::string &charStr, const int diff) {
    if (selectedLevelID.empty()) {
        ERROR("Empty selected level id");
        return false;
    }

    rapidjson::Document document;
    if(path.empty() || !LoadFile(path, document)) {
        ERROR("Failed to load file %s", path.c_str());
        return false;
    }

    int idx = 0;
    if (!FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx)) {
        ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
        return false;
    }

    const auto &songs = document.GetObject()["songs"].GetArray();
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    if (ITEM_LOCK == act) {
        if (!songs[idx].HasMember("char"))
            songs[idx].AddMember("char", charStr, allocator);
        else
            songs[idx]["char"].SetString(charStr, allocator);
        if (!songs[idx].HasMember("diff"))
            songs[idx].AddMember("diff", diff, allocator);
        else
            songs[idx]["diff"].SetInt(diff);
    } else { // ITEM_UNLOCK
        if (songs[idx].HasMember("char"))
            songs[idx].RemoveMember("char");
        if (songs[idx].HasMember("diff"))
            songs[idx].RemoveMember("diff");
    }

    if (!WriteFile(path, document)) {
        ERROR("failed to write file");
        return false;
    }
    return true;
}

bool FileUtils::FindSongCharDiff(const int selectedLevelIdx, const std::string &selectedLevelID, const std::string &path,
                                 std::string &charSO, int &diff) {
    charSO = "";
    diff = 0;

    rapidjson::Document document;
    if(path.empty() || !LoadFile(path, document)) {
        ERROR("failed to load file %s", path.c_str());
        return false;
    }

    int idx = 0;
    if (!FindSongIdx(document, selectedLevelIdx, selectedLevelID, idx)) {
        ERROR("Failed to find %s in playlist dir %s", selectedLevelID.c_str(), path.c_str());
        return false;
    }
    const auto &song = document.GetObject()["songs"].GetArray()[idx];
    if (!song.HasMember("char") || !song.HasMember("diff"))
        return false;

    charSO = song["char"].GetString();
    diff = song["diff"].GetInt();
    ERROR("find song idx: %d, with char: %s, diff: %d", idx, charSO.c_str(), diff);
    return true;
}

}