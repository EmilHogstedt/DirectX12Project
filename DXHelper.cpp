#include "pch.h"
#include "DXHelper.h"

Microsoft::WRL::ComPtr<ID3D12InfoQueue> DXHelper::s_pInfoQueue{ nullptr };