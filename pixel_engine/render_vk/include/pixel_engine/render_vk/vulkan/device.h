#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <optional>

#include "../vulkan_headers.h"

#define MAKE_VERSION(major, minor, patch) VK_MAKE_VERSION(major, minor, patch)

namespace pixel_engine {
namespace render_vk {
namespace vulkan {
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

    vk::Instance operator*() const { return instance; }

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
};

struct Device {
    Instance instance;
    vk::PhysicalDevice physical_device;
    vk::Device logical_device;
    VmaAllocator allocator;
    uint32_t queue_family_index;
    vk::Queue queue;
    vk::CommandPool command_pool;

    static Device create(Instance instance) {
        Device device;
        device.instance = instance;
        auto physical_devices = instance.instance.enumeratePhysicalDevices();
        if (physical_devices.empty()) {
            device.instance.logger->error("No physical devices found");
            throw std::runtime_error("No physical devices found");
        }
        device.physical_device = physical_devices[0];
        auto queue_family_properties =
            device.physical_device.getQueueFamilyProperties();
        uint32_t queue_family_index = 0;
        for (auto& queue_family_property : queue_family_properties) {
            if (queue_family_property.queueFlags &
                vk::QueueFlagBits::eGraphics) {
                break;
            }
            queue_family_index++;
        }
        if (queue_family_index == queue_family_properties.size()) {
            device.instance.logger->error(
                "No queue family with graphics support"
            );
            throw std::runtime_error("No queue family with graphics support");
        }
        device.queue_family_index = queue_family_index;
        float queue_priority = 1.0f;
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
                               .setQueueCreateInfoCount(1)
                               .setPQueueCreateInfos(&queue_create_info)
                               .setPEnabledFeatures(&device_features)
                               .setPNext(&dynamic_rendering_features)
                               .setPEnabledExtensionNames(device_extensions);
        device.logical_device =
            device.physical_device.createDevice(device_info);
        device.queue = device.logical_device.getQueue(queue_family_index, 0);
        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.physicalDevice = device.physical_device;
        allocator_info.device = device.logical_device;
        allocator_info.instance = device.instance.instance;
        vmaCreateAllocator(&allocator_info, &device.allocator);
        vk::CommandPoolCreateInfo command_pool_info;
        command_pool_info.setQueueFamilyIndex(queue_family_index)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        device.command_pool =
            device.logical_device.createCommandPool(command_pool_info);
        return device;
    }
    bool operator!() const { return !logical_device; }
    vk::Device operator*() const { return logical_device; }

    void destroy() {
        vmaDestroyAllocator(allocator);
        logical_device.destroyCommandPool(command_pool);
        logical_device.destroy();
    }
};

struct CommandBuffer {
    Device device;
    vk::CommandBuffer command_buffer;

    static CommandBuffer allocate(Device device) {
        auto command_buffer = device.logical_device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo()
                .setCommandPool(device.command_pool)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1)
        )[0];
        return CommandBuffer{device, command_buffer};
    }
    bool operator!() const { return !command_buffer; }
    vk::CommandBuffer operator*() { return command_buffer; }
    auto operator->() { return &command_buffer; }

    void free() {
        device.logical_device.freeCommandBuffers(
            device.command_pool, command_buffer
        );
    }
};

struct Buffer {
    Device device;
    vk::Buffer buffer;
    VmaAllocation allocation;

    static Buffer create_device(
        Device device, vk::BufferCreateInfo create_info
    ) {
        Buffer buffer;
        buffer.device = device;
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaCreateBuffer(
            device.allocator,
            reinterpret_cast<VkBufferCreateInfo*>(&create_info), &alloc_info,
            reinterpret_cast<VkBuffer*>(&buffer.buffer), &buffer.allocation,
            nullptr
        );
        return buffer;
    }
    static Buffer create_host(Device device, vk::BufferCreateInfo create_info) {
        Buffer buffer;
        buffer.device = device;
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        vmaCreateBuffer(
            device.allocator,
            reinterpret_cast<VkBufferCreateInfo*>(&create_info), &alloc_info,
            reinterpret_cast<VkBuffer*>(&buffer.buffer), &buffer.allocation,
            nullptr
        );
        return buffer;
    }
    bool operator!() const { return !buffer; }
    vk::Buffer operator*() const { return buffer; }

    void destroy() { vmaDestroyBuffer(device.allocator, buffer, allocation); }
};

struct Image {
    Device device;
    vk::Image image;
    vk::ImageView image_view;
    VmaAllocation allocation;

    Image() = default;
    Image(vk::Image image, vk::ImageView image_view)
        : image(image), image_view(image_view) {}

    static Image create(Device device, vk::ImageCreateInfo create_info) {
        Image image;
        image.device = device;
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaCreateImage(
            device.allocator,
            reinterpret_cast<VkImageCreateInfo*>(&create_info), &alloc_info,
            reinterpret_cast<VkImage*>(&image.image), &image.allocation, nullptr
        );
        vk::ImageViewType view_type;
        switch (create_info.imageType) {
            case vk::ImageType::e1D:
                view_type = vk::ImageViewType::e1D;
                break;
            case vk::ImageType::e2D:
                view_type = vk::ImageViewType::e2D;
                break;
            case vk::ImageType::e3D:
                view_type = vk::ImageViewType::e3D;
                break;
            default:
                throw std::runtime_error("Invalid image type");
        }
        vk::ImageAspectFlagBits aspect;
        if (create_info.usage & vk::ImageUsageFlagBits::eColorAttachment) {
            aspect = vk::ImageAspectFlagBits::eColor;
        } else if (create_info.usage &
                   vk::ImageUsageFlagBits::eDepthStencilAttachment) {
            aspect = vk::ImageAspectFlagBits::eDepth;
        } else {
            aspect = vk::ImageAspectFlagBits::eColor;
        }
        image.image_view = device.logical_device.createImageView(
            vk::ImageViewCreateInfo()
                .setImage(image.image)
                .setViewType(view_type)
                .setFormat(create_info.format)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(aspect)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(create_info.mipLevels)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(create_info.arrayLayers)
                )
        );
        return image;
    }
    bool operator!() const { return !image || !image_view; }
    std::pair<vk::Image, vk::ImageView> operator*() const {
        return {image, image_view};
    }

    void destroy() {
        vmaDestroyImage(device.allocator, image, allocation);
        device.logical_device.destroyImageView(image_view);
    }
};

struct Sampler {
    Device device;
    vk::Sampler sampler;

    static Sampler create(Device device, vk::SamplerCreateInfo create_info) {
        return Sampler{
            device, device.logical_device.createSampler(create_info)
        };
    }
    bool operator!() const { return !sampler; }
    vk::Sampler operator*() const { return sampler; }

    void destroy() { device.logical_device.destroySampler(sampler); }
};

struct SwapChain {
    Device device;
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    GLFWwindow* window;
    vk::Semaphore image_available_semaphore;
    vk::Semaphore render_finished_semaphore;
    vk::Fence in_flight_fence;
    uint32_t image_index = 0;
    bool vsync = true;

    static SwapChain create(
        Device device, GLFWwindow* window, bool vsync = false
    ) {
        SwapChain swapchain;
        swapchain.vsync = vsync;
        swapchain.device = device;
        swapchain.window = window;
        if (glfwCreateWindowSurface(
                device.instance.instance, window, nullptr,
                reinterpret_cast<VkSurfaceKHR*>(&swapchain.surface)
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }
        auto surface_capabilities =
            device.physical_device.getSurfaceCapabilitiesKHR(swapchain.surface);
        auto surface_formats =
            device.physical_device.getSurfaceFormatsKHR(swapchain.surface);
        auto present_modes =
            device.physical_device.getSurfacePresentModesKHR(swapchain.surface);
        auto format_iter = std::find_if(
            surface_formats.begin(), surface_formats.end(),
            [](const vk::SurfaceFormatKHR& format) {
                return format.format == vk::Format::eR8G8B8A8Unorm &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        );
        auto mode_iter = std::find(
            present_modes.begin(), present_modes.end(),
            vk::PresentModeKHR::eMailbox
        );
        if (format_iter == surface_formats.end()) {
            swapchain.surface_format = surface_formats[0];
        } else {
            swapchain.surface_format = *format_iter;
        }
        swapchain.present_mode = mode_iter == present_modes.end() | vsync
                                     ? vk::PresentModeKHR::eFifo
                                     : vk::PresentModeKHR::eMailbox;
        swapchain.extent = surface_capabilities.currentExtent;
        spdlog::info(
            "Swapchain Extent: {}x{}", swapchain.extent.width,
            swapchain.extent.height
        );
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount > 0 &&
            image_count > surface_capabilities.maxImageCount) {
            image_count = surface_capabilities.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR create_info;
        create_info.setSurface(swapchain.surface)
            .setMinImageCount(image_count)
            .setImageFormat(swapchain.surface_format.format)
            .setImageColorSpace(swapchain.surface_format.colorSpace)
            .setImageExtent(swapchain.extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surface_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(swapchain.present_mode)
            .setClipped(VK_TRUE);
        swapchain.swapchain =
            device.logical_device.createSwapchainKHR(create_info);
        swapchain.images =
            device.logical_device.getSwapchainImagesKHR(swapchain.swapchain);
        spdlog::info("Swapchain Image Count: {}", swapchain.images.size());
        swapchain.image_views.resize(swapchain.images.size());
        for (int i = 0; i < swapchain.images.size(); i++) {
            swapchain.image_views[i] = device.logical_device.createImageView(
                vk::ImageViewCreateInfo()
                    .setImage(swapchain.images[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(swapchain.surface_format.format)
                    .setComponents(vk::ComponentMapping())
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
        swapchain.image_available_semaphore =
            device.logical_device.createSemaphore(
                vk::SemaphoreCreateInfo().setFlags(vk::SemaphoreCreateFlagBits()
                )
            );
        swapchain.render_finished_semaphore =
            device.logical_device.createSemaphore(
                vk::SemaphoreCreateInfo().setFlags(vk::SemaphoreCreateFlagBits()
                )
            );
        swapchain.in_flight_fence = device.logical_device.createFence(
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)
        );
        return swapchain;
    }
    bool operator!() const { return !swapchain; }

    void recreate() {
        if (extent == device.physical_device.getSurfaceCapabilitiesKHR(surface)
                          .currentExtent) {
            return;
        }
        device.logical_device.waitIdle();
        for (auto& image_view : image_views) {
            device.logical_device.destroyImageView(image_view);
        }
        device.logical_device.destroySwapchainKHR(swapchain);
        auto surface_capabilities =
            device.physical_device.getSurfaceCapabilitiesKHR(surface);
        extent = surface_capabilities.currentExtent;
        uint32_t image_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount > 0 &&
            image_count > surface_capabilities.maxImageCount) {
            image_count = surface_capabilities.maxImageCount;
        }
        vk::SwapchainCreateInfoKHR create_info;
        create_info.setSurface(surface)
            .setMinImageCount(image_count)
            .setImageFormat(surface_format.format)
            .setImageColorSpace(surface_format.colorSpace)
            .setImageExtent(extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surface_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(present_mode)
            .setClipped(VK_TRUE);
        swapchain = device.logical_device.createSwapchainKHR(create_info);
        images = device.logical_device.getSwapchainImagesKHR(swapchain);
        image_views.resize(images.size());
        for (int i = 0; i < images.size(); i += 1) {
            image_views[i] = device.logical_device.createImageView(
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
    Image next_image() {
        device.logical_device.waitForFences(
            1, &in_flight_fence, VK_TRUE, UINT64_MAX
        );
        device.logical_device.resetFences(1, &in_flight_fence);
        image_index = device.logical_device
                          .acquireNextImageKHR(
                              swapchain, UINT64_MAX, image_available_semaphore,
                              vk::Fence()
                          )
                          .value;
        return Image{images[image_index], image_views[image_index]};
    }
    Image current_image() {
        return Image{images[image_index], image_views[image_index]};
    }
    auto current_image_view() { return image_views[image_index]; }
    void destroy() {
        device.logical_device.destroySemaphore(image_available_semaphore);
        device.logical_device.destroySemaphore(render_finished_semaphore);
        device.logical_device.destroyFence(in_flight_fence);
        for (auto& image_view : image_views) {
            device.logical_device.destroyImageView(image_view);
        }
        device.logical_device.destroySwapchainKHR(swapchain);
    }
};

struct RenderPass {
    Device device;
    vk::RenderPass render_pass;

    static RenderPass create(
        Device device,
        std::vector<vk::AttachmentDescription> attachments,
        std::vector<vk::SubpassDescription> subpasses,
        std::vector<vk::SubpassDependency> dependencies
    ) {
        return RenderPass{
            device, device.logical_device.createRenderPass(
                        vk::RenderPassCreateInfo()
                            .setAttachmentCount(attachments.size())
                            .setPAttachments(attachments.data())
                            .setSubpassCount(subpasses.size())
                            .setPSubpasses(subpasses.data())
                            .setDependencyCount(dependencies.size())
                            .setPDependencies(dependencies.data())
                    )
        };
    }
    bool operator!() const { return !render_pass; }

    void destroy() { device.logical_device.destroyRenderPass(render_pass); }
};

struct RenderPassCreateInfo {
    Device device;
    std::vector<vk::AttachmentDescription> color_attachments;
    std::optional<vk::AttachmentDescription> depth_attachment;
    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;
    std::vector<vk::AttachmentReference> attachment_refs;
    std::optional<vk::AttachmentReference> depth_attachment_ref;
    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency> dependencies;

    RenderPassCreateInfo(Device device) : device(device) {}

    static RenderPassCreateInfo create(Device device) {
        return RenderPassCreateInfo(device);
    }

    void add_color_attachment(
        uint32_t attachment,
        vk::Format format,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
        vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined,
        vk::ImageLayout final_layout = vk::ImageLayout::eColorAttachmentOptimal
    ) {
        color_attachments.push_back(
            vk::AttachmentDescription()
                .setFormat(format)
                .setSamples(samples)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(initial_layout)
                .setFinalLayout(final_layout)
        );
    }
    void add_depth_attachment(
        vk::Format format,
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
        vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined,
        vk::ImageLayout final_layout =
            vk::ImageLayout::eDepthStencilAttachmentOptimal
    ) {
        depth_attachment =
            vk::AttachmentDescription()
                .setFormat(format)
                .setSamples(samples)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(initial_layout)
                .setFinalLayout(final_layout);
    }
    void set_bind_point(vk::PipelineBindPoint bind_point) {
        this->bind_point = bind_point;
    }
    RenderPass create() {
        for (int i = 0; i < color_attachments.size(); i++) {
            attachment_refs.push_back(
                vk::AttachmentReference().setAttachment(i).setLayout(
                    vk::ImageLayout::eColorAttachmentOptimal
                )
            );
        }
        if (depth_attachment) {
            depth_attachment_ref =
                vk::AttachmentReference()
                    .setAttachment(color_attachments.size())
                    .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        }
        subpasses.push_back(
            vk::SubpassDescription()
                .setPipelineBindPoint(bind_point)
                .setColorAttachments(attachment_refs)
                .setPDepthStencilAttachment(
                    depth_attachment ? &depth_attachment_ref.value() : nullptr
                )
        );
        dependencies.push_back(
            vk::SubpassDependency()
                .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput |
                    vk::PipelineStageFlagBits::eEarlyFragmentTests
                )
                .setDstStageMask(
                    vk::PipelineStageFlagBits::eColorAttachmentOutput |
                    vk::PipelineStageFlagBits::eEarlyFragmentTests
                )
                .setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
                .setDstAccessMask(
                    vk::AccessFlagBits::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite
                )
        );
        if (depth_attachment) {
            color_attachments.push_back(depth_attachment.value());
        }
        return RenderPass::create(
            device, color_attachments, subpasses, dependencies
        );
    }
};

struct FrameBuffer {
    Device device;
    vk::Framebuffer frame_buffer;

    static FrameBuffer create(
        Device device,
        vk::RenderPass render_pass,
        std::vector<Image> attachments,
        vk::Extent2D extent
    ) {
        std::vector<vk::ImageView> image_views;
        for (auto& attachment : attachments) {
            image_views.push_back(attachment.image_view);
        }
        return FrameBuffer{
            device, device.logical_device.createFramebuffer(
                        vk::FramebufferCreateInfo()
                            .setRenderPass(render_pass)
                            .setAttachmentCount(image_views.size())
                            .setPAttachments(image_views.data())
                            .setWidth(extent.width)
                            .setHeight(extent.height)
                            .setLayers(1)
                    )
        };
    }
    bool operator!() const { return !frame_buffer; }

    void destroy() { device.logical_device.destroyFramebuffer(frame_buffer); }
};

struct ShaderModule {
    Device device;
    vk::ShaderModule shader_module;

    static ShaderModule create(
        Device device, const uint32_t* code, size_t size
    ) {
        return ShaderModule{
            device,
            device.logical_device.createShaderModule(
                vk::ShaderModuleCreateInfo().setCodeSize(size).setPCode(code)

            )
        };
    }
    static ShaderModule create(
        Device device, const std::vector<uint32_t>& code
    ) {
        return create(device, code.data(), code.size());
    }
    bool operator!() const { return !shader_module; }

    void destroy() { device.logical_device.destroyShaderModule(shader_module); }
};

struct DescriptorSetLayout {
    Device device;
    vk::DescriptorSetLayout descriptor_set_layout;

    static DescriptorSetLayout create(
        Device device, std::vector<vk::DescriptorSetLayoutBinding> bindings
    ) {
        return DescriptorSetLayout{
            device, device.logical_device.createDescriptorSetLayout(
                        vk::DescriptorSetLayoutCreateInfo()
                            .setBindingCount(bindings.size())
                            .setPBindings(bindings.data())
                    )
        };
    }
    bool operator!() const { return !descriptor_set_layout; }

    void destroy() {
        device.logical_device.destroyDescriptorSetLayout(descriptor_set_layout);
    }
};

struct PipelineLayout {
    Device device;
    vk::PipelineLayout pipeline_layout;

    static PipelineLayout create(
        Device device,
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts
    ) {
        return PipelineLayout{
            device, device.logical_device.createPipelineLayout(
                        vk::PipelineLayoutCreateInfo()
                            .setSetLayoutCount(descriptor_set_layouts.size())
                            .setPSetLayouts(descriptor_set_layouts.data())
                    )
        };
    }
    bool operator!() const { return !pipeline_layout; }

    void destroy() {
        device.logical_device.destroyPipelineLayout(pipeline_layout);
    }
};

struct Pipeline {
    Device device;
    vk::Pipeline pipeline;
    PipelineLayout layout;

    static Pipeline create(
        Device device,
        PipelineLayout layout,
        RenderPass render_pass,
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages,
        vk::PipelineVertexInputStateCreateInfo vertex_input_info,
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_info,
        vk::PipelineViewportStateCreateInfo viewport_state_info,
        vk::PipelineRasterizationStateCreateInfo rasterizer_info,
        vk::PipelineMultisampleStateCreateInfo multisampling_info,
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_info,
        vk::PipelineColorBlendStateCreateInfo color_blending_info,
        vk::PipelineDynamicStateCreateInfo dynamic_state_info
    ) {
        return Pipeline{
            device,
            device.logical_device
                .createGraphicsPipeline(
                    vk::PipelineCache(),
                    vk::GraphicsPipelineCreateInfo()
                        .setLayout(layout.pipeline_layout)
                        .setRenderPass(render_pass.render_pass)
                        .setStages(shader_stages)
                        .setPVertexInputState(&vertex_input_info)
                        .setPInputAssemblyState(&input_assembly_info)
                        .setPViewportState(&viewport_state_info)
                        .setPRasterizationState(&rasterizer_info)
                        .setPMultisampleState(&multisampling_info)
                        .setPDepthStencilState(&depth_stencil_info)
                        .setPColorBlendState(&color_blending_info)
                        .setPDynamicState(&dynamic_state_info)
                )
                .value,
            layout
        };
    }

    bool operator!() const { return !pipeline; }

    void destroy() { device.logical_device.destroyPipeline(pipeline); }
};

struct Descriptor {
    Device device;
    std::vector<DescriptorSetLayout> layouts;
    vk::DescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;

    struct PerSet {
        std::vector<vk::DescriptorBufferInfo> storage_buffers;
        std::vector<vk::DescriptorBufferInfo> uniform_buffers;
        std::vector<vk::DescriptorImageInfo> images;
        std::vector<vk::DescriptorImageInfo> textures;
    };

    std::vector<PerSet> per_set;

    static Descriptor create(
        Device device,
        std::vector<DescriptorSetLayout> layouts,
        std::vector<vk::DescriptorPoolSize> pool_sizes,
        uint32_t max_sets
    ) {
        vk::DescriptorPool descriptor_pool =
            device.logical_device.createDescriptorPool(
                vk::DescriptorPoolCreateInfo()
                    .setPoolSizeCount(pool_sizes.size())
                    .setPPoolSizes(pool_sizes.data())
                    .setMaxSets(max_sets)
            );
        std::vector<vk::DescriptorSetLayout> layout_ptrs;
        for (auto& layout : layouts) {
            layout_ptrs.push_back(layout.descriptor_set_layout);
        }
        std::vector<vk::DescriptorSet> descriptor_sets =
            device.logical_device.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo()
                    .setDescriptorPool(descriptor_pool)
                    .setSetLayouts(layout_ptrs)
            );
        return Descriptor{device, layouts, descriptor_pool, descriptor_sets};
    };

    void recreate(
        std::vector<DescriptorSetLayout> layouts,
        std::vector<vk::DescriptorPoolSize> pool_sizes,
        uint32_t max_sets
    ) {
        if (descriptor_pool) {
            device.logical_device.destroyDescriptorPool(descriptor_pool);
        }
        descriptor_pool = device.logical_device.createDescriptorPool(
            vk::DescriptorPoolCreateInfo()
                .setPoolSizeCount(pool_sizes.size())
                .setPPoolSizes(pool_sizes.data())
                .setMaxSets(max_sets)
        );
        std::vector<vk::DescriptorSetLayout> layout_ptrs;
        for (auto& layout : layouts) {
            layout_ptrs.push_back(layout.descriptor_set_layout);
        }
        if (descriptor_sets.size() > 0) {
            device.logical_device.freeDescriptorSets(
                descriptor_pool, descriptor_sets
            );
        }
        descriptor_sets = device.logical_device.allocateDescriptorSets(
            vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descriptor_pool)
                .setSetLayouts(layout_ptrs)
        );
    }

    void bind_storage_buffer(
        uint32_t set,
        uint32_t binding,
        Buffer buffer,
        vk::DeviceSize offset = 0,
        vk::DeviceSize range = VK_WHOLE_SIZE
    ) {
        if (per_set.size() <= set) {
            per_set.resize(set + 1);
        } else if (per_set[set].storage_buffers.size() <= binding) {
            per_set[set].storage_buffers.resize(binding + 1);
        } else {
            auto new_info = vk::DescriptorBufferInfo()
                                .setBuffer(buffer.buffer)
                                .setOffset(offset)
                                .setRange(range);
            if (per_set[set].storage_buffers[binding] == new_info) {
                return;
            }
        }
        device.logical_device.updateDescriptorSets(
            vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[set])
                .setDstBinding(binding)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setBufferInfo(per_set[set].storage_buffers[binding]),
            nullptr
        );
    }

    void bind_uniform_buffer(
        uint32_t set,
        uint32_t binding,
        Buffer buffer,
        vk::DeviceSize offset = 0,
        vk::DeviceSize range = VK_WHOLE_SIZE
    ) {
        if (per_set.size() <= set) {
            per_set.resize(set + 1);
        } else if (per_set[set].uniform_buffers.size() <= binding) {
            per_set[set].uniform_buffers.resize(binding + 1);
        } else {
            auto new_info = vk::DescriptorBufferInfo()
                                .setBuffer(buffer.buffer)
                                .setOffset(offset)
                                .setRange(range);
            if (per_set[set].uniform_buffers[binding] == new_info) {
                return;
            }
        }
        device.logical_device.updateDescriptorSets(
            vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[set])
                .setDstBinding(binding)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setBufferInfo(vk::DescriptorBufferInfo()
                                   .setBuffer(buffer.buffer)
                                   .setOffset(offset)
                                   .setRange(range)),
            nullptr
        );
    }

    void bind_image(
        uint32_t set, uint32_t binding, Image image, vk::ImageLayout layout
    ) {
        if (per_set.size() <= set) {
            per_set.resize(set + 1);
        } else if (per_set[set].images.size() <= binding) {
            per_set[set].images.resize(binding + 1);
        } else {
            auto new_info = vk::DescriptorImageInfo()
                                .setImageView(image.image_view)
                                .setImageLayout(layout);
            if (per_set[set].images[binding] == new_info) {
                return;
            }
        }
        device.logical_device.updateDescriptorSets(
            vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[set])
                .setDstBinding(binding)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setImageInfo(vk::DescriptorImageInfo()
                                  .setImageView(image.image_view)
                                  .setImageLayout(layout)),
            nullptr
        );
    }

    void bind_texture(
        uint32_t set, uint32_t binding, Image image, Sampler sampler
    ) {
        if (per_set.size() <= set) {
            per_set.resize(set + 1);
        } else if (per_set[set].textures.size() <= binding) {
            per_set[set].textures.resize(binding + 1);
        } else {
            auto new_info =
                vk::DescriptorImageInfo()
                    .setImageView(image.image_view)
                    .setSampler(sampler.sampler)
                    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            if (per_set[set].textures[binding] == new_info) {
                return;
            }
        }
        device.logical_device.updateDescriptorSets(
            vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[set])
                .setDstBinding(binding)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(
                    vk::DescriptorImageInfo()
                        .setImageView(image.image_view)
                        .setSampler(sampler.sampler)
                        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                ),
            nullptr
        );
    }
};

struct Renderer {
    Device device;
    Pipeline pipeline;
    RenderPass render_pass;
    PipelineLayout layout;
    Descriptor descriptor;

    static Renderer create(
        Device device,
        Pipeline pipeline,
        RenderPass render_pass,
        PipelineLayout layout,
        Descriptor descriptor
    ) {
        return Renderer{device, pipeline, render_pass, layout, descriptor};
    }
};
}  // namespace vulkan
}  // namespace render_vk
}  // namespace pixel_engine