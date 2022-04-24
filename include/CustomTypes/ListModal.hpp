#pragma once

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "HMUI/ModalView.hpp"
#include "questui/shared/CustomTypes/Components/ClickableText.hpp"

namespace PlaylistEditor
{

class ListModal
{
public:
    ListModal(UnityEngine::Transform *parent,
              const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
              const std::function<void(const int itemIdx, const std::string &itemName)> &onItemClick)
              : parent_(parent), anchoredPosition_(anchoredPosition), sizeDelta_(sizeDelta), onItemClick_(onItemClick) {}
    void SetListItem(const std::vector<std::string> &list);
    bool GetActive();
    void SetActive(const bool active);

private:
    void Destroy();

    UnityEngine::Transform *parent_;
    UnityEngine::Vector2 anchoredPosition_;
    UnityEngine::Vector2 sizeDelta_;
    std::function<void(const int itemIdx, const std::string &itemName)> onItemClick_;

    HMUI::ModalView *listModal_ = nullptr;
    UnityEngine::GameObject *listContainer_ = nullptr;
    std::vector<QuestUI::ClickableText *> listModalItem_;
};

}