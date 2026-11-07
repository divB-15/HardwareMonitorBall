#include "theme.h"

Palette g_pal{};
float   g_scale   = 1.0f;
float   g_uiScale = 0.9f;

int PX(float v) {
    int r = (int)(v * g_scale * g_uiScale + 0.5f);
    return r < 1 ? 1 : r;
}

void LoadPalette(bool dark) {
    if (dark) {
        g_pal.bg     = RGB(0x1A, 0x1B, 0x1E);
        g_pal.card   = RGB(0x24, 0x26, 0x2A);
        g_pal.border = RGB(0x33, 0x36, 0x3B);
        g_pal.text   = RGB(0xE8, 0xEA, 0xED);
        g_pal.text2  = RGB(0x9A, 0xA0, 0xA6);
        g_pal.track  = RGB(0x2E, 0x31, 0x35);
        g_pal.red    = RGB(0xF0, 0x95, 0x95);
        g_pal.blue   = RGB(0x85, 0xB7, 0xEB);
        g_pal.green  = RGB(0x97, 0xC4, 0x59);
        g_pal.amber  = RGB(0xEF, 0x9F, 0x27);
        g_pal.teal   = RGB(0x5D, 0xCA, 0xA5);
        g_pal.violet = RGB(0xAF, 0xA9, 0xEC);
        g_pal.cyan   = RGB(0x5A, 0xCF, 0xE6);
    } else {
        g_pal.bg     = RGB(0xF5, 0xF6, 0xF8);
        g_pal.card   = RGB(0xFF, 0xFF, 0xFF);
        g_pal.border = RGB(0xE3, 0xE5, 0xE9);
        g_pal.text   = RGB(0x1A, 0x1C, 0x1F);
        g_pal.text2  = RGB(0x6B, 0x72, 0x80);
        g_pal.track  = RGB(0xED, 0xEF, 0xF2);
        g_pal.red    = RGB(0xD8, 0x3B, 0x3A);
        g_pal.blue   = RGB(0x2A, 0x6F, 0xC2);
        g_pal.green  = RGB(0x4F, 0x82, 0x18);
        g_pal.amber  = RGB(0xC1, 0x7A, 0x0E);
        g_pal.teal   = RGB(0x0E, 0x7C, 0x86);
        g_pal.violet = RGB(0x5B, 0x4F, 0xC4);
        g_pal.cyan   = RGB(0x0D, 0x6E, 0x8C);
    }
}

void LoadSystemPalette() {
    DWORD light = 1, sz = sizeof(light), type = 0;
    if (RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, &type, &light,
                     &sz) == ERROR_SUCCESS) {
        LoadPalette(light == 0);
    } else {
        LoadPalette(false);
    }
}

COLORREF Mix(COLORREF a, COLORREF b, double t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t + 0.5);
    int g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t + 0.5);
    int c = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t + 0.5);
    return RGB(r, g, c);
}
