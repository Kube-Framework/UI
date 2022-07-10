/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of Item
 */

#include <gtest/gtest.h>

#include <Kube/UI/App.hpp>
#include <Kube/UI/UISystem.hpp>
#include <Kube/UI/Item.hpp>

using namespace kF;

static constexpr UI::Size DesiredWindowSizes[] {
    UI::Size { 200.0f, 200.0f },
    UI::Size { 200.0f, 400.0f },
    UI::Size { 400.0f, 200.0f }
};

TEST(Item, Fill)
{
    UI::App app("Item::Fill", UI::Point(), DesiredWindowSizes[0]);

    for (const auto desiredWindowSize : DesiredWindowSizes) {
        app.setWindowSize(desiredWindowSize);
        const auto windowSize = app.windowSize();
        auto &uiSystem = app.uiSystem();
        auto &root = uiSystem.emplaceRoot<UI::Item>();

        ASSERT_FALSE(uiSystem.tick()); // We don't have any graphics to draw

        const UI::Area expectedArea {
            .pos = UI::Point { 0.0f, 0.0f },
            .size = windowSize
        };
        ASSERT_EQ(root.get<UI::Area>(), expectedArea);
    }
}

TEST(Item, ChildrenFill)
{
    UI::App app("Item::ChildrenFill", UI::Point(), DesiredWindowSizes[0]);

    for (const auto desiredWindowSize : DesiredWindowSizes) {
        app.setWindowSize(desiredWindowSize);
        const auto windowSize = app.windowSize();
        auto &uiSystem = app.uiSystem();
        auto &root = uiSystem.emplaceRoot<UI::Item>();
        auto &child1 = root.addChild<UI::Item>();
        auto &child2 = root.addChild<UI::Item>();
        auto &child3 = root.addChild<UI::Item>();

        ASSERT_FALSE(uiSystem.tick()); // We don't have any graphics to draw

        const UI::Area expectedArea {
            .pos = UI::Point { 0.0f, 0.0f },
            .size = windowSize
        };
        ASSERT_EQ(root.get<UI::Area>(), expectedArea);
        ASSERT_EQ(child1.get<UI::Area>(), expectedArea);
        ASSERT_EQ(child2.get<UI::Area>(), expectedArea);
        ASSERT_EQ(child3.get<UI::Area>(), expectedArea);
    }
}

TEST(Item, StackChildrenFill)
{
    UI::App app("Item::StackChildrenFill", UI::Point(), DesiredWindowSizes[0]);

    for (const auto desiredWindowSize : DesiredWindowSizes) {
        app.setWindowSize(desiredWindowSize);
        const auto windowSize = app.windowSize();
        auto &uiSystem = app.uiSystem();
        auto &root = uiSystem.emplaceRoot<UI::Item>();
        auto &child1 = root.addChild<UI::Item>();
        auto &child2 = root.addChild<UI::Item>();
        auto &child3 = root.addChild<UI::Item>();

        const UI::Padding padding {
            .left = 2,
            .right = 4,
            .top = 6,
            .bottom = 8
        };
        root.attach(UI::Layout { .padding = padding });

        ASSERT_FALSE(uiSystem.tick()); // We don't have any graphics to draw

        const UI::Area expectedRootArea {
            .pos = UI::Point { 0.0f, 0.0f },
            .size = windowSize
        };
        const UI::Area expectedChildArea {
            .pos = UI::Point { padding.left, padding.top },
            .size = windowSize - UI::Size(padding.left + padding.right, padding.top + padding.bottom)
        };
        ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
        ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea);
        ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea);
        ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea);
    }
}

TEST(Item, StackChildrenAdvanced)
{
    UI::App app("Item::StackChildrenAdvanced", UI::Point(), DesiredWindowSizes[0]);

    for (const auto desiredWindowSize : DesiredWindowSizes) {
        app.setWindowSize(desiredWindowSize);
        const auto windowSize = app.windowSize();
        const auto halfWindowSize = windowSize / 2.0f;
        auto &uiSystem = app.uiSystem();
        auto &root = uiSystem.emplaceRoot<UI::Item>();
        auto &child1 = root.addChild<UI::Item>();
        auto &child2 = root.addChild<UI::Item>();
        auto &child3 = root.addChild<UI::Item>();

        const UI::Padding padding {
            .left = 2,
            .right = 4,
            .top = 6,
            .bottom = 8
        };
        root.attach(UI::Layout { .padding = padding, });
        child2.attach(UI::Constraints::Make(UI::Fill(), UI::Fixed(halfWindowSize.height)));
        child3.attach(UI::Constraints::Make(UI::Fixed(halfWindowSize.width), UI::Fill()));


        const auto changeAnchor = [&uiSystem, &root](const UI::Anchor anchor) {
            root.get<UI::Layout>().anchor = anchor;
            uiSystem.invalidate();
            ASSERT_FALSE(uiSystem.tick()); // We don't have any graphics to draw
        };

        const UI::Area expectedRootArea {
            .pos = UI::Point { 0.0f, 0.0f },
            .size = windowSize
        };
        const UI::Area expectedChildArea1 {
            .pos = UI::Point { padding.left, padding.top },
            .size = windowSize - UI::Size(padding.left + padding.right, padding.top + padding.bottom)
        };

        changeAnchor(UI::Anchor::Center);
        { // Anchor::Center
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height / 2.0f - halfWindowSize.height / 2.0f),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width / 2.0f - halfWindowSize.width / 2.0f, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::Left);
        { // Anchor::Left
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height / 2.0f - halfWindowSize.height / 2.0f),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::Right);
        { // Anchor::Right
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height / 2.0f - halfWindowSize.height / 2.0f),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width - halfWindowSize.width, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::Top);
        { // Anchor::Top
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width / 2.0f - halfWindowSize.width / 2.0f, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::Bottom);
        { // Anchor::Bottom
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height - halfWindowSize.height),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width / 2.0f - halfWindowSize.width / 2.0f, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::TopLeft);
        { // Anchor::TopLeft
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::TopRight);
        { // Anchor::TopRight
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width - halfWindowSize.width, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::BottomLeft);
        { // Anchor::BottomLeft
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height - halfWindowSize.height),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos,
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }

        changeAnchor(UI::Anchor::BottomRight);
        { // Anchor::BottomRight
            const UI::Area expectedChildArea2 {
                .pos = expectedChildArea1.pos + UI::Point(0.0f, expectedChildArea1.size.height - halfWindowSize.height),
                .size = UI::Size(expectedChildArea1.size.width, halfWindowSize.height)
            };
            const UI::Area expectedChildArea3 {
                .pos = expectedChildArea1.pos + UI::Point(expectedChildArea1.size.width - halfWindowSize.width, 0.0f),
                .size = UI::Size(halfWindowSize.width, expectedChildArea1.size.height)
            };
            ASSERT_EQ(root.get<UI::Area>(), expectedRootArea);
            ASSERT_EQ(child1.get<UI::Area>(), expectedChildArea1);
            ASSERT_EQ(child2.get<UI::Area>(), expectedChildArea2);
            ASSERT_EQ(child3.get<UI::Area>(), expectedChildArea3);
        }
    }
}

TEST(Item, FlexError)
{
    constexpr auto SetupTest = [](auto &&test, const auto &msg) {
        UI::App app("Item::Flex", UI::Point(), DesiredWindowSizes[0]);
        for (const auto desiredWindowSize : DesiredWindowSizes) {
            app.setWindowSize(desiredWindowSize);
            auto &uiSystem = app.uiSystem();
            auto &root = uiSystem.template emplaceRoot<UI::Item>();
            test(root);
            EXPECT_DEBUG_DEATH(uiSystem.tick(), msg);
        }
    };

    SetupTest([](auto &root) {
        root.template addChild<UI::Item>().attach(
            UI::Constraints::Make(UI::Hug(), UI::Fill()),
            UI::Layout { .flowType = UI::FlowType::FlexColumn }
        ).template addChild<UI::Item>().attach(UI::Constraints::Make(UI::Strict(100), UI::Strict(100)));
    }, ".*FlexColumn.*Hug.*width.*");

    SetupTest([](auto &root) {
        root.template addChild<UI::Item>().attach(
            UI::Constraints::Make(UI::Fill(), UI::Hug()),
            UI::Layout { .flowType = UI::FlowType::FlexColumn }
        ).template addChild<UI::Item>().attach(UI::Constraints::Make(UI::Strict(100), UI::Strict(100)));
    }, ".*FlexColumn.*Fill.*width.*height.*Hug.*");

    SetupTest([](auto &root) {
        root.template addChild<UI::Item>().attach(
            UI::Constraints::Make(UI::Fill(), UI::Hug()),
            UI::Layout { .flowType = UI::FlowType::FlexRow }
        ).template addChild<UI::Item>().attach(UI::Constraints::Make(UI::Strict(100), UI::Strict(100)));
    }, ".*FlexRow.*Hug.*height.*");

    SetupTest([](auto &root) {
        root.template addChild<UI::Item>().attach(
            UI::Constraints::Make(UI::Hug(), UI::Fill()),
            UI::Layout { .flowType = UI::FlowType::FlexRow }
        ).template addChild<UI::Item>().attach(UI::Constraints::Make(UI::Strict(100), UI::Strict(100)));
    }, ".*FlexRow.*Fill.*height.*width.*Hug.*");
}

TEST(Item, FlexHugBasics)
{
    constexpr auto WindowSize = 100.0f;

    UI::App app("Item::FlexBasics", UI::Point(), UI::Size(WindowSize, WindowSize));

    auto &uiSystem = app.uiSystem();
    auto &root = uiSystem.template emplaceRoot<UI::Item>();
    auto &flex = root.addChild<UI::Item>().attach(
        UI::Constraints::Make(UI::Fixed(WindowSize), UI::Hug()),
        UI::Layout { .flowType = UI::FlowType::FlexColumn }
    );
    flex.addChild<UI::Item>().attach(UI::Constraints::Make(UI::Fixed(WindowSize), UI::Fixed(WindowSize / 2)));
    flex.addChild<UI::Item>().attach(UI::Constraints::Make(UI::Fixed(WindowSize / 2), UI::Fixed(WindowSize / 2)));
    flex.addChild<UI::Item>().attach(UI::Constraints::Make(UI::Fixed(WindowSize / 2), UI::Fixed(WindowSize / 2)));

    uiSystem.tick();

    auto &area = flex.get<UI::Area>();
    ASSERT_EQ(area.pos, UI::Point(0.0f, 0.0f));
    ASSERT_EQ(area.size, UI::Size(WindowSize, WindowSize));

    ASSERT_EQ(flex.children().at(0)->get<UI::Area>(), UI::Area(UI::Point(), UI::Size(WindowSize, WindowSize / 2)));
    ASSERT_EQ(flex.children().at(1)->get<UI::Area>(), UI::Area(UI::Point(0, WindowSize / 2), UI::Size(WindowSize / 2, WindowSize / 2)));
    ASSERT_EQ(flex.children().at(2)->get<UI::Area>(), UI::Area(UI::Point(WindowSize / 2, WindowSize / 2), UI::Size(WindowSize / 2, WindowSize / 2)));
}