#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include <string>

struct Vector3 {
    float x, y, z;
};

void NopEx(BYTE* dest, int length) {
    DWORD protect;
    VirtualProtect(dest, length, PAGE_EXECUTE_READWRITE, &protect);
    memset(dest, 0x90, length);
    VirtualProtect(dest, length, protect, &protect);
}

void PatchEx(BYTE* dest, BYTE* src, int length) {
    DWORD protect;
    VirtualProtect(dest, length, PAGE_EXECUTE_READWRITE, &protect);
    memcpy(dest, src, length);
    VirtualProtect(dest, length, protect, &protect);
}

//FindDMAAddy
uintptr_t Resolve(uintptr_t baseAddr, std::vector<unsigned int> v) {
    uintptr_t addr = baseAddr;

    for (unsigned int i = 0; i < v.size(); ++i) {
        addr = *(uintptr_t*)addr;
        addr += v[i];
    }

    return addr;
}

DWORD WINAPI HackThread(HMODULE hModule) {

    //create console
    AllocConsole();
    FILE* main;
    freopen_s(&main, "CONOUT$", "w", stdout);
    

    //get base module address and create player pointer
    uintptr_t baseModule = (uintptr_t)GetModuleHandle(L"ac_client.exe");
    uintptr_t plrPointer = *(uintptr_t*)(baseModule + 0x10f4f4);

    //some settings
    bool changed = true;
    bool infiniteAmmo = false;
    bool noRecoil = false;
    bool teleportLocationSet = false;
    Vector3 savedCoords = { 0.0, 0.0, 0.0 };

    while (true) {
        if (changed) {
            system("cls");
            std::cout << "Successfully injected \n";
            std::cout << "Game: Assault cube \n";
            std::cout << "Type: Internal\n";
            std::cout << "[NUMPAD1] Infinite ammo: " << ((infiniteAmmo) ? "Enabled" : "Disabled") << "\n";
            std::cout << "[NUMPAD2] No Recoil: " << ((noRecoil) ? "Enabled" : "Disabled") << "\n";
            std::cout << "[NUMPAD3] Set Teleporter to Coords" << "\n";
            std::string coords = "[" + std::to_string(savedCoords.x) + ", " + std::to_string(savedCoords.y) + ", " + std::to_string(savedCoords.z) + "]";
            std::cout << "[NUMPAD4] Teleport to Coords " << ((teleportLocationSet) ? coords : "(Location has not yet been set)") << "\n";
            changed = false;
        }

        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            infiniteAmmo = !infiniteAmmo;
            changed = true;
            continue;
        }

        if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
            noRecoil = !noRecoil;
            changed = true;
            continue;
        }
        if (GetAsyncKeyState(VK_NUMPAD3) & 1) {
            Vector3 pos = *(Vector3*)(plrPointer + 0x0004);
            savedCoords = pos;
            teleportLocationSet = true;
            changed = true;
            continue;
        }
        if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
            if (savedCoords.x == 0.0 && savedCoords.y == 0.0 && savedCoords.z == 0.0) {
                std::cout << "You have to set Teleporter first!" << "\n";
                continue;
            }
            *(Vector3*)(plrPointer + 0x34) = savedCoords;
            changed = true;
            continue;
        }

        //infinite ammo
        if (infiniteAmmo) {
            NopEx((BYTE*)(baseModule + 0x637E9), 2);
        }
        else {
            PatchEx((BYTE*)(baseModule + 0x637E9), (BYTE*)"\xFF\x0E", 2);
        }

        if (noRecoil)
        {
            NopEx((BYTE*)(baseModule + 0x63786), 10);
        }
        else
        {
            PatchEx((BYTE*)(baseModule + 0x63786), (BYTE*)"\x50\x8D\x4C\x24\x1C\x51\x8B\xCE\xFF\xD2", 10);
        }
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

