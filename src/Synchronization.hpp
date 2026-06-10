#pragma once
#include<vector>
#include<vulkan/vulkan.hpp>


namespace de {
    class Device;

    class swapchain_sync
    {
    public:

        struct frame_sync
        {
            const vk::Fence& inFlightFence;
            const vk::Semaphore& readyForRenderingSemaphore;
            const vk::Semaphore& readyForPresentingSemaphore;
        };


        swapchain_sync(const Device& device, std::uint32_t maxImagesInFlight);

        frame_sync get_next_frame_sync();

    private:

        std::uint32_t m_maxImagesInFlight;
        std::uint32_t m_currentFrameIndex = 0;

        std::vector< vk::UniqueFence > m_inFlightFences;
        std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
        std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
    };

    class swapchain_state
    {
    public:

        

            swapchain_state(
                const Device& device,
                const vk::SwapchainKHR& swapchain,
                const vk::RenderPass& renderPass,
                const vk::Extent2D& imageExtent,
                const vk::Format& imageFormat,
                std::uint32_t maxImagesInFlight);

  

    private:

        std::uint32_t m_maxImagesInFlight;
        std::uint32_t m_currentFrameIndex = 0;

        std::vector< vk::UniqueImageView > m_imageViews;
        std::vector< vk::UniqueFramebuffer > m_framebuffers;

        std::vector< vk::UniqueFence > m_inFlightFences;
        std::vector< vk::UniqueSemaphore > m_readyForRenderingSemaphores;
        std::vector< vk::UniqueSemaphore > m_readyForPresentingSemaphores;
    };
}