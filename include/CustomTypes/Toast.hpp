// Edit from SongLoader\include\CustomTypes\SongLoader.hpp and SongLoader\include\LoadingUI.hpp
#pragma once

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "codegen/include/TMPro/TextMeshProUGUI.hpp"
#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/GameObject.hpp"

DECLARE_CLASS_CODEGEN(PlaylistEditor, Toast, UnityEngine::MonoBehaviour,
public:
    static Toast* GetInstance();

    void ShowMessage(const std::string &text);

    DECLARE_CTOR(ctor);
    DECLARE_SIMPLE_DTOR();

    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, Update);

private:
    void CreateCanvas();
    void UpdateLoadingProgress(const std::string &text);
    void SetActive(bool active);
    void UpdateState();

    static Toast* Instance;

    DECLARE_INSTANCE_FIELD(bool, IsLoading);
    DECLARE_INSTANCE_FIELD(bool, HasLoaded);

    DECLARE_INSTANCE_FIELD(bool, LoadingCancelled); //TODO: Implement this

    std::string newText;
    bool needsUpdate = false;

    UnityEngine::GameObject* canvas = nullptr;
    TMPro::TextMeshProUGUI* textObject = nullptr;

    std::chrono::high_resolution_clock::time_point lastActive;
    bool isActive = false;
)