/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: Item
 */

#include <Kube/Core/Assert.hpp>

#include "App.hpp"
#include "UISystem.hpp"
#include "Item.hpp"

using namespace kF;

UI::Item::~Item(void) noexcept
{
    if (!_entity) [[unlikely]]
        return;

    // Detach runtime components
    _uiSystem->dettach<TreeNode, Area, Depth>(_entity);
    if (Core::Utils::HasFlags(_componentFlags, ComponentFlags::Constraints))
        _uiSystem->dettach<Constraints>(_entity);
    if (Core::Utils::HasFlags(_componentFlags, ComponentFlags::Layout))
        _uiSystem->dettach<Layout>(_entity);
    if (Core::Utils::HasFlags(_componentFlags, ComponentFlags::PainterArea))
        _uiSystem->dettach<PainterArea>(_entity);
    if (Core::Utils::HasFlags(_componentFlags, ComponentFlags::MouseEventArea))
        _uiSystem->dettach<MouseEventArea>(_entity);
    if (Core::Utils::HasFlags(_componentFlags, ComponentFlags::KeyEventReceiver))
        _uiSystem->dettach<KeyEventReceiver>(_entity);

    // Remove the entity from UISystem
    _uiSystem->removeUnsafe(_entity);
}

UI::Item::Item(void) noexcept
    :   _uiSystem(&UI::App::Get().uiSystem()),
        _componentFlags(Core::Utils::MakeFlags(ComponentFlags::TreeNode, ComponentFlags::Area, ComponentFlags::Depth)),
        _entity(_uiSystem->add(
            TreeNode {
                .componentFlags = _componentFlags
            },
            Area {},
            Depth {}
        ))
{
}

UI::Item &UI::Item::addChild(ItemPtr &&item) noexcept
{
    item->_parent = this;
    item->get<TreeNode>().parent = _entity;
    auto &child = *_children.push(std::move(item));
    get<TreeNode>().children.push(child._entity);
    return child;
}

UI::Item &UI::Item::insertChild(const std::uint32_t index, ItemPtr &&item) noexcept
{
    item->_parent = this;
    item->get<TreeNode>().parent = _entity;
    auto &child = **_children.insert(_children.begin() + index, std::move(item));
    auto &node = get<TreeNode>();
    node.children.insert(node.children.begin() + index, child._entity);
    return child;
}

void UI::Item::removeChild(const Item * const target) noexcept
{
    const auto it = _children.find(target);

    kFAssert(it != _children.end(),
        "UI::Item::removeChild: Item '", target, "' not found inside children list");
    const auto index = std::distance(_children.begin(), it);
    _children.erase(it);
    auto &treeNode = get<TreeNode>();
    treeNode.children.erase(treeNode.children.begin() + index);
}

void UI::Item::removeChild(const std::uint32_t index) noexcept
{
    kFAssert(index < _children.size(),
        "UI::Item::removeChild: Item index '", index, "' out of range [0, ", _children.size(), '[');
    _children.erase(_children.begin() + index);
    auto &treeNode = get<TreeNode>();
    treeNode.children.erase(treeNode.children.begin() + index);
}