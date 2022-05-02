#pragma once

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs-string.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace PlaylistEditor
{

typedef enum FILE_ACTION {
    ITEM_INSERT, ITEM_REMOVE, ITEM_REMOVE_ALL, ITEM_MOVE_UP, ITEM_MOVE_DOWN
} FILE_ACTION_T;

class FileUtils
{
public:
    static bool ShrinkPlaylistPath();
    void ReloadPlaylistPath();
    std::string GetPlaylistPath(const int listIdx, const std::string &listID, const bool canRefresh = true);
    bool CreateFile(const std::string &name);
    bool DeleteFile(const std::string &path);
    bool UpdateFile(const int selectedLevelIdx, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path, const FILE_ACTION act, const std::string &insertPath = "");

private:
    bool WriteFile(const std::string &path, rapidjson::Document &document);
    bool LoadFile(const std::string &path, rapidjson::Document &document);
    bool FindSongIdx(const rapidjson::Document &document, const int selectedLevelIdx, const std::string &selectedLevelID, int &idx);
    bool FindAllSongIdx(const rapidjson::Document &document, const std::string &selectedLevelID, std::vector<int> &idxs);

    std::vector<std::tuple<std::string, std::string>> playlists;
};

}