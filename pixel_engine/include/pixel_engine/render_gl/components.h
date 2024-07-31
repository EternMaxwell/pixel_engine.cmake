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

namespace pipeline {

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
        uint32_t location, uint32_t size, int type, bool normalized,
        uint32_t stride, uint64_t offset);
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
    void bindBase(int target, int index);
    /**
     * @brief Checks if the buffer object is valid.
     *
     * @return `bool` True if the buffer object is valid.
     */
    bool valid() const;
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
        size_t index, const TexturePtr& image, int level, bool layered,
        int layer, int access, int format);
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
 * @brief Buffer bindings.
 *
 * This struct represents a bundle of buffer objects in OpenGL.
 *
 * Used in `PipelineBundle` to bind the buffers.
 *
 * @param vertex_buffer `BufferPtr` The vertex buffer object.
 * @param index_buffer `BufferPtr` The index buffer object.
 * @param draw_indirect_buffer `BufferPtr` The draw indirect buffer object.
 */
struct BufferBindings {
    BufferPtr vertex_buffer;
    BufferPtr index_buffer;
    BufferPtr draw_indirect_buffer;
    void bind() const;
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
struct PipelineCreationBundle {
    Bundle bundle;

    PipelineCreation creation;

    ProgramShaderAttachments shaders;

    VertexAttribs attribs;
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
 * @param vertex_buffer `BufferPtr` The vertex buffer object.
 * @param index_buffer `BufferPtr` The index buffer object.
 * @param draw_indirect_buffer `BufferPtr` The draw indirect buffer object.
 * @param uniform_buffers `UniformBufferBindings` The uniform buffer bindings.
 * @param storage_buffers `StorageBufferBindings` The storage buffer bindings.
 * @param textures `TextureBindings` The texture bindings.
 * @param images `ImageTextureBindings` The image texture bindings.
 * @param view_port `ViewPort` The viewport.
 * @param depth_range `DepthRange` The depth range.
 * @param scissor_test `ScissorTest` The scissor test.
 * @param stencil_test `StencilTest` The stencil test.
 * @param depth_test `DepthTest` The depth test.
 * @param blending `Blending` The blending.
 * @param logic_op `LogicOp` The logic operation.
 */
struct PipelineBundle {
    Bundle bundle;

    Pipeline pipeline;

    VertexArrayPtr vertex_array;
    BufferBindings buffers;

    ProgramPtr program;
};

struct PipelineLayout {
    UniformBufferBindings uniform_buffers;
    StorageBufferBindings storage_buffers;
    TextureBindings textures;
    ImageTextureBindings images;
};

struct PerSampleOperations {
    ScissorTest scissor_test;
    DepthTest depth_test;
    StencilTest stencil_test;
    Blending blending;
    LogicOp logic_op;
};

struct RenderBufferPtr {
    uint32_t id = 0;
    void create();
    void storage(int internal_format, int width, int height);
    bool check();
    bool valid() const;
};

struct FrameBufferPtr {
    uint32_t id = 0;
    void create();
    void bind() const;
    void attachTexture(
        uint32_t attachment, const TexturePtr& texture, int level);
    void attachTextureLayer(
        uint32_t attachment, const TexturePtr& texture, int level, int layer);
    void attachRenderBuffer(
        uint32_t attachment, const RenderBufferPtr& render_buffer);
    bool check();
    bool unique() const;
};

/**
 * @brief Pipeline entity ptr.
 *
 * This struct points to a pipeline entity in entity component system.
 */
struct PipelinePtr {
    entt::entity pipeline;
};

/**
 * @brief Render pass. Indicates the entity is a render pass entity.
 */
struct RenderPass {};

/**
 * @brief Render pass bundle.
 *
 * Used to tell the engine how to render.
 *
 * @param render_pass `RenderPass` The render pass object.
 * @param pipeline `PipelinePtr` The pipeline object.
 */
struct RenderPassBundle {
    Bundle bundle;

    RenderPass render_pass;

    PipelinePtr pipeline;
    FrameBufferPtr frame_buffer;

    ViewPort view_port;
    DepthRange depth_range;

    PipelineLayout layout;
    PerSampleOperations per_sample_operations;
};

struct RenderPassPtr {
    entt::entity render_pass;
};

struct DrawArrays {
    uint32_t mode;
    int first;
    int count;
};

struct DrawArraysBundle {
    Bundle bundle;

    RenderPassPtr render_pass;
    DrawArrays draw_arrays;
};
}  // namespace pipeline

/*! @brief A buffer object.
 *
 * This struct represents a buffer object in OpenGL.
 *
 * buffer is the OpenGL buffer id.
 * data is the data stored in the buffer. It is resized when the data written to
 * it is larger than the current size.
 */
struct Buffer {
    /*! @brief The OpenGL buffer id. */
    unsigned int buffer = 0;
    /*! @brief The data stored in the buffer. */
    std::vector<uint8_t> data;
    /*! @brief Writes data to the buffer.
     * @param data The data to write to the buffer.
     * @param size The size of the data to write.
     * @param offset The offset in the buffer to write the data to.
     */
    void write(const void* rdata, size_t size, size_t offset);
};

/*! @brief A pipeline object.
 * This struct represents a pipeline object in OpenGL.
 * @brief program is the OpenGL program id.
 * @brief vertex_array is the OpenGL vertex array id.
 * @brief vertex_count is the number of vertices to draw.
 * @brief vertex_buffer is the buffer object containing the vertex data.
 */
struct Pipeline {
    /*! @brief The OpenGL program id. Not need to be initialized by user. */
    int program = 0;
    /*! @brief The OpenGL vertex array id. Not need to be initialized by user.
     */
    unsigned int vertex_array = 0;
    /*! @brief The number of vertices to draw. Not need to be initialized by
     * user. */
    int vertex_count = 0;
    /*! @brief The buffer object containing the vertex data. Not need to be
     * initialized by user. */
    Buffer vertex_buffer = {};
};

/*! @brief shaders used in a pipeline.
 * @brief vertex_shader is the OpenGL vertex shader id.
 * @brief fragment_shader is the OpenGL fragment shader id.
 * @brief geometry_shader is the OpenGL geometry shader id.
 * @brief tess_control_shader is the OpenGL tessellation control shader id.
 * @brief tess_evaluation_shader is the OpenGL tessellation evaluation shader
 * id.
 * @brief 0 if no shader assigned. Assign shader by asset server is recommended.
 */
struct Shaders {
    int vertex_shader = 0;
    int fragment_shader = 0;
    int geometry_shader = 0;
    int tess_control_shader = 0;
    int tess_evaluation_shader = 0;
};

/*! @brief Buffers used in a pipeline.
 * @brief uniform_buffers is a vector of buffer objects used as uniform buffers.
 * @brief storage_buffers is a vector of buffer objects used as storage buffers.
 * @brief index of the buffer object in the vector is the binding point in the
 * shader.
 */
struct Buffers {
    std::vector<Buffer> uniform_buffers;
    std::vector<Buffer> storage_buffers;
};

/*! @brief An image object.
 * This struct represents an image object in OpenGL.
 * @brief texture is the OpenGL texture id.
 * @brief sampler is the OpenGL sampler id.
 * @brief 0 if no image assigned. Assign image by asset server is recommended.
 */
struct Image {
    unsigned int texture = 0;
    int sampler = 0;
};

/*! @brief Images used in a pipeline.
 * @brief images is a vector of image objects.
 * @brief index of the image object in the vector is the binding point in the
 * shader.
 */
struct Images {
    std::vector<Image> images;
};

/*! @brief Vertex attribute. All data need to be assigned by user.
 * @brief location is the location of the attribute in the shader.
 * @brief size is the number of components in the attribute.
 * @brief type is the data type of the attribute.
 * @brief normalized is whether the attribute is normalized.
 * @brief stride is the byte offset between consecutive attributes.
 * @brief offset is the byte offset of the first component of the attribute.
 */
struct VertexAttrib {
    uint32_t location;
    uint32_t size;
    int type;
    bool normalized;
    uint32_t stride;
    uint64_t offset;
};

/*! @brief Attributes used in a pipeline.
 * @brief attribs is a vector of vertex attributes.
 */
struct Attribs {
    std::vector<VertexAttrib> attribs;
};

/*! @brief A bundle of pipeline objects.
 * @brief pipeline is the pipeline object.
 * @brief shaders is the shaders object.
 * @brief attribs is the attribs object.
 * @brief buffers is the buffers object.
 * @brief images is the images object.
 */
struct PipelineBundle {
    Bundle bundle;
    Pipeline pipeline;
    Shaders shaders;
    Attribs attribs;
    Buffers buffers;
    Images images;
};

struct ProgramLinked {};
}  // namespace components
}  // namespace render_gl
}  // namespace pixel_engine