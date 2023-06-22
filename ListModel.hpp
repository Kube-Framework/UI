/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ListModel
 */

#pragma once

#include <Kube/Core/RemovableDispatcher.hpp>

#include "Base.hpp"

namespace kF::UI
{
    /** @brief Describes a ListModel event */
    struct alignas_quarter_cacheline ListModelEvent
    {
        /** @brief All kinds of list model event types */
        enum class Type : std::uint32_t
        {
            None,
            Insert,
            Erase,
            Update,
            Resize,
            Move
        };

        /** @brief Insert event */
        struct Insert
        {
            std::uint32_t from {};
            std::uint32_t to {};
        };

        /** @brief Erase event */
        struct Erase
        {
            std::uint32_t from {};
            std::uint32_t to {};
        };

        /** @brief Update event */
        struct Update
        {
            std::uint32_t from {};
            std::uint32_t to {};
        };

        /** @brief Resize event */
        struct Resize
        {
            std::uint32_t count {};
        };

        /** @brief Move event */
        struct Move
        {
            std::uint32_t from {};
            std::uint32_t to {};
            std::uint32_t out {};
        };

        Type type {};
        union {
            Core::DummyType dummy {};
            Insert insert;
            Erase erase;
            Update update;
            Resize resize;
            Move move;
        };

        /** @brief Data constructor */
        template<typename DataType>
        ListModelEvent(const DataType &data_) noexcept;

        /** @brief Copy constructor */
        inline ListModelEvent(const ListModelEvent &other) noexcept = default;

        /** @brief Copy assignment */
        inline ListModelEvent &operator=(const ListModelEvent &other) noexcept = default;
    };
    static_assert_fit_quarter_cacheline(ListModelEvent);


    /** @brief Requirements of ListModel's container */
    template<typename Type> // @todo implement this
    concept ListModelContainerRequirements = true;//typeid(Type) == kF::Core::Internal::VectorDetails;


    template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator>
    class ListModel;
}

template<kF::UI::ListModelContainerRequirements Container, kF::Core::StaticAllocatorRequirements Allocator = kF::Core::DefaultStaticAllocator>
class kF::UI::ListModel
{
public:
    /** @brief Static which indicates if the container is sorted */
    static constexpr bool IsSorted = Container::IsSorted;
    static_assert(!IsSorted, "UI::ListModel doesn't support sorted containers yiet");

    /** @brief Container range type */
    using Range = decltype(std::declval<const Container &>().size());

    /** @brief Container underlying type */
    using Type = std::remove_cvref_t<decltype(std::declval<const Container &>()[0])>;

    /** @brief Container Iterator */
    using Iterator = Container::Iterator;

    /** @brief Container ConstIterator */
    using ConstIterator = Container::ConstIterator;

    /** @brief Container ReverseIterator */
    using ReverseIterator = Container::ReverseIterator;

    /** @brief Container ConstReverseIterator */
    using ConstReverseIterator = Container::ConstReverseIterator;


    /** @brief Virtual destructor */
    ~ListModel(void) noexcept = default;

    /** @brief Default constructor */
    ListModel(void) noexcept = default;

    /** @brief Move constructor */
    ListModel(ListModel &&other) noexcept = default;

    /** @brief Move constructor */
    template<typename ...Args>
    inline ListModel(Args &&...args) noexcept : _container(std::forward<Args>(args)...) {}

    /** @brief Move assignment */
    ListModel &operator=(ListModel &&other) noexcept = default;

    /** @brief Container copy assignment */
    inline ListModel &operator=(const Container &other) noexcept
        { resize(other.begin(), other.end()); return *this; }

    /** @brief Container move assignment */
    inline ListModel &operator=(Container &&other) noexcept
        { resize(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end())); return *this; }


    /** @brief Get ListModel's event dispatcher */
    [[nodiscard]] inline auto &eventDispatcher(void) const noexcept { return _eventDispatcher; }


    /** @brief Invalidate all elements that must be updated */
    inline void invalidate(void) noexcept { invalidate(0, size()); }

    /** @brief Invalidate an element that must be updated */
    inline void invalidate(const ConstIterator at) noexcept { invalidate(at, at + 1); }

    /** @brief Invalidate a range of iterators that must be updated */
    void invalidate(const ConstIterator from, const ConstIterator to) noexcept;

    /** @brief Invalidate an index that must be updated */
    inline void invalidate(const Range at) noexcept { invalidate(at, at + 1); }

    /** @brief Invalidate a range of indexes that must be updated */
    void invalidate(const Range from, const Range to) noexcept;


    /** @brief Fast empty check */
    [[nodiscard]] inline bool empty(void) const noexcept { return _container.empty(); }

    /** @brief Fast non-empty check */
    [[nodiscard]] inline operator bool(void) const noexcept { return _container.operator bool(); }

    /** @brief Base container */
    [[nodiscard]] inline operator const Container &(void) const noexcept { return _container; }

    /** @brief Base container */
    [[nodiscard]] inline const Container &container(void) const noexcept { return _container; }


    /** @brief Begin / End helpers */
    [[nodiscard]] inline Iterator begin(void) noexcept { return _container.begin(); }
    [[nodiscard]] inline Iterator end(void) noexcept { return _container.end(); }
    [[nodiscard]] inline ConstIterator begin(void) const noexcept { return _container.begin(); }
    [[nodiscard]] inline ConstIterator end(void) const noexcept { return _container.end(); }
    [[nodiscard]] inline ConstIterator cbegin(void) const noexcept { return _container.cbegin(); }
    [[nodiscard]] inline ConstIterator cend(void) const noexcept { return _container.cend(); }
    [[nodiscard]] inline ReverseIterator rbegin(void) noexcept { return _container.rbegin(); }
    [[nodiscard]] inline ReverseIterator rend(void) noexcept { return _container.rend(); }
    [[nodiscard]] inline ConstReverseIterator rbegin(void) const noexcept { return _container.rbegin(); }
    [[nodiscard]] inline ConstReverseIterator rend(void) const noexcept { return _container.rend(); }
    [[nodiscard]] inline ConstReverseIterator crbegin(void) const noexcept { return _container.crbegin(); }
    [[nodiscard]] inline ConstReverseIterator crend(void) const noexcept { return _container.crend(); }


    /** @brief Access element at positon */
    [[nodiscard]] inline Type &at(const Range pos) noexcept { return _container.at(pos); }
    [[nodiscard]] inline const Type &at(const Range pos) const noexcept { return _container.at(pos); }

    /** @brief Access element at positon */
    [[nodiscard]] inline Type &operator[](const Range pos) noexcept { return _container[pos]; }
    [[nodiscard]] inline const Type &operator[](const Range pos) const noexcept { return _container[pos]; }

    /** @brief Get first element */
    [[nodiscard]] inline Type &front(void) noexcept { return _container.front(); }
    [[nodiscard]] inline const Type &front(void) const noexcept { return _container.front(); }

    /** @brief Get last element */
    [[nodiscard]] inline Type &back(void) noexcept { return _container.back(); }
    [[nodiscard]] inline const Type &back(void) const noexcept { return _container.back(); }


    /** @brief Get container size */
    [[nodiscard]] inline Range size(void) const noexcept { return _container.size(); }

    /** @brief Get container capacity */
    [[nodiscard]] inline Range capacity(void) const noexcept { return _container.capacity(); }


    /** @brief Push an element into the vector */
    template<typename ...Args>
    Type &push(Args &&...args) noexcept;

    /** @brief Pop the last element of the vector */
    void pop(void) noexcept;


    /** @brief Insert a range of default constructed values */
    Iterator insertDefault(const Iterator pos, const Range count) noexcept;

    /** @brief Insert a range of copies */
    Iterator insertFill(const Iterator pos, const Range count, const Type &value) noexcept;

    /** @brief Insert a value by copy */
    inline Iterator insert(const Iterator pos, const Type &value) noexcept
        { return insert(pos, &value, &value + 1); }

    /** @brief Insert a value by move */
    inline Iterator insert(const Iterator pos, Type &&value) noexcept
        { return insert(pos, std::make_move_iterator(&value), std::make_move_iterator(&value + 1)); }

    /** @brief Insert an initializer list */
    inline Iterator insert(const Iterator pos, std::initializer_list<Type> &&init) noexcept
        { return insert(pos, init.begin(), init.end()); }

    /** @brief Insert a range of element by iterating over iterators */
    template<std::input_iterator InputIterator>
    Iterator insert(const Iterator pos, const InputIterator from, const InputIterator to) noexcept;

    /** @brief Insert a range of element by using a map function over iterators */
    template<std::input_iterator InputIterator, typename Map>
    Iterator insert(const Iterator pos, const InputIterator from, const InputIterator to, Map &&map) noexcept;

    /** @brief Insert a range of element by using a custom insert functor
     *  @note The functor must have the following signature: void(Range count, Iterator output) */
    template<typename InsertFunc>
    Iterator insertCustom(const Iterator pos, const Range count, InsertFunc &&insertFunc) noexcept;


    /** @brief Remove a range of elements */
    Iterator erase(Iterator from, Iterator to) noexcept;

    /** @brief Remove a range of elements */
    inline Iterator erase(Iterator from, const Range count) noexcept
        { return erase(from, from + count); }

    /** @brief Remove a specific element */
    inline Iterator erase(const Iterator pos) noexcept
        { return erase(pos, pos + 1); }


    /** @brief Resize the vector using default constructor to initialize each element */
    void resize(const Range count) noexcept
        requires std::constructible_from<kF::UI::ListModel<Container, Allocator>::Type>;

    /** @brief Resize the vector by copying given element */
    void resize(const Range count, const Type &value) noexcept
        requires std::copy_constructible<kF::UI::ListModel<Container, Allocator>::Type>;

    /** @brief Resize the vector by initializing each element with a functor
     *  @note The initializer functor can take an optional argument of type 'Range' as index */
    template<typename Initializer>
    void resize(const Range count, Initializer &&initializer) noexcept;

    /** @brief Resize the vector with input iterators */
    template<std::input_iterator InputIterator>
    void resize(const InputIterator from, const InputIterator to) noexcept;

    /** @brief Resize the vector using a map function with input iterators */
    template<std::input_iterator InputIterator, typename Map>
    void resize(const InputIterator from, const InputIterator to, Map &&map) noexcept;


    /** @brief Destroy all elements */
    void clear(void) noexcept;


    /** @brief Destroy all elements and release the buffer instance */
    void release(void) noexcept;


    /** @brief Reserve memory for fast emplace only if asked capacity is higher than current capacity
     *  The data is either preserved or moved  */
    void reserve(const Range capacity) noexcept { _container.reserve(capacity); }


    /** @brief Move range [from, to[ into out */
    void move(const Range from, const Range to, const Range out) noexcept;


    /** @brief Comparison operators */
    [[nodiscard]] bool operator==(const Container &other) const noexcept
        requires std::equality_comparable<Type>
        { return _container == other; }
    [[nodiscard]] inline bool operator!=(const Container &other) const noexcept
        requires std::equality_comparable<Type>
        { return !operator==(other); }

    /** @brief Find an element by comparison */
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline Iterator find(const Comparable &comparable) noexcept
        { return _container.find(comparable); }
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ConstIterator find(const Comparable &comparable) const noexcept
        { return _container.find(comparable); }

    /** @brief Find an element by comparison, using begin iterator */
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline Iterator find(const Iterator from, const Comparable &comparable) noexcept
        { return _container.find(from, comparable); }
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ConstIterator find(const ConstIterator from, const Comparable &comparable) const noexcept
        { return _container.find(from, comparable); }

    /** @brief Find an element by comparison with reverse order */
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ReverseIterator rfind(const Comparable &comparable) noexcept
        { return _container.rfind(comparable); }
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ConstReverseIterator rfind(const Comparable &comparable) const noexcept
        { return _container.rfind(comparable); }

    /** @brief Find an element by comparison with reverse order, using begin iterator */
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ReverseIterator rfind(const ReverseIterator from, const Comparable &comparable) noexcept
        { return _container.rfind(from, comparable); }
    template<typename Comparable>
        requires requires(const Type &lhs, const Comparable &rhs) { lhs == rhs; }
    [[nodiscard]] inline ConstReverseIterator rfind(const ConstReverseIterator from, const Comparable &comparable) const noexcept
        { return _container.rfind(from, comparable); }

    /** @brief Find an element with functor */
    template<typename Functor>
        requires std::invocable<Functor, Type &>
    [[nodiscard]] inline Iterator find(Functor &&functor) noexcept
        { return _container.find(std::forward<Functor>(functor)); }
    template<typename Functor>
        requires std::invocable<Functor, const Type &>
    [[nodiscard]] inline ConstIterator find(Functor &&functor) const noexcept
        { return _container.find(std::forward<Functor>(functor)); }

    /** @brief Find an element with functor, using begin iterator */
    template<typename Functor>
        requires std::invocable<Functor, Type &>
    [[nodiscard]] inline Iterator find(const Iterator from, Functor &&functor) noexcept
        { return _container.find(from, std::forward<Functor>(functor)); }
    template<typename Functor>
        requires std::invocable<Functor, const Type &>
    [[nodiscard]] inline ConstIterator find(const ConstIterator from, Functor &&functor) const noexcept
        { return _container.find(from, std::forward<Functor>(functor)); }

    /** @brief Find an element with functor with reverse order */
    template<typename Functor>
        requires std::invocable<Functor, Type &>
    [[nodiscard]] inline ReverseIterator rfind(Functor &&functor) noexcept
        { return _container.rfind(std::forward<Functor>(functor)); }
    template<typename Functor>
        requires std::invocable<Functor, const Type &>
    [[nodiscard]] inline ConstReverseIterator rfind(Functor &&functor) const noexcept
        { return _container.rfind(std::forward<Functor>(functor)); }

    /** @brief Find an element with functor with reverse order, using reversed begin iterator */
    template<typename Functor>
        requires std::invocable<Functor, Type &>
    [[nodiscard]] inline ReverseIterator rfind(const ReverseIterator from, Functor &&functor) noexcept
        { return _container.rfind(from, std::forward<Functor>(functor)); }
    template<typename Functor>
        requires std::invocable<Functor, const Type &>
    [[nodiscard]] inline ConstReverseIterator rfind(const ConstReverseIterator from, Functor &&functor) const noexcept
        { return _container.rfind(from, std::forward<Functor>(functor)); }


    /** @brief Grow internal buffer of a given minimum */
    inline void grow(const Range minimum) noexcept { _container.grow(minimum); }


    /** @brief Convert instance into an output IteratorRange */
    [[nodiscard]] inline Core::IteratorRange<Iterator> toRange(void) noexcept
        { return _container.toRange(); }

    /** @brief Convert instance into an input IteratorRange */
    [[nodiscard]] inline Core::IteratorRange<ConstIterator> toRange(void) const noexcept
        { return _container.toRange(); }


    /** @brief Get the index of an iterator */
    [[nodiscard]] inline Range indexOf(const ConstIterator pos) const noexcept
        { return _container.indexOf(pos); }

private:
    Container _container {};
    mutable Core::RemovableDispatcher<void(const ListModelEvent &), Allocator> _eventDispatcher {};
};

#include "ListModel.ipp"