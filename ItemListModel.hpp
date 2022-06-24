/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: ItemListModel
 */

#pragma once

#include <Kube/Core/FunctionDecomposer.hpp>

#include "Item.hpp"
#include "ListModel.hpp"

namespace kF::UI
{
    class ItemListModel;
}

class kF::UI::ItemListModel : public UI::Item
{
public:
    /** @brief Virtual destructor */
    virtual ~ItemListModel(void) noexcept override = default;

    /** @brief Default constructor */
    ItemListModel(void) noexcept = default;

    /** @brief Model and delegate constructor */
    template<typename ListModelType, typename Delegate>
    inline ItemListModel(ListModelType &listModel, Delegate &&delegate) noexcept
        { setup(listModel, std::forward<Delegate>(delegate)); }


    /** @brief Reset instance to null state */
    void reset(void) noexcept;

    /** @brief Reset instance with a new model and delegate */
    template<typename ListModelType, typename Delegate>
    inline void reset(ListModelType &listModel, Delegate &&delegate) noexcept
        { reset(); setup(listModel, std::forward<Delegate>(delegate)); }


private:
    /** @brief Implementation of the reset function */
    template<typename ListModelType, typename Delegate>
    void setup(ListModelType &listModel, Delegate &&delegate) noexcept;


    /** @brief Callback when receiving a ListModelEvent from internal ListModel */
    void onListModelEvent(const ListModelEvent &event) noexcept;

    /** @brief Callback when receiving an insertion event from internal ListModel */
    void onInsert(const ListModelEvent::Insert &data) noexcept;

    /** @brief Callback when receiving an erase event from internal ListModel */
    void onErase(const ListModelEvent::Erase &data) noexcept;

    /** @brief Callback when receiving an update event from internal ListModel */
    void onUpdate(const ListModelEvent::Update &data) noexcept;

    /** @brief Callback when receiving a resize event from internal ListModel */
    void onResize(const ListModelEvent::Resize &data) noexcept;

    /** @brief Callback when receiving a move event from internal ListModel */
    void onMove(const ListModelEvent::Move &data) noexcept;


    /** @brief Disconnect function signature */
    using DisconnectFunc = void(*)(void * const, const std::uint32_t) noexcept;

    Core::Functor<void(ItemListModel &, void * const, const std::uint32_t)> _delegate {};
    void *_listModel {};
    DisconnectFunc _disconnect {};
    std::uint32_t _dispatcherHandle {};
    std::uint32_t _modelSize {};
};

#include "ItemListModel.ipp"