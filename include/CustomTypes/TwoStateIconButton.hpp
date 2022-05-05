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

class TwoStateIconButton : public IconButton
{
public:
    TwoStateIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                       const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                       const std::function<void(void)> &onClick1, UnityEngine::Sprite *icon1, const std::string &hint1,
                       const std::function<void(void)> &onClick2, UnityEngine::Sprite *icon2, const std::string &hint2,
                       const bool autoChangeState = true);

    void SetIsFirstState(const bool isFirstState);
    bool GetIsFirstState() { return isFirstState_; }

private:
    bool autoChangeState_ = true;
    bool isFirstState_ = true;
    std::string hint1_;
    std::string hint2_;
    UnityEngine::Sprite *icon1_ = nullptr;
    UnityEngine::Sprite *icon2_ = nullptr;
};

}
