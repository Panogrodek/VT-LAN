USE_STEAM_GNS = false 

workspace "Audio Diagnostics"
    startproject "Audio Diagnostics"
    architecture "x64"
    configurations {
        "debug", 
        "release",
        "distribute"
    }

    filter { "platforms:Win64" }
        system "Windows"

include "Audio Diagnostics"
