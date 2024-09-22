#include "pixel_engine/render_vk.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace pixel_engine::render_vk;
using namespace pixel_engine::render_vk::systems;
using namespace pixel_engine::render_vk::components;

using namespace pixel_engine::prelude;

static std::shared_ptr<spdlog::logger> logger =
    spdlog::default_logger()->clone("render_vk");

namespace tools {
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

vk::Instance create_instance(Device &device) {
    auto app_info = vk::ApplicationInfo()
                        .setPApplicationName("Pixel Engine")
                        .setPEngineName("Pixel Engine")
                        .setApiVersion(VK_MAKE_VERSION(0, 1, 0))
                        .setEngineVersion(VK_MAKE_VERSION(0, 1, 0))
                        .setApiVersion(VK_API_VERSION_1_2);
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
    auto instance_info =
        vk::InstanceCreateInfo().setPApplicationInfo(&app_info);

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

    device.instance = vk::createInstance(instance_info);
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
            .setPfnUserCallback(debugUtilsMessengerCallback);
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(
        device.instance, "vkCreateDebugUtilsMessengerEXT"
    );
    if (!func) {
        logger->error("Failed to load function vkCreateDebugUtilsMessengerEXT");
        throw std::runtime_error(
            "Failed to load function vkCreateDebugUtilsMessengerEXT"
        );
    }
    func(
        device.instance,
        reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(
            &debugMessengerInfo
        ),
        nullptr,
        reinterpret_cast<VkDebugUtilsMessengerEXT *>(&device.debug_messenger)
    );
#endif
}

void destroy_instance(Device &device) {
    logger->info("Destroying Vulkan Instance");
#if !defined(NDEBUG)
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)glfwGetInstanceProcAddress(
        device.instance, "vkDestroyDebugUtilsMessengerEXT"
    );
    if (!func) {
        logger->error("Failed to load function vkDestroyDebugUtilsMessengerEXT"
        );
        throw std::runtime_error(
            "Failed to load function vkDestroyDebugUtilsMessengerEXT"
        );
    }
    func(device.instance, device.debug_messenger, nullptr);
#endif
    device.instance.destroy();
}

void pick_physical_device(Device &device) {
    auto physical_devices = device.instance.enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        logger->error("No physical devices found");
        throw std::runtime_error("No physical devices found");
    }
    device.physical_device = physical_devices.front();
}

void create_logical_device(Device &device) {
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
        logger->error("No queue family found with graphics support");
        throw std::runtime_error("No queue family found with graphics support");
    }
    float queue_priority = 1.0f;
    auto queue_info = vk::DeviceQueueCreateInfo()
                          .setQueueFamilyIndex(queue_family_index)
                          .setQueueCount(1)
                          .setPQueuePriorities(&queue_priority);
    auto device_info =
        vk::DeviceCreateInfo().setQueueCreateInfoCount(1).setPQueueCreateInfos(
            &queue_info
        );
    device.logical_device = device.physical_device.createDevice(device_info);
    device.queue = device.logical_device.getQueue(queue_family_index, 0);
    device.queue_family_index = queue_family_index;
}

void destroy_logical_device(Device &device) {
    logger->info("Destroying Logical Device");
    device.logical_device.destroy();
}

void create_allocator(Device &device) {
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = device.physical_device;
    allocator_info.device = device.logical_device;
    allocator_info.instance = device.instance;
    if (vmaCreateAllocator(&allocator_info, &device.allocator) != VK_SUCCESS) {
        logger->error("Failed to create VMA allocator");
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

void destroy_allocator(Device &device) {
    logger->info("Destroying VMA Allocator");
    vmaDestroyAllocator(device.allocator);
}

void create_command_pool(Device &device) {
    auto command_pool_info =
        vk::CommandPoolCreateInfo()
            .setQueueFamilyIndex(device.queue_family_index)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    device.command_pool =
        device.logical_device.createCommandPool(command_pool_info);
}
}  // namespace tools

InFlightFrame &Frames::current() { return frames[current_frame]; }

void systems::create_device(Command cmd) {
    using namespace tools;
    Device device;
    create_instance(device);
    pick_physical_device(device);
    create_logical_device(device);
    create_allocator(device);
    create_command_pool(device);
    cmd.spawn(device, RenderVKContext());
}