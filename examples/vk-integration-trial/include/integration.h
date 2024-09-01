#include <pixel_engine/prelude.h>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "fragment_shader.h"
#include "vertex_shader.h"

using namespace pixel_engine::prelude;
using namespace pixel_engine::window;

namespace vk_trial {

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
    void * /*pUserData*/) {
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

    std::cerr << vk::to_string(
                     static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(
                         messageSeverity))
              << ": "
              << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(
                     messageTypes))
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
                             pCallbackData->pObjects[i].objectType))
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
            }));
        enabledExtensions.push_back(ext.data());
    }
#if !defined(NDEBUG)
    if (std::none_of(
            extensions.begin(), extensions.end(),
            [](std::string const &extension) {
                return extension == VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            }) &&
        std::any_of(
            extensionProperties.begin(), extensionProperties.end(),
            [](vk::ExtensionProperties const &ep) {
                return (
                    strcmp(
                        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ep.extensionName) ==
                    0);
            })) {
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
            }));
        enabledLayers.push_back(layer.data());
    }
#if !defined(NDEBUG)
    // Enable standard validation layer to find as much errors as possible!
    if (std::none_of(
            layers.begin(), layers.end(),
            [](std::string const &layer) {
                return layer == "VK_LAYER_KHRONOS_validation";
            }) &&
        std::any_of(
            layerProperties.begin(), layerProperties.end(),
            [](vk::LayerProperties const &lp) {
                return (
                    strcmp("VK_LAYER_KHRONOS_validation", lp.layerName) == 0);
            })) {
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
    std::vector<char const *> const &extensions) {
#if defined(NDEBUG)
    // in non-debug mode just use the InstanceCreateInfo for instance creation
    vk::StructureChain<vk::InstanceCreateInfo> instanceCreateInfo(
        {instanceCreateFlagBits, &applicationInfo, layers, extensions});
#else
    // in debug mode, addionally use the debugUtilsMessengerCallback in instance
    // creation!
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::StructureChain<
        vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
        instanceCreateInfo(
            {instanceCreateFlagBits, &applicationInfo, layers, extensions},
            {{}, severityFlags, messageTypeFlags, debugUtilsMessengerCallback});
#endif
    return instanceCreateInfo;
}

void init_instance(Command command) {
    vk::ApplicationInfo app_info(
        "Hello Vulkan", 1, "No Engine", 1, VK_API_VERSION_1_0);
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
        glfw_extensions + glfw_extension_count);
    spdlog::info("GLFW Extensions Count: {}", glfw_extension_count);
    for (auto const &layer : enabled_layers) {
        spdlog::info("Enabled Layer: {}", layer);
    }
    for (auto const &ext : enabled_extensions) {
        spdlog::info("Enabled Extension: {}", ext);
    }
    vk::Instance instance =
        vk::createInstance(makeInstanceCreateInfoChain(
                               {}, app_info, enabled_layers, enabled_extensions)
                               .get<vk::InstanceCreateInfo>());
    command.spawn(instance);
}

void get_physical_device(Command command, Query<Get<vk::Instance>> query) {
    if (!query.single().has_value()) return;
    auto [instance] = query.single().value();
    auto physical_devices = instance.enumeratePhysicalDevices().front();
    command.spawn(physical_devices);
    spdlog::info(
        "Physical Device: {}",
        physical_devices.getProperties().deviceName.data());
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
};

struct Queues {
    vk::Queue graphics_queue;
    vk::Queue present_queue;
};

void create_device(
    Command command, Query<Get<vk::PhysicalDevice>> query,
    Query<Get<vk::SurfaceKHR>> surface_query) {
    if (!query.single().has_value()) return;
    if (!surface_query.single().has_value()) return;
    auto [physical_device] = query.single().value();
    auto [surface] = surface_query.single().value();
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
        queue_family_indices.present_family.value()};
    float queue_priority = 1.0f;
    std::vector<char const *> enabled_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<vk::DeviceQueueCreateInfo> device_queue_create_info;
    for (uint32_t queue_family : unique_queue_families) {
        device_queue_create_info.push_back(
            vk::DeviceQueueCreateInfo({}, queue_family, 1, &queue_priority));
    }
    vk::Device device = physical_device.createDevice(vk::DeviceCreateInfo(
        {}, device_queue_create_info, {}, enabled_extensions));
    command.spawn(device);
    command.spawn(queue_family_indices);
    vk::Queue graphics_queue =
        device.getQueue(queue_family_indices.graphics_family.value(), 0);
    vk::Queue present_queue =
        device.getQueue(queue_family_indices.present_family.value(), 0);
    command.spawn(Queues{graphics_queue, present_queue});
}

void create_window_surface(
    Command command, Query<Get<vk::Instance>> query,
    Query<Get<const vk::PhysicalDevice>> physical_device_query,
    Query<Get<const WindowHandle>, With<PrimaryWindow>> window_query) {
    if (!query.single().has_value()) return;
    if (!physical_device_query.single().has_value()) return;
    if (!window_query.single().has_value()) return;
    auto [physical_device] = physical_device_query.single().value();
    auto [instance] = query.single().value();
    auto [window_handle] = window_query.single().value();
    spdlog::info("create window surface");
    vk::SurfaceKHR surface;
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(
                static_cast<VkInstance>(instance), window_handle.window_handle,
                nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        surface = _surface;
    }
    command.spawn(surface);
}

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

void query_swap_chain_support(
    Command command, Query<Get<vk::PhysicalDevice>> device_query,
    Query<Get<vk::SurfaceKHR>> surface_query) {
    if (!device_query.single().has_value()) return;
    if (!surface_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [surface] = surface_query.single().value();
    spdlog::info("query swap chain support");
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    if (details.formats.empty() || details.presentModes.empty()) {
        spdlog::error("No swap chain support!");
        throw std::runtime_error("No swap chain support!");
    }
    command.spawn(details);
}

struct ChosenSwapChainDetails {
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    uint32_t image_count;
};

void create_swap_chain(
    Command command, Query<Get<vk::PhysicalDevice>> physical_device_query,
    Query<Get<vk::Device>> device_query,
    Query<Get<vk::SurfaceKHR>> surface_query,
    Query<Get<QueueFamilyIndices>> family_query,
    Query<Get<const SwapChainSupportDetails>> swap_chain_support_query,
    Query<Get<const WindowSize>, With<PrimaryWindow>> window_query) {
    if (!physical_device_query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    if (!surface_query.single().has_value()) return;
    if (!family_query.single().has_value()) return;
    if (!swap_chain_support_query.single().has_value()) return;
    if (!window_query.single().has_value()) return;
    spdlog::info("create swap chain");
    auto [physical_device] = physical_device_query.single().value();
    auto [device] = device_query.single().value();
    auto [surface] = surface_query.single().value();
    auto [queue_family_indices] = family_query.single().value();
    auto [swap_chain_support] = swap_chain_support_query.single().value();
    auto [window_size] = window_query.single().value();
    // Choose the best surface format
    auto format_iter = std::find_if(
        swap_chain_support.formats.begin(), swap_chain_support.formats.end(),
        [](vk::SurfaceFormatKHR const &format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
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
        });
    if (present_mode_iter == swap_chain_support.presentModes.end()) {
        present_mode_iter = std::find_if(
            swap_chain_support.presentModes.begin(),
            swap_chain_support.presentModes.end(),
            [](vk::PresentModeKHR const &mode) {
                return mode == vk::PresentModeKHR::eFifo;
            });
    }
    vk::PresentModeKHR present_mode = *present_mode_iter;
    present_mode = vk::PresentModeKHR::eFifo;
    // Choose the best extent
    vk::Extent2D extent;
    if (swap_chain_support.capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        extent = swap_chain_support.capabilities.currentExtent;
    } else {
        extent.width = std::clamp(
            (uint32_t)window_size.width,
            swap_chain_support.capabilities.minImageExtent.width,
            swap_chain_support.capabilities.maxImageExtent.width);
        extent.height = std::clamp(
            (uint32_t)window_size.height,
            swap_chain_support.capabilities.minImageExtent.height,
            swap_chain_support.capabilities.maxImageExtent.height);
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
        queue_family_indices.present_family.value()};
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
    create_info.setPreTransform(
        swap_chain_support.capabilities.currentTransform);
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    create_info.setPresentMode(present_mode);
    create_info.setClipped(true);
    create_info.setOldSwapchain(nullptr);
    vk::SwapchainKHR swap_chain = device.createSwapchainKHR(create_info);
    image_count = device.getSwapchainImagesKHR(swap_chain).size();
    command.spawn(
        ChosenSwapChainDetails{format, present_mode, extent, image_count},
        swap_chain);
}

struct SwapChainImages {
    std::vector<vk::Image> images;
};

void get_swap_chain_images(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<vk::SwapchainKHR>> swap_chain_query) {
    if (!device_query.single().has_value()) return;
    if (!swap_chain_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [swap_chain] = swap_chain_query.single().value();
    spdlog::info("get swap chain images");
    command.spawn(SwapChainImages{device.getSwapchainImagesKHR(swap_chain)});
}

struct ImageViews {
    std::vector<vk::ImageView> views;
};

void create_image_views(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<SwapChainImages>> images_query,
    Query<Get<ChosenSwapChainDetails>> swap_chain_query) {
    if (!device_query.single().has_value()) return;
    if (!images_query.single().has_value()) return;
    if (!swap_chain_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [images] = images_query.single().value();
    auto [swap_chain] = swap_chain_query.single().value();
    spdlog::info("create image views");
    std::vector<vk::ImageView> image_views;
    image_views.reserve(images.images.size());
    for (auto const &image : images.images) {
        vk::ImageViewCreateInfo create_info;
        create_info.image = image;
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = swap_chain.format.format;
        create_info.components = {
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity};
        create_info.subresourceRange = {
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        image_views.push_back(device.createImageView(create_info));
    }
    command.spawn(ImageViews{image_views});
}

void create_render_pass(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<ChosenSwapChainDetails>> swap_chain_details_query) {
    if (!device_query.single().has_value()) return;
    if (!swap_chain_details_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [swap_chain_details] = swap_chain_details_query.single().value();
    spdlog::info("create render pass");
    vk::AttachmentDescription color_attachment(
        {}, swap_chain_details.format.format, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
    vk::AttachmentReference color_attachment_ref(
        0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass(
        {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
        &color_attachment_ref);
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    vk::RenderPass render_pass = device.createRenderPass(
        vk::RenderPassCreateInfo({}, color_attachment, subpass, dependency));
    command.spawn(render_pass);
}

void create_graphics_pipeline(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<ChosenSwapChainDetails>> swap_details_query,
    Query<Get<vk::RenderPass>> render_pass_query) {
    if (!device_query.single().has_value()) return;
    if (!swap_details_query.single().has_value()) return;
    if (!render_pass_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [swap_chain_details] = swap_details_query.single().value();
    auto [render_pass] = render_pass_query.single().value();
    spdlog::info("create graphics pipeline");
    // Create the shader modules and the shader stages
    auto vertex_code = std::vector<uint32_t>(
        vertex_spv, vertex_spv + sizeof(vertex_spv) / sizeof(uint32_t));
    auto fragment_code = std::vector<uint32_t>(
        fragment_spv, fragment_spv + sizeof(fragment_spv) / sizeof(uint32_t));
    vk::ShaderModule vertex_shader_module =
        device.createShaderModule(vk::ShaderModuleCreateInfo({}, vertex_code));
    vk::ShaderModule fragment_shader_module = device.createShaderModule(
        vk::ShaderModuleCreateInfo({}, fragment_code));
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        {{}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main"},
        {{},
         vk::ShaderStageFlagBits::eFragment,
         fragment_shader_module,
         "main"}};
    // create dynamic state
    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info(
        {}, dynamic_states);
    // create vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_create_info({}, {}, {});
    // create input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info(
        {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    // create viewport
    auto &extent = swap_chain_details.extent;
    vk::Viewport viewport(
        0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, extent);
    vk::PipelineViewportStateCreateInfo viewport_state_create_info(
        {}, 1, nullptr, 1, nullptr);
    // create rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer_create_info;
    rasterizer_create_info.setDepthClampEnable(VK_FALSE);
    rasterizer_create_info.setRasterizerDiscardEnable(VK_FALSE);
    rasterizer_create_info.setPolygonMode(vk::PolygonMode::eFill);
    rasterizer_create_info.setLineWidth(1.0f);
    rasterizer_create_info.setCullMode(vk::CullModeFlagBits::eBack);
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
        {0.0f, 0.0f, 0.0f, 0.0f});
    // create pipeline layout
    vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
    vk::PipelineLayout pipeline_layout =
        device.createPipelineLayout(pipeline_layout_create_info);
    command.spawn(pipeline_layout);

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
    create_info.setLayout(pipeline_layout);
    create_info.setRenderPass(render_pass);
    create_info.setSubpass(0);
    auto res = device.createGraphicsPipeline({}, create_info);
    if (res.result != vk::Result::eSuccess) {
        spdlog::error("Failed to create graphics pipeline!");
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    auto pipeline = res.value;
    command.spawn(pipeline);

    device.destroyShaderModule(vertex_shader_module);
    device.destroyShaderModule(fragment_shader_module);
}

struct Framebuffers {
    std::vector<vk::Framebuffer> framebuffers;
};

void create_framebuffer(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<vk::RenderPass>> render_pass_query,
    Query<Get<ImageViews>> image_views_query,
    Query<Get<ChosenSwapChainDetails>> swap_chain_query) {
    if (!device_query.single().has_value()) return;
    if (!render_pass_query.single().has_value()) return;
    if (!image_views_query.single().has_value()) return;
    if (!swap_chain_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [render_pass] = render_pass_query.single().value();
    auto [image_views] = image_views_query.single().value();
    auto [swap_chain] = swap_chain_query.single().value();
    spdlog::info("create framebuffer");
    Framebuffers framebuffers;
    framebuffers.framebuffers.reserve(image_views.views.size());
    for (auto const &view : image_views.views) {
        vk::FramebufferCreateInfo create_info(
            {}, render_pass, view, swap_chain.extent.width,
            swap_chain.extent.height, 1);
        framebuffers.framebuffers.push_back(
            device.createFramebuffer(create_info));
    }
    command.spawn(framebuffers);
}

void create_command_pool(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<QueueFamilyIndices>> family_query) {
    if (!device_query.single().has_value()) return;
    if (!family_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [queue_family_indices] = family_query.single().value();
    spdlog::info("create command pool");
    vk::CommandPoolCreateInfo create_info(
        {vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
        queue_family_indices.graphics_family.value());
    vk::CommandPool command_pool = device.createCommandPool(create_info);
    command.spawn(command_pool);
}

void create_command_buffer(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<vk::CommandPool>> command_pool_query) {
    if (!device_query.single().has_value()) return;
    if (!command_pool_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [command_pool] = command_pool_query.single().value();
    spdlog::info("create command buffer");
    vk::CommandBufferAllocateInfo allocate_info(
        command_pool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer command_buffer =
        device.allocateCommandBuffers(allocate_info).front();
    command.spawn(command_buffer);
}

struct CurrentFrame {
    uint32_t value;
};

struct ImageIndex {
    uint32_t value;
};

void record_command_buffer(
    Query<Get<vk::CommandBuffer>> cmd_query,
    Query<Get<vk::RenderPass>> render_pass_query,
    Resource<CurrentFrame> current_frame,
    Query<Get<ChosenSwapChainDetails>> swap_chain_details_query,
    Query<Get<Framebuffers>> framebuffers_query,
    Query<Get<vk::Pipeline>> pipeline_query, Resource<ImageIndex> image_index) {
    if (!cmd_query.single().has_value()) return;
    if (!render_pass_query.single().has_value()) return;
    if (!swap_chain_details_query.single().has_value()) return;
    if (!framebuffers_query.single().has_value()) return;
    if (!pipeline_query.single().has_value()) return;
    auto [cmd] = cmd_query.single().value();
    auto [render_pass] = render_pass_query.single().value();
    auto [swap_chain_details] = swap_chain_details_query.single().value();
    auto [framebuffers] = framebuffers_query.single().value();
    auto [pipeline] = pipeline_query.single().value();
    // spdlog::info("record command buffer");
    cmd.begin(vk::CommandBufferBeginInfo{});
    vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    if (!current_frame.has_value()) {
        spdlog::error("No current frame!");
        throw std::runtime_error("No current frame!");
    }
    auto &framebuffer = framebuffers.framebuffers[image_index->value];
    vk::RenderPassBeginInfo render_pass_begin_info(
        render_pass, framebuffer, {{0, 0}, swap_chain_details.extent},
        clear_color);
    cmd.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    vk::Viewport viewport(
        0.0f, 0.0f, (float)swap_chain_details.extent.width,
        (float)swap_chain_details.extent.height, 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, swap_chain_details.extent);
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, scissor);
    cmd.draw(3, 1, 0, 0);
    cmd.endRenderPass();
    cmd.end();
}

struct SyncObjects {
    vk::Semaphore image_available_semaphores;
    vk::Semaphore render_finished_semaphores;
    vk::Fence in_flight_fences;
};

void create_sync_object(
    Command command, Query<Get<vk::Device>> device_query,
    Query<Get<Framebuffers>> framebuffers_query) {
    if (!device_query.single().has_value()) return;
    if (!framebuffers_query.single().has_value()) return;
    auto [device] = device_query.single().value();
    auto [framebuffers] = framebuffers_query.single().value();
    spdlog::info("create sync object");
    SyncObjects sync_objects;
    auto &image_available_semaphores = sync_objects.image_available_semaphores;
    auto &render_finished_semaphores = sync_objects.render_finished_semaphores;
    auto &in_flight_fences = sync_objects.in_flight_fences;
    image_available_semaphores = device.createSemaphore({});
    render_finished_semaphores = device.createSemaphore({});
    in_flight_fences = device.createFence(
        vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled});
    command.spawn(sync_objects);
}

void add_resources(Command command) {
    command.insert_resource(CurrentFrame{0});
    command.insert_resource(ImageIndex{0});
}

void begin_draw(
    Query<Get<SyncObjects>> sync_objects_query,
    Query<Get<vk::Device>> device_query,
    Query<Get<vk::SwapchainKHR>> swap_chain_query, Resource<CurrentFrame> frame,
    Query<Get<vk::CommandBuffer>> cmd_query, Resource<ImageIndex> image_index) {
    if (!sync_objects_query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    if (!swap_chain_query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    auto [sync_objects] = sync_objects_query.single().value();
    auto [device] = device_query.single().value();
    auto [swap_chain] = swap_chain_query.single().value();
    auto [cmd] = cmd_query.single().value();
    spdlog::info("begin draw");
    device.waitForFences(
        sync_objects.in_flight_fences, VK_TRUE,
        std::numeric_limits<uint64_t>::max());
    device.resetFences(sync_objects.in_flight_fences);
    image_index->value =
        device
            .acquireNextImageKHR(
                swap_chain, std::numeric_limits<uint64_t>::max(),
                sync_objects.image_available_semaphores, vk::Fence{})
            .value;
    cmd.reset(vk::CommandBufferResetFlagBits{});
}

void end_draw(
    Query<Get<SyncObjects>> sync_objects_query,
    Query<Get<vk::Device>> device_query,
    Query<Get<vk::SwapchainKHR>> swap_chain_query, Resource<CurrentFrame> frame,
    Query<Get<vk::CommandBuffer>> cmd_query, Query<Get<Queues>> queue_query,
    Resource<ImageIndex> image_index_res) {
    if (!sync_objects_query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    if (!swap_chain_query.single().has_value()) return;
    if (!cmd_query.single().has_value()) return;
    if (!queue_query.single().has_value()) return;
    auto [sync_objects] = sync_objects_query.single().value();
    auto [device] = device_query.single().value();
    auto [swap_chain] = swap_chain_query.single().value();
    auto [cmd] = cmd_query.single().value();
    auto [queues] = queue_query.single().value();
    spdlog::info("end draw");
    auto image_index = image_index_res->value;
    vk::SubmitInfo submit_info;
    vk::PipelineStageFlags wait_stages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &sync_objects.image_available_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &sync_objects.render_finished_semaphores;
    auto &graphics_queue = queues.graphics_queue;
    auto &present_queue = queues.present_queue;
    graphics_queue.submit(submit_info, sync_objects.in_flight_fences);
    vk::PresentInfoKHR present_info;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &sync_objects.render_finished_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swap_chain;
    present_info.pImageIndices = &image_index;
    present_queue.presentKHR(present_info);
}

void destroy_sync_objects(
    Query<Get<Entity, SyncObjects>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, sync_objects] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroySemaphore(sync_objects.image_available_semaphores);
    device.destroySemaphore(sync_objects.render_finished_semaphores);
    device.destroyFence(sync_objects.in_flight_fences);
    command.entity(id).despawn();
}

void destroy_command_buffer(
    Query<Get<Entity, vk::CommandBuffer>> query, Command command,
    Query<Get<vk::Device>> device_query,
    Query<Get<vk::CommandPool>> pool_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    if (!pool_query.single().has_value()) return;
    auto [id, command_buffer] = query.single().value();
    auto [device] = device_query.single().value();
    auto [command_pool] = pool_query.single().value();
    device.freeCommandBuffers(command_pool, command_buffer);
    command.entity(id).despawn();
}

void destroy_command_pool(
    Query<Get<Entity, vk::CommandPool>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, command_pool] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroyCommandPool(command_pool);
    command.entity(id).despawn();
}

void destroy_framebuffers(
    Query<Get<Entity, Framebuffers>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, framebuffers] = query.single().value();
    auto [device] = device_query.single().value();
    for (auto const &framebuffer : framebuffers.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
    command.entity(id).despawn();
}

void destroy_pipeline(
    Query<Get<Entity, vk::Pipeline>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, pipeline] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroyPipeline(pipeline);
    command.entity(id).despawn();
}

void destroy_pipeline_layout(
    Query<Get<Entity, vk::PipelineLayout>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, pipeline_layout] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroyPipelineLayout(pipeline_layout);
    command.entity(id).despawn();
}

void destroy_render_pass(
    Query<Get<Entity, vk::RenderPass>> query, Command command,
    Query<Get<vk::Device>> device_query) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, render_pass] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroyRenderPass(render_pass);
    command.entity(id).despawn();
}

void destroy_image_views(
    Query<Get<Entity, ImageViews>> query, Query<Get<vk::Device>> device_query,
    Command command) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, image_views] = query.single().value();
    auto [device] = device_query.single().value();
    for (auto const &view : image_views.views) {
        device.destroyImageView(view);
    }
    command.entity(id).despawn();
}

void destroy_swap_chain(
    Query<Get<Entity, vk::SwapchainKHR>> query,
    Query<Get<vk::Device>> device_query, Command command) {
    if (!query.single().has_value()) return;
    if (!device_query.single().has_value()) return;
    auto [id, swap_chain] = query.single().value();
    auto [device] = device_query.single().value();
    device.destroySwapchainKHR(swap_chain);
    command.entity(id).despawn();
}

void destroy_surface(
    Query<Get<Entity, vk::SurfaceKHR>> query, Command command,
    Query<Get<vk::Instance>> instance_query) {
    if (!query.single().has_value()) return;
    if (!instance_query.single().has_value()) return;
    auto [id, surface] = query.single().value();
    auto [instance] = instance_query.single().value();
    instance.destroySurfaceKHR(surface);
    command.entity(id).despawn();
}

void destroy_device(Query<Get<Entity, vk::Device>> query, Command command) {
    if (!query.single().has_value()) return;
    auto [id, device] = query.single().value();
    device.destroy();
    command.entity(id).despawn();
}

void destroy_instance(Query<Get<Entity, vk::Instance>> query, Command command) {
    if (!query.single().has_value()) return;
    auto [id, instance] = query.single().value();
    instance.destroy();
    command.entity(id).despawn();
}

void wait_for_device(Query<Get<vk::Device>> query) {
    if (!query.single().has_value()) return;
    auto [device] = query.single().value();
    device.waitIdle();
}

struct VK_TrialPlugin : Plugin {
    void build(App &app) override {
        auto window_plugin = app.get_plugin<WindowPlugin>();
        window_plugin->set_primary_window_hints(
            {{GLFW_CLIENT_API, GLFW_NO_API}});

        app.add_plugin(LoopPlugin{});
        app.add_system_main(Startup{}, init_instance);
        app.add_system_main(PreStartup{}, add_resources);
        app.add_system_main(
            Startup{}, get_physical_device, after(init_instance));
        app.add_system_main(
            Startup{}, create_window_surface, after(get_physical_device));
        app.add_system_main(
            Startup{}, create_device, after(create_window_surface));
        app.add_system_main(
            Startup{}, query_swap_chain_support, after(create_device));
        app.add_system_main(
            Startup{}, create_swap_chain, after(query_swap_chain_support));
        app.add_system_main(
            Startup{}, get_swap_chain_images, after(create_swap_chain));
        app.add_system_main(
            Startup{}, create_image_views, after(get_swap_chain_images));
        app.add_system_main(
            Startup{}, create_render_pass, after(create_image_views));
        app.add_system_main(
            Startup{}, create_graphics_pipeline, after(create_render_pass));
        app.add_system_main(
            Startup{}, create_framebuffer, after(create_graphics_pipeline));
        app.add_system_main(
            Startup{}, create_command_pool, after(create_framebuffer));
        app.add_system_main(
            Startup{}, create_command_buffer, after(create_command_pool));
        app.add_system_main(
            Startup{}, create_sync_object, after(create_command_buffer));
        app.add_system_main(Render{}, begin_draw);
        app.add_system_main(Render{}, record_command_buffer, after(begin_draw));
        app.add_system_main(Render{}, end_draw, after(record_command_buffer));
        app.add_system_main(Exit{}, wait_for_device);
        app.add_system_main(Exit{}, destroy_instance);
        app.add_system_main(
            Exit{}, destroy_device, before(destroy_instance),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_surface, before(destroy_instance),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_pipeline, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_image_views, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_pipeline_layout, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_render_pass, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_framebuffers, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_swap_chain, before(destroy_device, destroy_surface),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_command_pool, before(destroy_device),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_command_buffer,
            before(destroy_device, destroy_command_pool),
            after(wait_for_device));
        app.add_system_main(
            Exit{}, destroy_sync_objects, before(destroy_device),
            after(wait_for_device));
    }
};

void run() {
    App app;
    app.add_plugin(pixel_engine::window::WindowPlugin{});
    app.add_plugin(vk_trial::VK_TrialPlugin{});
    app.run();
}
}  // namespace vk_trial