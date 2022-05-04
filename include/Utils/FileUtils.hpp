#pragma once

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs-string.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace PlaylistEditor
{

typedef enum FILE_ACTION {
    ITEM_INSERT, ITEM_REMOVE, ITEM_REMOVE_ALL, ITEM_MOVE_UP, ITEM_MOVE_DOWN, ITEM_LOCK, ITEM_UNLOCK
} FILE_ACTION_T;

class FileUtils
{
public:
    static bool ShrinkPlaylistPath();
    static void AppendPlaylistData();
    static void RestorePlaylistFile();
    static void RemoveTmpDir();
    void ReloadPlaylistPath();
    std::string GetPlaylistPath(const int listIdx, const std::string &listID, const bool canRefresh = true);
    bool CreateFile(const std::string &name);
    bool DeleteFile(const std::string &path);
    bool UpdateFile(const int selectedLevelIdx, GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path,
                    const FILE_ACTION act, const std::string &insertPath = "", const std::string &charStr = "", const int diff = -1);
    bool UpdateSongLock(const int selectedLevelIdx, const std::string &selectedLevelID, const std::string &path,
                        const FILE_ACTION act, const std::string &charStr = "", const int diff = -1);
    bool FindSongCharDiff(const int selectedLevelIdx, const std::string &selectedLevelID, const std::string &path,
                          std::string &charSO, int &diff);

    inline static bool askUserRestore = false;

private:
    static bool WriteFile(const std::string &path, rapidjson::Document &document);
    static bool LoadFile(const std::string &path, rapidjson::Document &document);
    static void AppendData(rapidjson::Document &document, const rapidjson::Document &bkpDocument);
    static void BackupFile(const std::string &path);
    bool FindSongIdx(const rapidjson::Document &document, const int selectedLevelIdx, const std::string &selectedLevelID, int &idx);
    bool FindAllSongIdx(const rapidjson::Document &document, const std::string &selectedLevelID, std::vector<int> &idxs);

    std::vector<std::tuple<std::string, std::string>> playlists;
};

}