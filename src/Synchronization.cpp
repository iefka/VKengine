#include"DE_Device.hpp"
#include "Synchronization.hpp"

namespace de{
    // presentation.cpp

    swapchain_sync::swapchain_sync(const Device& device, std::uint32_t maxImagesInFlight)
    {
        const auto& logicalDevice = device.getLogicalDevice();
        for (std::uint32_t i = 0; i < maxImagesInFlight; ++i)
        {
            m_inFlightFences.push_back(logicalDevice.createFenceUnique(
                vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)
            ));

            m_readyForRenderingSemaphores.push_back(logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ));

            m_readyForPresentingSemaphores.push_back(logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ));
        }
    }

    swapchain_sync::frame_sync swapchain_sync::get_next_frame_sync()
    {
        const auto result = frame_sync{
            *m_inFlightFences[m_currentFrameIndex],
            *m_readyForRenderingSemaphores[m_currentFrameIndex],
            *m_readyForPresentingSemaphores[m_currentFrameIndex]
        };
        m_currentFrameIndex = ++m_currentFrameIndex % m_maxImagesInFlight;
        return result;
    }

   /* swapchain_state::swapchain_state(
        const vk::Device& logicalDevice,
        const vk::SwapchainKHR& swapchain,
        const vk::RenderPass& renderPass,
        const vk::Extent2D& imageExtent,
        const vk::Format& imageFormat,
        std::uint32_t maxImagesInFlight
    )
        : m_maxImagesInFlight{ maxImagesInFlight }
        , m_imageViews{ create_swapchain_image_views(logicalDevice, swapChain, imageFormat) }
        , m_framebuffers{ create_framebuffers(logicalDevice, m_imageViews, imageExtent, renderPass) }
    {
        for (std::uint32_t i = 0; i < maxImagesInFlight; ++i)
        {
            m_inFlightFences.push_back(logicalDevice.createFenceUnique(
                vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)
            ));

            m_readyForRenderingSemaphores.push_back(logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ));

            m_readyForPresentingSemaphores.push_back(logicalDevice.createSemaphoreUnique(
                vk::SemaphoreCreateInfo{}
            ));
        }
    }*/
}
