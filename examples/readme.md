# Examples

In this directory you can find some examples of how to use the paradigm rendering library. The list in this readme will give you a pseudo-increasing difficulty of the examples, meaning earlier list entries should be easier to understand than later ones. They can often also be built upon concepts from earlier examples.

## HelloScreen

This is the simplest example, showing how to create a window and render a single screen. It is a good starting point for understanding the basic concepts of initializing the library and rendering a screen. It can also serve as a good smoke test to make sure your environment is set up correctly. If all goes well, you should see a screen with a single color (magenta) when running this example.

## HelloTriangle

In this example, we render a single triangle to the screen. We handwave the library's initialization and focus entirely on the triangle being rendered. If you skip the [HelloScreen](#HelloScreen) example, you might be missing some context on how the library is initialized.

The end result of running this example should be a single triangle, where each point is a different color (red, green, and blue).

## SimpleTextured

Showcases how to load the most minimal set of resources you need to render a textured triangle. It loads a material file, and a geometry file, which references shaders, textures, and a sampler.

## Instancing

Small sample showing how to instantiate a specific renderable multiple times with different instance data (such as position, rotation, scale, color, etc). It should spawn around 300'000 icospheres, each with a different color and position, and move them around every frame until they despawn in around 14-34 seconds after the start.

A little while off another group of 100 icospheres are continiously spawned every frame with a lifetime of 2 seconds.

Some general tips for instancing: Prefer doing bulk updates of instance data rather than setting each instance individually. You can inspect the `core::ecs::systems::render` how more complex instance updates are done (that is where the model matrix is updated every frame).