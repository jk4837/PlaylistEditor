// Edit from SongLoader\include\CustomTypes\SongLoader.hpp and SongLoader\include\LoadingUI.hpp
#pragma once

#include "beatsaber-hook/shared/utils/typedefs.h"
// #include "codegen/include/TMPro/TextMeshProUGUI.hpp"
// #include "custom-types/shared/macros.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "HMUI/ImageView.hpp"

namespace PlaylistEditor
{

class DoubleClickIconButton
{
public:
    DoubleClickIconButton(const std::string_view &name, UnityEngine::Transform* parent, const std::string_view &buttonTemplate,
                        const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                        const std::function<void(void)> &onDoubleClick, UnityEngine::Sprite* icon, const std::string_view &hint);
    DoubleClickIconButton(UnityEngine::UI::Button *btn, const std::function<void(void)> &onDoubleClick);

    UnityEngine::UI::Button *operator->() const { return btn_; }

    void ResetUI();
    void SetInteractable(const bool interactable);

private:
    UnityEngine::UI::Button *btn_ = nullptr;
    HMUI::ImageView *imageView_ = nullptr;
};

}