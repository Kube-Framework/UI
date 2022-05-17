/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of Components
 */

#include <memory>

#include <gtest/gtest.h>

#include <Kube/UI/Components.hpp>

using namespace kF;

TEST(Components, PainterAreaMakeStatic)
{
    constexpr int Value = 42;

    static std::atomic_size_t Counter {};

    constexpr auto Test = [](UI::Painter &, const UI::Area &, const std::unique_ptr<int> &value) {
        ++Counter;
        ASSERT_EQ(*value, Value);
    };

    UI::PainterArea painterArea;
    auto value = std::make_unique<int>(Value);
    const auto &ref = *value.get();

    // Setup painter area
    ASSERT_EQ(Counter, 0);
    painterArea = UI::PainterArea::Make<Test>(std::move(value));
    ASSERT_TRUE(painterArea.event);
    ASSERT_EQ(value.get(), nullptr);
    ASSERT_EQ(Counter, 0);

    // Process some events
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 1);
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 2);
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 3);
}

TEST(Components, PainterAreaMakeRuntime)
{
    constexpr int Value = 42;

    static std::atomic_size_t Counter {};

    auto test = [expected = std::make_unique<int>(Value)](UI::Painter &, const UI::Area &, const std::unique_ptr<int> &value) {
        ++Counter;
        ASSERT_EQ(*value, *expected);
        ASSERT_NE(value.get(), expected.get());
    };

    UI::PainterArea painterArea;
    auto value = std::make_unique<int>(Value);
    const auto &ref = *value.get();

    // Setup painter area
    ASSERT_EQ(Counter, 0);
    painterArea = UI::PainterArea::Make(std::move(test), std::move(value));
    ASSERT_TRUE(painterArea.event);
    ASSERT_EQ(value.get(), nullptr);
    ASSERT_EQ(Counter, 0);

    // Process some events
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 1);
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 2);
    painterArea.event(*static_cast<UI::Painter *>(nullptr), UI::Area());
    ASSERT_EQ(Counter, 3);
}