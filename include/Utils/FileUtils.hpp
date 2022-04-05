#pragma once

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs-string.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace PlaylistEditor::Utils
{

typedef enum FILE_ACTION {
    ITEM_INSERT, ITEM_REMOVE, ITEM_MOVE_UP, ITEM_MOVE_DOWN
} FILE_ACTION_T;

bool WriteFile(const std::string &path, rapidjson::Document &document);
bool LoadFile(const std::string &path, rapidjson::Document &document);
bool FindSongIdx(const rapidjson::Document &document, const std::string &selectedLevelID, int &idx);
std::string GetPlaylistPath(const StringW &listID = "", const bool fullRefresh = false);
bool CreateFile(const std::string &name);
bool DeleteFile(const std::string &path);
bool UpdateFile(GlobalNamespace::CustomPreviewBeatmapLevel *selectedLevel, const std::string &path, const FILE_ACTION act, const std::string &insertPath = "");

}