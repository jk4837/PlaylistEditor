#include "UnityEngine/GameObject.hpp"
#include "Backport/ClickableText.hpp"
#include "Backport/typedefs-string.hpp"
#include "questui/shared/BeatSaberUI.hpp"

namespace QuestUI::BeatSaberUI {

    /// @brief Creates clickable text, almost like a URL
    /// @param parent what to parent it to
    /// @param text the text to display
    /// @param italic is the text italic?
    /// @param onClick what to run when clicked
    /// @return created clickable text
    ClickableText* CreateClickableText(UnityEngine::Transform* parent, StringW text, bool italic = true, std::function<void()> onClick = nullptr);

}