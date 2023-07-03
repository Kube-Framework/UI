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
    AnimationState *state {};
    if (!index.success()) [[likely]] {
        state = &_states.push(AnimationState { .animation = Core::TaggedPtr(&animation) });
    } else {
        state = &_states.at(index.value());
        if (animation.statusEvent)
            animation.statusEvent(AnimationStatus::Stop);
    }
    state->elapsed = {};
    state->setReverse(animation.reverse);
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
    auto it = _states.find([&animation](const auto &state) { return &animation == state.animation.get(); });

    if (it != _states.end()) [[likely]]
        return Core::Expected<std::uint32_t>(Core::Distance<std::uint32_t>(_states.begin(), it));
    else [[unlikely]]
        return Core::Expected<std::uint32_t>();
}

void UI::Animator::onTick(const std::int64_t elapsed) noexcept
{
    const auto it = std::remove_if(_states.begin(), _states.end(), [elapsed](auto &state) {
        const auto &animation = *state.animation;
        const auto reverse = animation.reverse;
        const auto duration = std::max<std::int64_t>(animation.duration, 1);
        const auto totalElapsed = std::min(state.elapsed + elapsed, duration);
        if (animation.tickEvent) {
            const auto ratio = float(double(totalElapsed) / double(duration));
            const auto reversedRatio = reverse ? 1.0f - ratio : ratio;
            animation.tickEvent(reversedRatio);
        }
        if (animation.duration != totalElapsed) [[likely]] {
            state.elapsed = totalElapsed;
            return false;
        } else [[unlikely]] {
            if (animation.animationMode == AnimationMode::Bounce)
                state.setReverse(!reverse);
            const auto oldElapsed = state.elapsed;
            if (animation.statusEvent)
                animation.statusEvent(AnimationStatus::Finish);
            const bool manuallyRestarted = state.elapsed != oldElapsed;
            state.elapsed = {};
            return animation.animationMode == AnimationMode::Single && !manuallyRestarted;
        }
    });

    if (it != _states.end()) [[unlikely]]
        _states.erase(it, _states.end());
}