#pragma once
#include <string>
#include <vector>

namespace veo::diagnostic {

struct Note {
    std::string              Label;
    std::vector<std::string> Elements;
};

}
