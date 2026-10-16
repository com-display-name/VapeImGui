#include "texture.h"
#include <Windows.h>
#include <d3d11.h>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/imgui/stb_image.h"

bool LoadTextureFromFile(const std::string& path, ID3D11Device* device,
                         ID3D11ShaderResourceView** outSrv, int* outW, int* outH) {
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data || !device || !outSrv)
        return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)w;
    desc.Height = (UINT)h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = data;
    sub.SysMemPitch = (UINT)(w * 4);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
    stbi_image_free(data);
    if (FAILED(hr) || !tex)
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(tex, &srvDesc, outSrv);
    tex->Release();
    if (FAILED(hr))
        return false;

    if (outW) *outW = w;
    if (outH) *outH = h;
    return true;
}

std::string FindIconPath(const std::string& file) {
    namespace fs = std::filesystem;
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    const char* candidates[] = {
        "assets/icons/",
        "../VapeUI/assets/icons/",
        "../../VapeUI/assets/icons/",
    };
    // 1) relative to CWD
    for (auto c : candidates) {
        fs::path p = fs::current_path() / c / file;
        if (fs::exists(p))
            return p.string();
    }
    // 2) relative to exe dir (walk up a few levels: handles x64/Debug etc.)
    fs::path dir = exeDir;
    for (int i = 0; i < 5; ++i) {
        fs::path p = dir / "assets" / "icons" / file;
        if (fs::exists(p))
            return p.string();
        fs::path q = dir / "VapeUI" / "assets" / "icons" / file;
        if (fs::exists(q))
            return q.string();
        dir = dir.parent_path();
    }
    // 3) absolute project path fallback (dev machine)
    fs::path abs = fs::path("C:/Users/admin/Desktop/Workspace 1/VapeUI/VapeUI/assets/icons") / file;
    if (fs::exists(abs))
        return abs.string();
    return "";
}
