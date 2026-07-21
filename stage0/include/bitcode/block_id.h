#pragma once
#include <cstdint>

namespace veo::bitcode {

enum class BlockID : uint8_t { StringPool, TypePool, ModulePool, Symbols };

}
