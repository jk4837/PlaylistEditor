#pragma once

#include <string>

#include "CustomTypes/Logging.hpp"
#include "CustomTypes/DoubleClickIconButton.hpp"

#include "HMUI/InputFieldView.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Vector2.hpp"

namespace PlaylistEditor::Utils
{

UnityEngine::Sprite *FileToSprite(const std::string_view &image_name);

UnityEngine::UI::Button *CreateIconButton(const std::string_view &name, UnityEngine::Transform *parent, const std::string_view &buttonTemplate,
                                          const UnityEngine::Vector2 &anchoredPosition, const UnityEngine::Vector2 &sizeDelta,
                                          const std::function<void(void)> &onClick, UnityEngine::Sprite *icon, const std::string_view &hint);

HMUI::InputFieldView *CreateStringInput(UnityEngine::Transform *parent, const StringW &settingsName, const StringW &currentValue,
                                        const UnityEngine::Vector2 &anchoredPosition, const float width,
                                        const std::function<void(StringW)> &onEnter);

void ShowRestoreDialog(UnityEngine::Transform *parent, const std::function<void(void)> &onOK, const std::function<void(void)> &onCancel);

template <class T>
void listAllName(UnityEngine::Transform *parent, const std::string &prefix = "") {
    // INFO("%s #p: tag: %s, name: %s, id: %u", prefix.c_str(), std::string(parent->get_tag()).c_str(), std::string(parent->get_name()).c_str(), parent->GetInstanceID());
    auto childs = parent->GetComponentsInChildren<T *>();
    std::vector<size_t> vec;
    for (size_t i = 0, idx = 1; i < childs.Length(); i++) {
        if (parent->GetInstanceID() == childs.get(i)->GetInstanceID())
            continue;
        if (parent->GetInstanceID() != childs.get(i)->get_transform()->get_parent()->GetInstanceID())
            continue;
        vec.push_back(i);
    }
    for (size_t idx = 0; idx < vec.size(); idx++) {
        const int i = vec[idx];
        INFO("%s|--%zu: name: %s, id: %u %s", prefix.c_str(), idx, std::string(childs.get(i)->get_name()).c_str(), childs.get(i)->GetInstanceID(), "Untagged" != childs.get(i)->get_tag() ? std::string(", tag: " + childs.get(i)->get_tag()).c_str() : "");
        listAllName<T>(childs.get(i)->get_transform(), prefix + (idx != vec.size()-1 ? "|  " : "   "));
    }
}

}