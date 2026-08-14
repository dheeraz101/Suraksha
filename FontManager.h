#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>

using namespace Gdiplus;

class FontManager {
public:
    static FontManager& GetInstance() {
        static FontManager instance;
        return instance;
    }

    void Initialize() {
        if (m_initialized) return;
        m_initialized = true;

        wchar_t exePath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(exePath, L'\\');
        std::wstring baseDir = lastSlash ? std::wstring(exePath, lastSlash + 1) : L"";

        std::vector<std::wstring> fontPaths = {
            baseDir + L"assets\\fonts\\Inter.ttf",
            baseDir + L"fonts\\Inter.ttf",
            L"assets\\fonts\\Inter.ttf",
            L"..\\assets\\fonts\\Inter.ttf"
        };

        for (const auto& path : fontPaths) {
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
                m_fontCollection.AddFontFile(path.c_str());
                AddFontResourceExW(path.c_str(), FR_PRIVATE, 0);
            }
        }
    }

    const FontFamily* GetDisplayFamily() {
        Initialize();
        static FontFamily inter(L"Inter", &m_fontCollection);
        if (inter.IsAvailable()) return &inter;

        static FontFamily sysInter(L"Inter");
        if (sysInter.IsAvailable()) return &sysInter;

        static FontFamily segoeVar(L"Segoe UI Variable Display");
        if (segoeVar.IsAvailable()) return &segoeVar;

        static FontFamily segoe(L"Segoe UI");
        return &segoe;
    }

    const FontFamily* GetTextFamily() {
        Initialize();
        static FontFamily inter(L"Inter", &m_fontCollection);
        if (inter.IsAvailable()) return &inter;

        static FontFamily sysInter(L"Inter");
        if (sysInter.IsAvailable()) return &sysInter;

        static FontFamily segoeVar(L"Segoe UI Variable Text");
        if (segoeVar.IsAvailable()) return &segoeVar;

        static FontFamily segoe(L"Segoe UI");
        return &segoe;
    }

    const FontFamily* GetHindiFamily() {
        static FontFamily nirmala(L"Nirmala UI");
        if (nirmala.IsAvailable()) return &nirmala;
        return GetTextFamily();
    }

    const FontFamily* GetJapaneseFamily() {
        static FontFamily yuGothic(L"Yu Gothic UI");
        if (yuGothic.IsAvailable()) return &yuGothic;

        static FontFamily meiryo(L"Meiryo");
        if (meiryo.IsAvailable()) return &meiryo;

        return GetTextFamily();
    }

private:
    FontManager() : m_initialized(false) {}
    bool m_initialized;
    PrivateFontCollection m_fontCollection;
};
