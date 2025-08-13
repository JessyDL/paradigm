#pragma once
#include "psl/utility/enum.hpp"

namespace psl::ecs {

/// \brief tag to indicate the ecs to preseed the pack with all entities
///
/// the `on_add` and `on_combine` filtering operations normally will only return the entities
/// that satisfy the filter for that tick, but with this tag the first invocation all entities
/// within the state will be returned making the first turn operation more like a normal
/// filter rather than a lifetime. After that the pack will behave like normal.
///
/// This is useful for systems that need to react to specific components being added, but also
/// need to handle the pre-existing set without having to temporarily register multiple systems
/// to handle that.
struct preseed_tag {};

/// \brief tag that allows you to select entities (and components) that have recently added
/// the given component type
///
/// You can use this tag to listen to the event of a specific component type being added to
/// an entity.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct on_add {};

/// \brief tag that allows you to listen to the event of a component of the given type being
/// removed.
///
/// You can use this tag to listen to the event of a component of the given type being removed
/// to an entity.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct on_remove {};

/// \brief specialized tag of `on_add`
///
/// Will filter components based on when the given combination first appears.
/// This provides a way to not care if component X, or component Y was added last
/// and instead only cares if they both are used in a combination for the first time.
/// This is ideal for systems that need to do something based on the creation of certain
/// components. Take core::ecs::components::renderable and core::ecs::components::transform
/// as example, who, when combined, create a draw call. The system that creates them should
/// not need to care if the transform was added first or last.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct on_combine {};

/// \brief specialized tag of `on_remove`
///
/// Similarly to the `on_combine` tag, the on_break that denotes a combination group, but
/// instead of containing all recently combined components, it instead contains all those
/// who recently broke connection. This can be ideal to use in a system (such as a render
/// system), to erase draw calls.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct on_break {};

/// \brief tag that disallows a certain component to be present on the given entity.
///
/// Sometimes you want to filter on all items, except a subgroup. This tag can aid in this.
/// For example, if you had a debug system that would log an error for all entities that are
/// renderable, but lacked a transform, then you could use the `except` tag to denote the filter
/// what to do as a hint.
/// Except directives are always executed last, regardless of order in the parameter pack.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct except {};


/// \brief tag allows you to filter on sets of components
///
/// When you just want to create a generic filtering behaviour, without getting the components
/// themselves, then you can use this tag to mark those requested components.
/// \warning tags do not mean this component will be present in the pack, they are considered
/// filter directives (or specialized filter directives).
template <typename... Ts>
struct filter {};

template <typename Pred, typename... Ts>
struct on_condition {};

template <typename Pred, typename... Ts>
struct order_by {};


/// \brief tag that allows you to filter based on if a restricted mutability component has been mutated
///
/// When you constrain the mutation of a component the only way to modify its data is through a mutation instruction,
/// this tag allows you to listen to those mutations. You will receive the component as its pre-mutation state,
/// and the post-mutation state.
/// Mutation instructions are applied before the normal systems tick.
/// Note though that this filterin operation bears similarities to the `on_condition` filtering operation,
/// but this is a much more performant operation with limitations that it can only be applied to components which
/// are constrained to the restricted mutability mode.
/// \warning not all components that can mutate are able to fire this filter event.
template <typename T>
struct on_mutate {};

enum class entity_relationship : std::uint8_t {
	none			= 0 << 0,	 // not useful for users as this would yield a pack with no entities
	self			= 1 << 0,
	direct_children = 1 << 1,
	direct_parent	= 1 << 2,
	siblings		= 1 << 3,
	all_parents		= 1 << 4,
	all_children	= 1 << 5,
	all_relatives	= all_parents | all_children | siblings,	// all relatives of the entity
	all_direct_relatives =
	  direct_parent | direct_children | siblings,	 // all relatives of the entity that are directly connected to it
	any = 1 << 6,									 // any relationship, this is the equivalent of a
													 // pack with all entities in it
};

enum class hierarchy_change_event : std::uint8_t {
	none		  = 0 << 0,	   // not useful for users, but useful for internal operations
	child_added	  = 1 << 0,
	child_removed = 1 << 1,
	reparented	  = 1 << 2,
	child_changed = child_added | child_removed,
	any			  = child_added | child_removed | reparented | child_changed,
};

/// \brief tag that allows you to filter based on hierarchy changes
///
/// When a reparentage operation occurs this tag allows you to filter for the entities that have been affected
/// and additionally the entities that are associated by-proxy with the hierarchy change.
/// For example if you filter on the `direct_parent` relationship you will receive all the entities that have
/// had their hierarchy changed as well as their new parent in the pack. This can be useful if the entity has
/// a component that needs to recalculate its data based on the new parent.
/// \note this is not a component filtering operation, but an entity filtering operation.
template <hierarchy_change_event Change, typename T = void>
	requires(std::is_void_v<T> || std::is_same_v<T, preseed_tag>)
struct on_hierarchy_change {};


template <entity_relationship Relationship>
struct get_relationship {};

namespace details {
	template <typename T>
	struct is_component_filtering_op_t : std::false_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<filter<Ts...>> : std::true_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<on_add<Ts...>> : std::true_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<on_remove<Ts...>> : std::true_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<on_combine<Ts...>> : std::true_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<on_break<Ts...>> : std::true_type {};

	template <typename... Ts>
	struct is_component_filtering_op_t<except<Ts...>> : std::true_type {};

	template <typename Pred, typename... Ts>
	struct is_component_filtering_op_t<on_condition<Pred, Ts...>> : std::true_type {};

	template <typename Pred, typename... Ts>
	struct is_component_filtering_op_t<order_by<Pred, Ts...>> : std::true_type {};

	template <typename T>
	struct is_component_filtering_op_t<on_mutate<T>> : std::true_type {};

	template <typename T>
	struct is_entity_filtering_op_t : std::false_type {};

	template <hierarchy_change_event Change, typename T>
	struct is_entity_filtering_op_t<on_hierarchy_change<Change, T>> : std::true_type {};

	template <entity_relationship Relationship>
	struct is_entity_filtering_op_t<get_relationship<Relationship>> : std::true_type {};
}	 // namespace details

template <typename T>
concept IsEntityFilteringOp = details::is_entity_filtering_op_t<T>::value;

template <typename T>
concept IsFilteringOp = details::is_component_filtering_op_t<T>::value || IsEntityFilteringOp<T>;

template <typename T>
concept IsNotFilteringOp = !details::is_component_filtering_op_t<T>::value && !IsEntityFilteringOp<T>;

/// \brief allows packs to exist in a partial state
///
/// Special tag type that signifies that a pack can be split
/// up into smaller sub-packs by the scheduler when ticking systems
struct partial_t {};

/// \brief requires a pack to be whole when filled in
///
/// Certain packs require their data to be fully available to the system.
/// Using this tag you can guarantee this is the case.
struct full_t {};

static constexpr full_t full_v {};
static constexpr partial_t partial_v {};

template <typename T>
concept IsPackPartial = std::is_same_v<std::remove_cvref_t<T>, psl::ecs::partial_t>;
template <typename T>
concept IsPackFull = std::is_same_v<std::remove_cvref_t<T>, psl::ecs::full_t>;
template <typename T>
concept IsPolicy = IsPackFull<T> || IsPackPartial<T>;

/// \brief Guarantees direct access to the underlying data
/// \details The underlying data is guaranteed to be accessible without indirections contiguously and sequentially
/// in memory
struct direct_t {};

/// \brief The data is not optimized for direct access
/// \details In contrast to `direct_t` the data is only accessible through indirection. Sequential elements are not
/// guaranteed to be sequential in memory, and the data is not guaranteed to be contiguous. It is possible that the
/// data does satisfy those conditions, but there won't be any guarantee, or facilities to check so.
/// This access type's advantage to `direct_t` is less bookeeping and setup required so unless your system is
/// computationally heavy (and so would benefit from memory locality guarantees), it is usually better to use this.
struct indirect_t {};

static constexpr direct_t direct {};
static constexpr indirect_t indirect {};

template <typename T>
concept IsAccessDirect = std::is_same_v<std::remove_cvref_t<T>, psl::ecs::direct_t>;
template <typename T>
concept IsAccessIndirect = std::is_same_v<std::remove_cvref_t<T>, psl::ecs::indirect_t>;
template <typename T>
concept IsAccessType = IsAccessDirect<T> || IsAccessIndirect<T>;
}	 // namespace psl::ecs

template <>
inline constexpr psl::utility::enum_ops_t psl::utility::enable_enum_ops<psl::ecs::entity_relationship> =
  psl::utility::enum_ops_t::BIT;

template <>
inline constexpr psl::utility::enum_ops_t psl::utility::enable_enum_ops<psl::ecs::hierarchy_change_event> =
  psl::utility::enum_ops_t::BIT;
