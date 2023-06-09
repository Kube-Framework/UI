/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of SpriteManager
 */

#include <thread>

#include <gtest/gtest.h>

#include <Kube/UI/App.hpp>
#include <Kube/UI/UISystem.hpp>
#include <Kube/UI/RectangleProcessor.hpp>

using namespace kF;

KF_DECLARE_RESOURCE_ENVIRONMENT(UITests);

constexpr std::string_view TestSpritePath = ":/UITests/Resources/Test.png";

TEST(SpriteManager, Basics)
{
    UI::App app("AppTest");

    auto &uiSystem = app.executor().getSystem<UI::UISystem>();
    uiSystem.registerPrimitive<UI::Rectangle>();

    uiSystem.emplaceRoot<UI::Item>().attach(
        UI::PainterArea::Make(
            [](UI::Painter &painter, const UI::Area &area, const UI::SpriteIndex sprite) {
                painter.draw(UI::Rectangle {
                    .area = area,
                    .spriteIndex = sprite
                });
            },
            UI::Sprite(uiSystem.spriteManager(), TestSpritePath)
        ),
        UI::Timer {
            .event = [&app] {
                app.stop();
                return true;
            }
        }
    );
    app.run();
}