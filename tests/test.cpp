// Import google test and the string class
#include <gtest/gtest.h>
#include <Btk/painter.hpp>
#include <Btk/context.hpp>
#include <Btk/comctl.hpp>
#include <Btk/string.hpp>
#include <Btk/pixels.hpp>
#include <Btk/rect.hpp>

// Import internal libs
#include "../src/common/utils.hpp"

using namespace BTK_NAMESPACE;

TEST(StringTest, RunIterator) {
    // Make some English / Chinese / Japanese text
    auto str = u8string("Hello World 你好世界 こんにちは");
    // Try replace some characters

    std::cout << "Sample string: " << str << std::endl;

    std::cout << u8string_view("你好世界 in ") << str.find("你好世界") << std::endl;
    ASSERT_EQ(str.find("你好世界"), 12);

    str.replace("你好世界", "'Replaced Chinese'");

    std::cout << "Replaced string: " << str << std::endl;

    str.replace("こんにちは", "'Replaced Japanese'");

    std::cout << "Replaced string: " << str << std::endl;

    auto end = str.begin() + str.length();

    std::cout << "Eq:" << (end == str.end()) << std::endl;
    ASSERT_EQ(end, str.end());

    str.replace(0, 2, "开始:");

    std::cout << str << std::endl;

    str.erase(0, 3);

    std::cout << str << std::endl;
}
TEST(StringTest, AddString) {
    ASSERT_EQ(u8string("Hello") + "World", "HelloWorld");
    ASSERT_EQ("Hello" + u8string("World"), "HelloWorld");
    ASSERT_EQ(u8string("Hello") + u8string("World"), "HelloWorld");
}
TEST(StringTest, TestCollections) {
    StringList slist;
    slist.emplace_back(u8string_view("a"));
    slist.emplace_back("B");
    slist.emplace_back(std::string("C"));
    ASSERT_TRUE(slist.contains("a"));
}

TEST(MathTest, RectUnited) {
    Rect r = Rect(0, 0, 100, 100);
    Rect r2 = Rect(50, 50, 100, 100);
    auto ret = r.united(r2);
    Rect should = Rect(0, 0, 150, 150);
    ASSERT_EQ(ret, should);

    // Float version
    FRect rf = FRect(0.0f, 0.0f, 100.0f, 100.0f);
    FRect r2f = FRect(50.0f, 50.0f, 100.0f, 100.0f);
    auto retf = rf.united(r2f);
    FRect shouldf = FRect(0.0f, 0.0f, 150.0f, 150.0f);
    ASSERT_EQ(retf, shouldf);
}
TEST(MathTest, RectItersected) {
    Rect r = Rect(0, 0, 100, 100);
    Rect r2 = Rect(50, 50, 100, 100);
    auto ret = r.intersected(r2);
    Rect should = Rect(50, 50, 50, 50);
    ASSERT_EQ(ret, should);

    // Float version
    Rect rf = Rect(0.0f, 0.0f, 100.0f, 100.0f);
    Rect r2f = Rect(50.0f, 50.0f, 100.0f, 100.0f);
    auto retf = rf.intersected(r2f);
    Rect shouldf = Rect(50.0f, 50.0f, 50.0f, 50.0f);
    ASSERT_EQ(retf, shouldf);
}
TEST(MathTest, MatrixTransform) {
    FMatrix mat;
    mat.translate(10, 10);

    // Try to transform a point
    FPoint p = FPoint(0, 0);
    auto newp = p * mat;
    ASSERT_EQ(newp, FPoint(10, 10));

    FMatrix mat2;
    mat2.scale(2, 2);

    // Try to transform a point
    FPoint p2 = FPoint(10, 10);
    auto newp2 = p2 * mat2;
    ASSERT_EQ(newp2, FPoint(20, 20)) << mat2;

    // Try combining two matrices
    auto mat3 = mat2 * mat;

    // Try to transform a point
    FPoint p3 = FPoint(0, 0);
    auto newp3 = p3 * mat3;
    ASSERT_EQ(newp3, FPoint(20, 20));
    ASSERT_EQ(mat3, mat2 * mat);

    // Invert
    FMatrix mymat;
    mymat.translate(1, 1);
    FPoint result = mymat.transform_point(Point(1, 1));

    ASSERT_EQ(result, FPoint(2, 2));

    mymat.invert();
    result = mymat.transform_point(Point(1, 1));
    ASSERT_EQ(result, FPoint(0, 0));
}
TEST(MathTest, Lerp) {
    constexpr Color c(Color::Red);
    constexpr Color c2(Color::Green);

    constexpr Color c3 = lerp(c, c2, 0.5);
}

TEST(PixBufferTest, SetColor) {
    PixBuffer buf(500, 500);

    Color cbegin(Color::Red);
    Color cend(Color::Green);

    // Begin Gradient from red to green
    float dis = std::sqrt(500 * 500 + 500 * 500);
    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < 500; j++) {
            float dist = std::sqrt(i * i + j * j);
            auto c = lerp(cbegin, cend, dist / dis);
            buf.set_color(i, j, c);
        }
    }

    UIContext ctxt;
    ImageView v;
    v.set_image(buf);
    v.set_keep_aspect_ratio(true);
    v.show();

    Widget w;
    RadioButton s(&w);
    // s.set_value(50);
    s.set_text("Hello");
    w.show();

    ctxt.run();
}
TEST(PixBufferTest, Grayscale) {
    PixBuffer buf(PixFormat::Gray8, 500, 500);
    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < 500; j++) {
            Color c;
            c.r = 0;
            c.g = 0;
            c.b = 0;
            c.a = rand() % 254 + 1;

            buf.set_color(i, j, c); 
        }
    }

    for (int i = 0; i < 500; i++) {
        for (int j = 0; j < 500; j++) {
            auto c = buf.color_at(i, j);

            ASSERT_EQ(c.r, 0);
            ASSERT_EQ(c.g, 0);
            ASSERT_EQ(c.b, 0);
        }
    }
}

TEST(RefTest, Weak) {
    // struct Data : public WeakRefable<Data> {

    // };

    // WeakRef<Data> weak;
    // {
    //     auto data = Data::New();
    //     weak = data;
    // }
    // ASSERT_EQ(weak.expired(), true);
}

TEST(PainterTest, ListFamily) {
    PainterInitializer init;

    for (auto &family : Font::ListFamily()) {
        std::cout << family << std::endl;
    }
}

TEST(PixelTest, ParseColor) {
    Color white("rgba(255, 255, 255, 1.0)");
    Color white1("rgb(255, 255, 255)");
    Color white2("#FFFFFFFF");
    Color white3("#FFFFFF");

    ASSERT_EQ(white, Color::White);
    ASSERT_EQ(white1, Color::White);
    ASSERT_EQ(white2, Color::White);
    ASSERT_EQ(white3, Color::White);
}

TEST(EventTest, TestObject) {
    UIContext ctxt;
    EventLoop loop(ctxt.dispatcher());
    Object mainobject;

    bool called = false;
    mainobject.defer_call([&]() {
        std::cout << "mainobject" << std::endl;
        called = true;

        Object subobject;
        // Test below should never be called
        subobject.defer_call([]() {
            assert(false && "Should never be called");
        });
        subobject.defer_delete();

        loop.stop();
    });

    loop.run();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}