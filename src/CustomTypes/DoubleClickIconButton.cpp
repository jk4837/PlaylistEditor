#include "CustomTypes/DoubleClickIconButton.hpp"

#include "CustomTypes/IconButton.hpp"
#include "Utils/UIUtils.hpp"
#include "Utils/Utils.hpp"

#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"

namespace PlaylistEditor
{

DoubleClickIconButton::DoubleClickIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                             const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                             const std::function<void(void)> &onDoubleClick, UnityEngine::Sprite *icon, const std::string_view &hint)
                                             : IconButton(name, parent, buttonTemplate, anchoredPosition, sizeDelta, nullptr, icon, hint)
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

    this->btn_->get_onClick()->AddListener(PlaylistEditor::Utils::MakeDelegate<UnityEngine::Events::UnityAction*>(onClick));
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

}
