project "Audio Diagnostics"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    architecture "x64"

    targetdir "bin/%{cfg.buildcfg}"
    objdir    "bin/obj/%{cfg.buildcfg}"

    files {
        "**.cpp",
        "**.hpp",
        "**.h",
        "**.lua",
        "**.dll"
    }
    
    includedirs {
        "%{prj.location}/include",
        "%{wks.location}/include",
        "%{wks.location}/include/fish",
    }

    links {
        "Fish"
    }

	defines { "USE_STD_FILESYSTEM" }

	if USE_STEAM_GNS then
        defines { "USE_STEAM_GNS=1" }
		includedirs {
			"%{wks.location}/include/steamGNS"
		}
	else
		defines { 
			"USE_STEAM_GNS=0", 
			"STEAMNETWORKINGSOCKETS_OPENSOURCE",
			"STEAMNETWORKINGSOCKETS_STANDALONELIB",
			"STEAMNETWORKINGSOCKETS_STATIC_LINK"
		}
		includedirs {
			"%{wks.location}/include/standaloneGNS"
		}
	end

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "on"
		
		libdirs {
            "%{wks.location}/lib/debug"
        }

        links {
			"libboost_serialization-vc143-mt-sgd-x64-1_89",
			"libboost_wserialization-vc143-mt-sgd-x64-1_89"
        }

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "on"
		
		libdirs {
            "%{wks.location}/lib/release"
        }
		
		links {
			"libboost_serialization-vc143-mt-s-x64-1_89",
			"libboost_wserialization-vc143-mt-s-x64-1_89"
        }

    filter "configurations:distribute"
        defines { "NDEBUG", "DISTRIBUTE" }
        optimize "on"
		
		libdirs {
            "%{wks.location}/lib/release"
        }
		
		links {
			"libboost_serialization-vc143-mt-s-x64-1_89",
			"libboost_wserialization-vc143-mt-s-x64-1_89"
        }
