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
    this->imageView_ = QuestUI::ArrayUtil::First(btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>(), [] (auto x) -> bool { return x->get_name()->Equals(il2cpp_utils::newcsstr("Icon")); });
}

void IconButton::ResetUI()
{
    this->imageView_->set_color(UnityEngine::Color::get_white());
    btn_->set_interactable(true);
}

void IconButton::SetInteractable(const bool interactable)
{
    btn_->set_interactable(interactable);

    if (interactable)
        this->ResetUI();
    else
        this->imageView_->set_color(UnityEngine::Color::get_gray());
}

void IconButton::SetButtonBackgroundActive(const bool active)
{
    auto background = QuestUI::ArrayUtil::First(btn_->get_transform()->GetComponentsInChildren<HMUI::ImageView*>(), [] (auto x) -> bool { return x->get_name()->Equals(il2cpp_utils::newcsstr("BG")); });
    if (background)
        background->get_gameObject()->SetActive(active);
}

void IconButton::SetActive(const bool active)
{
    btn_->get_gameObject()->set_active(active);
}

void IconButton::ChangeHoverHint(const std::string &hint)
{
    QuestUI::ArrayUtil::First(this->btn_->get_gameObject()->GetComponentsInChildren<HMUI::HoverHint *>())->set_text(il2cpp_utils::newcsstr(hint));
}

}
