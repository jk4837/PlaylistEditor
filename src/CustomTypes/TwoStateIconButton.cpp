#include "CustomTypes/TwoStateIconButton.hpp"

#include "Utils/Utils.hpp"

#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/UI/Image.hpp"

namespace PlaylistEditor
{

TwoStateIconButton::TwoStateIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                       const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                       const std::function<void(void)> &onClick1, UnityEngine::Sprite *icon1, const std::string &hint1,
                                       const std::function<void(void)> &onClick2, UnityEngine::Sprite *icon2, const std::string &hint2,
                                       const bool autoChangeState)
                                       : hint1_(hint1), hint2_(hint2), icon1_(icon1), icon2_(icon2), autoChangeState_(autoChangeState),
                                         IconButton(name, parent, buttonTemplate, anchoredPosition, sizeDelta, nullptr, icon1, hint1)
{
    std::function<void()> onClick = (std::function<void()>) [this, onClick1, onClick2] () {
        const bool oldIsFirstState = this->isFirstState_;
        if (this->isFirstState_)
            onClick1();
        else
            onClick2();

        if (this->autoChangeState_ && (oldIsFirstState == this->isFirstState_)) // in case user change state in onClick and change again here
            this->SetIsFirstState(!isFirstState_);
    };

    this->btn_->get_onClick()->AddListener(PlaylistEditor::Utils::MakeDelegate<UnityEngine::Events::UnityAction*>(onClick));

    // avoiding icon to be free after song played
    cacheObject1 = UnityEngine::GameObject::New_ctor(name);
    cacheObject1->AddComponent<UnityEngine::UI::Image*>()->set_sprite(icon1);
    cacheObject1->get_transform()->set_parent(this->btn_->get_transform());
    cacheObject1->SetActive(false);

    cacheObject2 = UnityEngine::GameObject::New_ctor(name);
    cacheObject2->AddComponent<UnityEngine::UI::Image*>()->set_sprite(icon2);
    cacheObject2->get_transform()->set_parent(this->btn_->get_transform());
    cacheObject2->SetActive(false);
}

void TwoStateIconButton::SetIsFirstState(const bool isFirstState)
{
    this->isFirstState_ = isFirstState;
    this->imageView_->set_sprite(this->isFirstState_ ? this->icon1_ : this->icon2_);
    this->ChangeHoverHint(this->isFirstState_ ? this->hint1_ : this->hint2_);
}

}
