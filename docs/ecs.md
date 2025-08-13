
# ECS
## Introduction
The ECS (Entity Component System) present in `paradigm engine`is a pure ECS design, this means that data and logic are completely separate. This aids in easily parallelizing systems, as well as keeping data in a more cache friendly way.
This by itself doesn't sound great, and more often than not creates a very confusing API, so this ECS is written with not only performance in mind, but easy of use for the end user. Because of this, the ECS takes the road of "safety over performance" as a default, and allows you to opt-in to the more performant methods.

Hopefully after this short introduction & examples you will have enough of an idea of how the ECS works. If not, you can see the `core::ecs` namespace for how the ECS is used by the library.

|  |  |
|--|--|
| `entities` | an ID of type `uint32_t` |
| `components` | are required to satisfy the requirements of [`standard_layout`](https://en.cppreference.com/w/cpp/types/is_standard_layout). Depending on the complexity of the constructors available it will either be stored as a type-aware storage (following proper lifetime operations), or an untyped byte memory backing. |
| `systems` | any invocable (lambda, function or method), that satisfies the required signature (see the *Systems* section) |

##### note:
Components can be slightly more complex than a POD type, but it is best to minimize the complexity of components. If they have don't have special requirements so that they satisfy [`trivially_destructible`](https://en.cppreference.com/w/cpp/types/is_trivially_destructible), and [`trivially_copyable`](https://en.cppreference.com/w/cpp/types/is_trivially_copyable), they can be serialized to disk trivially. Additionally some slight overhead will be avoided from having to use the typed backing storage.

## concepts
### state
 Often other ECS implementations refer to this as their "world". The `state` contains the actual `component` data, to which `entity` the `component` maps, and a list of `systems` that should be executed per invocation.
 Its most notable methods are `add_components`, `remove_components`, `create`, `destroy`, `filter`, and `tick`. There are several others, but they are mostly self explanatory and explained in the reference documentation.
 
### command_buffer
 Due to the fact that the ECS can run multithreaded, we cannot allow the `state` to be mutated during the execution of systems (it would lead to a lock-mess when creating/destroying entities). This is where the `command_buffer` comes into play.

The `command_buffer` is a dummy container that keeps the instructions you wish to execute on the `ecs::state` around to be processed later. It mimics a minimal set of methods that are available in the normal `ecs::state`. These methods are `add_components`, `remove_components`, `create`, and `destroy`. They work exactly like their `state` counterparts, but their effects will only be visible after the `command_buffer` has been processed by the `state`.
 
### pack
A pack is non-owning collection of components. Internally it can be viewed as a `tuple` of `array_views` of the contained components. It optionally can also contain the entities themselves. A `pack` in a `system` signature is an instruction for the `ecs::state` to filter all entities that satisfy the `pack`'s component list.
As example:
```cpp
auto lifetime_system = 
[](/* default arguments */, pack<const position, const rotation, entity, health> units)
{
    for(auto [pos, rot, ent, h] : units)
    {
        /* do things */
    }
};
```

### advanced filtering
By default all argument types you pass to an `ecs::pack` are filtering instructions. But sometimes you prefer not getting the data, just want to filter on the types. For this the following templated filtering types are added:
- filter
- except
- on_add
- on_remove
- on_combine
- on_break
- on_mutate
- on_condition
- order_by

**filter:** requires the component to exist on the entity, but no data will be fetched for it.

**except:** requires the component to be **non**-existent on the entity.

**on_add:** only fills in data when this component was added to the entity since the last tick. This filtering operation also supports the `preseed_tag` (see examples).

**on_remove:** reverse scenario of on_add

**on_combine:** fills in data when this combination is first created. I.e. if you were filtering `on_combine<transform, renderable>`, then on any  entity with a transform component, when a renderable component gets added, then the pack will be filled in with data of that entity. The reverse is true as well, for any entity with a renderable component, when you add a transform component to it, this filter will trigger. This filtering operation also supports the `preseed_tag` (see examples).

**on_break:** same like on_combine, but the reverse situation (i.e. when an entity that has this component combination, and you remove one, then this filter will trigger).

**on_mutate:** only fills in data when the component was mutated since the last tick. This only is available for components that have a restricted mutability behaviour (see the *Restricted Mutability / mutate_components* section).

**on_condition:** only fills in data when the component satisfies a condition. The condition is a callable that takes the component as an argument and returns a boolean. This is useful for filtering components based on their state.

**order_by:** fairly rarely you need control over the order of the entities in the pack based on the data. This operation, which is not strictly a filtering operation, allows you to do so.

Notable is that components that have been "destroyed" are actually still alive for one more tick, this is so that the **on_remove** and **on_break** functionality can still read the data and send it to the systems. Normal systems will however not get the component anymore.

```cpp
// an example of advanced filtering
//
// Say we have a render system, and we want to create/destroy draw calls when entities 
// both have the renderable component, as well as the transform component.
// We only want to get them in once, because we don't want to keep checking if we've
// already created a drawcall out of them.
// We would write the signature as follows then (we ignore the full signature for brevity):

auto create_drawcall_system =
[](/* default arguments */, pack<const renderable, on_combine<transform, renderable>> renderables)
{ 
    // the pack we received, only holds read-only renderable component
    // data. All entities are guaranteed to be only sent once, when the combination triggers.
    // note that when the combination breaks, and then is recombined later, the same entity
    // will be sent again.
    /* interface with the graphics API */ 
};

auto destroy_drawcall_system =
[](/* default arguments */, pack<renderable, on_break<transform, renderable>> removed_renderables)
{ 
    /* interface with the graphics API */ 
};
```
## entities
### creating entities
Entities can be created both inside systems, as well as outside of systems, by calling the `.create(N)` method on a `ecs::state` object (where `N` is the amount to create).
When creating entities outside of a system, they are immediately visible to all filtering operations, etc... but when creating them inside of a system invoked by the `ecs::state`, they will only be visible when the `command_buffer` for that system has been processed, which is only guaranteed to be so *by the end of the tick*.

### destroying entities
Similarly to creating, but instead of passing the amount to create to the `.destroy(N)` method of a `ecs::state` object, you instead pass which entities you wish to destroy (either singular, or wrapped in an `array_view<entity>`).
Destroying entities does ***not*** clear the memory as you call destroy, but instead clears it *at the end of  the next invocation of `.tick()` of an `ecs::state` object*. This is because there are filtering operations available on components that return true for when a component is removed or a combination is broken.
### creating components
For performance reasons it's always better to add components on many entities all at once, instead of doing them one by one. The API reflects this suggestion by only exposing a signature for a container of `entities`.

You will also notice that the signatures are templated. The expected template type is the `component` you wish to add to those entities.

Furthermore, there are 4 ways to initialize components:
	- empty (no) initialization
	- default initialization
	- based on a "template/prototype"
	- invocable factory
	
The first 3 are pretty simple, the last one is a bit more interesting. For every entity that you wish to add this component to, the factory will be invoked and expected to return an instance of the component. This can be used to inject state into components during creation, such as position data for a transform component. Examples are provided for all three, in order.

Given the setup:
```cpp
// component examples
struct health
{
    int value;
}
struct player
{
    int notoriety;
}
struct speed
{
    float value;
}
```
**Method 1**
Adds a default initialized `health` component to all 100 entities.
```cpp
    ecs::state state{};
    
    auto entities = state.create(100);
    state.add_components<health>(entities); // notice that this is the only signature
                                            // that requires the component type to be
                                            // used as a template argument.
```
**Method 2**
Initialized using a template as the default value. This template can be created ahead of time, or created in-place when calling `add_components`. This example showcases the former, a later example will showcase the latter.
```cpp
    ecs::state state{};
    
    entities = state.create(100);
    health health_tmpl{150};  // you can declare this ahead, or in_place when invoking add_components
    state.add_components(entities, health_tmpl); // the template is the default value
                                                 // for all 100 entities
```
**Method 3**
Factory based initialization: invoked once per entity you wish to add the component to.
```cpp
    ecs::state state{};
    
    entities = state.create(100);
    state.add_components(entities, [](size_t index) -> health
    {
        return health{ 5 + (std::rand() % 10) };
    });
```
**Combining methods**
You can mix an match methods!
In this example you will see, in order, a default initialization, a template initialization and a invocable based initialization for the same 100 entities, in the same invocation of `add_components`.
```cpp
    ecs::state state{};
   
    entities = state.create(100);
    state.add_components(entities, 
        ecs::empty<health>{}, // ecs::empty<T> is a special instruction that mimicks an empty component
        player{5},            // template based initialization
        [](size_t index)      // factory based
        {
            return speed{35.5f + (std::rand() % 20)};
        });
}
```
**Extra**
You can also circumvent needing to call `add_components` completely, and pass these examples into the `create` method directly. The next example will create 100 entities, and immediately add a health component to all of them. The same rules and methods apply like the previous examples, so you can also initialize them with a template or invocable!
```cpp
    ecs::state state{};
    
    auto entities = state.create<health>(100);
```

### remove_components
Removing components is quite trivial. You can either call `.remove_components<Ts...>(entities)` on a `state` to remove select component types on a range of entities, or call `.destroy(entities)` to remove all components (and return the given entities back to the pool as an orphan).
Note that removing components will keep the actual component data around till the end of the next invocation of `.tick(dTime)`. This is for systems (and filtering instructions), that operate on removed components (the `on_break` and `on_remove` filtering instructions). For this reason, you cannot remove and add the same component in the same tick as of now, but this might change in the future.

### Restricted Mutability / mutate_components

Components come in 2 varieties, the default is an 'unrestricted' component which can be modified freely by any system, and at any time — taking into account for when the component is locked by a system. There is a second variety which is a `psl::ecs::component_mutability_behaviour_t::unrestricted`. When a component is marked (at compile time) as such the only way to modify it is through the `state_t::mutate_components(...)` API.

The advantage of this is that you can now listen for the `psl::ecs::on_mutate<>` filtering instruction in a system which will only contain entities who's components have been mutated between last tick and now. This is a much cheaper filtering operation than the `on_condition<>` filtering instruction.

To have your component be marked as restricted you will need to specialize the `psl::ecs::component_trait_mutability_t<>` which can be found at [psl/ecs/component_traits.hpp](psl/inc/psl/ecs/component_traits.hpp).

Following is a small sample of how the `on_mutate` filtering in a pack behaves.
```cpp
struct audio_t {
    float volume;
    bool is_playing;
};

// pay attention that we both request the audio_t component, as well as the on_mutate filtering instruction
// if we did not, we would only get the mutation info of the audio_t component, but not the audio_t component itself.
void system(psl::ecs::info& info, psl::ecs::pack<const audio_t, psl::ecs::on_mutate<audio_t>> audio_pack) {
    for(auto [audio, mutation_info] : audio_pack)
    {
        if(mutation_info.has_mutated<&audio_t::volume>()) {
            some_audio_system.set_volume(audio.volume);
        }
    }
})

```

### Hierarchy / Entity Relations

Entities can have relations to other entities. This is useful for creating parent-child relationships, such as a transform hierarchy in a 3D scene graph. These relations can be filtered on, or queried from the `ecs::state_t` directly. Do note that relations have no direct impact on the component data, this is something the user will need to handle. For example if you wished to have a parent-child relationship between transforms, you would need to filter on the `transform` component and then query the relations to find the parent-child relationships, make (ideally) a new transform-like component that would represent the world transform of the entity, and then update that component based on the parent-child relationship (and changes). An example of this is present, see `core::ecs::components::local_transform_t` & `core::ecs::components::world_transform_t`. The key part of this is that the local transform is mutable, but the world transform is not, and is updated based on the local transforms of the parent entities.

Filtering based on relation changes can be done through the `on_hierarchy_change<hierarchy_change_event>` where the `hierarchy_change_event` is one of the following:
- `child_added`: when the given entity became the parent of atleast one new entity.
- `child_removed`: when the given entity lost a child entity.
- `reparented`: when the given entity's parent has changed, which includes being parented for the first time or to _nothing_ (if it had a parent originally).
- `child_changed`: meta event based on any child related changes.
- `any`: meta event that triggers on any observed hierarchy change.

Additionally you can add the `preseed_tag` as the second argument to `on_hierarchy_change`. It will then also include entities that satisfy the filtering instruction but were not changed since the last tick. The only exception to this is the `child_removed` filter as there's no way to know.

Entity relations can be filtered using `get_relationship<entity_relationship>` where the `entity_relationship` indicates the type of data you wish to filter on, this can be one of the following:
- `self`: include the entity itself in the pack (elsewise it will be excluded).
- `direct_children`: only the direct children of the entity will be returned.
- `all_children`: all children of the entity, including indirect children, will be returned.
- `direct_parent`: only the direct parent of the entity will be returned.
- `all_parents`: all parents of the entity, including indirect parents, will be returned.
- `siblings`: only the siblings of the entity will be returned.

Both of these are flag-type enum values, so you can mix and match them as you see fit.

Lastly you can get the entity's specific relationship data by filtering on `const entity_relationship_data_t` in your pack. This is an immutable component managed by the state where you can get parent, children, and sibling relationships of the current entity.

As a-not-so-short example consider the following:
```cpp
pack<
    entity_t, 
    const local_position,
    world_position,
    const entity_relationship_data_t,
    on_hierarchy_change<hierarchy_change_event::child_added>,
    get_relationship<entity_relationship::self | entity_relationship::all_children>>
        example_pack;

// we use this as an early-out to avoid processing the same entity multiple times
std::unordered_set<entity_t> already_parsed_entities {};

auto calculate_new_wpos_helper = [&example_pack, &already_parsed_entities](
        entity_t entity,
        local_position const& local_pos,
        world_position& world_pos,
        entity_relationship_data_t const& relationship,
        auto& fn) {

    if (already_parsed_entities.contains(entity)) return;

    if(relationship.has_parent()) {
        auto parent = relationship.parent();
        fn( parent,
            example_pack.template get<const local_position>(parent),
            example_pack.template get<world_position>(),
            example_pack.template get<entity_relationship_data_t const>(),
            fn);

        auto parent_world_pos = example_pack.template get<world_position>(parent);

        world_pos = parent_world_pos + local_pos;
    } else {
        world_pos = local_pos; // no parent, so world position is just the local position
    }

    already_parsed_entities.insert(entity);
}

for (auto [entity, local_pos, world_pos, _relations] : example_pack) {
    calculate_new_wpos_helper(entity, local_pos, world_pos, calculate_new_wpos_helper);
}
```


## Packs
The `ecs::pack<>` type is both a view into the component data, as well as a set of filtering instructions of what requirements the entities are supposed to have. This might seem like an odd combination, but simplifies systems, as well as makes clear the constraints of the data a variable will be working with.

The `pack<>` type also (optionally) accepts an instruction on the divisibility of this data (either `pack<whole, ...>` or `pack<partial, ...>`). This is mostly important the state in knowing if this pack makes the system parallelizable or not, and to what degree. If neither `whole` or `partial` is supplied, then `whole` is assumed implicitly.

### read-only and read-write data
You define in the signature if you are going to be only reading from the component, or also will be writing to it, by adding `const` to the component type.
As an example, the following is the signature of a read-only view of position components:
```cpp
pack<const position> all_positions;
```

Read only data is faster to create than read-write data, so keep that in mind.

### Iterating
There are several ways of iterating through a pack, the traditional indexing operator is provided, but this gets you back a tuple of the contained data types at the given index.

The easiest way of iterating through a pack is to use structured bindings like in the following example, note that iterating through a pack this way will automatically return references (and const references) depending on if the type was declared const or not.
```cpp
pack<const position, rotation, const lifetime> example_pack;
for(auto [pos, rot, life] : example_pack)
{
    pos = {}; // error pos is const&
    rot = {}; // success! rot is a reference
    life -= 1.0f; // error life is const&
}
```

std::get<> also works, and will return you an `array_view<T>` for any given type of T.
```cpp
pack<const position, rotation, const lifetime> example_pack;
auto positions = std::get<const position>(example_pack);
auto rotations = std::get<rotation>(example_pack);
// error! 'lifetime' needs to be 'const lifetime'!
auto lifetimes = std::get<lifetime>(example_pack);
```
### partial and whole packs
`partial` and `whole` are identifiers to designate if a pack is divisible or not. These are important for the `ecs::state` to know if this pack can be divided onto multiple contexts or not. The **Systems** section will explain more about this.

### direct and indirect packs
Data within the packs can either be directly stored in the pack, or involve an indirection to the backing storage. Typically you'll want to use the indirect pack unless you benefit from direct contiguous memory access.

## Systems
The ECS takes a hands-off approach in dictating *how* systems should look as long as you supply an invocable function/method/object that satisfies the signature of `(psl::ecs::info& info, /* your filter instructions using the ecs::pack<> interface*/)`.
The filtering operations will be interpreted from this signature. See subsequent examples section for detailed examples of this in practice.

The first parameter is required (and required to always be the first parameter in a system). It contains a `const& ecs::state`, an `ecs::command_buffer`, the deltaTime of the frame, and the realTime of the ECS.

Because every system can be multithreaded, only a read-only view can be acquired of the `state`. If another concurrent thread would mutate the `state`, it could lead into all sorts of undefined, but very dangerous, behaviour.

You can guarantee your system is single threaded when you declare it to the `ecs::state` by tagging it with `threading::sequential` (default) instead of `threading::parallel`; or that it is single context by requiring all `pack<>` objects to be `ecs::whole`(default) instead of `ecs::partial`.

### multithreading
By default systems take the safest (but least performant) road, this means your system will run single context with no partial packs. This is equivalent to running your code on one core in one go.
There are 2 elements that affect how your code runs, and how parallelize-able it is. Those are the `ecs::threading` values you can set on a system when you declare it to the `ecs::state`, and the `ecs::partial`/`ecs::whole` you can set as the ***first*** template argument on a `ecs::pack<>`.

Next table is a reference chart of what the different combinations result in. `N` is the amount of workers assigned to the `state`. Because multiple `pack<>` objects can be in a method signature, the following chart's second row is true if *at least* one partial pack is present. 

| | threading::sequential| threading::parallel|
| -- | -- | -- |
| **pack<whole,...>** | single thread, invoked once per frame | equivalent to threading::sequential
| **pack<partial,...>** | multi-context (but not concurrent), invoked N times per frame | Invoked N times, concurrently per frame.

### System Groups

Sometimes you want systems to not tick at the same rate as others, as example physics is often ran at a fixed timestep rather than every tick. For this the concept of system groups exists. You can create a group by calling the member function `create_system_group()` on an instance of `psl::ecs::state_t`. This will return you an opaque handle to a group. You can then subsequently use this when you are declaring your systems (`state_t::declare()`) as one of its arguments.

#### limitations and caveats 

As component lifetime events such as `on_add`, `on_remove`, `on_combine`, `on_break` are properties of the component these will not be propogated to anything but the default system group (invoking `state_t::tick()` without the group arguments). At runtime an exception will be thrown if you try to register a system that belongs to a group which does one of these operations.

As a side consequence if you add components or entities during the ticking of a system group, these will not be visible until you do a normal tick.

### examples
A single threaded system that filters on all entities that have a `position` component. Note that position is *not* marked `const` because we will be adjusting the position value in our imaginary example. If it was marked as `const`, we would only get a read-only view into the data.
```cpp
auto move_system = 
[](psl::ecs::info& info, pack<position> positions) 
{ /* do things */};

state.declare(psl::ecs::threading::seq, move_system);
```

Next is a multi-context system, which never runs concurrently (i.e. no invocation of the system runs at the same time on *any* worker). What happens here is when the system gets invoked, the pack will contain 'N' elements, where 'N' is equal to the total size of all position components in the ECS, divided by the amount of workers available.
```cpp
auto move_system = 
[](psl::ecs::info& info, pack<partial, position> positions) 
{ /* do things */};

state.declare(threading::seq, move_system);
```
Lastly a multi-context, true multi threaded version. It will contain equal amounts of data in the positions pack as the last example, but now multiple threads *might* be running this code at the same time.
```cpp
auto move_system = 
[](psl::ecs::info& info, pack<partial, position> positions) 
{ /* do things */};

state.declare(psl::ecs::threading::par, move_system);
```

**multi-pack systems**
A single threaded system, that has a pack that gets all `attractor`'s that also have a `position` component and a pack that gets all `velocity` components to write to.

We don't require the position component's data in the first pack, which is why we wrap it in a filter instruction instead. As well as we will only *read* from the attractor, so we add `const` to the type declaration.

```cpp
auto attractor_system = 
[](psl::ecs::info& info,
   pack<whole, const attractor, filter<position>> attractors, 
   pack<whole, velocity> velocities)
{ /* change velocity based on attractor force & distance */ };

state.declare(psl::ecs::threading::seq, attractor_system );
```

A variation on the previous that allows you to run it concurrently in a multi threaded context. Note that the `attractor` pack is still marked as `whole`. This is needed because the velocities that we will be adjusting need to know about *all* attractors in the scene.
```cpp
auto attractor_system = 
[](psl::ecs::info& info,
   pack<whole, const attractor, filter<position>> attractors, 
   pack<partial, velocity> velocities)
{ /* change velocity based on attractor force & distance */ };

state.declare(psl::ecs::threading::par, attractor_system );
```

Preseeding example. Sometimes you want to run a system that reacts to the addition of a component, but also want it to run on all entities that already satisfy the filtering instruction. For this you can use the `preseed_tag`. Note that this tag is also available for the `on_combine` operation.

```cpp
state.create<some_component>(100);
state.tick(std::chrono::duration<float>(1.0f));

// normally as the system has been declared after the tick where the components were added,
// this system would never know about the pre-existing components. 
// But by using the `preseed_tag`, the next tick we will also get the 100 pre-existing components (one time).
state.declare(
    [](psl::ecs::info& info, 
       pack<whole, some_component, on_add<preseed_tag, some_component>> registrations)
    { /* do things */ };
);

state.tick(std::chrono::duration<float>(1.0f));
```

### serialization

All trivial components can be de/serialized to/from disk. You can test if your component is trivial by checking `psl::ecs::component_type_v` for your type. It will contain the enum value `psl::ecs::component_type`. Any value except `component_type::COMPLEX` can be serialized.

Furthermore any component that can be serialized can also be versioned (default off). You can control the versioning by specializing the `psl::ecs::component_trait_version_t` for your type. You will also need to provide an instance of `psl::ecs::component_updater_t` for your type which will be used to handle data migration of an older version to the newer version.

### See also

There are tests covering all of the functionality described here in `tests/src/ecs.cpp`. Additionally if you see missing documentation, or have questions please open an issue.
