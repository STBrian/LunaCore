#include "Core/Filesystem.hpp"

#include <CTRPluginFramework.hpp>

namespace CTRPF = CTRPluginFramework;

fslib::Path path_from_string(const std::string& str) {
    std::u16string out;
    CTRPF::Utils::ConvertUTF8ToUTF16(out, str);
    return out;
}

std::string path_to_string(const std::u16string &path) {
    std::string out;
    CTRPF::Utils::ConvertUTF16ToUTF8(out, path);
    return out;
}