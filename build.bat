.\vendor\premake5.exe vs2022 --file=audio_diagnostics_premake.lua

.\vendor\join.exe lib\debug\shaderc_combined.lib lib\debug\shaderc_combined.lib
.\vendor\join.exe lib\release\shaderc_combined.lib lib\release\shaderc_combined.lib

.\vendor\join.exe lib\debug\libprotobuf.lib lib\debug\libprotobuf.lib
.\vendor\join.exe lib\release\libprotobuf.lib lib\release\libprotobuf.lib

.\vendor\split.exe lib\debug\Fish.lib lib\debug\Fish.lib
.\vendor\split.exe lib\release\Fish.lib lib\release\Fish.lib

PAUSE