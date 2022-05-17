/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of Color
 */

#include <gtest/gtest.h>

#include <Kube/UI/Base.hpp>

using namespace kF;

TEST(Color, Basics)
{
    constexpr auto AssertColor = [](const auto color) {
        ASSERT_EQ(color.r, 0xFF);
        ASSERT_EQ(color.g, 0x00);
        ASSERT_EQ(color.b, 0x00);
        ASSERT_EQ(color.a, 0xFF);
    };
    constexpr UI::Color color1 = { .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF };
    UI::Color color2 = [] {
        UI::Color color;
        color.r = 0xFF;
        color.g = 0x00;
        color.b = 0x00;
        color.a = 0xFF;
        return color;
    }();

    AssertColor(color1);
    AssertColor(color2);
    ASSERT_TRUE(color1 == color2);
    ASSERT_FALSE(color1 != color2);

    UI::Color color3 {};
    ASSERT_NE(color1, color3);
    ASSERT_NE(color2, color3);
    color3 = color2;
    ASSERT_EQ(color1, color3);
    ASSERT_EQ(color2, color3);
}