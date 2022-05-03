#include "CustomTypes/TwoStateIconButton.hpp"

#include "Utils/Utils.hpp"

#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"

namespace PlaylistEditor
{

TwoStateIconButton::TwoStateIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                       const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                       const std::function<void(void)> &onClick1, UnityEngine::Sprite *icon1, const std::string &hint1,
                                       const std::function<void(void)> &onClick2, UnityEngine::Sprite *icon2, const std::string &hint2)
                                       : hint1_(hint1), hint2_(hint2), icon1_(icon1), icon2_(icon2),
                                         IconButton(name, parent, buttonTemplate, anchoredPosition, sizeDelta, nullptr, icon1, hint1)
{
    std::function<void()> onClick = (std::function<void()>) [this, onClick1, onClick2] () {
        if (this->isFirstState_)
            onClick1();
        else
            onClick2();
        this->SetIsFirstState(!isFirstState_);
    };

    this->btn_->get_onClick()->AddListener(PlaylistEditor::Utils::MakeDelegate<UnityEngine::Events::UnityAction*>(onClick));
}

void TwoStateIconButton::SetIsFirstState(const bool isFirstState)
{
    this->isFirstState_ = isFirstState;
    this->imageView_->set_sprite(this->isFirstState_ ? this->icon1_ : this->icon2_);
    this->ChangeHoverHint(this->isFirstState_ ? this->hint1_ : this->hint2_);
}

}
