#include "pixel_engine/render_vk/vulkan/device.h"

using namespace pixel_engine::render_vk::vulkan;

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
    void *pUserData
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

    auto *logger = (spdlog::logger *)pUserData;

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

Instance Instance::create(
    const char *app_name,
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
    std::vector<char const *> layers = {
#if !defined(NDEBUG)
        "VK_LAYER_KHRONOS_validation"
#endif
    };
    std::vector<char const *> extensions = {
#if !defined(NDEBUG)
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
    };
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions.insert(
        extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount
    );
    auto instance_info = vk::InstanceCreateInfo()
                             .setPApplicationInfo(&app_info)
                             .setPEnabledExtensionNames(extensions)
                             .setPEnabledLayerNames(layers);

    logger->info("Creating Vulkan Instance");

    std::string instance_layers_info = "Instance Layers:\n";
    for (auto &layer : layers) {
        instance_layers_info += std::format("\t{}\n", layer);
    }
    logger->info(instance_layers_info);
    std::string instance_extensions_info = "Instance Extensions:\n";
    for (auto &extension : extensions) {
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
        reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(
            &debugMessengerInfo
        ),
        nullptr,
        reinterpret_cast<VkDebugUtilsMessengerEXT *>(&instance.debug_messenger)
    );
#endif
}

Device Device::create(Instance instance) {
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
    for (auto &queue_family_property : queue_family_properties) {
        if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics) {
            break;
        }
        queue_family_index++;
    }
    if (queue_family_index == queue_family_properties.size()) {
        device.instance.logger->error("No queue family with graphics support");
        throw std::runtime_error("No queue family with graphics support");
    }
    device.queue_family_index = queue_family_index;
    float queue_priority = 1.0f;
    auto queue_create_info = vk::DeviceQueueCreateInfo()
                                 .setQueueFamilyIndex(queue_family_index)
                                 .setQueueCount(1)
                                 .setPQueuePriorities(&queue_priority);
    std::vector<char const *> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    auto device_features =
        vk::PhysicalDeviceFeatures().setSamplerAnisotropy(VK_TRUE);
    auto device_info = vk::DeviceCreateInfo()
                           .setQueueCreateInfoCount(1)
                           .setPQueueCreateInfos(&queue_create_info)
                           .setPEnabledFeatures(&device_features)
                           .setPEnabledExtensionNames(device_extensions);
    device.logical_device = device.physical_device.createDevice(device_info);
    device.queue = device.logical_device.getQueue(queue_family_index, 0);
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = device.physical_device;
    allocator_info.device = device.logical_device;
    allocator_info.instance = device.instance.instance;
    vmaCreateAllocator(&allocator_info, &device.allocator);
    vk::CommandPoolCreateInfo command_pool_info;
    command_pool_info.setQueueFamilyIndex(queue_family_index);
    device.command_pool =
        device.logical_device.createCommandPool(command_pool_info);
    return device;
}