/*
* ©# 2026 Auteur : Patrice Waechter-Ebling
* Programme Win32 pour définir une variable d’environnement GITHUB_TOKEN
* avec un jeton d’accès personnel GitHub (PAT) stocké de manière sécurisée
* dans le registre Windows en utilisant DPAPI pour le chiffrement.
* Fonctionnalités :
    dialogue Win32 (Edit + bouton)
• 	stockage chiffré avec DPAPI dans le registre
• 	lecture/déchiffrement
• 	mise à jour de  dans l’environnement du processus
*/
#include <windows.h>
#include <wincrypt.h>
#include <string>

#pragma comment(lib, "crypt32.lib")

#define IDD_GITHUB_ENV   100
#define IDC_EDIT_TOKEN   101
#define IDC_BUTTON_SET   102
#define IDC_BUTTON_CLOSE 103
HINSTANCE hInst;

    // Encode en base64 pour stockage texte dans le registre
bool JetonProtogeDPAPI(const std::wstring& plain, std::wstring& outBase64)
{
    DATA_BLOB in = { 0 };
    DATA_BLOB out = { 0 };
    in.pbData = (BYTE*)plain.c_str();
    in.cbData = (DWORD)((plain.size() + 1) * sizeof(wchar_t));
    if (!CryptProtectData(&in, L"GITHUB_TOKEN", nullptr, nullptr, nullptr, 0, &out)) return false;
    DWORD base64Len = 0;
    if (!CryptBinaryToStringW(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &base64Len)){LocalFree(out.pbData);return false;}
    outBase64.resize(base64Len);
    if (!CryptBinaryToStringW(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,&outBase64[0], &base64Len)){LocalFree(out.pbData);return false;}
    LocalFree(out.pbData);
    return true;
}

    // Décoder base64
bool JetonNonProtogeDPAPI(const std::wstring& base64, std::wstring& outPlain)
{
    if (base64.empty()) return false;
    DATA_BLOB in = { 0 };
    DATA_BLOB out = { 0 };
    DWORD binLen = 0;
    if (!CryptStringToBinaryW(base64.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &binLen, nullptr, nullptr))return false;
    in.pbData = (BYTE*)LocalAlloc(LPTR, binLen);
    if (!in.pbData) return false;
    in.cbData = binLen;
    if (!CryptStringToBinaryW(base64.c_str(), 0, CRYPT_STRING_BASE64, in.pbData, &in.cbData, nullptr, nullptr)){LocalFree(in.pbData);return false;}
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)){LocalFree(in.pbData);return false;}
    outPlain.assign((wchar_t*)out.pbData);
    LocalFree(in.pbData);
    LocalFree(out.pbData);
    return true;
}
bool SauvegarderJetonCrypteRegistre(const std::wstring& base64)
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\GitHubEnv", 0, nullptr, 0,
        KEY_SET_VALUE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return false;

    LONG res = RegSetValueExW(hKey, L"Token", 0, REG_SZ,
        (const BYTE*)base64.c_str(),
        (DWORD)((base64.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

bool LireJetonCrypteRegistre(std::wstring& base64)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\GitHubEnv", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)return false;
    wchar_t buffer[4096];
    DWORD type = 0;
    DWORD size = sizeof(buffer);
    LONG res = RegGetValueW(hKey, nullptr, L"Token", RRF_RT_REG_SZ, &type, buffer, &size);
    RegCloseKey(hKey);
    if (res != ERROR_SUCCESS)return false;
    base64.assign(buffer);
    return true;
}
void DefenirJetonEnvironnement(const std::wstring& token){SetEnvironmentVariableW(L"GITHUB_TOKEN", token.c_str());}

INT_PTR CALLBACK ProcedureEnvironnementGitHub(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        HICON hIconBig = (HICON)LoadImageW(hInst, MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        HICON hIconSmall = (HICON)LoadImageW(hInst, MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);  
		SetWindowTextW(hDlg, L"Configurer jeton pour les outils GitHub");
        std::wstring base64, plain;
        if (LireJetonCrypteRegistre(base64) && JetonNonProtogeDPAPI(base64, plain)){SetDlgItemTextW(hDlg, IDC_EDIT_TOKEN, plain.c_str());}
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_SET:
        {
            wchar_t jeton[4096];
            GetDlgItemTextW(hDlg, IDC_EDIT_TOKEN, jeton, 4096);
            std::wstring plain = jeton;
            std::wstring base64;
            if (plain.empty()){MessageBoxW(hDlg, L"Le token est vide.", L"Erreur", MB_OK | MB_ICONERROR);return TRUE;}
            if (!JetonProtogeDPAPI(plain, base64)){MessageBoxW(hDlg, L"Echec du chiffrement DPAPI.", L"Erreur", MB_OK | MB_ICONERROR);return TRUE;}
            if (!SauvegarderJetonCrypteRegistre(base64)){MessageBoxW(hDlg, L"Échec d'écriture dans le registre.", L"Erreur", MB_OK | MB_ICONERROR);return TRUE;}
            DefenirJetonEnvironnement(plain);
            MessageBoxW(hDlg, L"GITHUB_TOKEN chiffré et enregistré.\nVariable d'environnement mise à jour.",L"Succès", MB_OK | MB_ICONINFORMATION);
            return TRUE;
        }

        case IDC_BUTTON_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return FALSE;
}
DLGTEMPLATE* GenererDialogModele()
{
    static BYTE buffer[1024];
    ZeroMemory(buffer, sizeof(buffer));
    DLGTEMPLATE* pDlg = (DLGTEMPLATE*)buffer;
    pDlg->style = DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    pDlg->cdit = 3;
    pDlg->x = 10; pDlg->y = 10; pDlg->cx = 260; pDlg->cy = 70;
	pDlg->dwExtendedStyle = WS_EX_APPWINDOW|WS_EX_DLGMODALFRAME| WS_EX_CLIENTEDGE;
    BYTE* p = (BYTE*)(pDlg + 1);
    *(WORD*)p = 0; p += sizeof(WORD); 
    *(WORD*)p = 0; p += sizeof(WORD); 

    *(WCHAR*)p = 0; p += sizeof(WCHAR); 
    p = (BYTE*)(((ULONG_PTR)p + 3) & ~3);
    {
        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)p;
        item->style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
        item->x = 10; item->y = 10; item->cx = 240; item->cy = 14;
        item->id = IDC_EDIT_TOKEN;
        p += sizeof(DLGITEMTEMPLATE);
        *(WORD*)p = 0xFFFF; p += sizeof(WORD);
        *(WORD*)p = 0x0081; p += sizeof(WORD); // EDIT
        *(WORD*)p = 0; p += sizeof(WORD);
        *(WORD*)p = 0; p += sizeof(WORD);
        p = (BYTE*)(((ULONG_PTR)p + 3) & ~3);
    }
    {
        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)p;
        item->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
        item->x = 10; item->y = 40; item->cx = 80; item->cy = 14;
        item->id = IDC_BUTTON_SET;
        p += sizeof(DLGITEMTEMPLATE);
        *(WORD*)p = 0xFFFF; p += sizeof(WORD);
        *(WORD*)p = 0x0080; p += sizeof(WORD); // BUTTON
        const wchar_t text[] = L"&Definir";
        memcpy(p, text, sizeof(text)); p += sizeof(text);
        *(WORD*)p = 0; p += sizeof(WORD);
        p = (BYTE*)(((ULONG_PTR)p + 3) & ~3);
    }
    {
        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)p;
        item->style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
        item->x = 170; item->y = 40; item->cx = 80; item->cy = 14;
        item->id = IDC_BUTTON_CLOSE;
        p += sizeof(DLGITEMTEMPLATE);
        *(WORD*)p = 0xFFFF; p += sizeof(WORD);
        *(WORD*)p = 0x0080; p += sizeof(WORD); // BUTTON
        const wchar_t text[] = L"&Fermer";
        memcpy(p, text, sizeof(text)); p += sizeof(text);
        *(WORD*)p = 0; p += sizeof(WORD);
        p = (BYTE*)(((ULONG_PTR)p + 3) & ~3);
    }
    return pDlg;
}

int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE, PWSTR, int)
{
    hInst = Instance;
    DLGTEMPLATE* pDlg = GenererDialogModele();
    DialogBoxIndirectParamW(Instance, pDlg, nullptr, ProcedureEnvironnementGitHub, 0);
    return 0;
}