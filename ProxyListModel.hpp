/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ProxyListModel
 */

#pragma once

#include "ListModel.hpp"

namespace kF::UI
{
    template<typename ListModelType>
    class ProxyListModel;
}

/** @brief Proxy of a list model */
template<typename ListModelType>
class kF::UI::ProxyListModel
{
public:
    /** @brief List Model Allocator */
    using Allocator = ListModelType::Allocator;

    /** @brief List model underlying type */
    using Type = ListModelType::Type;

    /** @brief List model underlying type */
    using Range = ListModelType::Range;

    /** @brief Proxy list model iterator */
    template<typename Iterator>
    struct ProxyIterator
    {
        Iterator iterator {};

        /** @brief Comparison operators */
        [[nodiscard]] inline auto operator<=>(const ProxyIterator &other) const noexcept = default;

        /** @brief Increment / Decrement operators */
        [[nodiscard]] inline ProxyIterator &operator++(void) noexcept { ++iterator; return *this; }
        [[nodiscard]] inline ProxyIterator operator++(int) noexcept { return ProxyIterator { iterator++ }; }
        [[nodiscard]] inline ProxyIterator &operator--(void) noexcept { --iterator; return *this; }
        [[nodiscard]] inline ProxyIterator operator--(int) noexcept { return ProxyIterator { iterator-- }; }

        /** @brief Assignment binary operators */
        template<typename Range>
        [[nodiscard]] inline ProxyIterator &operator+=(const Range range) noexcept { iterator += range; return *this; }
        template<typename Range>
        [[nodiscard]] inline ProxyIterator &operator-=(const Range range) noexcept { iterator -= range; return *this; }

        /** @brief Binary operators */
        template<typename Range>
        [[nodiscard]] inline ProxyIterator operator+(const Range range) noexcept { return ProxyIterator { iterator + range }; }
        template<typename Range>
        [[nodiscard]] inline ProxyIterator operator-(const Range range) noexcept { return ProxyIterator { iterator - range }; }

        /** @brief Dereference operators */
        [[nodiscard]] inline auto *operator->(void) const noexcept { return &*iterator; }
        [[nodiscard]] inline auto &operator*(void) const noexcept { return *iterator; }
    };

    /** @brief Proxy list model iterator */
    using Iterator = ProxyIterator<typename ListModelType::Iterator>;

    /** @brief Proxy list model const iterator */
    using ConstIterator = ProxyIterator<typename ListModelType::ConstIterator>;

    /** @brief Proxy list model reverse iterator */
    using ReverseIterator = std::reverse_iterator<Iterator>;

    /** @brief Proxy list model const reverse iterator */
    using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

    /** @brief Proxy list model container */
    using Container = Core::Vector<Iterator, Allocator>;

    /** @brief Proxy list model dispatcher */
    using EventDispatcher = typename ListModelType::EventDispatcher;

    /** @brief Proxy filter function */
    using Filter = Core::Functor<bool(const Type &entry), Allocator>;

    /** @brief Proxy sort function */
    using Sort = Core::Functor<bool(const Type &lhs, const Type &rhs), Allocator>;


    /** @brief Destructor */
    ~ProxyListModel(void) noexcept = default;

    /** @brief Constructor */
    ProxyListModel(ListModelType &listModel, Filter &&filter = {}, Sort &&sort = {}) noexcept;

    /** @brief Move constructor */
    ProxyListModel(ProxyListModel &&other) noexcept = default;

    /** @brief Move assignment */
    ProxyListModel &operator=(ProxyListModel &&other) noexcept = default;


    /** @brief Get ProxyListModel's event dispatcher */
    [[nodiscard]] inline EventDispatcher &dispatcher(void) const noexcept { return _dispatcher; }


    /** @brief Set proxy filter function */
    inline void setFilter(Filter &&filter) noexcept { _filter = std::move(filter); applyProxy(); }

    /** @brief Set proxy sort function */
    inline void setSort(Sort &&sort) noexcept { _sort = std::move(sort); applyProxy(); }


    /** @brief Fast empty check */
    [[nodiscard]] inline bool empty(void) const noexcept { return _container.empty(); }

    /** @brief Fast non-empty check */
    [[nodiscard]] explicit inline operator bool(void) const noexcept { return _container.operator bool(); }

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
    [[nodiscard]] inline Type &at(const Range pos) noexcept { return *_container.at(pos); }
    [[nodiscard]] inline const Type &at(const Range pos) const noexcept { return *_container.at(pos); }

    /** @brief Access element at positon */
    [[nodiscard]] inline Type &operator[](const Range pos) noexcept { return *_container[pos]; }
    [[nodiscard]] inline const Type &operator[](const Range pos) const noexcept { return *_container[pos]; }

    /** @brief Get first element */
    [[nodiscard]] inline Type &front(void) noexcept { return *_container.front(); }
    [[nodiscard]] inline const Type &front(void) const noexcept { return *_container.front(); }

    /** @brief Get last element */
    [[nodiscard]] inline Type &back(void) noexcept { return *_container.back(); }
    [[nodiscard]] inline const Type &back(void) const noexcept { return *_container.back(); }


    /** @brief Get container size */
    [[nodiscard]] inline Range size(void) const noexcept { return _container.size(); }

    /** @brief Get container capacity */
    [[nodiscard]] inline Range capacity(void) const noexcept { return _container.capacity(); }


    /** @brief Filter and sort */
    void applyProxy(void) noexcept;

private:
    /** @brief Callback on list model event */
    void onListModelEvent(const ListModelEvent &event) noexcept;


    ListModelType &_listModel;
    mutable EventDispatcher _dispatcher {};
    Container _container {};
    Core::DispatcherSlot _listModelSlot {};
    Filter _filter {};
    Sort _sort {};
};

#include "ProxyListModel.ipp"