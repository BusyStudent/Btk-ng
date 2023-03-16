#include "build.hpp"

#include <Btk/rect.hpp>
#include <optional>

//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

BTK_NS_BEGIN2(BTK_NAMESPACE::_AtlasPack)

// Atlas based on Skyline Bin Packer by Jukka Jylänki
// From nanovg
class AtlasNode {
    public:
        short x = 0;
        short y = 0;
        short width = 0;
};

// Atlas for rect packing
class Atlas {
    public:
        Atlas(int w = 0, int h = 0);
        Atlas(const Atlas &) = delete;
        Atlas(Atlas &&);
        ~Atlas();

        // Org
        void reset(int width, int height);
        void expend(int width, int height);
        bool add_rect(int w, int h, int *x, int *y);

        // Ext
        void reset(const Size &size) { reset(size.w, size.h); }
        void expend(const Size &size) { expend(size.w, size.h); }

        std::optional<Rect>
             alloc_rect(const Size &size) {
                int x, y;
                if (add_rect(size.w, size.h, &x, &y)) {
                    return Rect(x, y, size.w, size.h);
                }
                return std::nullopt;
             }
        std::optional<Rect>
             alloc_rect(int w, int h) {
                int x, y;
                if (add_rect(w, h, &x, &y)) {
                    return Rect(x, y, w, h);
                }
                return std::nullopt;
             }
    private:
        int width, height;
        AtlasNode* nodes;
        int nnodes;
        int cnodes;

        void remove_node(int index);
        bool insert_node(int index, int x,int width, int height);
        bool add_skyline(int index, int x,int y,int w,int h);
        int  rect_fits(int i, int w, int h);
};

//Atlas from nanovg
inline Atlas::Atlas(int w,int h) {
    // Allocate memory for the font stash.
    int n = 255;
    Btk_memset(this, 0, sizeof(Atlas));
    
    width = w;
    height = h;

    // Allocate space for skyline nodes
    nodes = (AtlasNode*)Btk_malloc(sizeof(AtlasNode) * n);
    Btk_memset(nodes, 0, sizeof(AtlasNode) * n);
    nnodes = 0;
    cnodes = n;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
inline Atlas::Atlas(Atlas &&a) {
    Btk_memcpy(this, &a, sizeof(Atlas));
    Btk_memset(&a, 0, sizeof(Atlas));
}
inline Atlas::~Atlas(){
    Btk_free(nodes);
}
inline bool Atlas::insert_node(int idx, int x , int y, int w) {
    int i;
    // Insert node
    if (nnodes+1 > cnodes) {
        cnodes = cnodes == 0 ? 8 : cnodes * 2;
        nodes = (AtlasNode*)Btk_realloc(nodes, sizeof(AtlasNode) * cnodes);
        if (nodes == nullptr)
            return false;
    }
    for (i = nnodes; i > idx; i--)
        nodes[i] = nodes[i-1];
    nodes[idx].x = (short)x;
    nodes[idx].y = (short)y;
    nodes[idx].width = (short)w;
    nnodes++;
    return true;
}
inline void Atlas::remove_node(int idx) {
    int i;
    if (nnodes == 0) return;
    for (i = idx; i < nnodes-1; i++)
        nodes[i] = nodes[i+1];
    nnodes--;
}
inline void Atlas::expend(int w, int h) {
    // Insert node for empty space
    if (w > width)
        insert_node(nnodes, width, 0, w - width);
    width = w;
    height = h;
}
inline void Atlas::reset(int w, int h) {
    width = w;
    height = h;
    nnodes = 0;

    // Init root node.
    nodes[0].x = 0;
    nodes[0].y = 0;
    nodes[0].width = (short)w;
    nnodes++;
}
inline bool Atlas::add_skyline(int idx, int x, int y, int w, int h) {
    int i;

    // Insert new node
    if (insert_node(idx, x, y+h, w) == 0)
        return false;

    // Delete skyline segments that fall under the shadow of the new segment.
    for (i = idx+1; i < nnodes; i++) {
        if (nodes[i].x < nodes[i-1].x + nodes[i-1].width) {
            int shrink = nodes[i-1].x + nodes[i-1].width - nodes[i].x;
            nodes[i].x += (short)shrink;
            nodes[i].width -= (short)shrink;
            if (nodes[i].width <= 0) {
                remove_node(i);
                i--;
            } else {
                break;
            }
        } else {
            break;
        }
    }

    // Merge same height skyline segments that are next to each other.
    for (i = 0; i < nnodes-1; i++) {
        if (nodes[i].y == nodes[i+1].y) {
            nodes[i].width += nodes[i+1].width;
            remove_node(i+1);
            i--;
        }
    }

    return true;
}
inline int  Atlas::rect_fits(int i, int w, int h) {
    // Checks if there is enough space at the location of skyline span 'i',
    // and return the max height of all skyline spans under that at that location,
    // (think tetris block being dropped at that position). Or -1 if no space found.
    int x = nodes[i].x;
    int y = nodes[i].y;
    int spaceLeft;
    if (x + w > width)
        return -1;
    spaceLeft = w;
    while (spaceLeft > 0) {
        if (i == nnodes) return -1;
        y = std::max<int>(y, nodes[i].y);
        if (y + h > height) return -1;
        spaceLeft -= nodes[i].width;
        ++i;
    }
    return y;
}
inline bool Atlas::add_rect(int rw, int rh, int* rx, int* ry) {

    int besth = height, bestw = width, besti = -1;
    int bestx = -1, besty = -1, i;

    // Bottom left fit heuristic.
    for (i = 0; i < nnodes; i++) {
        int y = rect_fits(i, rw, rh);
        if (y != -1) {
            if (y + rh < besth || (y + rh == besth && nodes[i].width < bestw)) {
                besti = i;
                bestw = nodes[i].width;
                besth = y + rh;
                bestx = nodes[i].x;
                besty = y;
            }
        }
    }

    if (besti == -1)
        return false;

    // Perform the actual packing.
    if (add_skyline(besti, bestx, besty, rw, rh) == 0)
        return false;

    *rx = bestx;
    *ry = besty;

    return true;
}

BTK_NS_END2(BTK_NAMESPACE::_AtlasPack)

BTK_NS_BEGIN

using RectPacker = _AtlasPack::Atlas;

BTK_NS_END