#pragma once
#include <string>

struct ID3D11ShaderResourceView;
struct ID3D11Device;

bool LoadTextureFromFile(const std::string& Path, ID3D11Device* Device,
                         ID3D11ShaderResourceView** OutputResource, int* OutputWidth = nullptr,
                         int* OutputHeight = nullptr);

std::string FindIconPath(const std::string& File);
