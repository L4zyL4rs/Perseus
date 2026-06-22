#pragma onc
#include <ostream>
#include "glm_config.h"


struct LightSource {
    glm::vec4 pos{};
    glm::vec4 col{};
};

inline std::ostream& operator<<(std::ostream& out, const LightSource in) {
    return out << "Light source at " << 
           in.pos.x << " " << in.pos.y << " " << in.pos.z << " " << 
           " with colour " << 
           in.col.x << " " << in.col.y << " " << in.col.z << " " << in.col.w
           << "\n";
}
