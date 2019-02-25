# ECS
## Introduction
The ECS (Entity Component System) present in `paradigm engine`is a pure ECS design, this means that data and logic are completely separate. This aids in easily parallelizing systems, as well as keeping data in a more cache friendly way.
This by itself doesn't sound great, and more often than not creates a very confusing API, so this ECS is written with not only performance in mind, but easy of use for the end user. Hopefully after this short introduction & examples you will have enough of an idea of how the ECS works.

|  |  |
|--|--|
| `entities` | an ID of type `uint32_t` |
| `components` | are required to satisfy the requirements of [`standard_layout`](https://en.cppreference.com/w/cpp/types/is_standard_layout), [`trivially_destructible`](https://en.cppreference.com/w/cpp/types/is_trivially_destructible), and [`trivially_copyable`](https://en.cppreference.com/w/cpp/types/is_trivially_copyable). It is easiest of all to see them as a superset of POD types, with exception that you can provide a custom constructor, as long as they are `default` initializable. |
| `systems` | any invocable (lambda, function or method), that satisfies the required signature |

## concepts
### state
 Often other ECS implementations refer to this as their "world". The `state` contains the actual `component` data, to which `entity` the `component` maps, and a list of `systems` that should be executed per invocation.
 Its most notable methods are [`add_components`](#state::add_components), `remove_components`, `create`, `destroy`, `filter`, and `tick`. There are several others, but they are mostly self explanatory and explained in the reference documentation.
 
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

**filter:** requires the component to exist on the entity, but no data will be fetched for it.
**except:** requires the component to be **non**-existent on the entity.
**on_add:** only fills in data when this component was added to the entity since the last tick.
**on_remove:** reverse scenario of on_add
**on_combine:** fills in data when this combination is first created. I.e. if you were filtering `on_combine<transform, renderable>`, then on any  entity with a transform component, when a renderable component gets added, then the pack will be filled in with data of that entity. The reverse is true as well, for any entity with a renderable component, when you add a transform component to it, this filter will trigger.
**on_break:** same like on_combine, but the reverse situation (i.e. when an entity that has this component combination, and you remove one, then this filter will trigger).

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

### creating components
For performance reasons it's always better to add components on many entities all at once, instead of doing them one by one. The API reflects this suggestion by only exposing a signature for a container of `entities`.

You will also notice that the signatures are templated. The expected template type is the `component` you wish to add to those entities.

Furthermore, there are 4 ways to initialize components:
	- empty initialization
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
You can also circumvent needing to call `add_components` completely, and pass these examples into the `create` method directly. The next example will create 100 entities, and immediately add a health component to all of them. The same rules and methods apply like the previous examples, so you can also initialize them with a template or predicate!
```cpp
    ecs::state state{};
    
    auto entities = state.create<health>(100);
```
