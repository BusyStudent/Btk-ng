#pragma once

#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>
#include <Btk/defs.hpp>
#include <Btk/io.hpp>

BTK_NS_BEGIN

// Interface, needed for polymorphism, mixed in with other classes
class AbstractVideoSurface {
    public:
        virtual bool begin(PixFormat *req) = 0;
        virtual bool write(PixFormat fmt, cpointer_t data, int pitch, int w, int h) = 0;
        virtual bool end() = 0;
};

class VideoWidget : public AbstractVideoSurface, public Widget {
    public:
        VideoWidget(Widget *parent);
    private:
        //
};

class MediaStream : public IOStream {
    public:

};

class MediaPlayer : public Object {

};

BTK_NS_END