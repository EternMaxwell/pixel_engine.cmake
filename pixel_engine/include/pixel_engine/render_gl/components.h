/**
 * @file pixel_engine/render_gl/components.h
 *
 * @brief Components for OpenGL rendering.
 *
 * This file contains the components used in OpenGL rendering.
 *
 * @author Etern Maxwell <maxwell_dev@outlook.com>
 */

#pragma once

#include <vector>

#include "pixel_engine/entity.h"

namespace pixel_engine {
namespace render_gl {
namespace components {
using namespace prelude;

/**
 * @brief Shader object ptr.
 *
 * This struct points to a shader object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL shader id.
 */
struct ShaderPtr {
    /**
     * @brief The OpenGL shader id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual shader object.
     *
     * @param type `int` The type of the shader.
     */
    void create(int type);
    /**
     * @brief Sets the source code of the shader.
     *
     * @param source `const char*` The source code of the shader.
     */
    void source(const char* source);
    /**
     * @brief Compiles the shader.
     *
     * @return `bool` True if the shader is compiled successfully.
     */
    bool compile();
    /**
     * @brief Checks if the shader object is valid.
     *
     * @return `bool` True if the shader object is valid.
     */
    bool valid() const;
};

/**
 * @brief Program object ptr.
 *
 * This struct points to a program object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL program id.
 */
struct ProgramPtr {
    /**
     * @brief The OpenGL program id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual program object.
     */
    void create();
    /**
     * @brief Attaches a shader to the program.
     *
     * @param shader `const ShaderPtr&` The shader object to attach.
     */
    void attach(const ShaderPtr& shader);
    /**
     * @brief Links the program.
     *
     * @return `bool` True if the program is linked successfully.
     */
    bool link();
    /**
     * @brief Uses the program.
     */
    void use() const;
    /**
     * @brief Checks if the program object is valid.
     *
     * @return `bool` True if the program object is valid.
     */
    bool valid() const;
};

/**
 * @brief Vertex attribute.
 *
 * @param location `uint32_t` The location of the attribute in the shader.
 * @param size `uint32_t` The number of components in the attribute.
 * @param type `int` The data type of the attribute.
 * @param normalized `bool` Whether the attribute is normalized.
 * @param stride `uint32_t` The byte offset between consecutive attributes.
 * @param offset `uint64_t` The byte offset of the first component of the
 * attribute.
 */
struct VertexAttrib {
    uint32_t location;
    uint32_t size;
    int type;
    bool normalized;
    uint32_t stride;
    uint64_t offset;
};

struct VertexAttribs {
    std::vector<VertexAttrib> attribs;
    VertexAttrib& operator[](size_t index);
    void add(
        uint32_t location,
        uint32_t size,
        int type,
        bool normalized,
        uint32_t stride,
        uint64_t offset
    );
};

/**
 * @brief Vertex Array object ptr.
 *
 * This struct points to a vertex array object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL vertex array id.
 */
struct VertexArrayPtr {
    /**
     * @brief The OpenGL vertex array id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual vertex array object.
     */
    void create();
    /**
     * @brief Binds the vertex array object.
     */
    void bind() const;
    /**
     * @brief Checks if the vertex array object is valid.
     *
     * @return `bool` True if the vertex array object is valid.
     */
    bool valid() const;
};

/**
 * @brief Buffer object ptr.
 *
 * This struct points to a buffer object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL buffer id.
 */
struct BufferPtr {
    /**
     * @brief The OpenGL buffer id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual buffer object.
     */
    void create();
    /**
     * @brief Binds the buffer object.
     *
     * @param target `int` The target of the buffer.
     */
    void bind(int target) const;
    /**
     * @brief Binds the buffer object to a specific index.
     *
     * @param target `int` The target of the buffer.
     * @param index `int` The index to bind the buffer to.
     */
    void bindBase(int target, int index) const;
    /**
     * @brief Checks if the buffer object is valid.
     *
     * @return `bool` True if the buffer object is valid.
     */
    bool valid() const;
    /**
     * @brief Sets the data of the buffer.
     *
     * @param data `const void*` The data to set.
     * @param size `size_t` The size of the data.
     * @param usage `int` The usage of the data.
     */
    void data(const void* data, size_t size, int usage);
    /**
     * @brief Sub data of the buffer.
     *
     * @param data `const void*` The data to set.
     * @param size `size_t` The size of the data.
     * @param offset `size_t` The offset of the data.
     */
    void subData(const void* data, size_t size, size_t offset);
};

/**
 * @brief Uniform buffer bindings.
 *
 * This struct represents a list of uniform buffer bindings in OpenGL.
 *
 * Only used in PipelineBundle.
 *
 * @param buffers `std::vector<BufferPtr>` The list of buffer objects.
 */
struct UniformBufferBindings {
    /**
     * @brief The list of buffer objects.
     */
    std::vector<BufferPtr> buffers;
    /**
     * @brief Bind the uniform buffers.
     */
    void bind() const;
    /**
     * @brief Set the uniform buffer binds at index.
     *
     * @param index `size_t` The bind index
     * @param buffer `BufferPtr` The ptr to the buffer to bind
     */
    void set(size_t index, const BufferPtr& buffer);
    BufferPtr& operator[](size_t index);
};

/**
 * @brief Storage buffer bindings.
 *
 * This struct represents a list of storage buffer bindings in OpenGL.
 *
 * Only used in `PipelineBundle`.
 *
 * @param buffers `std::vector<BufferPtr>` The list of buffer objects.
 */
struct StorageBufferBindings {
    /**
     * @brief The list of buffer objects.
     */
    std::vector<BufferPtr> buffers;
    /**
     * @brief Bind the storage buffers.
     */
    void bind() const;
    /**
     * @brief Set the storage buffer binds at index.
     *
     * @param index `size_t` The bind index
     * @param buffer `BufferPtr` The ptr to the buffer to bind
     */
    void set(size_t index, const BufferPtr& buffer);
    BufferPtr& operator[](size_t index);
};

/**
 * @brief Texture object ptr.
 *
 * This struct points to a texture object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL texture id.
 */
struct TexturePtr {
    /**
     * @brief The texture type.
     */
    int type;
    /**
     * @brief The OpenGL texture id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual texture object.
     *
     * @param type `int` The type of the texture.
     */
    void create(int type);
    /**
     * @brief Checks if the texture object is valid.
     *
     * @return `bool` True if the texture object is valid.
     */
    bool valid() const;
};

/**
 * @brief Sampler object ptr.
 *
 * This struct points to a sampler object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL sampler id.
 */
struct SamplerPtr {
    /**
     * @brief The OpenGL sampler id.
     */
    uint32_t id = 0;
    /**
     * @brief Creates the actual sampler object.
     */
    void create();
    /**
     * @brief Checks if the sampler object is valid.
     *
     * @return `bool` True if the sampler object is valid.
     */
    bool valid() const;
};

/**
 * @brief Image object.
 *
 * This struct represents an texture used as an sampler in glsl.
 *
 * Only used in `TextureBindings`.
 *
 * @param texture `TexturePtr` The texture object.
 * @param sampler `SamplerPtr` The sampler object.
 */
struct Image {
    TexturePtr texture;
    SamplerPtr sampler;
};

/**
 * @brief Texture bindings.
 *
 * This struct represents a list of texture bindings in OpenGL.
 *
 * Only used in `PipelineBundle`.
 *
 * @param textures `std::vector<Image>` The list of texture objects.
 */
struct TextureBindings {
    /**
     * @brief The list of texture objects.
     */
    std::vector<Image> textures;
    /**
     * @brief Bind the textures.
     */
    void bind() const;
    /**
     * @brief Set the texture binds at index.
     *
     * @param index `size_t` The bind index
     * @param texture `Image` The texture to bind
     */
    void set(size_t index, const Image& image);
    Image& operator[](size_t index);
};

struct ImageTexture {
    TexturePtr texture;
    int level = 0;
    bool layered = false;
    int layer = 0;
    int access;
    int format;
};

/**
 * @brief Image texture bindings.
 *
 * This struct represents a list of image texture bindings in OpenGL.
 *
 * These textures are used as `image` in glsl.
 *
 * Only used in `PipelineBundle`.
 *
 * @param images `std::vector<TexturePtr>` The list of texture objects.
 */
struct ImageTextureBindings {
    /**
     * @brief The list of image texture objects.
     */
    std::vector<ImageTexture> images;
    /**
     * @brief Bind the image textures.
     */
    void bind() const;
    /**
     * @brief Set the image texture binds at index.
     *
     * @param index `size_t` The bind index
     * @param image `TexturePtr` The texture to bind
     * @param level `int` The mipmap level
     * @param layered `bool` Whether the texture is layered
     * @param layer `int` The layer of the texture
     * @param access `int` The access of the texture
     * @param format `int` The format of the texture
     */
    void set(
        size_t index,
        const TexturePtr& image,
        int level,
        bool layered,
        int layer,
        int access,
        int format
    );
    ImageTexture& operator[](size_t index);
};

/**
 * @brief Viewport.
 *
 * @param x `int` The x coordinate of the viewport.
 * @param y `int` The y coordinate of the viewport.
 * @param width `int` The width of the viewport.
 * @param height `int` The height of the viewport.
 */
struct ViewPort {
    int x;
    int y;
    int width;
    int height;

    void use() const;
};

/**
 * @brief Depth range.
 *
 * @param nearf `float` The near value of the depth range.
 * @param farf `float` The far value of the depth range.
 */
struct DepthRange {
    float nearf = 0.0f;
    float farf = 1.0f;

    void use() const;
};

/**
 * @brief Scissor test.
 *
 * @param enable `bool` Whether the scissor test is enabled.
 * @param x `int` The x coordinate of the scissor box.
 * @param y `int` The y coordinate of the scissor box.
 * @param width `int` The width of the scissor box.
 * @param height `int` The height of the scissor box.
 */
struct ScissorTest {
    bool enable = false;
    int x;
    int y;
    int width;
    int height;
};

/**
 * @brief Stencil function.
 *
 * @param func `int` The stencil function.
 * @param ref `int` The reference value.
 * @param mask `uint32_t` The mask value.
 */
struct StencilFunc {
    uint32_t func;
    uint32_t ref;
    uint32_t mask;
};

/**
 * @brief Stencil operation.
 *
 * @param sfail `int` The stencil fail operation.
 * @param dpfail `int` The depth fail operation.
 * @param dppass `int` The depth pass operation.
 */
struct StencilOp {
    int sfail;
    int dpfail;
    int dppass;
};

/**
 * @brief Stencil test.
 *
 * @param enable `bool` Whether the stencil test is enabled.
 * @param func_back `StencilFunc` The stencil function for back face.
 * @param func_front `StencilFunc` The stencil function for front face.
 * @param op_back `StencilOp` The stencil operation for back face.
 * @param op_front `StencilOp` The stencil operation for front face.
 */
struct StencilTest {
    bool enable = false;
    StencilFunc func_back;
    StencilFunc func_front;
    StencilOp op_back;
    StencilOp op_front;
};

/**
 * @brief Depth function.
 *
 * @param func `int` The depth function.
 */
struct DepthFunc {
    uint32_t func;
};

/**
 * @brief Depth test.
 *
 * @param enable `bool` Whether the depth test is enabled.
 * @param func `DepthFunc` The depth function.
 */
struct DepthTest {
    bool enable = false;
    DepthFunc func;
};

/**
 * @brief Blend equation.
 *
 * @param mode_rgb `int` The blend equation for RGB.
 * @param mode_alpha `int` The blend equation for alpha.
 */
struct BlendEquation {
    int mode_rgb;
    int mode_alpha;
};

/**
 * @brief Blend function.
 *
 * @param src_rgb `int` The source blend factor for RGB.
 * @param dst_rgb `int` The destination blend factor for RGB.
 * @param src_alpha `int` The source blend factor for alpha.
 * @param dst_alpha `int` The destination blend factor for alpha.
 */
struct BlendFunc {
    int src_rgb;
    int dst_rgb;
    int src_alpha;
    int dst_alpha;
};

/**
 * @brief Blend color.
 *
 * @param red `float` The red component of the blend color.
 * @param green `float` The green component of the blend color.
 * @param blue `float` The blue component of the blend color.
 * @param alpha `float` The alpha component of the blend color.
 */
struct BlendColor {
    float red;
    float green;
    float blue;
    float alpha;
};

/**
 * @brief Blending.
 *
 * @param enable `bool` Whether blending is enabled.
 * @param blend_equation `BlendEquation` The blend equation.
 * @param blend_func `BlendFunc` The blend function.
 * @param blend_color `BlendColor` The blend color.
 */
struct Blending {
    bool enable = false;
    BlendEquation blend_equation;
    BlendFunc blend_func;
    BlendColor blend_color;
};

/**
 * @brief Logic operation.
 *
 * @param enable `bool` Whether logic operation is enabled.
 * @param op `int` The logic operation.
 */
struct LogicOp {
    bool enable = false;
    int op;
};

/**
 * @brief Pipeline shader attachments.
 *
 * @param vertex_shader `ShaderPtr` The vertex shader.
 * @param fragment_shader `ShaderPtr` The fragment shader.
 * @param geometry_shader `ShaderPtr` The geometry shader.
 * @param tess_control_shader `ShaderPtr` The tessellation control shader.
 * @param tess_evaluation_shader `ShaderPtr` The tessellation evaluation shader.
 */
struct ProgramShaderAttachments {
    ShaderPtr vertex_shader;
    ShaderPtr fragment_shader;
    ShaderPtr geometry_shader;
    ShaderPtr tess_control_shader;
    ShaderPtr tess_evaluation_shader;
};

/**
 * @brief Pipeline creation. Indicates the entity is a pipeline creation entity.
 */
struct PipelineCreation {};

/**
 * @brief Pipeline creation bundle.
 *
 * This struct represents a bundle of pipeline creation objects in OpenGL.
 *
 * @param shaders `ProgramShaderAttachments` The shaders.
 * @param buffers `PipelineBuffersCreation` The buffers this pipeline will use.
 * @param attribs `VertexAttribs` The vertex attributes.
 */
struct PipelineCreationBundle : Bundle {
    PipelineCreation creation;
    ProgramShaderAttachments shaders;
    VertexAttribs attribs;

    auto unpack() { return std::tie(creation, shaders, attribs); }
};

/**
 * @brief Pipeline layout. Indicates the entity is a pipeline layout entity.
 */
struct PipelineLayout {
    VertexArrayPtr vertex_array;
    BufferPtr vertex_buffer;
    BufferPtr index_buffer;
    BufferPtr draw_indirect_buffer;
    UniformBufferBindings uniform_buffers;
    StorageBufferBindings storage_buffers;
    TextureBindings textures;
    ImageTextureBindings images;

    void use() const;
};

/**
 * @brief Per sample operations.
 *
 * @param scissor_test `ScissorTest` The scissor test.
 * @param depth_test `DepthTest` The depth test.
 * @param stencil_test `StencilTest` The stencil test.
 * @param blending `Blending` The blending.
 * @param logic_op `LogicOp` The logic operation.
 */
struct PerSampleOperations {
    ScissorTest scissor_test;
    DepthTest depth_test;
    StencilTest stencil_test;
    Blending blending;
    LogicOp logic_op;

    void use() const;
};

/**
 * @brief Render buffer object ptr. Points to a render buffer object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL render buffer id.
 */
struct RenderBufferPtr {
    uint32_t id = 0;
    void create();
    void storage(int internal_format, int width, int height);
    bool check();
    bool valid() const;
};

/**
 * @brief Frame buffer object ptr. Points to a frame buffer object in OpenGL.
 *
 * @param id `uint32_t` The OpenGL frame buffer id.
 */
struct FrameBufferPtr {
    uint32_t id = 0;
    void create();
    void bind() const;
    void attachTexture(
        uint32_t attachment, const TexturePtr& texture, int level
    );
    void attachTextureLayer(
        uint32_t attachment, const TexturePtr& texture, int level, int layer
    );
    void attachRenderBuffer(
        uint32_t attachment, const RenderBufferPtr& render_buffer
    );
    bool check();
    bool unique() const;
};

/**
 * @brief Pipeline. Indicates the entity is a pipeline entity.
 */
struct Pipeline {};

/**
 * @brief Pipeline bundle.
 *
 * This struct represents a bundle of pipeline objects in OpenGL.
 *
 * @param vertex_array `VertexArrayPtr` The vertex array object.
 * @param buffers `BufferBindings` The buffer bindings for vertex, index and
 * draw indirect buffers.
 * @param program `ProgramPtr` The program object.
 */
struct PipelineBundle : Bundle {
    Pipeline pipeline;

    PipelineLayout layout;
    ProgramPtr program;
    ViewPort viewport;
    DepthRange depth_range;
    PerSampleOperations per_sample_operations;
    FrameBufferPtr frame_buffer;

    auto unpack() {
        return std::forward_as_tuple(
            std::move(pipeline), std::move(layout), std::move(program),
            std::move(viewport), std::move(depth_range),
            std::move(per_sample_operations), std::move(frame_buffer)
        );
    }
};
}  // namespace components
}  // namespace render_gl
}  // namespace pixel_engine