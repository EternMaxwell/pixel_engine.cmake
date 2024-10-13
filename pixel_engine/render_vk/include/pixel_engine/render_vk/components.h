#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <vector>

#include "vulkan/device.h"
#include "vulkan_headers.h"

namespace pixel_engine {
namespace render_vk {
namespace components {
struct RenderContext {};
static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData
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

    auto* logger = (spdlog::logger*)pUserData;

    std::string msg = std::format(
        "{}: {}: {}\n",
        vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(
            messageSeverity
        )),
        vk::to_string(
            static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)
        ),
        pCallbackData->pMessage
    );

    msg += std::format(
        "\tmessageIDName   = <{}>\n\tmessageIdNumber = {}\n\tmessage         = "
        "<{}>\n",
        pCallbackData->pMessageIdName, pCallbackData->messageIdNumber,
        pCallbackData->pMessage
    );

    if (0 < pCallbackData->queueLabelCount) {
        msg += std::format(
            "\tqueueLabelCount = {}\n", pCallbackData->queueLabelCount
        );
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            msg += std::format(
                "\t\tlabelName = <{}>\n",
                pCallbackData->pQueueLabels[i].pLabelName
            );
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        msg += std::format(
            "\tcmdBufLabelCount = {}\n", pCallbackData->cmdBufLabelCount
        );
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            msg += std::format(
                "\t\tlabelName = <{}>\n",
                pCallbackData->pCmdBufLabels[i].pLabelName
            );
        }
    }
    if (0 < pCallbackData->objectCount) {
        msg += std::format("\tobjectCount = {}\n", pCallbackData->objectCount);
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            msg += std::format(
                "\t\tobjectType = {}\n",
                vk::to_string(static_cast<vk::ObjectType>(
                    pCallbackData->pObjects[i].objectType
                ))
            );
            msg += std::format(
                "\t\tobjectHandle = {}\n",
                pCallbackData->pObjects[i].objectHandle
            );
            if (pCallbackData->pObjects[i].pObjectName) {
                msg += std::format(
                    "\t\tobjectName = <{}>\n",
                    pCallbackData->pObjects[i].pObjectName
                );
            }
        }
    }

    logger->warn(msg);

    return vk::False;
}

struct Instance {
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debug_messenger;
    std::shared_ptr<spdlog::logger> logger;

    static Instance create(
        const char* app_name,
        uint32_t app_version,
        std::shared_ptr<spdlog::logger> logger
    ) {
        Instance instance;
        instance.logger = logger;
        auto app_info = vk::ApplicationInfo()
                            .setPApplicationName(app_name)
                            .setApplicationVersion(app_version)
                            .setPEngineName("Pixel Engine")
                            .setEngineVersion(VK_MAKE_VERSION(0, 1, 0))
                            .setApiVersion(VK_API_VERSION_1_3);
        std::vector<char const*> layers = {
#if !defined(NDEBUG)
            "VK_LAYER_KHRONOS_validation"
#endif
        };
        std::vector<char const*> extensions = {
#if !defined(NDEBUG)
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
        };
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        extensions.insert(
            extensions.end(), glfwExtensions,
            glfwExtensions + glfwExtensionCount
        );
        auto instance_info = vk::InstanceCreateInfo()
                                 .setPApplicationInfo(&app_info)
                                 .setPEnabledExtensionNames(extensions)
                                 .setPEnabledLayerNames(layers);

        logger->info("Creating Vulkan Instance");

        std::string instance_layers_info = "Instance Layers:\n";
        for (auto& layer : layers) {
            instance_layers_info += std::format("\t{}\n", layer);
        }
        logger->info(instance_layers_info);
        std::string instance_extensions_info = "Instance Extensions:\n";
        for (auto& extension : extensions) {
            instance_extensions_info += std::format("\t{}\n", extension);
        }
        logger->info(instance_extensions_info);

        instance.instance = vk::createInstance(instance_info);
#if !defined(NDEBUG)
        auto debugMessengerInfo =
            vk::DebugUtilsMessengerCreateInfoEXT()
                .setMessageSeverity(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                )
                .setMessageType(
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                )
                .setPfnUserCallback(debugUtilsMessengerCallback)
                .setPUserData(logger.get());
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(
                instance.instance, "vkCreateDebugUtilsMessengerEXT"
            );
        if (!func) {
            logger->error(
                "Failed to load function vkCreateDebugUtilsMessengerEXT"
            );
            throw std::runtime_error(
                "Failed to load function vkCreateDebugUtilsMessengerEXT"
            );
        }
        func(
            instance.instance,
            reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(
                &debugMessengerInfo
            ),
            nullptr,
            reinterpret_cast<VkDebugUtilsMessengerEXT*>(
                &instance.debug_messenger
            )
        );
#endif
        return instance;
    }

    void destroy() {
        if (debug_messenger) {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT")
            );
            if (func) {
                func(instance, debug_messenger, nullptr);
            }
        }
        instance.destroy();
    }

    operator vk::Instance() const { return instance; }

    operator VkInstance() const { return instance; }

    bool operator!() const { return !instance; }

    auto* operator->() { return &instance; }

    auto operator*() { return instance; }
};

struct PhysicalDevice {
    vk::PhysicalDevice physical_device;

    static PhysicalDevice create(Instance instance) {
        PhysicalDevice physical_device;
        auto physical_devices = instance->enumeratePhysicalDevices();
        if (physical_devices.size() == 0) {
            instance.logger->error("No physical devices found");
            throw std::runtime_error("No physical devices found");
        }
        physical_device.physical_device = physical_devices[0];
        return physical_device;
    }

    operator vk::PhysicalDevice() const { return physical_device; }

    operator VkPhysicalDevice() const { return physical_device; }

    bool operator!() const { return !physical_device; }

    auto* operator->() { return &physical_device; }

    auto operator*() { return physical_device; }
};

struct Device {
    vk::Device device;
    VmaAllocator allocator;
    uint32_t queue_family_index;

    static Device create(
        Instance& instance,
        PhysicalDevice& physical_device,
        vk::QueueFlags queue_flags = vk::QueueFlagBits::eGraphics
    ) {
        Device device;
        float queue_priority = 1.0f;
        uint32_t queue_family_index = 0;
        for (auto& queue_family_property :
             physical_device->getQueueFamilyProperties()) {
            if (queue_family_property.queueFlags & queue_flags) {
                break;
            }
            queue_family_index++;
        }
        if (queue_family_index ==
            physical_device->getQueueFamilyProperties().size()) {
            instance.logger->error("No queue family with graphics support");
            throw std::runtime_error("No queue family with graphics support");
        }
        device.queue_family_index = queue_family_index;
        auto queue_create_info = vk::DeviceQueueCreateInfo()
                                     .setQueueFamilyIndex(queue_family_index)
                                     .setQueueCount(1)
                                     .setPQueuePriorities(&queue_priority);
        std::vector<char const*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };
        auto device_features =
            vk::PhysicalDeviceFeatures().setSamplerAnisotropy(VK_TRUE);
        auto dynamic_rendering_features =
            vk::PhysicalDeviceDynamicRenderingFeaturesKHR().setDynamicRendering(
                VK_TRUE
            );
        auto device_info = vk::DeviceCreateInfo()
                               .setQueueCreateInfos(queue_create_info)
                               .setPEnabledFeatures(&device_features)
                               .setPNext(&dynamic_rendering_features)
                               .setPEnabledExtensionNames(device_extensions);
        device.device = physical_device->createDevice(device_info);
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.physicalDevice = physical_device;
        allocator_info.device = device.device;
        allocator_info.instance = instance.instance;
        vmaCreateAllocator(&allocator_info, &device.allocator);
        return device;
    }

    void destroy() {
        vmaDestroyAllocator(allocator);
        device.destroy();
    }

    operator vk::Device() const { return device; }

    operator VkDevice() const { return device; }

    bool operator!() const { return !device; }

    auto* operator->() { return &device; }

    auto operator*() { return device; }
};

struct Queue {
    vk::Queue queue;

    static Queue create(Device& device) {
        Queue queue;
        queue.queue = device->getQueue(device.queue_family_index, 0);
        return queue;
    }

    operator vk::Queue() const { return queue; }

    operator VkQueue() const { return queue; }

    bool operator!() const { return !queue; }

    auto* operator->() { return &queue; }

    auto operator*() { return queue; }
};

struct CommandPool {
    vk::CommandPool command_pool;

    static CommandPool create(Device& device) {
        auto command_pool_info =
            vk::CommandPoolCreateInfo()
                .setQueueFamilyIndex(device.queue_family_index)
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        CommandPool command_pool;
        command_pool.command_pool =
            device->createCommandPool(command_pool_info);
        return command_pool;
    }

    void destroy(Device device) { device->destroyCommandPool(command_pool); }

    operator vk::CommandPool() const { return command_pool; }

    operator VkCommandPool() const { return command_pool; }

    bool operator!() const { return !command_pool; }

    auto* operator->() { return &command_pool; }

    auto operator*() { return command_pool; }
};

struct CommandBuffer {
    vk::CommandBuffer command_buffer;

    static CommandBuffer allocate_primary(
        Device& device, CommandPool& command_pool
    ) {
        auto command_buffer = device->allocateCommandBuffers(
            vk::CommandBufferAllocateInfo()
                .setCommandPool(command_pool)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1)
        )[0];
        return CommandBuffer{command_buffer};
    }

    static CommandBuffer allocate_secondary(
        Device& device, CommandPool& command_pool
    ) {
        auto command_buffer = device->allocateCommandBuffers(
            vk::CommandBufferAllocateInfo()
                .setCommandPool(command_pool)
                .setLevel(vk::CommandBufferLevel::eSecondary)
                .setCommandBufferCount(1)
        )[0];
        return CommandBuffer{command_buffer};
    }

    void free(Device& device, CommandPool& command_pool) {
        device->freeCommandBuffers(command_pool, command_buffer);
    }

    operator vk::CommandBuffer() const { return command_buffer; }

    operator VkCommandBuffer() const { return command_buffer; }

    bool operator!() const { return !command_buffer; }

    auto* operator->() { return &command_buffer; }

    auto& operator*() { return command_buffer; }
};

struct AllocationCreateInfo {
    VmaAllocationCreateInfo create_info;

    AllocationCreateInfo() { create_info = {}; }

    operator VmaAllocationCreateInfo() const { return create_info; }

    auto& setUsage(VmaMemoryUsage usage) {
        create_info.usage = usage;
        return *this;
    }

    auto& setFlags(VmaAllocationCreateFlags flags) {
        create_info.flags = flags;
        return *this;
    }

    auto& setRequiredFlags(VkMemoryPropertyFlags flags) {
        create_info.requiredFlags = flags;
        return *this;
    }

    auto& setPreferredFlags(VkMemoryPropertyFlags flags) {
        create_info.preferredFlags = flags;
        return *this;
    }

    auto& setMemoryTypeBits(uint32_t bits) {
        create_info.memoryTypeBits = bits;
        return *this;
    }

    auto& setPool(VmaPool pool) {
        create_info.pool = pool;
        return *this;
    }

    auto& setPUserData(void* data) {
        create_info.pUserData = data;
        return *this;
    }

    auto& setPriority(float priority) {
        create_info.priority = priority;
        return *this;
    }

    auto operator*() { return create_info; }
};

struct Buffer {
    vk::Buffer buffer;
    VmaAllocation allocation;

    static Buffer create(
        Device& device,
        vk::BufferCreateInfo create_info,
        AllocationCreateInfo alloc_info
    ) {
        Buffer buffer;
        vmaCreateBuffer(
            device.allocator,
            reinterpret_cast<VkBufferCreateInfo*>(&create_info),
            reinterpret_cast<VmaAllocationCreateInfo*>(&alloc_info),
            reinterpret_cast<VkBuffer*>(&buffer.buffer), &buffer.allocation,
            nullptr
        );
        return buffer;
    }

    void destroy(Device& device) {
        vmaDestroyBuffer(device.allocator, buffer, allocation);
    }

    void* map(Device& device) {
        void* data;
        vmaMapMemory(device.allocator, allocation, &data);
        return data;
    }

    void unmap(Device& device) { vmaUnmapMemory(device.allocator, allocation); }

    operator vk::Buffer() const { return buffer; }

    operator VkBuffer() const { return buffer; }

    bool operator!() const { return !buffer; }

    auto* operator->() { return &buffer; }

    auto operator*() { return buffer; }
};

struct Image {
    vk::Image image;
    VmaAllocation allocation;

    static Image create(
        Device& device,
        vk::ImageCreateInfo create_info,
        AllocationCreateInfo alloc_info
    ) {
        Image image;
        vmaCreateImage(
            device.allocator,
            reinterpret_cast<VkImageCreateInfo*>(&create_info),
            reinterpret_cast<VmaAllocationCreateInfo*>(&alloc_info),
            reinterpret_cast<VkImage*>(&image.image), &image.allocation, nullptr
        );
        return image;
    }

    void destroy(Device& device) {
        vmaDestroyImage(device.allocator, image, allocation);
    }

    operator vk::Image() const { return image; }

    operator VkImage() const { return image; }

    bool operator!() const { return !image; }

    auto* operator->() { return &image; }

    auto operator*() { return image; }
};

struct ImageView {
    vk::ImageView image_view;

    static ImageView create(
        Device& device, vk::ImageViewCreateInfo create_info
    ) {
        ImageView image_view;
        image_view.image_view = device->createImageView(create_info);
        return image_view;
    }

    void destroy(Device& device) { device->destroyImageView(image_view); }

    operator vk::ImageView() const { return image_view; }

    operator VkImageView() const { return image_view; }

    bool operator!() const { return !image_view; }

    auto* operator->() { return &image_view; }

    auto operator*() { return image_view; }
};

struct Sampler {
    vk::Sampler sampler;

    static Sampler create(Device& device, vk::SamplerCreateInfo create_info) {
        Sampler sampler;
        sampler.sampler = device->createSampler(create_info);
        return sampler;
    }

    void destroy(Device& device) { device->destroySampler(sampler); }

    operator vk::Sampler() const { return sampler; }

    operator VkSampler() const { return sampler; }

    bool operator!() const { return !sampler; }

    auto* operator->() { return &sampler; }

    auto operator*() { return sampler; }
};

struct DescriptorSetLayout {
    vk::DescriptorSetLayout descriptor_set_layout;

    static DescriptorSetLayout create(
        Device& device, vk::DescriptorSetLayoutCreateInfo create_info
    ) {
        DescriptorSetLayout descriptor_set_layout;
        descriptor_set_layout.descriptor_set_layout =
            device->createDescriptorSetLayout(create_info);
        return descriptor_set_layout;
    }

    void destroy(Device& device) {
        device->destroyDescriptorSetLayout(descriptor_set_layout);
    }

    operator vk::DescriptorSetLayout() const { return descriptor_set_layout; }

    operator VkDescriptorSetLayout() const { return descriptor_set_layout; }

    bool operator!() const { return !descriptor_set_layout; }

    auto* operator->() { return &descriptor_set_layout; }

    auto& operator*() { return descriptor_set_layout; }
};

struct PipelineLayout {
    vk::PipelineLayout pipeline_layout;

    static PipelineLayout create(
        Device& device, vk::PipelineLayoutCreateInfo create_info
    ) {
        PipelineLayout pipeline_layout;
        pipeline_layout.pipeline_layout =
            device->createPipelineLayout(create_info);
        return pipeline_layout;
    }

    void destroy(Device& device) {
        device->destroyPipelineLayout(pipeline_layout);
    }

    operator vk::PipelineLayout() const { return pipeline_layout; }

    operator VkPipelineLayout() const { return pipeline_layout; }

    bool operator!() const { return !pipeline_layout; }

    auto* operator->() { return &pipeline_layout; }
};

struct Pipeline {
    vk::Pipeline pipeline;

    static Pipeline create(
        Device& device,
        vk::PipelineCache pipeline_cache,
        vk::GraphicsPipelineCreateInfo create_info
    ) {
        Pipeline pipeline;
        pipeline.pipeline =
            device->createGraphicsPipeline(pipeline_cache, create_info).value;
        return pipeline;
    }

    static Pipeline create(
        Device& device, vk::GraphicsPipelineCreateInfo create_info
    ) {
        return create(device, vk::PipelineCache(), create_info);
    }

    void destroy(Device& device) { device->destroyPipeline(pipeline); }

    operator vk::Pipeline() const { return pipeline; }

    operator VkPipeline() const { return pipeline; }

    bool operator!() const { return !pipeline; }

    auto* operator->() { return &pipeline; }
};

struct DescriptorPool {
    vk::DescriptorPool descriptor_pool;

    static DescriptorPool create(
        Device& device, vk::DescriptorPoolCreateInfo create_info
    ) {
        DescriptorPool descriptor_pool;
        descriptor_pool.descriptor_pool =
            device->createDescriptorPool(create_info);
        return descriptor_pool;
    }

    void destroy(Device& device) {
        device->destroyDescriptorPool(descriptor_pool);
    }

    operator vk::DescriptorPool() const { return descriptor_pool; }

    operator VkDescriptorPool() const { return descriptor_pool; }

    bool operator!() const { return !descriptor_pool; }

    auto* operator->() { return &descriptor_pool; }
};

struct DescriptorSet {
    vk::DescriptorSet descriptor_set;

    static DescriptorSet create(
        Device& device, vk::DescriptorSetAllocateInfo allocate_info
    ) {
        DescriptorSet descriptor_set;
        descriptor_set.descriptor_set =
            device->allocateDescriptorSets(allocate_info)[0];
        return descriptor_set;
    }

    static DescriptorSet create(
        Device& device,
        DescriptorPool& descriptor_pool,
        vk::DescriptorSetLayout descriptor_set_layout
    ) {
        vk::DescriptorSetAllocateInfo allocate_info;
        allocate_info.setDescriptorPool(descriptor_pool)
            .setSetLayouts(descriptor_set_layout);
        return create(device, allocate_info);
    }

    void destroy(Device& device, DescriptorPool& descriptor_pool) {
        device->freeDescriptorSets(descriptor_pool, descriptor_set);
    }

    operator vk::DescriptorSet() const { return descriptor_set; }

    operator VkDescriptorSet() const { return descriptor_set; }

    bool operator!() const { return !descriptor_set; }

    auto* operator->() { return &descriptor_set; }
};

struct Fence {
    vk::Fence fence;

    static Fence create(Device& device, vk::FenceCreateInfo create_info) {
        Fence fence;
        fence.fence = device->createFence(create_info);
        return fence;
    }

    void destroy(Device& device) { device->destroyFence(fence); }

    operator vk::Fence() const { return fence; }

    operator VkFence() const { return fence; }

    bool operator!() const { return !fence; }

    auto* operator->() { return &fence; }
};

struct Semaphore {
    vk::Semaphore semaphore;

    static Semaphore create(
        Device& device, vk::SemaphoreCreateInfo create_info
    ) {
        Semaphore semaphore;
        semaphore.semaphore = device->createSemaphore(create_info);
        return semaphore;
    }

    void destroy(Device& device) { device->destroySemaphore(semaphore); }

    operator vk::Semaphore() const { return semaphore; }

    operator VkSemaphore() const { return semaphore; }

    bool operator!() const { return !semaphore; }

    auto* operator->() { return &semaphore; }
};

struct Surface {
    vk::SurfaceKHR surface;

    static Surface create(Instance& instance, GLFWwindow* window) {
        Surface surface_component;
        if (glfwCreateWindowSurface(
                instance, window, nullptr,
                reinterpret_cast<VkSurfaceKHR*>(&surface_component.surface)
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
        return surface_component;
    }

    void destroy(Instance& instance) { instance->destroySurfaceKHR(surface); }

    operator vk::SurfaceKHR() const { return surface; }

    operator VkSurfaceKHR() const { return surface; }

    bool operator!() const { return !surface; }

    auto* operator->() { return &surface; }
};

struct Swapchain {
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<ImageView> image_views;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    uint32_t image_index = 0;
    uint32_t current_frame = 0;

    vk::Semaphore image_available_semaphore[2];
    vk::Semaphore render_finished_semaphore[2];
    vk::Fence in_flight_fence[2];

    static Swapchain create(
        PhysicalDevice& physical_device,
        Device& device,
        Surface& surface,
        bool vsync = false
    ) {
        Swapchain swapchain;
        auto surface_capabilities =
            physical_device->getSurfaceCapabilitiesKHR(surface);
        auto surface_formats = physical_device->getSurfaceFormatsKHR(surface);
        auto present_modes =
            physical_device->getSurfacePresentModesKHR(surface);
        auto format_iter = std::find_if(
            surface_formats.begin(), surface_formats.end(),
            [](const vk::SurfaceFormatKHR& format) {
                return format.format == vk::Format::eB8G8R8A8Srgb &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        );
        auto mode_iter = std::find(
            present_modes.begin(), present_modes.end(),
            vk::PresentModeKHR::eMailbox
        );
        if (format_iter == surface_formats.end()) {
            throw std::runtime_error("No suitable surface format");
        }
        swapchain.surface_format = *format_iter;
        swapchain.present_mode = mode_iter == present_modes.end() || vsync
                                     ? vk::PresentModeKHR::eFifo
                                     : *mode_iter;
        swapchain.extent = surface_capabilities.currentExtent;
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        image_count = std::clamp(
            image_count, surface_capabilities.minImageCount,
            surface_capabilities.maxImageCount
        );
        vk::SwapchainCreateInfoKHR create_info;
        create_info.setSurface(surface)
            .setMinImageCount(image_count)
            .setImageFormat(swapchain.surface_format.format)
            .setImageColorSpace(swapchain.surface_format.colorSpace)
            .setImageExtent(swapchain.extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndexCount(1)
            .setPQueueFamilyIndices(&device.queue_family_index)
            .setPreTransform(surface_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(swapchain.present_mode)
            .setClipped(VK_TRUE);
        swapchain.swapchain = device->createSwapchainKHR(create_info);
        swapchain.images = device->getSwapchainImagesKHR(swapchain.swapchain);
        spdlog::info("Swapchain Image Count: {}", swapchain.images.size());
        swapchain.image_views.resize(swapchain.images.size());
        for (int i = 0; i < swapchain.images.size(); i++) {
            swapchain.image_views[i] = ImageView::create(
                device,
                vk::ImageViewCreateInfo()
                    .setImage(swapchain.images[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(swapchain.surface_format.format)
                    .setComponents(vk::ComponentMapping()
                                       .setR(vk::ComponentSwizzle::eIdentity)
                                       .setG(vk::ComponentSwizzle::eIdentity)
                                       .setB(vk::ComponentSwizzle::eIdentity)
                                       .setA(vk::ComponentSwizzle::eIdentity))
                    .setSubresourceRange(
                        vk::ImageSubresourceRange()
                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setBaseMipLevel(0)
                            .setLevelCount(1)
                            .setBaseArrayLayer(0)
                            .setLayerCount(1)
                    )
            );
        }
        for (int i = 0; i < 2; i++) {
            swapchain.image_available_semaphore[i] =
                device->createSemaphore(vk::SemaphoreCreateInfo());
            swapchain.render_finished_semaphore[i] =
                device->createSemaphore(vk::SemaphoreCreateInfo());
            swapchain.in_flight_fence[i] =
                device->createFence(vk::FenceCreateInfo().setFlags(
                    vk::FenceCreateFlagBits::eSignaled
                ));
        }
        return swapchain;
    }

    void recreate(
        PhysicalDevice& physical_device, Device& device, Surface& surface
    ) {
        if (extent ==
            physical_device->getSurfaceCapabilitiesKHR(surface).currentExtent) {
            return;
        }
        device->waitIdle();
        for (auto& image_view : image_views) {
            image_view.destroy(device);
        }
        device->destroySwapchainKHR(swapchain);
        auto surface_capabilities =
            physical_device->getSurfaceCapabilitiesKHR(surface);
        extent = surface_capabilities.currentExtent;
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        image_count = std::clamp(
            image_count, surface_capabilities.minImageCount,
            surface_capabilities.maxImageCount
        );
        vk::SwapchainCreateInfoKHR create_info;
        create_info.setSurface(surface)
            .setMinImageCount(image_count)
            .setImageFormat(surface_format.format)
            .setImageColorSpace(surface_format.colorSpace)
            .setImageExtent(extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndexCount(1)
            .setPQueueFamilyIndices(&device.queue_family_index)
            .setPreTransform(surface_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(present_mode)
            .setClipped(VK_TRUE);
        swapchain = device->createSwapchainKHR(create_info);
        images = device->getSwapchainImagesKHR(swapchain);
        image_views.resize(images.size());
        for (int i = 0; i < images.size(); i += 1) {
            image_views[i] = ImageView::create(
                device,
                vk::ImageViewCreateInfo()
                    .setImage(images[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(surface_format.format)
                    .setComponents(vk::ComponentMapping()
                                       .setR(vk::ComponentSwizzle::eIdentity)
                                       .setG(vk::ComponentSwizzle::eIdentity)
                                       .setB(vk::ComponentSwizzle::eIdentity)
                                       .setA(vk::ComponentSwizzle::eIdentity))
                    .setSubresourceRange(
                        vk::ImageSubresourceRange()
                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setBaseMipLevel(0)
                            .setLevelCount(1)
                            .setBaseArrayLayer(0)
                            .setLayerCount(1)
                    )
            );
        }
    }

    void destroy(Device& device) {
        for (int i = 0; i < 2; i++) {
            device->destroySemaphore(image_available_semaphore[i]);
            device->destroySemaphore(render_finished_semaphore[i]);
            device->destroyFence(in_flight_fence[i]);
        }
        for (auto& image_view : image_views) {
            image_view.destroy(device);
        }
        device->destroySwapchainKHR(swapchain);
    }

    operator vk::SwapchainKHR() const { return swapchain; }

    operator vk::SwapchainKHR&() { return swapchain; }

    operator VkSwapchainKHR() const { return swapchain; }

    bool operator!() const { return !swapchain; }

    auto* operator->() { return &swapchain; }

    Image next_image(Device& device) {
        current_frame = (current_frame + 1) % 2;
        auto res = device->waitForFences(
            1, &in_flight_fence[current_frame], VK_TRUE, UINT64_MAX
        );
        res = device->resetFences(1, &in_flight_fence[current_frame]);
        image_index =
            device
                ->acquireNextImageKHR(
                    swapchain, UINT64_MAX,
                    image_available_semaphore[current_frame], vk::Fence()
                )
                .value;
        return Image{.image = images[image_index]};
    }

    Image current_image() const { return Image{.image = images[image_index]}; }

    ImageView current_image_view() const { return image_views[image_index]; }

    auto& image_available() { return image_available_semaphore[current_frame]; }

    auto& render_finished() { return render_finished_semaphore[current_frame]; }

    auto& fence() { return in_flight_fence[current_frame]; }

    auto& operator*() { return swapchain; }
};

struct RenderPass {
    vk::RenderPass render_pass;

    static RenderPass create(
        Device& device, vk::RenderPassCreateInfo create_info
    ) {
        RenderPass render_pass;
        render_pass.render_pass = device->createRenderPass(create_info);
        return render_pass;
    }

    void destroy(Device& device) { device->destroyRenderPass(render_pass); }

    operator vk::RenderPass() const { return render_pass; }

    operator VkRenderPass() const { return render_pass; }

    bool operator!() const { return !render_pass; }

    auto* operator->() { return &render_pass; }
};

struct Framebuffer {
    vk::Framebuffer framebuffer;

    static Framebuffer create(
        Device& device, vk::FramebufferCreateInfo create_info
    ) {
        Framebuffer framebuffer;
        framebuffer.framebuffer = device->createFramebuffer(create_info);
        return framebuffer;
    }

    void destroy(Device& device) { device->destroyFramebuffer(framebuffer); }

    operator vk::Framebuffer() const { return framebuffer; }

    operator VkFramebuffer() const { return framebuffer; }

    bool operator!() const { return !framebuffer; }

    auto* operator->() { return &framebuffer; }
};

struct ShaderModule {
    vk::ShaderModule shader_module;

    static ShaderModule create(
        Device& device, const vk::ShaderModuleCreateInfo& create_info
    ) {
        ShaderModule shader_module;
        shader_module.shader_module = device->createShaderModule(create_info);
        return shader_module;
    }

    void destroy(Device& device) { device->destroyShaderModule(shader_module); }

    operator vk::ShaderModule() const { return shader_module; }

    operator VkShaderModule() const { return shader_module; }

    bool operator!() const { return !shader_module; }

    auto* operator->() { return &shader_module; }
};

struct PipelineCache {
    vk::PipelineCache pipeline_cache;

    static PipelineCache create(Device& device) {
        PipelineCache pipeline_cache;
        pipeline_cache.pipeline_cache = device->createPipelineCache({});
        return pipeline_cache;
    }

    void destroy(Device& device) {
        device->destroyPipelineCache(pipeline_cache);
    }

    operator vk::PipelineCache() const { return pipeline_cache; }

    operator VkPipelineCache() const { return pipeline_cache; }

    bool operator!() const { return !pipeline_cache; }

    auto* operator->() { return &pipeline_cache; }
};

struct DescriptorSetLayoutBinding {
    vk::DescriptorSetLayoutBinding binding;

    static DescriptorSetLayoutBinding create(
        uint32_t binding,
        vk::DescriptorType descriptor_type,
        vk::ShaderStageFlags stage_flags,
        uint32_t count = 1
    ) {
        DescriptorSetLayoutBinding descriptor_set_layout_binding;
        descriptor_set_layout_binding.binding.setBinding(binding)
            .setDescriptorType(descriptor_type)
            .setDescriptorCount(count)
            .setStageFlags(stage_flags);
        return descriptor_set_layout_binding;
    }

    operator vk::DescriptorSetLayoutBinding() const { return binding; }

    operator VkDescriptorSetLayoutBinding() const { return binding; }

    auto* operator->() { return &binding; }
};
}  // namespace components
}  // namespace render_vk
}  // namespace pixel_engine