/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of UI Base
 */

#include <gtest/gtest.h>

#include <Kube/UI/App.hpp>

using namespace kF;

TEST(Area, ContainsPoint)
{
    constexpr auto ContainsTest = [](const bool result, const UI::Area &area, const UI::Point &point) {
        ASSERT_EQ(area.contains(point), result);
    };

    const UI::Area area {
        { 0, 0 },
        { 100, 100 }
    };

    { // Top-Left
        ContainsTest(false, area, UI::Point { 0, -1 });
        ContainsTest(false, area, UI::Point { -1, 0 });
        ContainsTest(false, area, UI::Point { -1, -1 });
        ContainsTest(true, area, UI::Point { 0, 0 });
    }

    { // Top-Right
        ContainsTest(false, area, UI::Point { 100, -1 });
        ContainsTest(false, area, UI::Point { 101, 0 });
        ContainsTest(false, area, UI::Point { 101, -1 });
        ContainsTest(true, area, UI::Point { 100, 0 });
    }

    { // Bottom-Left
        ContainsTest(false, area, UI::Point { 0, 101 });
        ContainsTest(false, area, UI::Point { -1, 100 });
        ContainsTest(false, area, UI::Point { -1, 101 });
        ContainsTest(true, area, UI::Point { 0, 100 });
    }

    { // Bottom-Right
        ContainsTest(false, area, UI::Point { 100, 101 });
        ContainsTest(false, area, UI::Point { 101, 100 });
        ContainsTest(false, area, UI::Point { 101, 101 });
        ContainsTest(true, area, UI::Point { 100, 100 });
    }

    { // In
        ContainsTest(true, area, UI::Point { 50, 50 });
    }
}

TEST(Area, ContainsArea)
{
    constexpr auto ContainsTest = [](const bool result, const UI::Area &a1, const UI::Area &a2) {
        ASSERT_EQ(a1.contains(a2), result);
        ASSERT_EQ(a2.contains(a1), result);
    };

    const UI::Area area {
        { 0, 0 },
        { 100, 100 }
    };

    { // Top-Left
        ContainsTest(false, area, UI::Area {
            { -100, -100 },
            { 100, 100 }
        });
        ContainsTest(true, area, UI::Area {
            { -99, -99 },
            { 100, 100 }
        });
    }

    { // Top-Right
        ContainsTest(false, area, UI::Area {
            { 100, -100 },
            { 100, 100 }
        });
        ContainsTest(true, area, UI::Area {
            { 99, -99 },
            { 100, 100 }
        });
    }

    { // Bottom-Left
        ContainsTest(false, area, UI::Area {
            { -100, 100 },
            { 100, 100 }
        });
        ContainsTest(true, area, UI::Area {
            { -99, 99 },
            { 100, 100 }
        });
    }

    { // Bottom-Right
        ContainsTest(false, area, UI::Area {
            { 100, 100 },
            { 100, 100 }
        });
        ContainsTest(true, area, UI::Area {
            { 99, 99 },
            { 100, 100 }
        });
    }

    { // In
        ContainsTest(true, area, UI::Area {
            { 25, 25 },
            { 50, 50 }
        });
    }
}

TEST(Area, Clip)
{
    const UI::Area area {
        { 0, 0 },
        { 100, 100 }
    };

    { // Left clip
        const UI::Area subArea {
            { -50, 0 },
            { 100, 100 }
        };

        const auto result = UI::Area::ApplyClip(subArea, area);
        ASSERT_EQ(result.pos, UI::Point(0, 0));
        ASSERT_EQ(result.size, UI::Size(50, 100));
    }

    { // Right clip
        const UI::Area subArea {
            { 50, 0 },
            { 100, 100 }
        };

        const auto result = UI::Area::ApplyClip(subArea, area);
        ASSERT_EQ(result.pos, UI::Point(50, 0));
        ASSERT_EQ(result.size, UI::Size(50, 100));
    }

    { // Top clip
        const UI::Area subArea {
            { 0, -50 },
            { 100, 100 }
        };

        const auto result = UI::Area::ApplyClip(subArea, area);
        ASSERT_EQ(result.pos, UI::Point(0, 0));
        ASSERT_EQ(result.size, UI::Size(100, 50));
    }

    { // Bottom clip
        const UI::Area subArea {
            { 0, 50 },
            { 100, 100 }
        };

        const auto result = UI::Area::ApplyClip(subArea, area);
        ASSERT_EQ(result.pos, UI::Point(0, 50));
        ASSERT_EQ(result.size, UI::Size(100, 50));
    }
}