#include "build.hpp"
#include "common/utils.hpp"

#include <Btk/painter.hpp>

BTK_PRIV_BEGIN

class PhyFontSource : public Refable<PhyFontSource> {
    public:
        PhyFontSource();
    private:
        u8string path; //< Path

};


BTK_PRIV_END