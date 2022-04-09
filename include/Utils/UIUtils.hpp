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

template <class T>
void listAllName(UnityEngine::Transform *parent, const std::string &prefix = "") {
    INFO("%s #p: tag: %s, name: %s, id: %u", prefix.c_str(), std::string(parent->get_tag()).c_str(), std::string(parent->get_name()).c_str(), parent->GetInstanceID());
    auto childs = parent->GetComponentsInChildren<T *>();
    for (size_t i = 0; i < childs.Length(); i++) {
        if (parent->GetInstanceID() == childs.get(i)->GetInstanceID())
            continue;
        INFO("%s #%zu: tag: %s, name: %s, id: %u", prefix.c_str(), i, std::string(childs.get(i)->get_tag()).c_str(), std::string(childs.get(i)->get_name()).c_str(), childs.get(i)->GetInstanceID());
    }
}

}