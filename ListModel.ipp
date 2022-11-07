/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ListModel
 */

#include "ListModel.hpp"

template<typename DataType>
inline kF::UI::ListModelEvent::ListModelEvent(const DataType &data_) noexcept
{
    static_assert(std::is_same_v<DataType, Insert>
        || std::is_same_v<DataType, Erase>
        || std::is_same_v<DataType, Update>
        || std::is_same_v<DataType, Resize>
        || std::is_same_v<DataType, Move>,
            "UI::ListModelEvent: Invalid DataType");

    if constexpr (std::is_same_v<DataType, Insert>) {
        type = Type::Insert;
        new (&insert) ListModelEvent::Insert { data_ };
    } else if constexpr (std::is_same_v<DataType, Erase>) {
        type = Type::Erase;
        new (&erase) ListModelEvent::Erase { data_ };
    } else if constexpr (std::is_same_v<DataType, Update>) {
        type = Type::Update;
        new (&update) ListModelEvent::Update { data_ };
    } else if constexpr (std::is_same_v<DataType, Resize>) {
        type = Type::Resize;
        new (&resize) ListModelEvent::Resize { data_ };
    } else if constexpr (std::is_same_v<DataType, Move>) {
        type = Type::Move;
        new (&move) ListModelEvent::Move { data_ };
    }
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::invalidate(const ConstIterator from, const ConstIterator to) noexcept
{
    const auto begin_ = cbegin();
    invalidate(static_cast<Range>(std::distance(begin_, from)), static_cast<Range>(std::distance(begin_, to)));
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::invalidate(const Range from, const Range to) noexcept
{
    _eventDispatcher.dispatch(ListModelEvent::Update {
        .from = from,
        .to = to
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<typename ...Args>
inline kF::UI::ListModel<Container, Allocator>::Type &kF::UI::ListModel<Container, Allocator>::push(Args &&...args) noexcept
{
    const auto index = _container.size();
    auto &ref = _container.push(std::forward<Args>(args)...);
    _eventDispatcher.dispatch(ListModelEvent::Insert {
        .from = index,
        .to = index + 1
    });
    return ref;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::pop(void) noexcept
{
    _container.pop();
    const auto index = _container.size();
    _eventDispatcher.dispatch(ListModelEvent::Erase {
        .from = index,
        .to = index + 1
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline kF::UI::ListModel<Container, Allocator>::Iterator kF::UI::ListModel<Container, Allocator>::insertDefault(const Iterator pos, const Range count) noexcept
{
    const auto index = static_cast<Range>(std::distance(begin(), pos));
    const auto it = _container.insertDefault(pos, count);
    _eventDispatcher.dispatch(ListModelEvent::Insert {
        .from = index,
        .to = index + count
    });
    return it;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline kF::UI::ListModel<Container, Allocator>::Iterator kF::UI::ListModel<Container, Allocator>::insertFill(const Iterator pos, const Range count, const Type &value) noexcept
{
    const auto index = static_cast<Range>(std::distance(begin(), pos));
    const auto it = _container.insertFill(pos, count, value);
    _eventDispatcher.dispatch(ListModelEvent::Insert {
        .from = index,
        .to = index + count
    });
    return it;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<std::input_iterator InputIterator>
inline kF::UI::ListModel<Container, Allocator>::Iterator kF::UI::ListModel<Container, Allocator>::insert(const Iterator pos, const InputIterator from, const InputIterator to) noexcept
{
    const auto index = static_cast<Range>(std::distance(begin(), pos));
    const auto count = static_cast<Range>(std::distance(from, to));
    const auto it = _container.insert(pos, from, to);
    _eventDispatcher.dispatch(ListModelEvent::Insert {
        .from = index,
        .to = index + count
    });
    return it;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<std::input_iterator InputIterator, typename Map>
inline kF::UI::ListModel<Container, Allocator>::Iterator kF::UI::ListModel<Container, Allocator>::insert(const Iterator pos, const InputIterator from, const InputIterator to, Map &&map) noexcept
{
    const auto index = static_cast<Range>(std::distance(begin(), pos));
    const auto count = static_cast<Range>(std::distance(from, to));
    const auto it = _container.insert(pos, from, to, std::forward<Map>(map));
    _eventDispatcher.dispatch(ListModelEvent::Insert {
        .from = index,
        .to = index + count
    });
    return it;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline kF::UI::ListModel<Container, Allocator>::Iterator kF::UI::ListModel<Container, Allocator>::erase(Iterator from, Iterator to) noexcept
{
    const auto index = static_cast<Range>(std::distance(begin(), from));
    const auto count = static_cast<Range>(std::distance(from, to));
    const auto it = _container.erase(from, to);
    _eventDispatcher.dispatch(ListModelEvent::Erase {
        .from = index,
        .to = index + count
    });
    return it;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::resize(const Range count) noexcept
    requires std::constructible_from<Type>
{
    _container.resize(count);
    _eventDispatcher.dispatch(ListModelEvent::Resize {
        .count = count
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::resize(const Range count, const Type &value) noexcept
    requires std::copy_constructible<Type>
{
    _container.resize(count, value);
    _eventDispatcher.dispatch(ListModelEvent::Resize {
        .count = count
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<typename Initializer>
void kF::UI::ListModel<Container, Allocator>::resize(const Range count, Initializer &&initializer) noexcept
{
    _container.resize(count, std::forward<Initializer>(initializer));
    _eventDispatcher.dispatch(ListModelEvent::Resize {
        .count = count
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<std::input_iterator InputIterator>
inline void kF::UI::ListModel<Container, Allocator>::resize(const InputIterator from, const InputIterator to) noexcept
{
    _container.resize(from, to);
    _eventDispatcher.dispatch(ListModelEvent::Resize {
        .count = static_cast<Range>(std::distance(from, to))
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
template<std::input_iterator InputIterator, typename Map>
inline void kF::UI::ListModel<Container, Allocator>::resize(const InputIterator from, const InputIterator to, Map &&map) noexcept
{
    _container.resize(from, to, std::forward<Map>(map));
    _eventDispatcher.dispatch(ListModelEvent::Resize {
        .count = static_cast<Range>(std::distance(from, to))
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::clear(void) noexcept
{
    const auto count = _container.size();
    _container.clear();
    _eventDispatcher.dispatch(ListModelEvent::Erase {
        .from = 0,
        .to = count
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::release(void) noexcept
{
    const auto count = _container.size();
    _container.release();
    _eventDispatcher.dispatch(ListModelEvent::Erase {
        .from = 0,
        .to = count
    });
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
inline void kF::UI::ListModel<Container, Allocator>::move(const Range from, const Range to, const Range out) noexcept
    requires (!IsSorted)
{
    const auto count = _container.size();
    _container.move(from, to, out);
    _eventDispatcher.dispatch(ListModelEvent::Move {
        .from = from,
        .to = to,
        .out = out
    });
}
