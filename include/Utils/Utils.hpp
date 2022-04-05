#pragma once

#include <map>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace PlaylistEditor::Utils
{
const std::string CustomLevelPackPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/PlaylistManager/Playlists/";
const std::string IconPathTemplate = "/sdcard/ModData/com.beatgames.beatsaber/Mods/" ID "/Icons/%s.png";
const std::string CustomLevelPackPrefixID = "custom_levelPack_";
const std::string CustomLevelPrefixID = "custom_level_";
const std::string CustomLevelID = "custom_levelPack_CustomLevels";
const std::string CustomLevelName = "Custom Levels";

template <class T, typename Method>
T MakeDelegate(Method fun)
{
    return il2cpp_utils::MakeDelegate<T>(classof(T), fun);
}

}