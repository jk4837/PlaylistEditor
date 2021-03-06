#include "CustomTypes/IconButton.hpp"

#include "Utils/UIUtils.hpp"

#include "UnityEngine/GameObject.hpp"
#include "HMUI/HoverHint.hpp"

namespace PlaylistEditor
{

IconButton::IconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                       const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                       const std::function<void(void)> &onClick, UnityEngine::Sprite *icon, const std::string_view &hint)
{
    this->btn_ = Utils::CreateIconButton(name, parent, buttonTemplate, anchoredPosition, sizeDelta, onClick, icon, hint);
    this->imageView_ = btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([] (auto x) -> bool { return "Icon" == x->get_name(); });
}

void IconButton::ResetUI()
{
    if (!this->imageView_) {
        ERROR("Null imageView");
        return;
    }
    this->imageView_->set_color(UnityEngine::Color::get_white());
    btn_->set_interactable(true);
}

void IconButton::SetInteractable(const bool interactable)
{
    if (!this->imageView_) {
        ERROR("Null imageView");
        return;
    }

    btn_->set_interactable(interactable);

    if (interactable)
        this->ResetUI();
    else
        this->imageView_->set_color(UnityEngine::Color::get_gray());
}

void IconButton::SetButtonBackgroundActive(const bool active)
{
    auto background = btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().First([] (auto x) -> bool { return "BG" == x->get_name(); });
    if (background)
        background->get_gameObject()->SetActive(active);
}

void IconButton::SetActive(const bool active)
{
    btn_->get_gameObject()->set_active(active);
}

void IconButton::ChangeHoverHint(const std::string &hint)
{
    this->btn_->get_gameObject()->GetComponentsInChildren<HMUI::HoverHint *>().First()->set_text(hint);
}

}
