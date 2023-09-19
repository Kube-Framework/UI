#include "ProxyListModel.hpp"

template<typename ListModelType>
inline kF::UI::ProxyListModel<ListModelType>::ProxyListModel(ListModelType &listModel, Filter &&filter, Sort &&sort) noexcept
    : _listModel(listModel)
    , _listModelSlot(listModel.dispatcher().template add<&ProxyListModel::onListModelEvent>(this))
    , _filter(std::move(filter))
    , _sort(std::move(sort))
{
    applyProxy();
}

template<typename ListModelType>
inline void kF::UI::ProxyListModel<ListModelType>::applyProxy(void) noexcept
{
    // Check if filter functor is valid
    if (_filter) {
        // Clear container
        _container.clear();

        // Add filtered item into the container
        _container.reserve(_listModel.size());
        for (auto it = _listModel.begin(), end = _listModel.end(); it != end; ++it) {
            if (_filter(*it))
                _container.push(it);
        }
    // Else take all elements
    } else
        _container.resize(_listModel.begin(), _listModel.end(), [](auto &elem) { return &elem; });

    // Sort
    if (_sort)
        _container.sort([this](const Iterator &lhs, const Iterator &rhs) { return _sort(*lhs, *rhs); });

    // Dispatch event
    _dispatcher.dispatch(ListModelEvent(ListModelEvent::Resize { .count = _container.size() }));
}

template<typename ListModelType>
inline void kF::UI::ProxyListModel<ListModelType>::onListModelEvent(const ListModelEvent &event) noexcept
{
    if (event.type != ListModelEvent::Type::None)
        applyProxy();
}