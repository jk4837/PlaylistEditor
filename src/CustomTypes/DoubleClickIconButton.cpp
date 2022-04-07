#include "CustomTypes/DoubleClickIconButton.hpp"

#include "CustomTypes/Logging.hpp"
#include "Utils/UIUtils.hpp"
#include "Utils/Utils.hpp"

#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"

namespace PlaylistEditor
{

DoubleClickIconButton::DoubleClickIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                             const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                             const std::function<void(void)> &onDoubleClick, UnityEngine::Sprite *icon, const std::string_view &hint)
{
    std::function<void()> onClick = (std::function<void()>) [this, onDoubleClick] () {
        if (!this->imageView_) {
            ERROR("Null imageView");
            return;
        }
        if (UnityEngine::Color::get_red() != this->imageView_->get_color()) {
            this->imageView_->set_color(UnityEngine::Color::get_red());
            return;
        }
        onDoubleClick();
        this->ResetUI();
    };

    this->btn_ = Utils::CreateIconButton(name, parent, buttonTemplate, anchoredPosition, sizeDelta, onClick, icon, hint);
    this->imageView_ = btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([] (auto x) -> bool { return "Icon" == x->get_name(); });
}

DoubleClickIconButton::DoubleClickIconButton(UnityEngine::UI::Button *btn, const std::function<void(void)> &onDoubleClick) {
    std::function<void()> onClick = (std::function<void()>) [this, onDoubleClick] () {
        if (!this->imageView_) {
            ERROR("Null imageView");
            return;
        }
        if (UnityEngine::Color::get_red() != this->imageView_->get_color()) {
            this->imageView_->set_color(UnityEngine::Color::get_red());
            return;
        }
        onDoubleClick();
        this->ResetUI();
    };
    this->btn_ = btn;
    this->btn_->set_onClick(UnityEngine::UI::Button::ButtonClickedEvent::New_ctor());
    this->btn_->get_onClick()->AddListener(PlaylistEditor::Utils::MakeDelegate<UnityEngine::Events::UnityAction*>(onClick));
    this->imageView_ = btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([] (auto x) -> bool { return "Icon" == x->get_name(); });
}

void DoubleClickIconButton::ResetUI()
{
    this->imageView_->set_color(UnityEngine::Color::get_white());
}

void DoubleClickIconButton::SetInteractable(const bool interactable)
{
    btn_->set_interactable(interactable);

    if (interactable)
        this->ResetUI();
    else
        this->imageView_->set_color(UnityEngine::Color::get_gray());
}

}
