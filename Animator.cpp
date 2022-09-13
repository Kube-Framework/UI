/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Animator
 */

#include <Kube/Core/Abort.hpp>

#include "Animator.hpp"

using namespace kF;

void UI::Animator::start(const Animation &animation) noexcept
{
    const auto index = findIndex(animation);
    AnimationState *state { nullptr };

    kFEnsure(animation.tickEvent,
        "UI::Animator::start: Animation must have a tick event callback");
    if (!index.success()) [[likely]] {
        state = &_states.push(AnimationState { .animation = &animation });
    } else {
        state = &_states.at(index.value());
    }
    // if (state->elapsed) // This is a restart
    state->elapsed = {};
    state->reverse = animation.reverse;
    if (animation.statusEvent)
        animation.statusEvent(AnimationStatus::Start);
}

void UI::Animator::stop(const Animation &animation) noexcept
{
    const auto index = findIndex(animation);

    if (!index.success())
        return;
    auto &state = _states.at(index.value());
    if (animation.statusEvent)
        animation.statusEvent(AnimationStatus::Stop);
    _states.erase(&state);
}

Core::Expected<std::uint32_t> UI::Animator::findIndex(const Animation &animation) const noexcept
{
    auto it = _states.find([&animation](const auto &state) { return &animation == state.animation; });

    if (it != _states.end()) [[likely]]
        return Core::Expected<std::uint32_t>(Core::Distance<std::uint32_t>(_states.begin(), it));
    else [[unlikely]]
        return Core::Expected<std::uint32_t>();
}

void UI::Animator::onTick(const std::int64_t elapsed) noexcept
{
    const auto it = std::remove_if(_states.begin(), _states.end(), [this, elapsed](auto &state) {
        const auto &animation = *state.animation;
        const auto totalElapsed = std::min(state.elapsed + elapsed, animation.duration);
        const auto ratio = static_cast<float>(totalElapsed) / static_cast<float>(animation.duration);
        const auto reversedRatio = !state.reverse ? ratio : 1.0f - ratio;
        animation.tickEvent(reversedRatio);
        if (animation.duration != totalElapsed) [[likely]] {
            state.elapsed = totalElapsed;
            return false;
        } else [[unlikely]] {
            state.elapsed = 0;
            if (animation.animationMode == AnimationMode::Bounce)
                state.reverse = !state.reverse;
            if (animation.statusEvent)
                animation.statusEvent(AnimationStatus::Finish);
            return animation.animationMode == AnimationMode::Single;
        }
    });

    if (it != _states.end()) [[unlikely]]
        _states.erase(it, _states.end());
}
