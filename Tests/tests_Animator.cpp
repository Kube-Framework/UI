/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Unit tests of Animation
 */

#include <gtest/gtest.h>

#include <Kube/UI/Animator.hpp>

using namespace kF;

template<std::int64_t Duration, UI::AnimationMode Mode = UI::AnimationMode::Single, bool Reverse = false>
struct TrackedAnimation : public UI::Animation
{
    float lastRatio {};
    UI::AnimationStatus lastStatus {};
    std::uint32_t statusCount {};
    std::uint32_t expectedStatusCount {};

    TrackedAnimation(void) noexcept
        : UI::Animation {
            .duration = Duration,
            .animationMode = Mode,
            .reverse = Reverse,
            .tickEvent = [this](const float ratio) { lastRatio = ratio; },
            .statusEvent = [this](const UI::AnimationStatus status) { lastStatus = status; ++statusCount; }
        } {}

    void testRatio(const float expectedRatio) const noexcept
        { testRatioExact(expectedRatio, Reverse); }

    void testRatioReverse(const float expectedRatio) const noexcept
        { testRatioExact(expectedRatio, !Reverse); }

    void testRatioExact(const float expectedRatio, const bool reverse) const noexcept
    {
        const auto tmp = static_cast<std::int64_t>(expectedRatio * Duration);
        const auto lhs = reverse ? Duration - tmp : tmp;
        const auto rhs = static_cast<std::int64_t>(lastRatio * Duration);
        EXPECT_EQ(lhs, rhs);
    }

    void testStatus(const UI::AnimationStatus expectedStatus) noexcept
        { ++expectedStatusCount; testStatusUnchanged(expectedStatus); }

    void testStatusUnchanged(const UI::AnimationStatus expectedStatus) noexcept
        { EXPECT_EQ(expectedStatus, lastStatus); EXPECT_EQ(statusCount, expectedStatusCount); }

    void testFinish(void) noexcept
        { testFinishExact(Reverse); }

    void testFinishReverse(void) noexcept
        { testFinishExact(!Reverse); }

    void testFinishExact(const bool reverse) noexcept
        { testRatioExact(1.0f, reverse); testStatus(UI::AnimationStatus::Finish); }
};


// Utility to start an animation
constexpr auto StartAnims = [](auto &animator, auto &...animations) {
    const auto func = [&animator](auto &animation) {
        animator.start(animation);
        animation.testStatus(UI::AnimationStatus::Start);
    };
    (func(animations), ...);
};

// Utility to stop an animation
constexpr auto StopAnims = [](auto &animator, auto &...animations) {
    const auto func = [](auto &animator, auto &animation) {
        animator.stop(animation);
        animation.testStatus(UI::AnimationStatus::Stop);
    };
    (func(animator, animations), ...);
};

// Utility to generate tick noise
constexpr auto MakeNoise = [](auto &animator) {
    for (auto i = 0; i != 10; ++i)
        ASSERT_FALSE(animator.tick(static_cast<std::int64_t>(1ll) << i));
};

// Utility to assert that a tick occured
constexpr auto Tick = [](auto &animator, const std::int64_t elapsed) {
    ASSERT_TRUE(animator.tick(elapsed));
};

TEST(Animation, SingleBasics)
{
    constexpr std::int64_t TotalDuration = 1000;
    constexpr std::int64_t ThirdDuration = TotalDuration / 3;
    constexpr std::int64_t QuarterDuration = TotalDuration / 4;

    constexpr auto SingleTest = [](auto &&animation) {
        UI::Animator animator;

        // Start animation
        StartAnims(animator, animation);

        // Advance animation by third
        Tick(animator, ThirdDuration);
        animation.testRatio(1.0f / 3.0f);

        // Advance animation by third
        Tick(animator, ThirdDuration);
        animation.testRatio(2.0f / 3.0f);

        // Abort animation
        StopAnims(animator, animation);

        // Restart aborted animation
        animator.start(animation);
        animation.testStatus(UI::AnimationStatus::Start);

        // Advance animation by quarter
        Tick(animator, QuarterDuration);
        animation.testRatio(1.0f / 4.0f);

        // Advance animation by quarter
        Tick(animator, QuarterDuration);
        animation.testRatio(2.0f / 4.0f);

        // Advance animation by quarter
        Tick(animator, QuarterDuration);
        animation.testRatio(3.0f / 4.0f);

        // Finish animation
        Tick(animator, QuarterDuration);
        animation.testFinish();
    };

    // Create animation
    SingleTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Single> {});

    // Create reverse animation
    SingleTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Single, true> {});
}

TEST(Animation, RepeatBasics)
{
    constexpr std::int64_t TotalDuration = 1000;
    constexpr std::int64_t QuarterDuration = TotalDuration / 4;

    constexpr auto RepeatTest = [QuarterDuration](auto &&animation) {
        UI::Animator animator;

        // Start animation
        StartAnims(animator, animation);

        const auto iterate = [QuarterDuration](auto &animator, auto &animation) {
            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(1.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(2.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(3.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testFinish();
        };

        for (auto i = 0; i != 1000; ++i)
            iterate(animator, animation);

        // Stop animation
        StopAnims(animator, animation);
    };

    // Create animation
    RepeatTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Repeat> {});

    // Create reverse animation
    RepeatTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Repeat, true> {});
}

TEST(Animation, BounceBasics)
{
    constexpr std::int64_t TotalDuration = 1000;
    constexpr std::int64_t QuarterDuration = TotalDuration / 4;

    constexpr auto BounceTest = [QuarterDuration](auto &&animation) {
        UI::Animator animator;

        // Start animation
        StartAnims(animator, animation);

        const auto iterate = [QuarterDuration](auto &animator, auto &animation) {
            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(1.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(2.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatio(3.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testFinish();

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatioReverse(1.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatioReverse(2.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testRatioReverse(3.0f / 4.0f);

            // Advance animation by quarter
            Tick(animator, QuarterDuration);
            animation.testFinishReverse();
        };

        for (auto i = 0; i != 1000; ++i)
            iterate(animator, animation);

        // Stop animation
        StopAnims(animator, animation);
    };

    // Create animation
    BounceTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Bounce> {});

    // Create reverse animation
    BounceTest(TrackedAnimation<TotalDuration, UI::AnimationMode::Bounce, true> {});
}

TEST(Animation, Parallel)
{
    constexpr std::int64_t SlowDuration = 10000;
    constexpr std::int64_t MediumDuration = SlowDuration / 2;
    constexpr std::int64_t FastDuration = MediumDuration / 2;

    UI::Animator animator;

    // Create animation
    TrackedAnimation<SlowDuration> slow;
    TrackedAnimation<MediumDuration> medium;
    TrackedAnimation<FastDuration> fast;

    // Generate tick noise
    MakeNoise(animator);

    // Start then stop all animations
    StartAnims(animator, slow, medium, fast);
    StopAnims(animator, slow, medium, fast);

    // Generate tick noise
    MakeNoise(animator);

    // Start all animations
    StartAnims(animator, slow, medium, fast);

    // 1/4 tick
    Tick(animator, FastDuration);
    fast.testFinish();
    medium.testRatio(static_cast<float>(FastDuration) / static_cast<float>(MediumDuration));
    slow.testRatio(static_cast<float>(FastDuration) / static_cast<float>(SlowDuration));

    // 2/4 tick
    Tick(animator, FastDuration);
    fast.testStatusUnchanged(UI::AnimationStatus::Finish);
    medium.testFinish();
    slow.testRatio(static_cast<float>(MediumDuration) / static_cast<float>(SlowDuration));

    // 3/4 tick
    Tick(animator, FastDuration);
    fast.testStatusUnchanged(UI::AnimationStatus::Finish);
    medium.testStatusUnchanged(UI::AnimationStatus::Finish);
    slow.testRatio(static_cast<float>(FastDuration + MediumDuration) / static_cast<float>(SlowDuration));

    // 4/4 tick
    Tick(animator, FastDuration);
    fast.testStatusUnchanged(UI::AnimationStatus::Finish);
    medium.testStatusUnchanged(UI::AnimationStatus::Finish);
    slow.testFinish();

    // Noise tick
    MakeNoise(animator);
    fast.testStatusUnchanged(UI::AnimationStatus::Finish);
    medium.testStatusUnchanged(UI::AnimationStatus::Finish);
    slow.testStatusUnchanged(UI::AnimationStatus::Finish);
}