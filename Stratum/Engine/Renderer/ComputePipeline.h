#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <nvrhi/nvrhi.h>
#include <glm/ext.hpp>

#include "znmsp.h"

#include "Core/Ref.h"

BEGIN_ENGINE
namespace Render {

    struct ComputePipelineDesc
    {
        std::string path;
        uint32_t permIndex;
        std::vector<nvrhi::BindingLayoutItem> BindingItems;

        ComputePipelineDesc& addBindingItem(const nvrhi::BindingLayoutItem& item) { BindingItems.push_back(item); return *this; }
    };

    class ComputePipeline
    {
    public:

        ComputePipeline(const ComputePipelineDesc& desc);
         ~ComputePipeline();

        nvrhi::ComputePipelineHandle Handle;
        nvrhi::BindingLayoutHandle BindingLayout;

        ComputePipelineDesc ShaderDesc;
    };
}
END_ENGINE