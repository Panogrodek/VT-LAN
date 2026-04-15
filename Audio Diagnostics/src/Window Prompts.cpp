#include "window prompts/Window Prompts.hpp"

#include <filesystem>
#include <vector>

// Warning: this version of room builder will not compile on linux distributions
// because it uses the win32 api
// this means linux users won't be able to use room builder until we update this file
// since we are both working on windows and this app is currently only for our own use
// i decided not to care for now

#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <shlobj.h>

bool WindowPromptUserFileOutput(std::string& outPath, std::string& outFileName, const std::string& extension) {
	WCHAR szFile[MAX_PATH] = { 0 };

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    std::wstring cwd = std::filesystem::current_path().wstring();

    std::wstring wExt(extension.begin(), extension.end());
	std::wstring filter;
	filter += L"Room files (*" + wExt + L")";
	filter.push_back(L'\0');
	filter += L"*" + wExt;
	filter.push_back(L'\0');
	filter += L"All Files (*.*)";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');
	filter.push_back(L'\0');

    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFilter     = filter.c_str();
    ofn.nFilterIndex    = 1;
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt     = wExt.c_str();
    ofn.lpstrInitialDir = cwd.c_str();

    if (!GetSaveFileNameW(&ofn))
        return false;

    std::wstring wpath(szFile);
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string path(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &path[0], size_needed, nullptr, nullptr);

    outPath     = path;
    outFileName = std::filesystem::path(path).filename().string();

    return true;
}

bool WindowPromptUserFileInput(std::string& outPath, std::string& outFileName, const std::string& extension) {
	WCHAR szFile[MAX_PATH] = { 0 };

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    std::wstring cwd = std::filesystem::current_path().wstring();

	std::wstring wExt(extension.begin(), extension.end());
	std::wstring filter;
	filter += L"Room files (*" + wExt + L")";
	filter.push_back(L'\0');
	filter += L"*" + wExt;
	filter.push_back(L'\0');
	filter += L"All Files (*.*)";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');
	filter.push_back(L'\0');

    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFilter     = filter.c_str();
    ofn.nFilterIndex    = 1;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrInitialDir = cwd.c_str();

    if (!GetOpenFileNameW(&ofn))
        return false;

    std::wstring wpath(szFile);
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string path(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &path[0], size_needed, nullptr, nullptr);

    outPath     = path;
    outFileName = std::filesystem::path(path).filename().string();

    return true;
}

bool WindowPromptUserSelectDirectory(std::string& outDirectoryPath) {
	HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool coInitCalled = SUCCEEDED(hrCo);

	IFileDialog* pfd = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (FAILED(hr) || pfd == nullptr) {
		if (coInitCalled) 
			CoUninitialize();

		return false;
	}

	DWORD options = 0;
	if (SUCCEEDED(pfd->GetOptions(&options))) {
		pfd->SetOptions(options | FOS_PICKFOLDERS);
	}
	else {
		CoUninitialize();
		return false;
	}

	std::filesystem::path folderPath;

	if (SUCCEEDED(pfd->Show(nullptr))) {
		IShellItem* psi = nullptr;
		if (SUCCEEDED(pfd->GetResult(&psi)) && psi != nullptr) {
			PWSTR pszPath = nullptr;
			if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath != nullptr) {
				folderPath = std::filesystem::path(pszPath);
				CoTaskMemFree(pszPath);
			}
			psi->Release();
		}
	}
	else {
		CoUninitialize();
		return false;
	}

	pfd->Release();

	if (!folderPath.empty())
		outDirectoryPath = folderPath.string();
	else {
		CoUninitialize();
		return false;
	}

	if (coInitCalled) 
		CoUninitialize();

	return true;
}

void PromptWarningSound() {
	MessageBeep(0x00000000L); // this still works only for windows.
}
