#include "CustomTypes/ListModal.hpp"
#include "CustomTypes/Logging.hpp"

#include "UnityEngine/WaitForFixedUpdate.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "HMUI/ScrollView.hpp"
#include "questui/shared/BeatSaberUI.hpp"

namespace PlaylistEditor
{

void ListModal::SetListItem(const std::vector<std::string> &list, const int initSelectIdx)
{
     // not knowning how to refresh container, so destroy and recreate every time
    this->Destroy();

    this->initSelectIdx_ = initSelectIdx;
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
    if (this->listModal_) {
        this->listModal_->get_gameObject()->set_active(active);
        if (this->initSelectIdx_ > 0)
            routine_ = GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(this->ScrollToInitSelect()));
    }

    if (!active) {
        this->Destroy();
        if (routine_) {
            GlobalNamespace::SharedCoroutineStarter::get_instance()->StopCoroutine(routine_);
            routine_ = nullptr;
        }
    }
}

custom_types::Helpers::Coroutine ListModal::ScrollToInitSelect()
{
    co_yield reinterpret_cast<System::Collections::IEnumerator *>(UnityEngine::WaitForFixedUpdate::New_ctor());

    auto scroll = this->listContainer_->get_gameObject()->GetComponentInParent<HMUI::ScrollView *>();
    if (!scroll) {
        ERROR("No scroll view found");
        co_return;
    }
    scroll->ScrollTo(this->initSelectIdx_ * (scroll->get_contentSize() / listModalItem_.size()), true);

    co_return;
}

}
