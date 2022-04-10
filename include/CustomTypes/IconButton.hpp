// Edit from SongLoader\include\CustomTypes\SongLoader.hpp and SongLoader\include\LoadingUI.hpp
#pragma once

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Vector2.hpp"
#include "HMUI/ImageView.hpp"

namespace PlaylistEditor
{

class IconButton
{
public:
    IconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
               const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
               const std::function<void(void)> &onClick, UnityEngine::Sprite *icon, const std::string_view &hint);

    void ResetUI();
    void SetInteractable(const bool interactable);
    void SetActive(const bool active);
    void ChangeHoverHint(const std::string &hint);

protected:
    IconButton() = default;

    UnityEngine::UI::Button *btn_ = nullptr;
    HMUI::ImageView *imageView_ = nullptr;
};

}