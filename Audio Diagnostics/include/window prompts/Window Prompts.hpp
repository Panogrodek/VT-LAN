#pragma once

#include <string>

//note: read the warning inside "Window Prompts.cpp"

bool WindowPromptUserFileOutput(std::string& outPath, std::string& outFileName, const std::string& extension);
bool WindowPromptUserFileInput(std::string& outPath, std::string& outFileName, const std::string& extension);
bool WindowPromptUserSelectDirectory(std::string& outDirectoryPath);
void PromptWarningSound();
