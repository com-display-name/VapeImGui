// PNG -> DirectX11 texture helper (uses stb_image).
#pragma once
#include <string>

struct ID3D11ShaderResourceView;
struct ID3D11Device;

bool LoadTextureFromFile(const std::string& path, ID3D11Device* device,
                         ID3D11ShaderResourceView** outSrv, int* outW = nullptr, int* outH = nullptr);

// Searches likely locations for assets/icons/<file>.
std::string FindIconPath(const std::string& file);
