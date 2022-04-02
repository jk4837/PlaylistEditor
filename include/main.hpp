#pragma once

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "modloader/shared/modloader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils-classes.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "songloader/shared/API.hpp"

static std::string DeleteLevelButtonIcon = R"(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)";

// Define these functions here so that we can easily read configuration and log information from other files
Configuration& getConfig();
