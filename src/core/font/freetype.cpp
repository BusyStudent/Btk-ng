#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/painter.hpp>

BTK_PRIV_BEGIN

class PhyFontSource : public Refable<PhyFontSource> {
    public:
        PhyFontSource(const void *buffer, size_t n);
        PhyFontSource(u8string path);
        ~PhyFontSource();
    private:
        u8string path; //< Path
        void *buffer = nullptr;
        size_t buffer_size = 0;
};

PhyFontSource::~PhyFontSource() {
    if (!path.empty()) {
        // Free If
    }
}

BTK_PRIV_END