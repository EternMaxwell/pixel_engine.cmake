#include <pixel_engine/prelude.h>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1002000
#include <vk_mem_alloc.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include "fragment_shader.h"
#include "vertex_shader.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;

namespace vk_trial {

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
    void * /*pUserData*/
) {
#if !defined(NDEBUG)
    switch (static_cast<uint32_t>(pCallbackData->messageIdNumber)) {
        case 0:
            // Validation Warning: Override layer has override paths set to
            // C:/VulkanSDK/<version>/Bin
            return vk::False;
        case 0x822806fa:
            // Validation Warning: vkCreateInstance(): to enable extension
            // VK_EXT_debug_utils, but this extension is intended to support use
            // by applications when debugging and it is strongly recommended
            // that it be otherwise avoided.
            return vk::False;
        case 0xe8d1a9fe:
            // Validation Performance Warning: Using debug builds of the
            // validation layers *will* adversely affect performance.
            return vk::False;
    }
#endif

    std::cerr
        << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(
               messageSeverity
           ))
        << ": "
        << vk::to_string(
               static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)
           )
        << ":\n";
    std::cerr << std::string("\t") << "messageIDName   = <"
              << pCallbackData->pMessageIdName << ">\n";
    std::cerr << std::string("\t")
              << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    std::cerr << std::string("\t") << "message         = <"
              << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        std::cerr << std::string("\t") << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            std::cerr << std::string("\t\t") << "labelName = <"
                      << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        std::cerr << std::string("\t") << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            std::cerr << std::string("\t\t") << "labelName = <"
                      << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        std::cerr << std::string("\t") << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            std::cerr << std::string("\t\t") << "Object " << i << "\n";
            std::cerr << std::string("\t\t\t") << "objectType   = "
                      << vk::to_string(static_cast<vk::ObjectType>(
                             pCallbackData->pObjects[i].objectType
                         ))
                      << "\n";
            std::cerr << std::string("\t\t\t") << "objectHandle = "
                      << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                std::cerr << std::string("\t\t\t") << "objectName   = <"
                          << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }
    return vk::False;
}

std::vector<char const *> gatherExtensions(
    std::vector<std::string> const &extensions
#if !defined(NDEBUG)
    ,
    std::vector<vk::ExtensionProperties> const &extensionProperties
#endif
) {
    std::vector<char const *> enabledExtensions;
    enabledExtensions.reserve(extensions.size());
    for (auto const &ext : extensions) {
        assert(std::any_of(
            extensionProperties.begin(), extensionProperties.end(),
            [ext](vk::ExtensionProperties const &ep) {
                return ext == ep.extensionName;
            }
        ));
        enabledExtensions.push_back(ext.data());
    }
#if !defined(NDEBUG)
    if (std::none_of(
            extensions.begin(), extensions.end(),
            [](std::string const &extension) {
                return extension == VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            }
        ) &&
        std::any_of(
            extensionProperties.begin(), extensionProperties.end(),
            [](vk::ExtensionProperties const &ep) {
                return (
                    strcmp(
                        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName
                    ) == 0
                );
            }
        )) {
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#endif
    return enabledExtensions;
}

std::vector<char const *> gatherLayers(
    std::vector<std::string> const &layers
#if !defined(NDEBUG)
    ,
    std::vector<vk::LayerProperties> const &layerProperties
#endif
) {
    std::vector<char const *> enabledLayers;
    enabledLayers.reserve(layers.size());
    for (auto const &layer : layers) {
        assert(std::any_of(
            layerProperties.begin(), layerProperties.end(),
            [layer](vk::LayerProperties const &lp) {
                return layer == lp.layerName;
            }
        ));
        enabledLayers.push_back(layer.data());
    }
#if !defined(NDEBUG)
    // Enable standard validation layer to find as much errors as possible!
    if (std::none_of(
            layers.begin(), layers.end(),
            [](std::string const &layer) {
                return layer == "VK_LAYER_KHRONOS_validation";
            }
        ) &&
        std::any_of(
            layerProperties.begin(), layerProperties.end(),
            [](vk::LayerProperties const &lp) {
                return (
                    strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0
                );
            }
        )) {
        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
#endif
    return enabledLayers;
}

#if defined(NDEBUG)
vk::StructureChain<vk::InstanceCreateInfo>
#else
vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
#endif
makeInstanceCreateInfoChain(
    vk::InstanceCreateFlagBits instanceCreateFlagBits,
    vk::ApplicationInfo const &applicationInfo,
    std::vector<char const *> const &layers,
    std::vector<char const *> const &extensions
) {
#if defined(NDEBUG)
    // in non-debug mode just use the InstanceCreateInfo for instance creation
    vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo(
        {instanceCreateFlagBits, &applicationInfo, layers, extensions}
    );
#else
    // in debug mode, addionally use the debugUtilsMessengerCallback in instance
    // creation!
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    vk::StructureChain<
        vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
        instanceCreateInfo(
            {instanceCreateFlagBits, &applicationInfo, layers, extensions},
            {{}, severityFlags, messageTypeFlags, debugUtilsMessengerCallback}
        );
#endif
    return instanceCreateInfo;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
};

struct Queues {
    vk::Queue graphics_queue;
    vk::Queue present_queue;
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct ChosenSwapChainDetails {
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    uint32_t image_count;
};

struct VKContext {
    vk::Instance instance;
    vk::PhysicalDevice physical_device;
    QueueFamilyIndices queue_family_indices;
    Queues queues;
    vk::Device device;
    VmaAllocator allocator;
    vk::SurfaceKHR surface;
    SwapChainSupportDetails swap_chain_support;
    ChosenSwapChainDetails swap_chain_details;
    vk::SwapchainKHR swap_chain;
    std::vector<vk::Image> swap_chain_images;
    std::vector<vk::ImageView> swap_chain_image_views;
    std::vector<vk::Framebuffer> swap_chain_framebuffers;
    vk::Image depth_image;
    VmaAllocation depth_image_allocation;
    vk::ImageView depth_image_view;
    vk::CommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<vk::Semaphore> image_available_semaphores;
    std::vector<vk::Semaphore> render_finished_semaphores;
    std::vector<vk::Fence> in_flight_fences;
    uint32_t current_frame = 0;
    uint32_t image_index = 0;
};

struct Renderer {
    vk::RenderPass render_pass;
    vk::Pipeline graphics_pipeline;
    vk::PipelineLayout pipeline_layout;
    vk::Buffer vertex_buffer;
    VmaAllocation vertex_buffer_allocation;
    vk::Buffer index_buffer;
    VmaAllocation index_buffer_allocation;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DescriptorPool descriptor_pool;

    vk::Image texture_image;
    VmaAllocation texture_image_allocation;
    vk::ImageView texture_image_view;
    vk::Sampler texture_sampler;

    std::vector<vk::DescriptorSet> descriptor_sets;
    std::vector<vk::Buffer> uniform_buffers;
    std::vector<VmaAllocation> uniform_buffer_allocations;
    std::vector<void *> uniform_buffer_data;

    static vk::DescriptorSetLayoutBinding getDescriptorSetLayoutBinding() {
        return vk::DescriptorSetLayoutBinding(
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex
        );
    }

    static vk::DescriptorSetLayoutBinding getTextureDescriptorSetLayoutBinding(
    ) {
        return vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment
        );
    }
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(
            0, sizeof(Vertex), vk::VertexInputRate::eVertex
        );
    }

    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)
            ),
            vk::VertexInputAttributeDescription(
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)
            ),
            vk::VertexInputAttributeDescription(
                2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)
            )
        };
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

static const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

void insert_vk_context(Command command) { command.init_resource<VKContext>(); }

void init_instance(Resource<VKContext> vk_context) {
    vk::ApplicationInfo app_info(
        "Hello Vulkan", 1, "No Engine", 1, VK_API_VERSION_1_0
    );
    std::vector<char const *> enabled_layers = {
#if !defined(NDEBUG)
        "VK_LAYER_KHRONOS_validation"
#endif
    };
    std::vector<char const *> enabled_extensions = {
#if !defined(NDEBUG)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    enabled_extensions.insert(
        enabled_extensions.end(), glfw_extensions,
        glfw_extensions + glfw_extension_count
    );
    spdlog::info("GLFW Extensions Count: {}", glfw_extension_count);
    for (auto const &layer : enabled_layers) {
        spdlog::info("Enabled Layer: {}", layer);
    }
    for (auto const &ext : enabled_extensions) {
        spdlog::info("Enabled Extension: {}", ext);
    }
    vk_context->instance =
        vk::createInstance(makeInstanceCreateInfoChain(
                               {}, app_info, enabled_layers, enabled_extensions
        )
                               .get<vk::InstanceCreateInfo>());
}

void get_physical_device(Resource<VKContext> vk_context) {
    auto &instance = vk_context->instance;
    vk_context->physical_device = instance.enumeratePhysicalDevices().front();
    spdlog::info(
        "Physical Device: {}",
        vk_context->physical_device.getProperties().deviceName.data()
    );
}

void create_device(Resource<VKContext> vk_context) {
    auto &physical_device = vk_context->physical_device;
    auto &surface = vk_context->surface;
    auto queue_family_properties = physical_device.getQueueFamilyProperties();
    spdlog::info("create device");
    QueueFamilyIndices queue_family_indices;
    int index = 0;
    for (auto const &queue_family : queue_family_properties) {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            queue_family_indices.graphics_family = index;
        }
        if (physical_device.getSurfaceSupportKHR(index, surface)) {
            queue_family_indices.present_family = index;
        }
        if (queue_family_indices.graphics_family.has_value() &&
            queue_family_indices.present_family.has_value()) {
            break;
        }
        index++;
    }
    if (!queue_family_indices.graphics_family.has_value() ||
        !queue_family_indices.present_family.has_value()) {
        spdlog::error("No queue family for graphics found!");
        throw std::runtime_error("No queue family for graphics found!");
    }
    std::set<uint32_t> unique_queue_families = {
        queue_family_indices.graphics_family.value(),
        queue_family_indices.present_family.value()
    };
    float queue_priority = 1.0f;
    std::vector<char const *> enabled_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<vk::DeviceQueueCreateInfo> device_queue_create_info;
    for (uint32_t queue_family : unique_queue_families) {
        device_queue_create_info.push_back(
            vk::DeviceQueueCreateInfo({}, queue_family, 1, &queue_priority)
        );
    }
    vk::DeviceCreateInfo device_create_info(
        {}, device_queue_create_info, {}, enabled_extensions
    );
    vk::PhysicalDeviceFeatures device_features;
    device_features.samplerAnisotropy = VK_TRUE;
    device_create_info.setPEnabledFeatures(&device_features);
    vk::Device device = physical_device.createDevice(device_create_info);
    vk_context->device = device;
    vk_context->queue_family_indices = queue_family_indices;
    vk::Queue graphics_queue =
        device.getQueue(queue_family_indices.graphics_family.value(), 0);
    vk::Queue present_queue =
        device.getQueue(queue_family_indices.present_family.value(), 0);
    vk_context->queues = Queues{graphics_queue, present_queue};
}

void create_vma_allocator(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    spdlog::info("create vma allocator");
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = vk_context->physical_device;
    allocator_info.device = device;
    allocator_info.instance = vk_context->instance;
    VmaAllocator allocator;
    if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
    vk_context->allocator = allocator;
}

void destroy_vma_allocator(Resource<VKContext> vk_context) {
    spdlog::info("destroy vma allocator");
    auto &allocator = vk_context->allocator;
    vmaDestroyAllocator(allocator);
}

void create_vertex_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    auto &device = vk_context->device;
    auto &vertices = vk_trial::vertices;
    spdlog::info("create vertex buffer");
    vk::BufferCreateInfo buffer_create_info(
        {}, vertices.size() * sizeof(Vertex),
        vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive
    );
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vk::Buffer buffer;
    VmaAllocation allocation;
    if (vmaCreateBuffer(
            allocator, (VkBufferCreateInfo *)&buffer_create_info, &alloc_info,
            (VkBuffer *)&buffer, &allocation, nullptr
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    vk::BufferCreateInfo staging_buffer_create_info(
        {}, vertices.size() * sizeof(Vertex),
        vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive
    );
    VmaAllocationCreateInfo staging_alloc_info = {};
    staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    vk::Buffer staging_buffer;
    VmaAllocation staging_allocation;
    if (vmaCreateBuffer(
            allocator, (VkBufferCreateInfo *)&staging_buffer_create_info,
            &staging_alloc_info, (VkBuffer *)&staging_buffer,
            &staging_allocation, nullptr
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    void *data;
    vmaMapMemory(allocator, staging_allocation, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(allocator, staging_allocation);

    vk::CommandBufferAllocateInfo allocate_info(
        vk_context->command_pool, vk::CommandBufferLevel::ePrimary, 1
    );
    vk::CommandBuffer command_buffer =
        device.allocateCommandBuffers(allocate_info).front();
    command_buffer.begin(vk::CommandBufferBeginInfo{});
    vk::BufferCopy copy_region(0, 0, vertices.size() * sizeof(Vertex));
    command_buffer.copyBuffer(staging_buffer, buffer, copy_region);
    command_buffer.end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(command_buffer);
    vk_context->queues.graphics_queue.submit(submit_info, nullptr);
    vk_context->queues.graphics_queue.waitIdle();
    device.freeCommandBuffers(vk_context->command_pool, command_buffer);
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);

    renderer.vertex_buffer = buffer;
    renderer.vertex_buffer_allocation = allocation;
}

void free_vertex_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    spdlog::info("free vertex buffer");
    vmaDestroyBuffer(
        allocator, renderer.vertex_buffer, renderer.vertex_buffer_allocation
    );
}

void create_index_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    auto &device = vk_context->device;
    auto &indices = vk_trial::indices;
    spdlog::info("create index buffer");
    vk::BufferCreateInfo buffer_create_info(
        {}, indices.size() * sizeof(uint16_t),
        vk::BufferUsageFlagBits::eIndexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive
    );
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vk::Buffer buffer;
    VmaAllocation allocation;
    if (vmaCreateBuffer(
            allocator, (VkBufferCreateInfo *)&buffer_create_info, &alloc_info,
            (VkBuffer *)&buffer, &allocation, nullptr
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    vk::BufferCreateInfo staging_buffer_create_info(
        {}, indices.size() * sizeof(uint16_t),
        vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive
    );
    VmaAllocationCreateInfo staging_alloc_info = {};
    staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    vk::Buffer staging_buffer;
    VmaAllocation staging_allocation;
    if (vmaCreateBuffer(
            allocator, (VkBufferCreateInfo *)&staging_buffer_create_info,
            &staging_alloc_info, (VkBuffer *)&staging_buffer,
            &staging_allocation, nullptr
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    void *data;
    vmaMapMemory(allocator, staging_allocation, &data);
    memcpy(data, indices.data(), indices.size() * sizeof(uint16_t));
    vmaUnmapMemory(allocator, staging_allocation);

    vk::CommandBufferAllocateInfo allocate_info(
        vk_context->command_pool, vk::CommandBufferLevel::ePrimary, 1
    );
    vk::CommandBuffer command_buffer =
        device.allocateCommandBuffers(allocate_info).front();
    command_buffer.begin(vk::CommandBufferBeginInfo{});
    vk::BufferCopy copy_region(0, 0, indices.size() * sizeof(uint16_t));
    command_buffer.copyBuffer(staging_buffer, buffer, copy_region);
    command_buffer.end();
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(command_buffer);
    vk_context->queues.graphics_queue.submit(submit_info, nullptr);
    vk_context->queues.graphics_queue.waitIdle();
    device.freeCommandBuffers(vk_context->command_pool, command_buffer);
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);

    renderer.index_buffer = buffer;
    renderer.index_buffer_allocation = allocation;
}

void free_index_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    spdlog::info("free index buffer");
    vmaDestroyBuffer(
        allocator, renderer.index_buffer, renderer.index_buffer_allocation
    );
}

void create_descriptor_set_layout(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("create descriptor set layout");
    vk::DescriptorSetLayoutBinding ubo_layout_binding =
        Renderer::getDescriptorSetLayoutBinding();
    vk::DescriptorSetLayoutBinding texture_layout_binding =
        Renderer::getTextureDescriptorSetLayoutBinding();
    auto bindings = {ubo_layout_binding, texture_layout_binding};
    vk::DescriptorSetLayoutCreateInfo layout_info;
    layout_info.setBindings(bindings);
    vk::DescriptorSetLayout descriptor_set_layout =
        device.createDescriptorSetLayout(layout_info);
    renderer.descriptor_set_layout = descriptor_set_layout;
}

void destroy_descriptor_set_layout(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("destroy descriptor set layout");
    device.destroyDescriptorSetLayout(renderer.descriptor_set_layout);
}

void create_uniform_buffers(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    auto &device = vk_context->device;
    spdlog::info("create uniform buffers");
    size_t buffer_size = sizeof(UniformBufferObject);
    renderer.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    renderer.uniform_buffer_allocations.resize(MAX_FRAMES_IN_FLIGHT);
    renderer.uniform_buffer_data.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::BufferCreateInfo buffer_create_info(
            {}, buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::SharingMode::eExclusive
        );
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        vk::Buffer buffer;
        VmaAllocation allocation;
        if (vmaCreateBuffer(
                allocator, (VkBufferCreateInfo *)&buffer_create_info,
                &alloc_info, (VkBuffer *)&buffer, &allocation, nullptr
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create uniform buffer!");
        }
        renderer.uniform_buffers[i] = buffer;
        renderer.uniform_buffer_allocations[i] = allocation;
        vmaMapMemory(allocator, allocation, &renderer.uniform_buffer_data[i]);
    }
}

void destroy_uniform_buffers(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    spdlog::info("destroy uniform buffers");
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vmaUnmapMemory(allocator, renderer.uniform_buffer_allocations[i]);
        vmaDestroyBuffer(
            allocator, renderer.uniform_buffers[i],
            renderer.uniform_buffer_allocations[i]
        );
    }
}

void update_uniform_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &swap_chain_details = vk_context->swap_chain_details;
    auto &uniform_buffer_data = renderer.uniform_buffer_data;
    float time = glfwGetTime();
    UniformBufferObject ubo;
    ubo.model = glm::rotate(
        glm::mat4(1.0f), (float)time * glm::radians(90.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swap_chain_details.extent.width /
            static_cast<float>(swap_chain_details.extent.height),
        0.1f, 10.0f
    );
    ubo.proj[1][1] *= -1;
    size_t current_frame = vk_context->current_frame;
    memcpy(uniform_buffer_data[current_frame], &ubo, sizeof(ubo));
}

void create_depth_image(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    vk::Format depth_format = vk::Format::eD32Sfloat;
    auto &allocator = vk_context->allocator;
    auto &depth_image = vk_context->depth_image;
    auto &depth_image_allocation = vk_context->depth_image_allocation;
    spdlog::info("create depth image");
    vk::ImageCreateInfo image_create_info;
    image_create_info.setImageType(vk::ImageType::e2D);
    image_create_info.setExtent(vk::Extent3D(
        vk_context->swap_chain_details.extent.width,
        vk_context->swap_chain_details.extent.height, 1
    ));
    image_create_info.setMipLevels(1);
    image_create_info.setArrayLayers(1);
    image_create_info.setFormat(depth_format);
    image_create_info.setTiling(vk::ImageTiling::eOptimal);
    image_create_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_create_info.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
    image_create_info.setSamples(vk::SampleCountFlagBits::e1);
    VmaAllocationCreateInfo image_alloc_info = {};
    image_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    image_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    image_alloc_info.priority = 1.0f;
    if (vmaCreateImage(
            allocator, (VkImageCreateInfo *)&image_create_info,
            &image_alloc_info, (VkImage *)&depth_image, &depth_image_allocation,
            nullptr
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image!");
    }
}

void destroy_depth_image(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    auto &allocator = vk_context->allocator;
    auto &depth_image = vk_context->depth_image;
    auto &depth_image_allocation = vk_context->depth_image_allocation;
    spdlog::info("destroy depth image");
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
}

void create_depth_image_view(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    auto &device = vk_context->device;
    auto &depth_image = vk_context->depth_image;
    spdlog::info("create depth image view");
    vk::ImageViewCreateInfo view_create_info;
    view_create_info.setImage(depth_image);
    view_create_info.setViewType(vk::ImageViewType::e2D);
    view_create_info.setFormat(vk::Format::eD32Sfloat);
    view_create_info.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
    );
    vk::ImageView depth_image_view = device.createImageView(view_create_info);
    vk_context->depth_image_view = depth_image_view;
}

void destroy_depth_image_view(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    auto &device = vk_context->device;
    auto &depth_image_view = vk_context->depth_image_view;
    spdlog::info("destroy depth image view");
    device.destroyImageView(depth_image_view);
}

void create_texture_image(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    try {
        if (!renderer_query.single().has_value()) return;
        auto [renderer] = renderer_query.single().value();
        auto &allocator = vk_context->allocator;
        auto &device = vk_context->device;
        spdlog::info("create texture image");
        int tex_width, tex_height, tex_channels;
        stbi_uc *pixels = stbi_load(
            "./../assets/textures/test.png", &tex_width, &tex_height,
            &tex_channels, 4
        );
        spdlog::info("size: {}, {}", tex_width, tex_height);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }
        size_t image_size = tex_width * tex_height * 4;
        vk::BufferCreateInfo staging_buffer_create_info(
            {}, image_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive
        );
        VmaAllocationCreateInfo staging_alloc_info = {};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        vk::Buffer staging_buffer;
        VmaAllocation staging_allocation;
        if (vmaCreateBuffer(
                allocator, (VkBufferCreateInfo *)&staging_buffer_create_info,
                &staging_alloc_info, (VkBuffer *)&staging_buffer,
                &staging_allocation, nullptr
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create staging buffer!");
        }
        void *data;
        vmaMapMemory(allocator, staging_allocation, &data);
        memcpy(data, pixels, image_size);
        vmaUnmapMemory(allocator, staging_allocation);
        stbi_image_free(pixels);

        vk::ImageCreateInfo image_create_info;
        image_create_info.setImageType(vk::ImageType::e2D);
        image_create_info.setExtent(vk::Extent3D(tex_width, tex_height, 1));
        image_create_info.setMipLevels(1);
        image_create_info.setArrayLayers(1);
        image_create_info.setFormat(vk::Format::eR8G8B8A8Srgb);
        image_create_info.setTiling(vk::ImageTiling::eOptimal);
        image_create_info.setInitialLayout(vk::ImageLayout::eUndefined);
        image_create_info.setUsage(
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled
        );
        image_create_info.setSamples(vk::SampleCountFlagBits::e1);
        VmaAllocationCreateInfo image_alloc_info = {};
        image_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        image_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        image_alloc_info.priority = 1.0f;
        vk::Image texture_image;
        VmaAllocation texture_image_allocation;
        if (vmaCreateImage(
                allocator, (VkImageCreateInfo *)&image_create_info,
                &image_alloc_info, (VkImage *)&texture_image,
                &texture_image_allocation, nullptr
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image!");
        }
        renderer.texture_image = texture_image;
        renderer.texture_image_allocation = texture_image_allocation;

        // Transition image layout
        vk::CommandBufferAllocateInfo allocate_info(
            vk_context->command_pool, vk::CommandBufferLevel::ePrimary, 1
        );
        vk::CommandBuffer command_buffer =
            device.allocateCommandBuffers(allocate_info).front();
        command_buffer.begin(vk::CommandBufferBeginInfo{});
        vk::ImageMemoryBarrier barrier;
        barrier.setOldLayout(vk::ImageLayout::eUndefined);
        barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(texture_image);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        ));
        barrier.setSrcAccessMask({});
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier
        );
        command_buffer.end();
        vk::SubmitInfo submit_info;
        submit_info.setCommandBuffers(command_buffer);
        vk_context->queues.graphics_queue.submit(submit_info, nullptr);
        vk_context->queues.graphics_queue.waitIdle();
        device.freeCommandBuffers(vk_context->command_pool, command_buffer);
        // Copy buffer to image
        vk::CommandBufferAllocateInfo allocate_info2(
            vk_context->command_pool, vk::CommandBufferLevel::ePrimary, 1
        );
        command_buffer = device.allocateCommandBuffers(allocate_info2).front();
        command_buffer.begin(vk::CommandBufferBeginInfo{});
        vk::BufferImageCopy region;
        region.setBufferOffset(0);
        region.setBufferRowLength(0);
        region.setBufferImageHeight(0);
        region.setImageSubresource(
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
        );
        region.setImageOffset({0, 0, 0});
        region.setImageExtent({(uint32_t)tex_width, (uint32_t)tex_height, 1u});
        command_buffer.copyBufferToImage(
            staging_buffer, renderer.texture_image,
            vk::ImageLayout::eTransferDstOptimal, region
        );
        command_buffer.end();
        submit_info.setCommandBuffers(command_buffer);
        vk_context->queues.graphics_queue.submit(submit_info, nullptr);
        vk_context->queues.graphics_queue.waitIdle();
        device.freeCommandBuffers(vk_context->command_pool, command_buffer);
        // transition image layout
        vk::CommandBufferAllocateInfo allocate_info3(
            vk_context->command_pool, vk::CommandBufferLevel::ePrimary, 1
        );
        command_buffer = device.allocateCommandBuffers(allocate_info3).front();
        command_buffer.begin(vk::CommandBufferBeginInfo{});
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.setImage(texture_image);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        ));
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier
        );
        command_buffer.end();
        submit_info.setCommandBuffers(command_buffer);
        vk_context->queues.graphics_queue.submit(submit_info, nullptr);
        vk_context->queues.graphics_queue.waitIdle();
        device.freeCommandBuffers(vk_context->command_pool, command_buffer);
        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    } catch (const std::exception &e) {
        spdlog::error("error at create texture image : {}", e.what());
    }
}

void destroy_texture_image(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &allocator = vk_context->allocator;
    vmaDestroyImage(
        allocator, renderer.texture_image, renderer.texture_image_allocation
    );
}

void create_texture_image_view(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("create texture image view");
    vk::ImageViewCreateInfo view_info;
    view_info.setImage(renderer.texture_image);
    view_info.setViewType(vk::ImageViewType::e2D);
    view_info.setFormat(vk::Format::eR8G8B8A8Srgb);
    view_info.setSubresourceRange(
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    vk::ImageView texture_image_view = device.createImageView(view_info);
    renderer.texture_image_view = texture_image_view;
}

void destroy_texture_image_view(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("destroy texture image view");
    device.destroyImageView(renderer.texture_image_view);
}

void create_texture_image_sampler(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("create texture image sampler");
    vk::SamplerCreateInfo sampler_info;
    sampler_info.setMagFilter(vk::Filter::eLinear);
    sampler_info.setMinFilter(vk::Filter::eLinear);
    sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAnisotropyEnable(VK_TRUE);
    vk::PhysicalDeviceProperties properties =
        vk_context->physical_device.getProperties();
    sampler_info.setMaxAnisotropy(properties.limits.maxSamplerAnisotropy);
    sampler_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    sampler_info.setUnnormalizedCoordinates(VK_FALSE);
    sampler_info.setCompareEnable(VK_FALSE);
    sampler_info.setCompareOp(vk::CompareOp::eAlways);
    sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    sampler_info.setMipLodBias(0.0f);
    sampler_info.setMinLod(0.0f);
    sampler_info.setMaxLod(0.0f);
    vk::Sampler texture_sampler = device.createSampler(sampler_info);
    renderer.texture_sampler = texture_sampler;
}

void destroy_texture_image_sampler(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("destroy texture image sampler");
    device.destroySampler(renderer.texture_sampler);
}

void create_descriptor_pool(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("create descriptor pool");
    auto pool_sizes = {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT
        ),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT
        )
    };
    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.setPoolSizes(pool_sizes);
    pool_info.setMaxSets(MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorPool descriptor_pool = device.createDescriptorPool(pool_info);
    renderer.descriptor_pool = descriptor_pool;
}

void destroy_descriptor_pool(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    spdlog::info("destroy descriptor pool");
    device.destroyDescriptorPool(renderer.descriptor_pool);
}

void create_descriptor_sets(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &descriptor_set_layout = renderer.descriptor_set_layout;
    auto &descriptor_pool = renderer.descriptor_pool;
    auto &uniform_buffers = renderer.uniform_buffers;
    spdlog::info("create descriptor sets");
    std::vector<vk::DescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT, descriptor_set_layout
    );
    vk::DescriptorSetAllocateInfo allocate_info;
    allocate_info.setDescriptorPool(descriptor_pool);
    allocate_info.setSetLayouts(layouts);
    std::vector<vk::DescriptorSet> descriptor_sets =
        device.allocateDescriptorSets(allocate_info);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo buffer_info(
            uniform_buffers[i], 0, sizeof(UniformBufferObject)
        );
        vk::DescriptorImageInfo image_info(
            renderer.texture_sampler, renderer.texture_image_view,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        vk::WriteDescriptorSet descriptor_write;
        descriptor_write.setDstSet(descriptor_sets[i]);
        descriptor_write.setDstBinding(0);
        descriptor_write.setDstArrayElement(0);
        descriptor_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_write.setDescriptorCount(1);
        descriptor_write.setPBufferInfo(&buffer_info);
        vk::WriteDescriptorSet descriptor_write2;
        descriptor_write2.setDstSet(descriptor_sets[i]);
        descriptor_write2.setDstBinding(1);
        descriptor_write2.setDstArrayElement(0);
        descriptor_write2.setDescriptorType(
            vk::DescriptorType::eCombinedImageSampler
        );
        descriptor_write2.setDescriptorCount(1);
        descriptor_write2.setPImageInfo(&image_info);
        auto descriptor_writes = {descriptor_write, descriptor_write2};
        device.updateDescriptorSets(descriptor_writes, nullptr);
    }
    renderer.descriptor_sets = descriptor_sets;
}

void create_window_surface(
    Resource<VKContext> vk_context,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!window_query.single().has_value()) return;
    auto [window_handle] = window_query.single().value();
    spdlog::info("create window surface");
    vk::SurfaceKHR surface;
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(
                static_cast<VkInstance>(vk_context->instance),
                window_handle.window_handle, nullptr, &_surface
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        surface = _surface;
    }
    vk_context->surface = surface;
}

void query_swap_chain_support(Resource<VKContext> vk_context) {
    auto &device = vk_context->physical_device;
    auto &surface = vk_context->surface;
    spdlog::info("query swap chain support");
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    if (details.formats.empty() || details.presentModes.empty()) {
        spdlog::error("No swap chain support!");
        throw std::runtime_error("No swap chain support!");
    }
    vk_context->swap_chain_support = details;
}

struct WindowSize {
    int width;
    int height;
};

void create_swap_chain(
    Resource<VKContext> vk_context,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!window_query.single().has_value()) return;
    spdlog::info("create swap chain");
    auto &physical_device = vk_context->physical_device;
    auto &device = vk_context->device;
    auto &surface = vk_context->surface;
    auto &queue_family_indices = vk_context->queue_family_indices;
    auto &swap_chain_support = vk_context->swap_chain_support;
    auto [window_handle] = window_query.single().value();
    // Choose the best surface format
    auto format_iter = std::find_if(
        swap_chain_support.formats.begin(), swap_chain_support.formats.end(),
        [](vk::SurfaceFormatKHR const &format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );
    if (format_iter == swap_chain_support.formats.end()) {
        format_iter = swap_chain_support.formats.begin();
    }
    vk::SurfaceFormatKHR format = *format_iter;
    // Choose the best present mode
    auto present_mode_iter = std::find_if(
        swap_chain_support.presentModes.begin(),
        swap_chain_support.presentModes.end(),
        [](vk::PresentModeKHR const &mode) {
            return mode == vk::PresentModeKHR::eMailbox;
        }
    );
    if (present_mode_iter == swap_chain_support.presentModes.end()) {
        present_mode_iter = std::find_if(
            swap_chain_support.presentModes.begin(),
            swap_chain_support.presentModes.end(),
            [](vk::PresentModeKHR const &mode) {
                return mode == vk::PresentModeKHR::eFifo;
            }
        );
    }
    vk::PresentModeKHR present_mode = *present_mode_iter;
    // present_mode = vk::PresentModeKHR::eFifo;
    //  Choose the best extent
    vk::Extent2D extent;
    WindowSize window_size;
    glfwGetWindowSize(
        window_handle.window_handle, &window_size.width, &window_size.height
    );
    if (swap_chain_support.capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        extent = swap_chain_support.capabilities.currentExtent;
    } else {
        extent.width = std::clamp(
            (uint32_t)window_size.width,
            swap_chain_support.capabilities.minImageExtent.width,
            swap_chain_support.capabilities.maxImageExtent.width
        );
        extent.height = std::clamp(
            (uint32_t)window_size.height,
            swap_chain_support.capabilities.minImageExtent.height,
            swap_chain_support.capabilities.maxImageExtent.height
        );
    }
    // Choose the number of images in the swap chain
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }
    // Create the swap chain
    uint32_t queue_family_indices_array[] = {
        queue_family_indices.graphics_family.value(),
        queue_family_indices.present_family.value()
    };
    bool differ = queue_family_indices.graphics_family !=
                  queue_family_indices.present_family;
    vk::SwapchainCreateInfoKHR create_info;
    create_info.setSurface(surface);
    create_info.setMinImageCount(image_count);
    create_info.setImageFormat(format.format);
    create_info.setImageColorSpace(format.colorSpace);
    create_info.setImageExtent(extent);
    create_info.setImageArrayLayers(1);
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    if (differ) {
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndexCount(2);
        create_info.setPQueueFamilyIndices(queue_family_indices_array);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    create_info.setPreTransform(swap_chain_support.capabilities.currentTransform
    );
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    create_info.setPresentMode(present_mode);
    create_info.setClipped(true);
    create_info.setOldSwapchain(nullptr);
    vk::SwapchainKHR swap_chain = device.createSwapchainKHR(create_info);
    image_count = device.getSwapchainImagesKHR(swap_chain).size();
    vk_context->swap_chain_details =
        ChosenSwapChainDetails{format, present_mode, extent, image_count};
    vk_context->swap_chain = swap_chain;
}

void get_swap_chain_images(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &swap_chain = vk_context->swap_chain;
    spdlog::info("get swap chain images");
    vk_context->swap_chain_images = device.getSwapchainImagesKHR(swap_chain);
}

struct ImageViews {
    std::vector<vk::ImageView> views;
};

void create_image_views(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &images = vk_context->swap_chain_images;
    auto &swap_chain = vk_context->swap_chain_details;
    spdlog::info("create image views");
    std::vector<vk::ImageView> image_views;
    image_views.reserve(images.size());
    for (auto const &image : images) {
        vk::ImageViewCreateInfo create_info;
        create_info.image = image;
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = swap_chain.format.format;
        create_info.components = {
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity
        };
        create_info.subresourceRange = {
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        };
        image_views.push_back(device.createImageView(create_info));
    }
    vk_context->swap_chain_image_views = image_views;
}

void create_renderer(Command command) { command.spawn(Renderer{}); }

void create_render_pass(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &swap_chain_details = vk_context->swap_chain_details;
    spdlog::info("create render pass");
    vk::AttachmentDescription color_attachment(
        {}, swap_chain_details.format.format, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
    );
    vk::AttachmentDescription depth_attachment(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    vk::AttachmentReference color_attachment_ref(
        0, vk::ImageLayout::eColorAttachmentOptimal
    );
    vk::AttachmentReference depth_attachment_ref(
        1, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    vk::SubpassDescription subpass(
        {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
        &color_attachment_ref
    );
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(color_attachment_ref);
    subpass.setPDepthStencilAttachment(&depth_attachment_ref);
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = {};
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                               vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    std::vector<vk::AttachmentDescription> attachments = {
        color_attachment, depth_attachment
    };
    vk::RenderPass render_pass = device.createRenderPass(
        vk::RenderPassCreateInfo({}, attachments, subpass, dependency)
    );
    renderer.render_pass = render_pass;
}

void create_graphics_pipeline(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &swap_chain_details = vk_context->swap_chain_details;
    auto &render_pass = renderer.render_pass;
    spdlog::info("create graphics pipeline");
    // Create the shader modules and the shader stages
    auto vertex_code = std::vector<uint32_t>(
        vertex_spv, vertex_spv + sizeof(vertex_spv) / sizeof(uint32_t)
    );
    auto fragment_code = std::vector<uint32_t>(
        fragment_spv, fragment_spv + sizeof(fragment_spv) / sizeof(uint32_t)
    );
    vk::ShaderModule vertex_shader_module =
        device.createShaderModule(vk::ShaderModuleCreateInfo({}, vertex_code));
    vk::ShaderModule fragment_shader_module =
        device.createShaderModule(vk::ShaderModuleCreateInfo({}, fragment_code)
        );
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        {{}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main"}
    };
    // create dynamic state
    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info(
        {}, dynamic_states
    );
    // create vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info({}, {}, {});
    auto attribute_descriptions = Vertex::getAttributeDescriptions();
    auto binding_description = Vertex::getBindingDescription();
    vertex_input_create_info.setVertexAttributeDescriptions(
        attribute_descriptions
    );
    vertex_input_create_info.setVertexBindingDescriptions(binding_description);
    // create input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info(
        {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE
    );
    // create viewport
    auto &extent = swap_chain_details.extent;
    vk::Viewport viewport(
        0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f
    );
    vk::Rect2D scissor({0, 0}, extent);
    vk::PipelineViewportStateCreateInfo viewport_state_create_info(
        {}, 1, nullptr, 1, nullptr
    );
    // create rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer_create_info;
    rasterizer_create_info.setDepthClampEnable(VK_FALSE);
    rasterizer_create_info.setRasterizerDiscardEnable(VK_FALSE);
    rasterizer_create_info.setPolygonMode(vk::PolygonMode::eFill);
    rasterizer_create_info.setLineWidth(1.0f);
    rasterizer_create_info.setCullMode(vk::CullModeFlagBits::eFront);
    rasterizer_create_info.setFrontFace(vk::FrontFace::eClockwise);
    rasterizer_create_info.setDepthBiasEnable(VK_FALSE);
    // create multisampling
    vk::PipelineMultisampleStateCreateInfo multisample_create_info;
    multisample_create_info.sampleShadingEnable = VK_FALSE;
    multisample_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
    // create color blending
    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;
    vk::PipelineColorBlendStateCreateInfo color_blend_create_info(
        {}, VK_FALSE, vk::LogicOp::eCopy, color_blend_attachment,
        {0.0f, 0.0f, 0.0f, 0.0f}
    );
    // create pipeline layout
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    pipeline_layout_create_info.setSetLayouts(renderer.descriptor_set_layout);
    vk::PipelineLayout pipeline_layout =
        device.createPipelineLayout(pipeline_layout_create_info);
    renderer.pipeline_layout = pipeline_layout;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info;
    depth_stencil_create_info.setDepthTestEnable(VK_TRUE);
    depth_stencil_create_info.setDepthWriteEnable(VK_TRUE);
    depth_stencil_create_info.setDepthCompareOp(vk::CompareOp::eLess);
    depth_stencil_create_info.setDepthBoundsTestEnable(VK_FALSE);
    depth_stencil_create_info.setMinDepthBounds(0.0f);
    depth_stencil_create_info.setMaxDepthBounds(1.0f);
    depth_stencil_create_info.setStencilTestEnable(VK_FALSE);

    // create pipeline
    vk::GraphicsPipelineCreateInfo create_info;
    create_info.setStages(shader_stages);
    create_info.setPVertexInputState(&vertex_input_create_info);
    create_info.setPInputAssemblyState(&input_assembly_create_info);
    create_info.setPViewportState(&viewport_state_create_info);
    create_info.setPRasterizationState(&rasterizer_create_info);
    create_info.setPMultisampleState(&multisample_create_info);
    create_info.setPColorBlendState(&color_blend_create_info);
    create_info.setPDynamicState(&dynamic_state_create_info);
    create_info.setPDepthStencilState(&depth_stencil_create_info);
    create_info.setLayout(pipeline_layout);
    create_info.setRenderPass(render_pass);
    create_info.setSubpass(0);
    auto res = device.createGraphicsPipeline({}, create_info);
    if (res.result != vk::Result::eSuccess) {
        spdlog::error("Failed to create graphics pipeline!");
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    auto pipeline = res.value;
    renderer.graphics_pipeline = pipeline;

    device.destroyShaderModule(vertex_shader_module);
    device.destroyShaderModule(fragment_shader_module);
}

struct Framebuffers {
    std::vector<vk::Framebuffer> framebuffers;
};

void create_framebuffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &render_pass = renderer.render_pass;
    auto &image_views = vk_context->swap_chain_image_views;
    auto &swap_chain = vk_context->swap_chain_details;
    spdlog::info("create framebuffer");
    Framebuffers framebuffers;
    framebuffers.framebuffers.reserve(image_views.size());
    for (auto const &view : image_views) {
        auto views = {view, vk_context->depth_image_view};
        vk::FramebufferCreateInfo create_info(
            {}, render_pass, views, swap_chain.extent.width,
            swap_chain.extent.height, 1
        );
        framebuffers.framebuffers.push_back(device.createFramebuffer(create_info
        ));
    }
    vk_context->swap_chain_framebuffers = framebuffers.framebuffers;
}

void create_command_pool(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &queue_family_indices = vk_context->queue_family_indices;
    spdlog::info("create command pool");
    vk::CommandPoolCreateInfo create_info(
        {vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
        queue_family_indices.graphics_family.value()
    );
    vk::CommandPool command_pool = device.createCommandPool(create_info);
    vk_context->command_pool = command_pool;
}

void create_command_buffer(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &command_pool = vk_context->command_pool;
    spdlog::info("create command buffer");
    vk::CommandBufferAllocateInfo allocate_info(
        command_pool, vk::CommandBufferLevel::ePrimary, 1
    );
    allocate_info.setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);
    vk_context->command_buffers = device.allocateCommandBuffers(allocate_info);
}

void record_command_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &cmds = vk_context->command_buffers;
    auto &render_pass = renderer.render_pass;
    auto &swap_chain_details = vk_context->swap_chain_details;
    auto &framebuffers = vk_context->swap_chain_framebuffers;
    auto &pipeline = renderer.graphics_pipeline;
    auto &cmd = cmds[vk_context->current_frame];
    // spdlog::info("record command buffer");
    cmd.begin(vk::CommandBufferBeginInfo{});
    vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    vk::ClearValue clear_depth;
    clear_depth.setDepthStencil({1.0f, 0});
    auto clear_values = {clear_color, clear_depth};
    auto &framebuffer = framebuffers[vk_context->image_index];
    vk::RenderPassBeginInfo render_pass_begin_info(
        render_pass, framebuffer, {{0, 0}, swap_chain_details.extent},
        clear_values
    );
    cmd.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    vk::Viewport viewport(
        0.0f, 0.0f, (float)swap_chain_details.extent.width,
        (float)swap_chain_details.extent.height, 0.0f, 1.0f
    );
    vk::Rect2D scissor({0, 0}, swap_chain_details.extent);
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, scissor);
    cmd.bindVertexBuffers(0, renderer.vertex_buffer, {0});
    cmd.bindIndexBuffer(renderer.index_buffer, 0, vk::IndexType::eUint16);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, renderer.pipeline_layout, 0,
        renderer.descriptor_sets[vk_context->current_frame], {}
    );
    cmd.drawIndexed(indices.size(), 1, 0, 0, 0);
    cmd.endRenderPass();
    cmd.end();
}

void create_sync_object(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &framebuffers = vk_context->swap_chain_framebuffers;
    spdlog::info("create sync object");
    auto &image_available_semaphores = vk_context->image_available_semaphores;
    auto &render_finished_semaphores = vk_context->render_finished_semaphores;
    auto &in_flight_fences = vk_context->in_flight_fences;
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        image_available_semaphores[i] = device.createSemaphore({});
        render_finished_semaphores[i] = device.createSemaphore({});
        in_flight_fences[i] = device.createFence(
            vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled}
        );
    }
}

void cleanup_swap_chain(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &swap_chain = vk_context->swap_chain;
    auto &image_views = vk_context->swap_chain_image_views;
    auto &framebuffers = vk_context->swap_chain_framebuffers;
    spdlog::info("cleanup swap chain");
    for (auto const &framebuffer : framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    for (auto const &image_view : image_views) {
        device.destroyImageView(image_view);
    }
    device.destroySwapchainKHR(swap_chain);
}

void swap_chain_recreate(
    Resource<VKContext> vk_context,
    Query<Get<Renderer>> renderer_query,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &swap_chain = vk_context->swap_chain;
    auto &swap_chain_details = vk_context->swap_chain_details;
    auto &device = vk_context->device;
    auto &image_views = vk_context->swap_chain_image_views;
    auto &framebuffers = vk_context->swap_chain_framebuffers;
    auto &sync_objects = vk_context->image_available_semaphores;
    auto &cmd = vk_context->command_buffers[vk_context->current_frame];
    device.waitIdle();
    cleanup_swap_chain(vk_context);
    query_swap_chain_support(vk_context);
    create_swap_chain(vk_context, window_query);
    get_swap_chain_images(vk_context);
    create_image_views(vk_context);
    destroy_depth_image_view(vk_context, renderer_query);
    destroy_depth_image(vk_context, renderer_query);
    create_depth_image(vk_context, renderer_query);
    create_depth_image_view(vk_context, renderer_query);
    create_framebuffer(vk_context, renderer_query);
}

bool begin_draw(
    Resource<VKContext> vk_context,
    Query<Get<Renderer>> renderer_query,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!renderer_query.single().has_value()) return false;
    auto [renderer] = renderer_query.single().value();
    auto &in_flight_fences = vk_context->in_flight_fences;
    auto &device = vk_context->device;
    auto &swap_chain = vk_context->swap_chain;
    // spdlog::info("begin draw");
    device.waitForFences(
        in_flight_fences[vk_context->current_frame], VK_TRUE,
        std::numeric_limits<uint64_t>::max()
    );
    try {
        auto res = device.acquireNextImageKHR(
            swap_chain, std::numeric_limits<uint64_t>::max(),
            vk_context->image_available_semaphores[vk_context->current_frame],
            nullptr
        );
        device.resetFences(in_flight_fences[vk_context->current_frame]);
        vk_context->image_index = res.value;
        auto &cmd = vk_context->command_buffers[vk_context->current_frame];
        cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        return true;
    } catch (vk::OutOfDateKHRError const &e) {
        spdlog::info("swap chain out of date : {}", e.what());
        swap_chain_recreate(vk_context, renderer_query, window_query);
        return false;
    }
}

void end_draw(
    Resource<VKContext> vk_context,
    Query<Get<Renderer>> renderer_query,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &swap_chain = vk_context->swap_chain;
    auto &cmds = vk_context->command_buffers;
    auto &queues = vk_context->queues;
    auto &cmd = cmds[vk_context->current_frame];
    // spdlog::info("end draw");
    auto image_index = vk_context->image_index;
    vk::SubmitInfo submit_info;
    vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submit_info.setWaitSemaphores(
        vk_context->image_available_semaphores[vk_context->current_frame]
    );
    submit_info.setWaitDstStageMask(wait_stages);
    submit_info.setCommandBuffers(cmd);
    submit_info.setSignalSemaphores(
        vk_context->render_finished_semaphores[vk_context->current_frame]
    );
    auto &graphics_queue = queues.graphics_queue;
    auto &present_queue = queues.present_queue;
    graphics_queue.submit(
        submit_info, vk_context->in_flight_fences[vk_context->current_frame]
    );
    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphores(
        vk_context->render_finished_semaphores[vk_context->current_frame]
    );
    present_info.setSwapchains(swap_chain);
    present_info.setImageIndices(image_index);
    try {
        auto res = present_queue.presentKHR(present_info);
    } catch (vk::OutOfDateKHRError const &e) {
        spdlog::info("swap chain out of date : {}", e.what());
        swap_chain_recreate(vk_context, renderer_query, window_query);
    }
    vk_context->current_frame =
        (vk_context->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void draw_frame(
    Resource<VKContext> vk_context,
    Query<Get<Renderer>> renderer_query,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query
) {
    if (!begin_draw(vk_context, renderer_query, window_query)) {
        return;
    }
    update_uniform_buffer(vk_context, renderer_query);
    record_command_buffer(vk_context, renderer_query);
    end_draw(vk_context, renderer_query, window_query);
}

void destroy_sync_objects(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    for (auto const &semaphore : vk_context->image_available_semaphores) {
        device.destroySemaphore(semaphore);
    }
    for (auto const &fence : vk_context->in_flight_fences) {
        device.destroyFence(fence);
    }
    for (auto const &semaphore : vk_context->render_finished_semaphores) {
        device.destroySemaphore(semaphore);
    }
}

void destroy_command_buffer(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &command_pool = vk_context->command_pool;
    auto &command_buffers = vk_context->command_buffers;
    spdlog::info("destroy command buffer");
    device.freeCommandBuffers(command_pool, command_buffers);
}

void destroy_command_pool(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &command_pool = vk_context->command_pool;
    spdlog::info("destroy command pool");
    device.destroyCommandPool(command_pool);
}

void destroy_framebuffers(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &framebuffers = vk_context->swap_chain_framebuffers;
    spdlog::info("destroy framebuffers");
    for (auto const &framebuffer : framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
}

void destroy_pipeline(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &pipeline = renderer.graphics_pipeline;
    auto &pipeline_layout = renderer.pipeline_layout;
    spdlog::info("destroy pipeline");
    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(pipeline_layout);
}

void destroy_render_pass(
    Resource<VKContext> vk_context, Query<Get<Renderer>> renderer_query
) {
    if (!renderer_query.single().has_value()) return;
    auto [renderer] = renderer_query.single().value();
    auto &device = vk_context->device;
    auto &render_pass = renderer.render_pass;
    spdlog::info("destroy render pass");
    device.destroyRenderPass(render_pass);
}

void destroy_image_views(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &image_views = vk_context->swap_chain_image_views;
    spdlog::info("destroy image views");
    for (auto const &view : image_views) {
        device.destroyImageView(view);
    }
}

void destroy_swap_chain(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    auto &swap_chain = vk_context->swap_chain;
    spdlog::info("destroy swap chain");
    device.destroySwapchainKHR(swap_chain);
}

void destroy_surface(Resource<VKContext> vk_context) {
    auto &instance = vk_context->instance;
    auto &surface = vk_context->surface;
    spdlog::info("destroy surface");
    instance.destroySurfaceKHR(surface);
}

void destroy_device(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    spdlog::info("destroy device");
    device.destroy();
}

void destroy_instance(Resource<VKContext> vk_context) {
    auto &instance = vk_context->instance;
    spdlog::info("destroy instance");
    instance.destroy();
}

void wait_for_device(Resource<VKContext> vk_context) {
    auto &device = vk_context->device;
    spdlog::info("wait for device");
    device.waitIdle();
}

struct VK_TrialPlugin : Plugin {
    void build(App &app) override {
        auto window_plugin = app.get_plugin<WindowPlugin>();
        window_plugin->set_primary_window_hints({{GLFW_CLIENT_API, GLFW_NO_API}}
        );

        app.enable_loop();
        app.add_system(Startup(), init_instance).use_worker("single");
        app.add_system(PreStartup(), insert_vk_context).use_worker("single");
        app.add_system(PreStartup(), create_renderer).use_worker("single");
        app.add_system(Startup(), get_physical_device, after(init_instance))
            .use_worker("single");
        app.add_system(
               Startup(), create_window_surface, after(get_physical_device)
        )
            .use_worker("single");
        app.add_system(Startup(), create_device, after(create_window_surface))
            .use_worker("single");
        app.add_system(Startup(), create_vma_allocator, after(create_device))
            .before(create_swap_chain)
            .use_worker("single");
        app.add_system(
               Startup(), create_vertex_buffer,
               after(create_vma_allocator, create_command_pool)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_index_buffer,
               after(create_vma_allocator, create_command_pool)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_uniform_buffers, after(create_vma_allocator)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_uniform_buffers,
               before(destroy_vma_allocator), after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(Startup(), create_depth_image)
            .after(create_vma_allocator)
            .after(create_swap_chain)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_depth_image)
            .after(wait_for_device)
            .before(destroy_vma_allocator)
            .use_worker("single");
        app.add_system(Startup(), create_depth_image_view)
            .after(create_depth_image)
            .before(create_framebuffer)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_depth_image_view)
            .after(wait_for_device)
            .before(destroy_depth_image)
            .use_worker("single");
        app.add_system(Startup(), create_texture_image)
            .after(create_vma_allocator, create_command_pool)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_texture_image)
            .after(wait_for_device)
            .before(destroy_vma_allocator)
            .use_worker("single");
        app.add_system(Startup(), create_texture_image_view)
            .after(create_texture_image)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_texture_image_view)
            .after(wait_for_device)
            .before(destroy_texture_image)
            .use_worker("single");
        app.add_system(Startup(), create_texture_image_sampler)
            .after(create_texture_image_view)
            .before(create_descriptor_sets)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_texture_image_sampler)
            .after(wait_for_device)
            .before(destroy_texture_image_view)
            .use_worker("single");
        app.add_system(
               Startup(), create_descriptor_set_layout, after(create_device)
        )
            .before(create_descriptor_pool)
            .before(create_graphics_pipeline)
            .use_worker("single");
        app.add_system(Shutdown(), destroy_descriptor_set_layout)
            .before(destroy_device)
            .after(wait_for_device)
            .use_worker("single");
        app.add_system(Startup(), create_descriptor_pool, after(create_device))
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_descriptor_pool,
               before(destroy_device, destroy_descriptor_set_layout),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_descriptor_sets,
               after(create_descriptor_pool, create_uniform_buffers)
        )
            .use_worker("single");
        app.add_system(
               Startup(), query_swap_chain_support, after(create_device)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_swap_chain, after(query_swap_chain_support)
        )
            .use_worker("single");
        app.add_system(
               Startup(), get_swap_chain_images, after(create_swap_chain)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_image_views, after(get_swap_chain_images)
        )
            .use_worker("single");
        app.add_system(Startup(), create_render_pass, after(create_image_views))
            .use_worker("single");
        app.add_system(
               Startup(), create_graphics_pipeline, after(create_render_pass)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_framebuffer, after(create_graphics_pipeline)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_command_pool, after(create_framebuffer)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_command_buffer, after(create_command_pool)
        )
            .use_worker("single");
        app.add_system(
               Startup(), create_sync_object, after(create_command_buffer)
        )
            .use_worker("single");
        app.add_system(Render(), draw_frame).use_worker("single");
        app.add_system(Shutdown(), wait_for_device).use_worker("single");
        app.add_system(Shutdown(), destroy_instance).use_worker("single");
        app.add_system(
               Shutdown(), destroy_device, before(destroy_instance),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_surface, before(destroy_instance),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_vma_allocator, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), free_vertex_buffer, before(destroy_vma_allocator),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), free_index_buffer, before(destroy_vma_allocator),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_pipeline, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_image_views, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_render_pass, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_framebuffers, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_swap_chain,
               before(destroy_device, destroy_surface), after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_command_pool, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_command_buffer,
               before(destroy_device, destroy_command_pool),
               after(wait_for_device)
        )
            .use_worker("single");
        app.add_system(
               Shutdown(), destroy_sync_objects, before(destroy_device),
               after(wait_for_device)
        )
            .use_worker("single");
    }
};

void run() {
    App app;
    app.add_plugin(pixel_engine::window::WindowPlugin{});
    app.add_plugin(vk_trial::VK_TrialPlugin{});
    app.run();
}
}  // namespace vk_trial