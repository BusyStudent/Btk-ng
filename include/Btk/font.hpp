#pragma once

#include <Btk/string.hpp>
#include <Btk/pixels.hpp>
#include <Btk/defs.hpp>

BTK_NS_BEGIN

class TextLayoutImpl;
class FontImpl;

enum class TextLayoutHandle : uint8_t {
    IDWriteFactory,
    IDWriteTextLayout
};
enum class FontHandle  : uint8_t {
    IDWriteFactory,       //< DWrite IDWriteFactory object
    IDWriteTextFormat,    //< DWrite IDWriteTextFormat object
    PangoFontDescription  //< PangoFontDescription object
};
enum class FontStyle : uint8_t {
    Normal = 0,
    Bold   = 1 << 0,
    Italic = 1 << 1,
};

BTK_FLAGS_OPERATOR(FontStyle, uint8_t);

/**
 * @brief Hit results for TextLayout
 * 
 */
class TextHitResult {
    public:
        // Properties unused in hit_test_range()
        bool   inside;
        bool   trailing;

        size_t text; //< Text position
        size_t length; //< Text length
        FRect  box; //< Bounding box
};
/**
 * @brief Line's metrics for TextLayout
 * 
 */
class TextLineMetrics {
    public:
        float height;   //< Line's height
        float baseline; //< Line's baseline
};

using TextHitResults = std::vector<TextHitResult>;
using TextHitResultList = std::vector<TextHitResult>;
using TextLineMetricsList = std::vector<TextLineMetrics>;

/**
 * @brief For analizing and drawing text
 * 
 */
class BTKAPI TextLayout {
    public:
        TextLayout();
        TextLayout(const TextLayout &);
        TextLayout(TextLayout &&);
        ~TextLayout();

        void swap(TextLayout &);

        TextLayout &operator =(TextLayout &&);
        TextLayout &operator =(const TextLayout &);
        TextLayout &operator =(std::nullptr_t);

        /**
         * @brief Set the text object
         * 
         * @param text 
         */
        void set_text(u8string_view text);
        /**
         * @brief Set the font object
         * 
         */
        void set_font(const Font &);

        /**
         * @brief Get the size of the text block
         * 
         * @return Size 
         */
        FSize  size() const;
        /**
         * @brief Get how many lines
         * 
         * @return size_t 
         */
        size_t line() const;
        /**
         * @brief Get font of this layout
         * 
         * @return Font 
         */
        Font   font() const;
        /**
         * @brief Get the text of the text block
         * 
         * @return u8string_view 
         */
        u8string_view text() const;
        /**
         * @brief Get all text outlines from it
         * 
         * @param dpi 
         * @return PainterPath 
         */
        PainterPath outline(float dpi = 96.0f) const;
        /**
         * @brief Rasterize glyphruns into grayscale image
         * 
         * @param dpi The dpi of output image
         * @param bitmaps The output pointer of glyph run image
         * @param rect The output pointer of image rect
         * @return true OK
         * @return false FAILURE
         */
        bool rasterize(float dpi, std::vector<PixBuffer> *bitmaps, std::vector<Rect> *rect) const;
        /**
         * @brief Hit test by mouse position
         * 
         * @param x logical x pos (begin at 0)
         * @param y logical y pos (begin at 0)
         * @param res The pointer to hit result
         * @return true 
         * @return false 
         */
        bool hit_test(float x, float y, TextHitResult *res = nullptr) const;
        /**
         * @brief Hit test for single char
         * 
         * @param pos The char position
         * @param trailing_hit 
         * @param x 
         * @param y 
         * @param res 
         * @return true 
         * @return false 
         */
        bool hit_test_pos(size_t pos, bool trailing_hit, float *x, float *y, TextHitResult *res = nullptr) const;
        /**
         * @brief Hit test for text block
         * 
         * @param text The text position
         * @param len The length of text
         * @param org_x The x position of the text layout
         * @param org_y The y position of the text layout
         * @param res The pointer to hit results
         * @return true 
         * @return false 
         */
        bool hit_test_range(size_t text, size_t len, float org_x, float org_y, TextHitResults *res = nullptr) const;
        /**
         * @brief Get lines's metrics
         * 
         * @param metrics Pointer to vector<TextLineMetrics>
         * @return true 
         * @return false 
         */
        bool line_metrics(TextLineMetricsList *metrics = nullptr) const;

        /**
         * @brief Get the native handle of the textlayout
         * 
         * @param what The requested handle
         * @param out The pointer to the output buffer
         * @return true Has the requested handle
         * @return false 
         */
        bool native_handle(TextLayoutHandle what, void *out) const;
    public:
        /**
         * @brief Bind the deviced depended resource
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @param resource The device resource (could not be nullptr)
         */
        void      bind_device_resource(void *key, PaintResource *resource) const;
        /**
         * @brief Get the deviced depended resources
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @return PaintResource * The resource, nullptr on not found
         */
        auto      query_device_resource(void *key) const -> PaintResource *;
    private:
        void begin_mut();

        TextLayoutImpl *priv;
    friend class Painter;
};
/**
 * @brief Logical font description
 * 
 */
class BTKAPI Font {
    public:
        /**
         * @brief Construct a new Font object
         * 
         */
        Font();
        /**
         * @brief Construct a new Font object
         * 
         * @param family The family of the font 
         * @param size The size of the font
         */
        Font(u8string_view s, float size);
        Font(const Font &);
        Font(Font &&);
        ~Font();

        void swap(Font &);

        Font &operator =(Font &&);
        Font &operator =(const Font &);
        Font &operator =(std::nullptr_t);

        /**
         * @brief Get the size of the font (in pixels)
         * 
         * @return float 
         */
        float size() const;
        /**
         * @brief Check the font is empty or not
         * 
         * @return true 
         * @return false 
         */
        bool  empty() const;
        /**
         * @brief Set the size object (in pixels)
         * 
         * @param size 
         */
        void  set_size(float size);
        /**
         * @brief Set the bold
         * 
         * @param bold true on enabled
         */
        void  set_bold(bool bold);
        /**
         * @brief Set the italic
         * 
         * @param italic true on enabled
         */
        void  set_italic(bool italic);
        /**
         * @brief Set the family object
         * 
         * @param family The family of the font
         */
        void  set_family(u8string_view family);
        /**
         * @brief Set the ptsize object
         * 
         * @param size The font size, in pt
         */
        void  set_ptsize(float size);
        /**
         * @brief Get the native handle of the font
         * 
         * @param what The requested handle
         * @param out The pointer to the output buffer
         * @return true Has the requested handle
         * @return false 
         */
        bool  native_handle(FontHandle what, void *out) const;

        static auto FromFile(u8string_view fname, float size) -> Font;
        static auto ListFamily()                              -> StringList;

        static void Init();
        static void Shutdown();
    public:
        /**
         * @brief Bind the deviced depended resource
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @param resource The device resource (could not be nullptr)
         */
        void      bind_device_resource(void *key, PaintResource *resource) const;
        /**
         * @brief Get the deviced depended resources
         * @note Is is provided for backend
         * 
         * @param key The key of the resource
         * @return PaintResource * The resource, nullptr on not found
         */
        auto      query_device_resource(void *key) const -> PaintResource *;
    private:
        void begin_mut();

        FontImpl *priv;
    friend class TextLayout;
    friend class Painter;
};


inline void Font::set_ptsize(float ptsize) {
    set_size(ptsize * 1.333);
}

BTK_NS_END