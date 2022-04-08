// Edit from SongLoader\include\CustomTypes\SongLoader.hpp and SongLoader\include\LoadingUI.hpp
#pragma once

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Vector2.hpp"
#include "HMUI/ImageView.hpp"

#include "CustomTypes/IconButton.hpp"

namespace PlaylistEditor
{

class DoubleClickIconButton : public IconButton
{
public:
    DoubleClickIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                          const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                          const std::function<void(void)> &onDoubleClick, UnityEngine::Sprite *icon, const std::string_view &hint);
    DoubleClickIconButton(UnityEngine::UI::Button *btn, const std::function<void(void)> &onDoubleClick);
};

}