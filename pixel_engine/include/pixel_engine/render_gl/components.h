#pragma once

namespace pixel_engine {
    namespace render_gl {
        namespace components {
            using namespace prelude;

            /*! @brief A buffer object.
             * This struct represents a buffer object in OpenGL.
             * @brief buffer is the OpenGL buffer id.
             * @brief data is the data stored in the buffer. It is resized when the data written to it is larger than
             * the current size.
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
                void write(const void* data, size_t size, size_t offset) {
                    if (offset + size > this->data.size()) {
                        this->data.resize(offset + size);
                    }
                    memcpy(this->data.data() + offset, data, size);
                }
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
                /*! @brief The OpenGL vertex array id. Not need to be initialized by user. */
                unsigned int vertex_array = 0;
                /*! @brief The number of vertices to draw. Not need to be initialized by user. */
                int vertex_count = 0;
                /*! @brief The buffer object containing the vertex data. Not need to be initialized by user. */
                Buffer vertex_buffer = {};
            };

            /*! @brief shaders used in a pipeline.
             * @brief vertex_shader is the OpenGL vertex shader id.
             * @brief fragment_shader is the OpenGL fragment shader id.
             * @brief geometry_shader is the OpenGL geometry shader id.
             * @brief tess_control_shader is the OpenGL tessellation control shader id.
             * @brief tess_evaluation_shader is the OpenGL tessellation evaluation shader id.
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
             * @brief index of the buffer object in the vector is the binding point in the shader.
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
                int texture = 0;
                int sampler = 0;
            };

            /*! @brief Images used in a pipeline.
             * @brief images is a vector of image objects.
             * @brief index of the image object in the vector is the binding point in the shader.
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
                int location;
                int size;
                int type;
                bool normalized;
                int stride;
                int offset;
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
    }      // namespace render_gl
}  // namespace pixel_engine