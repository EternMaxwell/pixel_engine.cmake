# Render-Ogl Spec

This is a sub library inside the pixel engine. Render ogl should be a render graph-like
rendering backend, in which the render graph might be portable to other backends.

## Details

Unlike normal render graph or frame graph, render ogl does not handle the task graph (
    or the execution sequence of the render passes
) it self, it is the core app system in the pixel engine does it.

So in this case, each render pass in the graph is actually a system in the app, which
can potentially retrieve resources from the app. So to put things simple, the renderogl
lib or plugin behaves like a resource manager, which handles the resource creation and
resource destruction. which may look like what in below:

```cpp
using your_resource_type = resource<context_t, desc_t, actual_t, usage_t>;

// creation:
void create_resource(Query<Get<framegraph>> fg, Command command) {
    auto [rendergraph] = fg.single().value();
    desc_t description;
    usage_t usage;
    // do something.
    auto res = rendergraph.create_resource("name", description, usage);
    command.spawn(res, your_own_specifier_type);
}

// destruction:
void destroy_resource(Query<Get<framegraph>> fg, Query<Get<your_resource_type>> res) {
    auto [resource] = res.single().value();
    auto [rendergraph] = fg.single().value();
    rendergraph.remove_resource(resource);
}
// or you can do:
void destroy_resource(Query<Get<framegraph>> fg) {
    auto [rendergraph] = fg.single().value();
    rendergraph.remove_resource("name");
} 
// the second one does the same thing but will be slower, and may cause some
// synchronization issues
```

The above code is basically how you can create and destroy resources with the framegraph.

However, you may notice that the resource template need you to specify four types to use
it.

### context

The context_t is the data type you want to store in the framegraph, this may not be
useful in opengl, but in vulkan we need to store our device, queue, and other context
data in the framegraph so that the framegraph can successfully create the resource.
To add context data to framegraph, you can do:

```cpp
void add_context_to_framegraph(Query<Get<framegraph>> fg) {
    auto [rendergraph] = fg.single().value();
    rendergraph.create_context<context_t>(args...);
}
// the args is the arguments for the create context function for this type
```

And you also need to specify the create_context template function for this type if it
doesn't have a proper constructor:

```cpp
template<>
std::unique_ptr<context_t> create_context(Args... args) {
    // create the context unique_ptr
    // return the ptr
    return ptr;
}
```

To destroy the context, do:

```cpp
template<>
void destroy_context(std::unique<context_t> ctx) {
    // do destruction.
} // specify this if no context_t destructor

void destroy_context_in_framegraph(Query<Get<framegraph>> fg) {
    auto [rendergraph] = fg.single().value();
    rendergraph.destroy_context<context_t>();
}
```

or you can just leave it along and let the unique_ptr do the job if has a destructor.
However, the destruction time will be at the end of the destruction of the framegraph.

### description

The desc_t is the description type you use for the type of resource you specified.

For example if the resource refers to a opengl buffer and texture, then it could be
like:

```cpp
struct buffer_description {
    size_t size;
    GLenum usage;
};
struct texture_description {
    size_t levels;
    size_t layers;
    GLenum format;
    std::array<size_t, 3> size;
};

using buffer_resource = resource<context_t, buffer_description, actual_t, usage_t>;
using texture_resource = resource<context_t, texture_description, actual_t, usage_t>;
```

### actual

Same as many other framegraphs, the framegraph in render ogl does not create the actual
resource but a virtual resource, or a handle if you want. So the virtual resource
has to know the actual resource type so that it can create the actual resource along
with the framegraph before the rendering begins.

And to get it work, here is an example:

```cpp
using buffer_resource = resource<context_t, buffer_description, int, usage_t>;

template<>
std::unique_ptr<int> create_actual(context_t* ctx, buffer_description desc) {
    int buffer = glCreateBuffer();
    glNamedBufferData(buffer, desc.size, NULL, desc.usage);
    return std::make_unique<int>(buffer);
}

template<>
void destroy_actual(context_t* ctx, int* actual) {
    glDeleteBuffers(1, actual);
}
```

With the template function specified, the framegraph will call these functions to
realize (or create) the resource, then you can get the actual resources you need
in render passes (or render systems) like this:

```cpp
void render(Query<Get<buffer_resource>> res, other_args... args) {
    auto [vr] = res.single().value();
    int* real_res = vr.get_actual(); // note that get_actual returns actual_t*.
    // do the rendering with the actual res.
}
```

Also if you need to use barriers, which is not needed in opengl but vulkan, you
also need to specify the read write barrier function, and this is where the
usage_t take in place.

### usage

usage is used in barrier, which is not useful and is not necessary in opengl, so
we will not talk about it.
