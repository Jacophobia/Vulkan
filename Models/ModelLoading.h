#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "../Rendering/Vertex.h"

namespace model_loading
{

void load_model(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string& model_path);

}
