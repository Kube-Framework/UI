/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Components
 */

#include "Components.hpp"

template<auto Functor, typename ...Args>
    requires std::is_invocable_v<decltype(Functor), kF::UI::Painter &, const kF::UI::Area &, std::remove_cvref_t<Args> &...>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Args &&...args) noexcept
{
    return PainterArea {
        [... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            Functor(painter, area, args...);
        }
    };
}

template<typename Functor, typename ...Args>
    requires std::is_invocable_v<Functor, kF::UI::Painter &, const kF::UI::Area &, std::remove_cvref_t<Args> &...>
inline kF::UI::PainterArea kF::UI::PainterArea::Make(Functor &&functor, Args &&...args) noexcept
{
    return PainterArea {
        [functor = std::forward<Functor>(functor), ... args = std::forward<Args>(args)](Painter &painter, const Area &area) {
            functor(painter, area, args...);
        }
    };
}
