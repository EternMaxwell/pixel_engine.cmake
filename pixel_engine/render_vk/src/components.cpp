#include "pixel_engine/render_vk.h"

using namespace pixel_engine;
using namespace pixel_engine::prelude;
using namespace pixel_engine::window;
using namespace pixel_engine::render_vk::components;

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
        "\tmessageIDName   = <{}>\n\tmessageIdNumber = {:#018x}\n",
        pCallbackData->pMessageIdName,
        *((uint32_t*)(&pCallbackData->messageIdNumber))
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
                "\t\tobjectHandle = {:#018x}\n",
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

Instance Instance::create(
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
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto instance_extensions = vk::enumerateInstanceExtensionProperties();
    logger->info("Creating Vulkan Instance");

    std::vector<const char*> extensions;
    for (auto& extension : instance_extensions) {
        extensions.push_back(extension.extensionName);
    }
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        if (std::find_if(
                extensions.begin(), extensions.end(),
                [&](const char* ext) { return strcmp(ext, glfwExtensions[i]); }
            ) == extensions.end()) {
            logger->error(
                "GLFW requested extension {} not supported by Vulkan",
                glfwExtensions[i]
            );
            throw std::runtime_error(
                "GLFW requested extension not supported by Vulkan. "
                "Extension: " +
                std::string(glfwExtensions[i])
            );
        }
    }

    auto instance_info = vk::InstanceCreateInfo()
                             .setPApplicationInfo(&app_info)
                             .setPEnabledExtensionNames(extensions)
                             .setPEnabledLayerNames(layers);

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
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(
        instance.instance, "vkCreateDebugUtilsMessengerEXT"
    );
    if (!func) {
        logger->error("Failed to load function vkCreateDebugUtilsMessengerEXT");
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
        reinterpret_cast<VkDebugUtilsMessengerEXT*>(&instance.debug_messenger)
    );
#endif
    return instance;
}

void Instance::destroy() {
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

Instance::operator vk::Instance() const { return instance; }
Instance::operator VkInstance() const { return instance; }
bool Instance::operator!() const { return !instance; }
vk::Instance* Instance::operator->() { return &instance; }
vk::Instance& Instance::operator*() { return instance; }

PhysicalDevice PhysicalDevice::create(Instance instance) {
    PhysicalDevice physical_device;
    auto physical_devices = instance->enumeratePhysicalDevices();
    if (physical_devices.size() == 0) {
        instance.logger->error("No physical devices found");
        throw std::runtime_error("No physical devices found");
    }
    physical_device.physical_device = physical_devices[0];
    return physical_device;
}

PhysicalDevice::operator vk::PhysicalDevice() const { return physical_device; }
PhysicalDevice::operator VkPhysicalDevice() const { return physical_device; }
bool PhysicalDevice::operator!() const { return !physical_device; }
vk::PhysicalDevice* PhysicalDevice::operator->() { return &physical_device; }
vk::PhysicalDevice& PhysicalDevice::operator*() { return physical_device; }

Device Device::create(
    Instance& instance,
    PhysicalDevice& physical_device,
    vk::QueueFlags queue_flags
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
    std::vector<char const*> required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    };
    auto extensions = physical_device->enumerateDeviceExtensionProperties();
    for (auto& extension : required_extensions) {
        if (std::find_if(
                extensions.begin(), extensions.end(),
                [&](const vk::ExtensionProperties& ext) {
                    return strcmp(ext.extensionName, extension) == 0;
                }
            ) == extensions.end()) {
            instance.logger->error(
                "Device required extension {} not supported by physical device",
                extension
            );
            throw std::runtime_error(
                "Device required extension not supported by physical device. "
                "Extension: " +
                std::string(extension)
            );
        }
    }
    std::vector<char const*> device_extensions;
    for (auto& extension : extensions) {
        device_extensions.push_back(extension.extensionName);
    }

    std::string device_extensions_info = "Device Extensions:\n";
    for (auto& extension : device_extensions) {
        device_extensions_info += std::format("\t{}\n", extension);
    }
    instance.logger->info(device_extensions_info);

    auto dynamic_rendering_features =
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR().setDynamicRendering(
            VK_TRUE
        );
    auto device_feature2 = physical_device->getFeatures2();
    device_feature2.pNext = &dynamic_rendering_features;
    auto device_info = vk::DeviceCreateInfo()
                           .setQueueCreateInfos(queue_create_info)
                           .setPNext(&device_feature2)
                           .setPEnabledExtensionNames(device_extensions);
    device.device = physical_device->createDevice(device_info);
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device.device;
    allocator_info.instance = instance.instance;
    vmaCreateAllocator(&allocator_info, &device.allocator);
    return device;
}

void Device::destroy() {
    vmaDestroyAllocator(allocator);
    device.destroy();
}

Device::operator vk::Device() const { return device; }
Device::operator VkDevice() const { return device; }
bool Device::operator!() const { return !device; }
vk::Device* Device::operator->() { return &device; }
vk::Device& Device::operator*() { return device; }

Queue Queue::create(Device& device) {
    Queue queue;
    queue.queue = device->getQueue(device.queue_family_index, 0);
    return queue;
}

Queue::operator vk::Queue() const { return queue; }
Queue::operator VkQueue() const { return queue; }
bool Queue::operator!() const { return !queue; }
vk::Queue* Queue::operator->() { return &queue; }
vk::Queue& Queue::operator*() { return queue; }

CommandPool CommandPool::create(Device& device) {
    auto command_pool_info =
        vk::CommandPoolCreateInfo()
            .setQueueFamilyIndex(device.queue_family_index)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    CommandPool command_pool;
    command_pool.command_pool = device->createCommandPool(command_pool_info);
    return command_pool;
}

void CommandPool::destroy(Device device) {
    device->destroyCommandPool(command_pool);
}
CommandPool::operator vk::CommandPool() const { return command_pool; }
CommandPool::operator VkCommandPool() const { return command_pool; }
bool CommandPool::operator!() const { return !command_pool; }
vk::CommandPool* CommandPool::operator->() { return &command_pool; }
vk::CommandPool& CommandPool::operator*() { return command_pool; }

CommandBuffer CommandBuffer::allocate_primary(
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
CommandBuffer CommandBuffer::allocate_secondary(
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
std::vector<CommandBuffer> CommandBuffer::allocate_primary(
    Device& device, CommandPool& command_pool, uint32_t count
) {
    auto command_buffers = device->allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(count)
    );
    std::vector<CommandBuffer> buffers;
    for (auto& command_buffer : command_buffers) {
        buffers.push_back(CommandBuffer{command_buffer});
    }
    return buffers;
}
std::vector<CommandBuffer> CommandBuffer::allocate_secondary(
    Device& device, CommandPool& command_pool, uint32_t count
) {
    auto command_buffers = device->allocateCommandBuffers(
        vk::CommandBufferAllocateInfo()
            .setCommandPool(command_pool)
            .setLevel(vk::CommandBufferLevel::eSecondary)
            .setCommandBufferCount(count)
    );
    std::vector<CommandBuffer> buffers;
    for (auto& command_buffer : command_buffers) {
        buffers.push_back(CommandBuffer{command_buffer});
    }
    return buffers;
}

void CommandBuffer::free(Device& device, CommandPool& command_pool) {
    device->freeCommandBuffers(command_pool, command_buffer);
}
CommandBuffer::operator vk::CommandBuffer() const { return command_buffer; }
CommandBuffer::operator VkCommandBuffer() const { return command_buffer; }
bool CommandBuffer::operator!() const { return !command_buffer; }
vk::CommandBuffer* CommandBuffer::operator->() { return &command_buffer; }
vk::CommandBuffer& CommandBuffer::operator*() { return command_buffer; }

AllocationCreateInfo::AllocationCreateInfo() { create_info = {}; }
AllocationCreateInfo::AllocationCreateInfo(
    const VmaMemoryUsage& usage, const VmaAllocationCreateFlags& flags
) {
    create_info.usage = usage;
    create_info.flags = flags;
}

AllocationCreateInfo::operator VmaAllocationCreateInfo() const {
    return create_info;
}
AllocationCreateInfo& AllocationCreateInfo::setUsage(VmaMemoryUsage usage) {
    create_info.usage = usage;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setFlags(
    VmaAllocationCreateFlags flags
) {
    create_info.flags = flags;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setRequiredFlags(
    VkMemoryPropertyFlags flags
) {
    create_info.requiredFlags = flags;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setPreferredFlags(
    VkMemoryPropertyFlags flags
) {
    create_info.preferredFlags = flags;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setMemoryTypeBits(uint32_t bits) {
    create_info.memoryTypeBits = bits;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setPool(VmaPool pool) {
    create_info.pool = pool;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setPUserData(void* data) {
    create_info.pUserData = data;
    return *this;
}
AllocationCreateInfo& AllocationCreateInfo::setPriority(float priority) {
    create_info.priority = priority;
    return *this;
}
VmaAllocationCreateInfo& AllocationCreateInfo::operator*() {
    return create_info;
}

Buffer Buffer::create(
    Device& device,
    vk::BufferCreateInfo create_info,
    AllocationCreateInfo alloc_info
) {
    Buffer buffer;
    vmaCreateBuffer(
        device.allocator, reinterpret_cast<VkBufferCreateInfo*>(&create_info),
        reinterpret_cast<VmaAllocationCreateInfo*>(&alloc_info),
        reinterpret_cast<VkBuffer*>(&buffer.buffer), &buffer.allocation, nullptr
    );
    return buffer;
}
Buffer Buffer::create_device(
    Device& device, uint64_t size, vk::BufferUsageFlags usage
) {
    return create(
        device,
        vk::BufferCreateInfo().setSize(size).setUsage(usage).setSharingMode(
            vk::SharingMode::eExclusive
        ),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    );
}
Buffer Buffer::create_host(
    Device& device, uint64_t size, vk::BufferUsageFlags usage
) {
    return create(
        device,
        vk::BufferCreateInfo().setSize(size).setUsage(usage).setSharingMode(
            vk::SharingMode::eExclusive
        ),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
            .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)
    );
}

void Buffer::destroy(Device& device) {
    vmaDestroyBuffer(device.allocator, buffer, allocation);
}
void* Buffer::map(Device& device) {
    void* data;
    vmaMapMemory(device.allocator, allocation, &data);
    return data;
}
void Buffer::unmap(Device& device) {
    vmaUnmapMemory(device.allocator, allocation);
}
Buffer::operator vk::Buffer() const { return buffer; }
Buffer::operator VkBuffer() const { return buffer; }
bool Buffer::operator!() const { return !buffer; }
vk::Buffer* Buffer::operator->() { return &buffer; }
vk::Buffer& Buffer::operator*() { return buffer; }

Image Image::create(
    Device& device,
    vk::ImageCreateInfo create_info,
    AllocationCreateInfo alloc_info
) {
    Image image;
    vmaCreateImage(
        device.allocator, reinterpret_cast<VkImageCreateInfo*>(&create_info),
        reinterpret_cast<VmaAllocationCreateInfo*>(&alloc_info),
        reinterpret_cast<VkImage*>(&image.image), &image.allocation, nullptr
    );
    return image;
}
Image Image::create_1d(
    Device& device,
    uint32_t width,
    uint32_t levels,
    uint32_t layers,
    vk::Format format,
    vk::ImageUsageFlags usage
) {
    return create(
        device,
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e1D)
            .setExtent(vk::Extent3D(width, 1, 1))
            .setMipLevels(levels)
            .setArrayLayers(layers)
            .setFormat(format)
            .setUsage(usage)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    );
}
Image Image::create_2d(
    Device& device,
    uint32_t width,
    uint32_t height,
    uint32_t levels,
    uint32_t layers,
    vk::Format format,
    vk::ImageUsageFlags usage
) {
    return create(
        device,
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setExtent(vk::Extent3D(width, height, 1))
            .setMipLevels(levels)
            .setArrayLayers(layers)
            .setFormat(format)
            .setUsage(usage)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    );
}
Image Image::create_3d(
    Device& device,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    uint32_t levels,
    uint32_t layers,
    vk::Format format,
    vk::ImageUsageFlags usage
) {
    return create(
        device,
        vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e3D)
            .setExtent(vk::Extent3D(width, height, depth))
            .setMipLevels(levels)
            .setArrayLayers(layers)
            .setFormat(format)
            .setUsage(usage)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setSharingMode(vk::SharingMode::eExclusive),
        AllocationCreateInfo()
            .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
    );
}

void Image::destroy(Device& device) {
    vmaDestroyImage(device.allocator, image, allocation);
}
Image::operator vk::Image() const { return image; }
Image::operator VkImage() const { return image; }
bool Image::operator!() const { return !image; }
vk::Image* Image::operator->() { return &image; }
vk::Image& Image::operator*() { return image; }

ImageView ImageView::create(
    Device& device, vk::ImageViewCreateInfo create_info
) {
    ImageView image_view;
    image_view.image_view = device->createImageView(create_info);
    return image_view;
}
ImageView ImageView::create_1d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags,
    uint32_t base_level,
    uint32_t level_count,
    uint32_t base_layer,
    uint32_t layer_count
) {
    return create(
        device,
        vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e1D)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange(
                aspect_flags, base_level, level_count, base_layer, layer_count
            ))
    );
}
ImageView ImageView::create_1d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags
) {
    return create_1d(device, image, format, aspect_flags, 0, 1, 0, 1);
}
ImageView ImageView::create_2d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags,
    uint32_t base_level,
    uint32_t level_count,
    uint32_t base_layer,
    uint32_t layer_count
) {
    return create(
        device,
        vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange(
                aspect_flags, base_level, level_count, base_layer, layer_count
            ))
    );
}
ImageView ImageView::create_2d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags
) {
    return create_2d(device, image, format, aspect_flags, 0, 1, 0, 1);
}
ImageView ImageView::create_3d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags,
    uint32_t base_level,
    uint32_t level_count,
    uint32_t base_layer,
    uint32_t layer_count
) {
    return create(
        device,
        vk::ImageViewCreateInfo()
            .setImage(image)
            .setViewType(vk::ImageViewType::e3D)
            .setFormat(format)
            .setSubresourceRange(vk::ImageSubresourceRange(
                aspect_flags, base_level, level_count, base_layer, layer_count
            ))
    );
}
ImageView ImageView::create_3d(
    Device& device,
    Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags
) {
    return create_3d(device, image, format, aspect_flags, 0, 1, 0, 1);
}

void ImageView::destroy(Device& device) {
    device->destroyImageView(image_view);
}
ImageView::operator vk::ImageView() const { return image_view; }
ImageView::operator VkImageView() const { return image_view; }
bool ImageView::operator!() const { return !image_view; }
vk::ImageView* ImageView::operator->() { return &image_view; }
vk::ImageView& ImageView::operator*() { return image_view; }

Sampler Sampler::create(Device& device, vk::SamplerCreateInfo create_info) {
    Sampler sampler;
    sampler.sampler = device->createSampler(create_info);
    return sampler;
}
Sampler Sampler::create(
    Device& device,
    vk::Filter min_filter,
    vk::Filter mag_filter,
    vk::SamplerAddressMode u_address_mode,
    vk::SamplerAddressMode v_address_mode,
    vk::SamplerAddressMode w_address_mode,
    vk::BorderColor border_color
) {
    return create(
        device, vk::SamplerCreateInfo()
                    .setMagFilter(mag_filter)
                    .setMinFilter(min_filter)
                    .setAddressModeU(u_address_mode)
                    .setAddressModeV(v_address_mode)
                    .setAddressModeW(w_address_mode)
                    .setAnisotropyEnable(VK_TRUE)
                    .setBorderColor(border_color)
                    .setCompareEnable(VK_TRUE)
                    .setCompareOp(vk::CompareOp::eAlways)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                    .setMipLodBias(0.0f)
                    .setMinLod(0.0f)
                    .setMaxLod(0.0f)
    );
}

void Sampler::destroy(Device& device) { device->destroySampler(sampler); }
Sampler::operator vk::Sampler() const { return sampler; }
Sampler::operator VkSampler() const { return sampler; }
bool Sampler::operator!() const { return !sampler; }
vk::Sampler* Sampler::operator->() { return &sampler; }
vk::Sampler& Sampler::operator*() { return sampler; }

DescriptorSetLayout DescriptorSetLayout::create(
    Device& device, vk::DescriptorSetLayoutCreateInfo create_info
) {
    DescriptorSetLayout descriptor_set_layout;
    descriptor_set_layout.descriptor_set_layout =
        device->createDescriptorSetLayout(create_info);
    return descriptor_set_layout;
}
DescriptorSetLayout DescriptorSetLayout::create(
    Device& device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings
) {
    return create(
        device, vk::DescriptorSetLayoutCreateInfo().setBindings(bindings)
    );
}

void DescriptorSetLayout::destroy(Device& device) {
    device->destroyDescriptorSetLayout(descriptor_set_layout);
}
DescriptorSetLayout::operator vk::DescriptorSetLayout() const {
    return descriptor_set_layout;
}
DescriptorSetLayout::operator VkDescriptorSetLayout() const {
    return descriptor_set_layout;
}
bool DescriptorSetLayout::operator!() const { return !descriptor_set_layout; }
vk::DescriptorSetLayout* DescriptorSetLayout::operator->() {
    return &descriptor_set_layout;
}
vk::DescriptorSetLayout& DescriptorSetLayout::operator*() {
    return descriptor_set_layout;
}

PipelineLayout PipelineLayout::create(
    Device& device, vk::PipelineLayoutCreateInfo create_info
) {
    PipelineLayout pipeline_layout;
    pipeline_layout.pipeline_layout = device->createPipelineLayout(create_info);
    return pipeline_layout;
}
PipelineLayout PipelineLayout::create(
    Device& device, DescriptorSetLayout& descriptor_set_layout
) {
    return create(
        device,
        vk::PipelineLayoutCreateInfo().setSetLayouts(*descriptor_set_layout)
    );
}
PipelineLayout PipelineLayout::create(
    Device& device,
    DescriptorSetLayout& descriptor_set_layout,
    vk::PushConstantRange& push_constant_range
) {
    return create(
        device, vk::PipelineLayoutCreateInfo()
                    .setSetLayouts(*descriptor_set_layout)
                    .setPushConstantRanges(push_constant_range)
    );
}
PipelineLayout PipelineLayout::create(
    Device& device,
    std::vector<DescriptorSetLayout>& descriptor_set_layouts,
    std::vector<vk::PushConstantRange>& push_constant_ranges
) {
    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_vk;
    descriptor_set_layouts_vk.reserve(descriptor_set_layouts.size());
    for (auto& layout : descriptor_set_layouts) {
        descriptor_set_layouts_vk.push_back(*layout);
    }
    return create(
        device, vk::PipelineLayoutCreateInfo()
                    .setSetLayouts(descriptor_set_layouts_vk)
                    .setPushConstantRanges(push_constant_ranges)
    );
}

void PipelineLayout::destroy(Device& device) {
    device->destroyPipelineLayout(pipeline_layout);
}
PipelineLayout::operator vk::PipelineLayout() const { return pipeline_layout; }
PipelineLayout::operator VkPipelineLayout() const { return pipeline_layout; }
bool PipelineLayout::operator!() const { return !pipeline_layout; }
vk::PipelineLayout* PipelineLayout::operator->() { return &pipeline_layout; }
vk::PipelineLayout& PipelineLayout::operator*() { return pipeline_layout; }

Pipeline Pipeline::create(
    Device& device,
    vk::PipelineCache pipeline_cache,
    vk::GraphicsPipelineCreateInfo create_info
) {
    Pipeline pipeline;
    pipeline.pipeline =
        device->createGraphicsPipeline(pipeline_cache, create_info).value;
    return pipeline;
}
Pipeline Pipeline::create(
    Device& device, vk::GraphicsPipelineCreateInfo create_info
) {
    return create(device, vk::PipelineCache(), create_info);
}
Pipeline Pipeline::create(
    Device& device,
    vk::PipelineCache pipeline_cache,
    vk::ComputePipelineCreateInfo create_info
) {
    Pipeline pipeline;
    pipeline.pipeline =
        device->createComputePipeline(pipeline_cache, create_info).value;
    return pipeline;
}
Pipeline Pipeline::create(
    Device& device, vk::ComputePipelineCreateInfo create_info
) {
    return create(device, vk::PipelineCache(), create_info);
}

void Pipeline::destroy(Device& device) { device->destroyPipeline(pipeline); }
Pipeline::operator vk::Pipeline() const { return pipeline; }
Pipeline::operator VkPipeline() const { return pipeline; }
bool Pipeline::operator!() const { return !pipeline; }
vk::Pipeline* Pipeline::operator->() { return &pipeline; }
vk::Pipeline& Pipeline::operator*() { return pipeline; }

DescriptorPool DescriptorPool::create(
    Device& device, vk::DescriptorPoolCreateInfo create_info
) {
    DescriptorPool descriptor_pool;
    descriptor_pool.descriptor_pool = device->createDescriptorPool(create_info);
    return descriptor_pool;
}

void DescriptorPool::destroy(Device& device) {
    device->destroyDescriptorPool(descriptor_pool);
}
DescriptorPool::operator vk::DescriptorPool() const { return descriptor_pool; }
DescriptorPool::operator VkDescriptorPool() const { return descriptor_pool; }
bool DescriptorPool::operator!() const { return !descriptor_pool; }
vk::DescriptorPool* DescriptorPool::operator->() { return &descriptor_pool; }
vk::DescriptorPool& DescriptorPool::operator*() { return descriptor_pool; }

DescriptorSet DescriptorSet::create(
    Device& device, vk::DescriptorSetAllocateInfo allocate_info
) {
    DescriptorSet descriptor_set;
    descriptor_set.descriptor_set =
        device->allocateDescriptorSets(allocate_info)[0];
    return descriptor_set;
}
DescriptorSet DescriptorSet::create(
    Device& device,
    DescriptorPool& descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout
) {
    vk::DescriptorSetAllocateInfo allocate_info;
    allocate_info.setDescriptorPool(descriptor_pool)
        .setSetLayouts(descriptor_set_layout);
    return create(device, allocate_info);
}
std::vector<DescriptorSet> DescriptorSet::create(
    Device& device,
    DescriptorPool& descriptor_pool,
    std::vector<DescriptorSetLayout>& descriptor_set_layouts
) {
    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_vk;
    descriptor_set_layouts_vk.reserve(descriptor_set_layouts.size());
    for (auto& layout : descriptor_set_layouts) {
        descriptor_set_layouts_vk.push_back(*layout);
    }
    vk::DescriptorSetAllocateInfo allocate_info;
    allocate_info.setDescriptorPool(descriptor_pool)
        .setSetLayouts(descriptor_set_layouts_vk);
    auto descriptor_sets = device->allocateDescriptorSets(allocate_info);
    std::vector<DescriptorSet> sets;
    sets.reserve(descriptor_sets.size());
    for (auto& descriptor_set : descriptor_sets) {
        sets.push_back(DescriptorSet{descriptor_set});
    }
    return sets;
}

void DescriptorSet::destroy(Device& device, DescriptorPool& descriptor_pool) {
    device->freeDescriptorSets(descriptor_pool, descriptor_set);
}
DescriptorSet::operator vk::DescriptorSet() const { return descriptor_set; }
DescriptorSet::operator VkDescriptorSet() const { return descriptor_set; }
bool DescriptorSet::operator!() const { return !descriptor_set; }
vk::DescriptorSet* DescriptorSet::operator->() { return &descriptor_set; }
vk::DescriptorSet& DescriptorSet::operator*() { return descriptor_set; }

Fence Fence::create(Device& device, vk::FenceCreateInfo create_info) {
    Fence fence;
    fence.fence = device->createFence(create_info);
    return fence;
}

void Fence::destroy(Device& device) { device->destroyFence(fence); }
Fence::operator vk::Fence() const { return fence; }
Fence::operator VkFence() const { return fence; }
bool Fence::operator!() const { return !fence; }
vk::Fence* Fence::operator->() { return &fence; }
vk::Fence& Fence::operator*() { return fence; }

Semaphore Semaphore::create(
    Device& device, vk::SemaphoreCreateInfo create_info
) {
    Semaphore semaphore;
    semaphore.semaphore = device->createSemaphore(create_info);
    return semaphore;
}

void Semaphore::destroy(Device& device) { device->destroySemaphore(semaphore); }
Semaphore::operator vk::Semaphore() const { return semaphore; }
Semaphore::operator VkSemaphore() const { return semaphore; }
bool Semaphore::operator!() const { return !semaphore; }
vk::Semaphore* Semaphore::operator->() { return &semaphore; }
vk::Semaphore& Semaphore::operator*() { return semaphore; }

Surface Surface::create(Instance& instance, GLFWwindow* window) {
    Surface surface_component;
    if (glfwCreateWindowSurface(
            instance, window, nullptr,
            reinterpret_cast<VkSurfaceKHR*>(&surface_component.surface)
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
    return surface_component;
}

void Surface::destroy(Instance& instance) {
    instance->destroySurfaceKHR(surface);
}
Surface::operator vk::SurfaceKHR() const { return surface; }
Surface::operator VkSurfaceKHR() const { return surface; }
bool Surface::operator!() const { return !surface; }
vk::SurfaceKHR* Surface::operator->() { return &surface; }
vk::SurfaceKHR& Surface::operator*() { return surface; }

Swapchain Swapchain::create(
    PhysicalDevice& physical_device,
    Device& device,
    Surface& surface,
    bool vsync
) {
    Swapchain swapchain;
    auto surface_capabilities =
        physical_device->getSurfaceCapabilitiesKHR(surface);
    auto surface_formats = physical_device->getSurfaceFormatsKHR(surface);
    auto present_modes = physical_device->getSurfacePresentModesKHR(surface);
    auto format_iter = std::find_if(
        surface_formats.begin(), surface_formats.end(),
        [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );
    auto mode_iter = std::find(
        present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox
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
        .setImageUsage(
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eInputAttachment
        )
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
        swapchain.in_flight_fence[i] = device->createFence(
            vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)
        );
    }
    return swapchain;
}

void Swapchain::recreate(
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
        .setImageUsage(
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eInputAttachment
        )
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
void Swapchain::destroy(Device& device) {
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
Swapchain::operator vk::SwapchainKHR() const { return swapchain; }
Swapchain::operator VkSwapchainKHR() const { return swapchain; }
Swapchain::operator vk::SwapchainKHR&() { return swapchain; }
Image Swapchain::next_image(Device& device) {
    current_frame = (current_frame + 1) % 2;
    auto res = device->waitForFences(
        1, &in_flight_fence[current_frame], VK_TRUE, UINT64_MAX
    );
    res = device->resetFences(1, &in_flight_fence[current_frame]);
    image_index = device
                      ->acquireNextImageKHR(
                          swapchain, UINT64_MAX,
                          image_available_semaphore[current_frame], vk::Fence()
                      )
                      .value;
    return Image{.image = images[image_index]};
}
Image Swapchain::current_image() const {
    return Image{.image = images[image_index]};
}
ImageView Swapchain::current_image_view() const {
    return image_views[image_index];
}
vk::Semaphore& Swapchain::image_available() {
    return image_available_semaphore[current_frame];
}
vk::Semaphore& Swapchain::render_finished() {
    return render_finished_semaphore[current_frame];
}
vk::Fence& Swapchain::fence() { return in_flight_fence[current_frame]; }
bool Swapchain::operator!() const { return !swapchain; }
vk::SwapchainKHR* Swapchain::operator->() { return &swapchain; }
vk::SwapchainKHR& Swapchain::operator*() { return swapchain; }

RenderPass RenderPass::create(
    Device& device, vk::RenderPassCreateInfo create_info
) {
    RenderPass render_pass;
    render_pass.render_pass = device->createRenderPass(create_info);
    return render_pass;
}

void RenderPass::destroy(Device& device) {
    device->destroyRenderPass(render_pass);
}
RenderPass::operator vk::RenderPass() const { return render_pass; }
RenderPass::operator VkRenderPass() const { return render_pass; }
bool RenderPass::operator!() const { return !render_pass; }
vk::RenderPass* RenderPass::operator->() { return &render_pass; }
vk::RenderPass& RenderPass::operator*() { return render_pass; }

Framebuffer Framebuffer::create(
    Device& device, vk::FramebufferCreateInfo create_info
) {
    Framebuffer framebuffer;
    framebuffer.framebuffer = device->createFramebuffer(create_info);
    return framebuffer;
}

void Framebuffer::destroy(Device& device) {
    device->destroyFramebuffer(framebuffer);
}
Framebuffer::operator vk::Framebuffer() const { return framebuffer; }
Framebuffer::operator VkFramebuffer() const { return framebuffer; }
bool Framebuffer::operator!() const { return !framebuffer; }
vk::Framebuffer* Framebuffer::operator->() { return &framebuffer; }
vk::Framebuffer& Framebuffer::operator*() { return framebuffer; }

ShaderModule ShaderModule::create(
    Device& device, const vk::ShaderModuleCreateInfo& create_info
) {
    ShaderModule shader_module;
    shader_module.shader_module = device->createShaderModule(create_info);
    return shader_module;
}

void ShaderModule::destroy(Device& device) {
    device->destroyShaderModule(shader_module);
}
ShaderModule::operator vk::ShaderModule() const { return shader_module; }
ShaderModule::operator VkShaderModule() const { return shader_module; }
bool ShaderModule::operator!() const { return !shader_module; }
vk::ShaderModule* ShaderModule::operator->() { return &shader_module; }
vk::ShaderModule& ShaderModule::operator*() { return shader_module; }

PipelineCache PipelineCache::create(Device& device) {
    PipelineCache pipeline_cache;
    pipeline_cache.pipeline_cache = device->createPipelineCache({});
    return pipeline_cache;
}

void PipelineCache::destroy(Device& device) {
    device->destroyPipelineCache(pipeline_cache);
}
PipelineCache::operator vk::PipelineCache() const { return pipeline_cache; }
PipelineCache::operator VkPipelineCache() const { return pipeline_cache; }
bool PipelineCache::operator!() const { return !pipeline_cache; }
vk::PipelineCache* PipelineCache::operator->() { return &pipeline_cache; }
vk::PipelineCache& PipelineCache::operator*() { return pipeline_cache; }