#include "Texture.h"
#include <Windows.h>
#include <d3d11.h>
#include <filesystem>
#include <memory>
#include <wrl/client.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/imgui/stb_image.h"

bool LoadTextureFromFile(const std::string& Path, ID3D11Device* Device,
                         ID3D11ShaderResourceView** OutputResource, int* OutputWidth, int* OutputHeight) {
    if (!Device || !OutputResource)
        return false;
    *OutputResource = nullptr;

    int Width = 0, Height = 0, Channels = 0;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> Pixels(
        stbi_load(Path.c_str(), &Width, &Height, &Channels, 4), stbi_image_free);
    if (!Pixels)
        return false;

    D3D11_TEXTURE2D_DESC Description = {};
    Description.Width = static_cast<UINT>(Width);
    Description.Height = static_cast<UINT>(Height);
    Description.MipLevels = 1;
    Description.ArraySize = 1;
    Description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;
    Description.Usage = D3D11_USAGE_DEFAULT;
    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA InitialData = {};
    InitialData.pSysMem = Pixels.get();
    InitialData.SysMemPitch = static_cast<UINT>(Width * 4);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
    if (FAILED(Device->CreateTexture2D(&Description, &InitialData, Texture.GetAddressOf())))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescription = {};
    ViewDescription.Format = Description.Format;
    ViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ViewDescription.Texture2D.MipLevels = 1;
    if (FAILED(Device->CreateShaderResourceView(Texture.Get(), &ViewDescription, OutputResource)))
        return false;

    if (OutputWidth)
        *OutputWidth = Width;
    if (OutputHeight)
        *OutputHeight = Height;
    return true;
}

std::string FindIconPath(const std::string& File) {
    namespace Filesystem = std::filesystem;
    constexpr const char* RelativeDirectories[] = {"assets/icons/", "../VapeUI/assets/icons/",
                                                   "../../VapeUI/assets/icons/"};
    for (const char* Directory : RelativeDirectories) {
        const auto Candidate = Filesystem::current_path() / Directory / File;
        if (Filesystem::exists(Candidate))
            return Candidate.string();
    }

    char ExecutablePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, ExecutablePath, MAX_PATH);
    auto Directory = Filesystem::path(ExecutablePath).parent_path();
    for (int Level = 0; Level < 5; ++Level) {
        for (const char* Relative : {"assets/icons", "VapeUI/assets/icons"}) {
            const auto Candidate = Directory / Relative / File;
            if (Filesystem::exists(Candidate))
                return Candidate.string();
        }
        Directory = Directory.parent_path();
    }

    const auto LegacyPath =
        Filesystem::path("C:/Users/admin/Desktop/Workspace 1/VapeUI/VapeUI/assets/icons") / File;
    return Filesystem::exists(LegacyPath) ? LegacyPath.string() : std::string();
}
