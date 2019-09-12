#pragma once

namespace psl::ecs
{
	/// \brief tag that allows you to select entities (and components) that have recently added
	/// the given component type
	///
	/// You can use this tag to listen to the event of a specific component type being added to
	/// an entity.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_add
	{};

	/// \brief tag that allows you to listen to the event of a component of the given type being
	/// removed.
	///
	/// You can use this tag to listen to the event of a component of the given type being removed
	/// to an entity.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_remove
	{};

	/// \brief specialized tag of `on_add`
	///
	/// Will filter components based on when the given combination first appears.
	/// This provides a way to not care if component X, or component Y was added last
	/// and instead only cares if they both are used in a combination for the first time.
	/// This is ideal for systems that need to do something based on the creation of certain
	/// components. Take core::ecs::components::renderable and core::ecs::components::transform
	/// as example, who, when combined, create a draw call. The system that creates them should
	/// not need to care if the transform was added first or last.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_combine
	{};

	/// \brief specialized tag of `on_remove`
	///
	/// Similarly to the `on_combine` tag, the on_break that denotes a combination group, but
	/// instead of containing all recently combined components, it instead contains all those
	/// who recently broke connection. This can be ideal to use in a system (such as a render
	/// system), to erase draw calls.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct on_break
	{};

	/// \brief tag that disallows a certain component to be present on the given entity.
	///
	/// Sometimes you want to filter on all items, except a subgroup. This tag can aid in this.
	/// For example, if you had a debug system that would log an error for all entities that are
	/// renderable, but lacked a transform, then you could use the `except` tag to denote the filter
	/// what to do as a hint.
	/// Except directives are always executed last, regardless of order in the parameter pack.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct except
	{};


	/// \brief tag allows you to filter on sets of components
	///
	/// When you just want to create a generic filtering behaviour, without getting the components
	/// themselves, then you can use this tag to mark those requested components.
	/// \warn tags do not mean this component will be present in the pack, they are considered
	/// filter directives (or specialized filter directives).
	template <typename... Ts>
	struct filter
	{};

	template <typename Pred, typename... Ts>
	struct on_condition
	{};

	template <typename Pred, typename... Ts>
	struct order_by
	{};

	
	/// \brief allows packs to exist in a partial state
	///
	/// Special tag type that signifies that a pack can be split
	/// up into smaller sub-packs by the scheduler when ticking systems
	struct partial
	{};

	/// \brief requires a pack to be whole when filled in
	///
	/// Certain packs require their data to be fully available to the system.
	/// Using this tag you can guarantee this is the case.
	struct full
	{};
} // namespace psl::ecs