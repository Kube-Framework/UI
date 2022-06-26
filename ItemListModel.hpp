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

/** @brief Create a list of item synchronized with a ListModel */
class kF::UI::ItemListModel : public UI::Item
{
public:
    /** @brief Virtual destructor */
    virtual ~ItemListModel(void) noexcept override = default;

    /** @brief Default constructor */
    ItemListModel(void) noexcept = default;

    /** @brief Model and delegate constructor
     *  @note The delegate Item type must be the first argument of the delegate functor
     *        The Item type must have one of the following constructors:
     *          - ItemType(Args...)
     *          - ItemType(Model &, Args...)
     *          - ItemType(Args..., Model &)
    */
    template<typename ListModelType, typename Delegate, typename ...Args>
    inline ItemListModel(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept
        { setup(listModel, std::forward<Delegate>(delegate), std::forward<Args>(args)...); }

    /** @brief Model and a generic delegate for ItemType constructor
     *  @note The Item type must have one of the following constructors:
     *          - ItemType(Args...)
     *          - ItemType(Model &, Args...)
     *          - ItemType(Args..., Model &)
    */
    template<typename ListModelType, typename ItemType, typename ...Args>
    inline ItemListModel(ListModelType &listModel, std::type_identity<ItemType>, Args &&...args) noexcept
        { setup<ItemType>(listModel, std::forward<Args>(args)...); }


    /** @brief Reset instance to null state */
    void reset(void) noexcept;

    /** @brief Reset instance with a new model and delegate
     *  @note The delegate Item type must be the first argument of the delegate functor
     *        The Item type must have one of the following constructors:
     *          - ItemType(Args...)
     *          - ItemType(Model &, Args...)
     *          - ItemType(Args..., Model &)
    */
    template<typename ListModelType, typename Delegate, typename ...Args>
    inline void reset(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept
        { reset(); setup(listModel, std::forward<Delegate>(delegate), std::forward<Args>(args)...); }

    /** @brief Reset instance with a new model and a generic delegate for ItemType
     *  @note The Item type must have one of the following constructors:
     *          - ItemType(Args...)
     *          - ItemType(Model &, Args...)
     *          - ItemType(Args..., Model &)
    */
    template<typename ItemType, typename ListModelType, typename ...Args>
    inline void reset(ListModelType &listModel, Args &&...args) noexcept
        { reset(); setup<ItemType>(listModel, std::forward<Args>(args)...); }


private:
    /** @brief Setup list model with a list model and a custom delegate */
    template<typename ListModelType, typename Delegate, typename ...Args>
    void setup(ListModelType &listModel, Delegate &&delegate, Args &&...args) noexcept;

    /** @brief Setup list model and a generic delegate for ItemType
     *  @note The Item type must have one of the following constructors:
     *          - ItemType(Args...)
     *          - ItemType(Model &, Args...)
     *          - ItemType(Args..., Model &)
    */
    template<typename ItemType, typename ListModelType, typename ...Args>
    void setup(ListModelType &listModel, Args &&...args) noexcept;


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