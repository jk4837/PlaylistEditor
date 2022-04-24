#include "CustomTypes/ListModal.hpp"

#include "questui/shared/BeatSaberUI.hpp"

namespace PlaylistEditor
{

void ListModal::SetListItem(const std::vector<std::string> &list)
{
     // not knowning how to refresh container, so destroy and recreate every time
    this->Destroy();

    this->listModal_ = QuestUI::BeatSaberUI::CreateModal(this->parent_, this->anchoredPosition_, this->sizeDelta_, nullptr);
    this->listContainer_ = QuestUI::BeatSaberUI::CreateScrollableModalContainer(this->listModal_);
    this->listModal_->get_transform()->set_localScale({0.75, 0.75, 1});

    for (int i = 0; i < list.size(); i++) {
        const auto name = list[i];
        this->listModalItem_.push_back(
            QuestUI::BeatSaberUI::CreateClickableText(this->listContainer_->get_transform(), name, false,
                                                      [this, i, name] () { return this->onItemClick_(i, name); }));
    }
}

void ListModal::Destroy()
{
    for (auto item : this->listModalItem_) {
        UnityEngine::Object::Destroy(item);
    }
    this->listModalItem_.clear();

    if (this->listModal_)
        UnityEngine::Object::Destroy(this->listModal_);
    this->listModal_ = nullptr;

    if (this->listContainer_)
        UnityEngine::Object::Destroy(this->listContainer_);
    this->listContainer_ = nullptr;
}

bool ListModal::GetActive()
{
    return this->listModal_ && this->listModal_->get_gameObject()->get_active();
}

void ListModal::SetActive(const bool active)
{
    if (this->listModal_)
        this->listModal_->get_gameObject()->set_active(active);

    if (!active)
        this->Destroy();
}

}