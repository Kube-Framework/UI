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
    // Dettach each component if it exists
    constexpr auto DettachComponents = []<typename ...Components>(UISystem &uiSystem, const ECS::Entity entity, const ComponentFlags componentFlags,
            std::type_identity<std::tuple<Components...>>) {
        constexpr auto DettachComponent = []<typename Component>(UISystem &uiSystem, const ECS::Entity entity, const ComponentFlags componentFlags,
            std::type_identity<Component>) {
            if (Core::HasFlags(componentFlags, GetComponentFlag<Component>()))
                uiSystem.dettach<Component>(entity);
        };

        (DettachComponent(uiSystem, entity, componentFlags, std::type_identity<Components> {}), ...);
    };

    if (!_entity) [[unlikely]]
        return;

    DettachComponents(*_uiSystem, _entity, _componentFlags, std::type_identity<UISystem::ComponentsTuple> {});

    // Remove the entity from UISystem
    _uiSystem->removeUnsafe(_entity);
}

UI::Item::Item(void) noexcept
    :   _uiSystem(&UI::App::Get().uiSystem()),
        _componentFlags(Core::MakeFlags(ComponentFlags::TreeNode, ComponentFlags::Area, ComponentFlags::Depth)),
        _entity(_uiSystem->add(
            TreeNode { .componentFlags = _componentFlags },
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
    kFAssert(index <= _children.size(),
        "UI::Item::insertChild: Insert index '", index, "' out of children range [0, ", _children.size(), "]");

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

void UI::Item::removeChild(const std::uint32_t from, const std::uint32_t to) noexcept
{
    kFAssert(from < _children.size() && to <= _children.size() && from < to,
        "UI::Item::removeChild: Invalid arguments [", from, ", ", to, "] out of range [0, ", _children.size(), '[');
    _children.erase(_children.begin() + from, _children.begin() + to);
    auto &treeNode = get<TreeNode>();
    treeNode.children.erase(treeNode.children.begin() + from, treeNode.children.begin() + to);
}

void UI::Item::clearChildren(void) noexcept
{
    if (!_children.empty()) {
        _children.clear();
        auto &treeNode = get<TreeNode>();
        treeNode.children.clear();
    }
}

void UI::Item::swapChild(const std::uint32_t source, const std::uint32_t output) noexcept
{
    std::swap(_children[source], _children[output]);
    auto &treeNode = get<TreeNode>();
    std::swap(treeNode.children[source], treeNode.children[output]);
}

void UI::Item::moveChild(const std::uint32_t from_, const std::uint32_t to_, const std::uint32_t output_) noexcept
{
    kFAssert(output_ < from_ || output_ > to_,
        "UI::Item::moveChild: Invalid move range [", from_, ", ", to_, "] -> ", output_);

    auto from = from_;
    auto to = to_;
    auto output = output_;
    if (output < from) {
        const auto tmp = from;
        from = output;
        output = to;
        to = tmp;
    } else if (output)
        ++output;

    const auto it = childrenUnsafe().begin();
    std::rotate(it + from, it + to, it + output);

    const auto treeIt = get<TreeNode>().children.begin();
    std::rotate(treeIt + from, treeIt + to, treeIt + output);
}
