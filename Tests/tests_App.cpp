/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of App
 */

#include <gtest/gtest.h>

#include <Kube/UI/App.hpp>

using namespace kF;

TEST(App, Basics)
{
    UI::App app("AppTest");
}