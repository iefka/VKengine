# Переход на dynamic rendering: план по коммитам

Состояние на входе: ветка `dev` @ `1482501` (upload-этап закрыт, мёртвый код убран).
Версия Vulkan, которую просит приложение: **1.4** (`DE_Instance.cpp:36`), значит
`dynamicRendering` и `synchronization2` — core, расширения не нужны, но фичи надо включить
явно.

Границы задачи:

- входит: `vkCmdBeginRendering` вместо `vkCmdBeginRenderPass`, пайплайны без `VkRenderPass`,
  явные переходы layout'ов перед/после кадра, перевод imgui, удаление ставших ненужными классов;
- **не входит**: `vkCmdPipelineBarrier2`, depth-образ на кадр вместо общего, синхронизация
  кадра (fences/semaphores), мини-рендер-граф.

---

## 0. Кратко: четыре коммита

| # | Что | Ломает сборку | Объём |
|---|---|---|---|
| **A** | включить фичи в `featuresChainEnable` | нет | 5 строк |
| **B** | переключить отрисовку: `beginRendering`, пайплайны без render pass, imgui | да, целиком и атомарно | ~8 файлов |
| **C** | удалить `RenderPass`, `RenderTarget`, framebuffer'ы, `renderPass_` из `Renderer` и `Swapchain` | нет | ~10 файлов, механически |
| **D** | `FrameData::extent`, разрыв `MainRenderPass → Renderer` | нет | 4 файла |

**Почему B нельзя разрезать.** Пайплайн, созданный с `renderPass != nullptr`, несовместим
с `vkCmdBeginRendering`, и наоборот. Любое промежуточное состояние — это кадр, который
не рисуется: либо пайплайн старый, а команды новые, либо наоборот. Поэтому «пайплайны отдельно,
кадр отдельно» не существует.

**Почему C идёт после B.** После B `VkRenderPass` и `VkFramebuffer` больше не участвуют
в отрисовке, но продолжают создаваться в конструкторах. C — это чистое удаление мёртвого кода,
и оно безопасно ровно потому, что B уже работает. Если начать с удаления, получится каскад
(`Swapchain::getFrameBuffer` → `RenderTarget::getFramebuffer` → `SwapchainRenderTarget` →
`GUIRenderTarget` → `Application`) поверх атомарного коммита.

---

## 1. Коммит A — включить фичи

### 1.1 Проблема

`DE_Device.cpp:277–281`:

```cpp
featuresChainCheck.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering  = vk::True;
featuresChainCheck.get<vk::PhysicalDeviceVulkan13Features>().synchronization2   = vk::True;
featuresChainCheck.get<vk::PhysicalDeviceVulkan14Features>().dynamicRenderingLocalRead = vk::True;
featuresChainCheck.get<vk::PhysicalDeviceVulkan14Features>().pushDescriptor              = vk::True;
featuresChainCheck.get<vk::PhysicalDeviceVulkan14Features>().maintenance6               = vk::True;
```

Пишется в **`featuresChainCheck`** (цепочка *проверки*), а в `vkCreateDevice` уходит
`featuresChainEnable` (`:285`). Фичи проверяются, но не включаются.

### 1.2 Правка

Те же пять строк продублировать для `featuresChainEnable` (или заменить имя в левой части —
`Check` использовался только для чтения через `getFeatures2(&featuresChainCheck.get())`):

```cpp
featuresChainEnable.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = vk::True;
featuresChainEnable.get<vk::PhysicalDeviceVulkan13Features>().synchronization2  = vk::True;
featuresChainEnable.get<vk::PhysicalDeviceVulkan14Features>().dynamicRenderingLocalRead = vk::True;
featuresChainEnable.get<vk::PhysicalDeviceVulkan14Features>().pushDescriptor              = vk::True;
featuresChainEnable.get<vk::PhysicalDeviceVulkan14Features>().maintenance6               = vk::True;
```

### 1.3 Отдельно решить: 1.4-only фичи

`dynamicRenderingLocalRead`, `pushDescriptor`, `maintenance6` — это **Vulkan 1.4**. Строки
`:256–264` их *требуют*, то есть приложение откажется стартовать на любом драйвере 1.3.
Если local read не нужен сейчас — не требуй и не включай: три строки проверки +
одна строка включения уходят.

### 1.4 Проверка

```cpp
// временно, в конце createLogicalDevice
DE_ASSERT(logicalDevice.getProcAddr("vkCmdBeginRendering") != nullptr);
```

Ошибка здесь означает, что фича не включена, и искать её потом в первом кадре
(«`dynamicRendering` feature is not enabled») — потеря времени.

---

## 2. Карта текущего состояния

| Файл | Что держит сейчас | В B | В C |
|---|---|---|---|
| `DE_Renderer.cpp:19–24` | создаёт `RenderPass` (цвет + глубина) | без изменений | удалить |
| `DE_Renderer.cpp:26–40` | swapchain через `Builder{device_, *renderPass_}` | без изменений | второй аргумент убрать |
| `DE_Renderer.cpp:89–104` | `recreateSwapchain()` — тот же `Builder` | без изменений | второй аргумент убрать |
| `DE_Renderer.cpp:159–184` | `beginRenderPass` (clear values, viewport, scissor) | → `beginRendering` | — |
| `DE_Renderer.cpp:206–211` | `endRenderPass` | → `endRendering` | — |
| `DE_Renderer.cpp:145–157` | `beginFrame` (acquire, `commandBuffer.begin`) | + переходы в начале кадра | — |
| `DE_Renderer.hpp:27–45` | `SwapchainRenderTarget` — прокси в `Swapchain` | + 4 геттера вьюх | удалить |
| `DE_Renderer.hpp:85–88` | `getSwapchainRenderTarget`, `getMainRenderPass{,Ptr}` | — | удалить |
| `DE_Renderer.hpp:114` | член `unique_ptr<RenderPass>` | — | удалить |
| `DE_IRenderPass.hpp:14–18` | `getClearValues`, `getRenderTarget`, `preRecord`, `onSwapchainRecreation` | clear → optional'ы, `getRenderTarget` оставить до C | остальное удалить |
| `core/DE_PassContext.hpp:10–11` | `renderPass`, `frameBuffer` | → `colorFormat`, `depthFormat` | — |
| `DE_PipelineTraits.hpp:159,212,229,252` | `setRenderPass`/`getRenderPass`/член/передача | → `PipelineRenderingCreateInfo` | — |
| `DE_RenderSystem.cpp:157`, `PointLightSystem.cpp:114`, `SkyBoxSystem.cpp:101` | `.setRenderPass(*renderPass_->getRenderPass())` | → `.setRenderingFormats(...)` | — |
| `DE_Swapchain.cpp:94`, `:103+` | framebuffer'ы | без изменений | удалить |
| `DE_Swapchain.hpp:113,115,123,133` | `getFrameBuffer`, `getRenderPass`, член, `framebuffers_` | + 4 геттера | удалить |
| `DE_Gui.cpp:39–56` | свой `RenderPass` для GUI | без изменений | удалить |
| `DE_Gui.cpp:84–91` | imgui `initInfo` с `RenderPass` | → dynamic | — |
| `DE_MainRenderPass.cpp:66–73` | clear values + `getRenderTarget` | clear → optional'ы | `getRenderTarget` удалить |
| `DE_Application.cpp:341–344` | `passContext_.renderPass = getMainRenderPassPtr()` | → форматы | — |

Глубина `eD32Sfloat` сейчас захардкожена в трёх местах: `DE_Renderer.cpp:22`,
`DE_Swapchain.cpp:89–90`, `DE_Gui.cpp:49`. С dynamic rendering она нужна ещё в
`PipelineRenderingCreateInfo` каждого пайплайна — вынеси в одну константу
(например `constexpr vk::Format kDepthFormat` в `core/DE_PassContext.hpp`), иначе
формат пайплайна и формат вложения разъедутся, а это ошибка совместимости в рантайме.

---

## 3. Коммит B — переключение отрисовки

### 3.1 `core/DE_PassContext.hpp`

```cpp
#pragma once

#include<vulkan/vulkan.hpp>

namespace de{

    constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

    struct PassContext{
        vk::Extent2D extent;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
        vk::Format colorFormat;                             // из window_.getSurfaceFormat().format
        vk::Format depthFormat = kDepthFormat;
    };
}
```

Заголовок перестаёт включать `DE_RenderPass.hpp` — это единственное место, откуда
`PassContext` тянул render pass.

Заполнение, `DE_Application.cpp:341–344`:

```cpp
passContext_ = {
    .extent      = window_.getExtent(),
    .colorFormat = window_.getSurfaceFormat().format,
    .depthFormat = de::kDepthFormat,
};
```

### 3.2 `DE_PipelineTraits.hpp`

**`:159–162`** — было:

```cpp
Builder& setRenderPass(const vk::RenderPass renderPass){ renderPass_ = renderPass; return *this; }
```

стало:

```cpp
Builder& setRenderingFormats(vk::Format color, vk::Format depth = vk::Format::eUndefined){
    renderingInfo_.setColorAttachmentFormats(color).setDepthAttachmentFormat(depth);
    return *this;
}
```

**`:229`** — член `vk::RenderPass renderPass_;` → `vk::PipelineRenderingCreateInfo renderingInfo_{};`

**`:212`** — `getRenderPass()` → `getRenderingInfo()`:

```cpp
const vk::PipelineRenderingCreateInfo* getRenderingInfo() const noexcept { return &renderingInfo_; }
```

**`:252`** — в `create()`:

```cpp
.setRenderPass(nullptr)
.setPNext(builder.getRenderingInfo())
```

Ловушка: `PipelineRenderingCreateInfo` передаётся **указателем**. Он обязан жить до вызова
`createGraphicsPipelineUnique`, поэтому хранится членом `Builder`'а. Если собрать его
временным объектом в цепочке вызовов — пайплайн создастся с мусорными форматами, и это
не даст ни ошибки компиляции, ни внятной ошибки валидации.

### 3.3 Системы — одинаково в трёх местах

`DE_RenderSystem`, `PointLightSystem`, `SkyBoxSystem`.

Было (например `DE_RenderSystem.cpp:157`):

```cpp
.setRenderPass(*renderPass_->getRenderPass())
```

Стало:

```cpp
.setRenderingFormats(colorFormat, depthFormat)
```

Плюс:

- убрать член `const RenderPass* renderPass_;` (`DE_RenderSystem.hpp:64`, `PointLightSystem.hpp`,
  `SkyBoxSystem.hpp:48`);
- `createPipeline(const vk::Extent2D extent, const std::vector<vk::DescriptorSetLayout>&, vk::PipelineCache)`
  → добавить `vk::Format colorFormat, vk::Format depthFormat` (или передать `const PassContext&`
  целиком — тогда подпись не будет расти при следующем формате);
- в конструкторе брать форматы из `passContext`.

`.setViewportState(extent)` и `.setDynamicStates({eViewport, eScissor})` **не трогать**:
viewport и scissor у тебя динамические, а после `beginRendering` их придётся ставить руками —
ровно поэтому они должны остаться динамическими.

### 3.4 `DE_IRenderPass.hpp`

```cpp
#pragma once

#include<optional>
#include<vulkan/vulkan.hpp>

namespace de {
    struct FrameData;
    class RenderTarget;

    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        // nullopt   -> loadOp = eLoad     (рисовать поверх существующего)
        // значение  -> loadOp = eClear
        virtual std::optional<vk::ClearValue> colorClearValue() const { return std::nullopt; }

        // nullopt   -> loadOp = eDontCare
        virtual std::optional<vk::ClearValue> depthClearValue() const { return std::nullopt; }

        // false -> в RenderingInfo не передаём depth-вложение вообще,
        //          пайплайн прохода создаётся с depthAttachmentFormat = eUndefined
        virtual bool usesDepth() const { return true; }

        virtual void record(const FrameData& frameData) = 0;

        // удаляется в коммите C
        virtual const RenderTarget& getRenderTarget() const = 0;
    };
}
```

Почему не `std::vector<vk::ClearValue>`, как было: вектор кодировал ту же информацию
позиционно и неявно («0 — цвет, 1 — глубина, пусто — не чистить»). `GUIRenderPass::getClearValues()`
возвращает `{}` (`DE_Gui.cpp:111–115`), и это работает только потому, что вложения его render pass'а
объявлены `eLoad`/`eDontCare`. С `RenderingAttachmentInfo` load op берётся из вложения,
и «пустой вектор» молча стал бы «очистить мусорным значением».

`MainRenderPass` (`DE_MainRenderPass.cpp:66–73`):

```cpp
std::optional<vk::ClearValue> MainRenderPass::colorClearValue() const {
    return vk::ClearValue{}.setColor(std::array<float,4>{{0.f,0.f,0.f,1.f}});
}
std::optional<vk::ClearValue> MainRenderPass::depthClearValue() const {
    return vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{1.f, 0});
}
```

`GUIRenderPass` — оба `nullopt` и `usesDepth() == false`.

### 3.5 `Renderer` — новый путь кадра

**Переходы.** Приватный хелпер (пока на `vk::ImageMemoryBarrier`, `Barrier2` — отдельным коммитом):

```cpp
void Renderer::transitionImage(vk::CommandBuffer cmd, vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess,
        vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess,
        vk::ImageAspectFlags aspect)
{
    const auto barrier = vk::ImageMemoryBarrier{}
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setImage(image)
        .setSrcAccessMask(srcAccess)
        .setDstAccessMask(dstAccess)
        .setSubresourceRange(vk::ImageSubresourceRange{}
            .setAspectMask(aspect)
            .setBaseMipLevel(0).setLevelCount(1)
            .setBaseArrayLayer(0).setLayerCount(1));

    cmd.pipelineBarrier(srcStage, dstStage, {}, nullptr, nullptr, barrier);
}
```

**Куда ставить.** Два места, без флагов состояния:

1. `beginFrame()`, сразу после `commandBuffer.begin(...)` (`DE_Renderer.cpp:149`):

```cpp
transitionImage(commandBuffer, target.getColorImage(imageIndex),
    vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
    vk::PipelineStageFlagBits::eTopOfPipe, {},
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentWrite,
    vk::ImageAspectFlagBits::eColor);

transitionImage(commandBuffer, target.getDepthImage(),
    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
    vk::PipelineStageFlagBits::eTopOfPipe, {},
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    vk::ImageAspectFlagBits::eDepth);
```

`oldLayout = eUndefined` каждый кадр — законно: `Undefined` означает «предыдущее содержимое
не нужно», поэтому учёт состояния между кадрами не требуется. Ровно так же работал render pass
с `initialLayout = Undefined`.

2. `renderFrame()`, после цикла проходов и до `endFrame()`:

```cpp
transitionImage(frame.commandBuffer, target.getColorImage(frame.imageIndex),
    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite,
    vk::PipelineStageFlagBits::eBottomOfPipe, {},
    vk::ImageAspectFlagBits::eColor);
```

**`beginRendering`** вместо `beginRenderPass` (`:159–184`):

```cpp
void Renderer::beginRendering(const FrameData& frameData, const RenderTarget& target, IRenderPass& pass)
{
    if (!isFrameStarted_) {
        throw std::runtime_error("beginRendering called outside of a frame");
    }

    const auto& cmd = frameData.commandBuffer;
    const auto extent = target.getExtent();

    vk::RenderingAttachmentInfo color{};
    color.setImageView(target.getColorView(frameData.imageIndex))
         .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
         .setStoreOp(vk::AttachmentStoreOp::eStore);

    if (const auto clear = pass.colorClearValue()) {
        color.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(*clear);
    } else {
        color.setLoadOp(vk::AttachmentLoadOp::eLoad);
    }

    vk::RenderingAttachmentInfo depth{};
    if (pass.usesDepth()) {
        depth.setImageView(target.getDepthView())
             .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
             .setStoreOp(vk::AttachmentStoreOp::eDontCare);

        if (const auto clear = pass.depthClearValue()) {
            depth.setLoadOp(vk::AttachmentLoadOp::eClear).setClearValue(*clear);
        } else {
            depth.setLoadOp(vk::AttachmentLoadOp::eDontCare);
        }
    }

    cmd.beginRendering(vk::RenderingInfo{}
        .setRenderArea(vk::Rect2D{{0, 0}, extent})
        .setLayerCount(1)
        .setColorAttachments(color)
        .setPDepthAttachment(pass.usesDepth() ? &depth : nullptr));

    cmd.setViewport(0, vk::Viewport{}
        .setX(0.f).setY(0.f)
        .setWidth(static_cast<float>(extent.width))
        .setHeight(static_cast<float>(extent.height))
        .setMinDepth(0.f).setMaxDepth(1.f));
    cmd.setScissor(0, vk::Rect2D{{0, 0}, extent});
}
```

`setPDepthAttachment(&depth)` — указатель на локальную переменную; `beginRendering` читает её
сразу, но если кто-то «сократит» до временного объекта прямо в цепочке вызовов, получится
висячий указатель.

**`endRenderPass`** (`:206–211`) → `cmd.endRendering();`

**Цикл** (`:195–201`):

```cpp
for (auto& pass : passes_) {
    beginRendering(frame, pass->getRenderTarget(), *pass);
    pass->record(frame);
    endRendering(frame);
}
```

`preRecord` не вызываем — ни один проход его не переопределяет.

### 3.6 `RenderTarget` — аддитивно, чтобы не тянуть коммит C в B

Добавить в `DE_RenderTarget.hpp` четыре чистых виртуальных:

```cpp
virtual vk::Image     getColorImage(uint32_t imageIndex) const = 0;
virtual vk::ImageView getColorView(uint32_t imageIndex) const = 0;
virtual vk::Image     getDepthImage() const = 0;
virtual vk::ImageView getDepthView() const = 0;
```

`getFramebuffer`/`getRenderPass` пока оставить — их удалит C.

`SwapchainRenderTarget` (`DE_Renderer.hpp:34–42`) и `GUIRenderTarget` (`DE_Gui.cpp:17–25`)
пробрасывают всё в `Swapchain`, поэтому в самом `Swapchain` нужно добавить четыре геттера.
Образов swapchain'а он сейчас не хранит (только `imageViews_`), значит:

```cpp
// DE_Swapchain.hpp, private
std::vector<vk::Image> images_;

// DE_Swapchain.cpp, в конструкторе, после createSwapchainKHRUnique:
images_ = logicalDevice.getSwapchainImagesKHR(*swapchain_);
```

```cpp
vk::Image     getColorImage(uint32_t i) const { return images_.at(i); }
vk::ImageView getColorView(uint32_t i)  const { return *imageViews_.at(i); }
vk::Image     getDepthImage()           const { return *depthImage_.image; }
vk::ImageView getDepthView()            const { return *depthImageView_; }
```

Эти четыре геттера останутся в C, остальное из `Swapchain` уйдёт.

### 3.7 imgui, `DE_Gui.cpp:84–91`

```cpp
.PipelineInfoMain = {
    .RenderPass = nullptr,
    .Subpass = 0,
    .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    .ExtraDynamicStates = {},
    .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{}
        .setColorAttachmentFormats(window.getSurfaceFormat().format),
},
.UseDynamicRendering = true,
```

Три вещи, которые здесь важны:

1. **`RenderPass` обязан остаться `nullptr`.** Сейчас там `*renderPass_->getRenderPass()`
   (`:85`) — при `UseDynamicRendering = true` и ненулевом render pass backend создаст
   пайплайн под render pass, а рисовать будет в dynamic-режиме.
2. **Поля живут под `PipelineInfoMain`** — это новое расположение в 1.93 WIP
   (changelog `imgui_impl_vulkan.cpp:47`). Старые `.RenderPass`/`.MSAASamples` в `initInfo`
   просто не скомпилируются.
3. **`PipelineRenderingCreateInfo` здесь передаётся по значению** — backend делает deep-copy
   массива форматов (changelog `2025-04-07`), поэтому временный объект тут безопасен,
   в отличие от `Builder`'а в 3.2.

`ImGui_ImplVulkan_RenderDrawData` вызывается внутри уже открытого rendering instance:
backend сам `vkCmdBeginRendering` не вызывает, это проверено по исходнику
(`imgui_impl_vulkan.cpp:588` — ни одного вызова). То есть `GUIRenderPass::record` остаётся
как есть, а обёртку даёт `Renderer`.

### 3.8 Проверка B

- приложение запускается, картинка на 1200×1000 визуально та же;
- ноль ошибок валидации;
- ресайз окна не ломается (переходы выполняются каждый кадр заново);
- `Select-String -Path src\*.cpp,src\Systems\*.cpp -Pattern "setRenderPass\("` — пусто.

---

## 4. Коммит C — удаление

Всё ниже — механически, потому что после B это мертвый код.

### 4.1 `DE_Renderer.cpp` — конструктор

```cpp
// удалить целиком
renderPass_ = std::make_unique<de::RenderPass>(
    de::RenderPass::Builder{ device_ }
    .setColorAttachment(window_.getSurfaceFormat().format)
    .setDepthAttachment(vk::Format::eD32Sfloat)
    .build());

// в двух местах (ctor :27 и recreateSwapchain :90)
Swapchain::Builder{ device_, *renderPass_ }   →   Swapchain::Builder{ device_ }
```

### 4.2 `DE_Swapchain`

- `Builder(const Device&, const RenderPass&)` → `explicit Builder(const Device&)`, поле `renderPass_`
  удалить (`:58`);
- `Swapchain(const Device&, const RenderPass&, const vk::SwapchainCreateInfoKHR&)` → без render pass
  (`:83`), поле `const RenderPass& renderPass_` (`:123`) удалить;
- `framebuffers_ = createFramebuffers(...)` (`:94`) — удалить; саму функцию `createFramebuffers`
  (`:103+`, объявление `:47`) — удалить;
- `getFrameBuffer` (`:113`), `getRenderPass` (`:115`), `framebuffers_` (`:133`) — удалить;
- `images_` и четыре геттера из 3.6 — оставить.

### 4.3 `DE_Renderer.hpp`

- `:114` член `std::unique_ptr<RenderPass> renderPass_` — удалить;
- `:87–88` `getMainRenderPass`, `getMainRenderPassPtr` — удалить;
- `:15` forward-декларация `class RenderPass` и `#include "DE_RenderPass.hpp"` — удалить;
- `SwapchainRenderTarget` (`:27–45`) и член `swapchainTarget_` (`:104`),
  `getSwapchainRenderTarget()` (`:85`) — удалить.

### 4.4 `RenderTarget` и проходы

- `DE_RenderTarget.hpp` — удалить (после C у него не остаётся содержимого:
  framebuffer'ов и render pass'ов нет, а `extent`/вьюхи берутся прямо из `Swapchain`);
- `GUIRenderTarget` (`DE_Gui.hpp:21–37`), `GUIRenderTarget::renderTarget_`, `rebind`
  (`DE_Gui.cpp:58`) — удалить;
- `IRenderPass::getRenderTarget()` — удалить; `beginRendering` берёт `const Swapchain&`
  напрямую (его даёт `Renderer`);
- `IRenderPass::preRecord`, `onSwapchainRecreation` — удалить;
  `MainRenderPass::onSwapchainRecreation` (`:109–112`) и `GUIRenderPass::onSwapchainRecreation`
  (`:126–128`) — удалить;
- `DE_RenderPass.hpp/cpp` — удалить файлы;
- `GUIRenderPass::getRenderPass` (`DE_Gui.cpp:130–133`), член `renderPass_`, его создание
  (`:39–56`) — удалить;
- `MainRenderPass::getRenderTarget` (`:71–73`) — удалить;
- `unregisterPass`, `unregisterCompute` (`DE_Renderer.hpp:73,80`) — удалить, не вызываются.

### 4.5 Приёмка C

```powershell
git grep -n "vk::RenderPass|vk::Framebuffer|UniqueRenderPass" -- src   # пусто
git grep -n "RenderTarget" -- src                                      # пусто
git grep -n "getMainRenderPass|preRecord|onSwapchainRecreation" -- src  # пусто
```

Ожидаемый эффект: `Renderer::Renderer` короче на 7 строк, `Renderer` не знает про `RenderPass`,
`Swapchain` не знает про framebuffer'ы, ресайз окна больше ничего не пересоздаёт кроме
самого swapchain'а.

---

## 5. Коммит D — `extent` в `FrameData`

Зачем: после C `MainRenderPass` всё ещё держит `Renderer&` только чтобы спросить
`getSwapchainExtent()` (`DE_MainRenderPass.cpp:87`). Это цикл `Renderer → passes → Renderer`,
а `extent` — данные кадра.

```cpp
struct FrameData {
    uint32_t imageIndex;
    uint32_t frameIndex;
    vk::CommandBuffer commandBuffer;
    vk::Extent2D extent;          // новое
};
```

`beginFrame` (`:151–155`) заполняет `target.getExtent()`; `MainRenderPass::record` берёт
`frameData.extent`. После этого `Renderer&` в `MainRenderPass` нужен только за
`framesInFlight` в конструкторе — его можно взять из `PassContext`.

---

## 6. Режимы отказа

| Симптом | Причина | Где искать |
|---|---|---|
| Валидация: «`dynamicRendering` feature is not enabled» | фичи в `featuresChainCheck` | `DE_Device.cpp:277–281` |
| Валидация: «pipeline was created with `VkPipelineRenderingCreateInfo`, but no rendering instance is active» | `beginRendering` не вызван (или вызван после `record`) | `Renderer::renderFrame` |
| Валидация: формат вложения не совпадает с пайплайном | `setRenderingFormats` и `RenderingAttachmentInfo` с разными форматами; чаще всего SRGB против UNORM | `PipelineTraits` ↔ `Renderer::beginRendering` |
| Экран чёрный/мерцает, валидация молчит | забыт переход в `PresentSrcKHR` перед `endFrame` | `renderFrame`, после цикла проходов |
| Сцена пропала, видно только GUI | у второго прохода `loadOp = eClear` вместо `eLoad` | `GUIRenderPass::colorClearValue` |
| Геометрия не рисуется, ошибок нет | не выставлены viewport/scissor после `beginRendering` | `Renderer::beginRendering` |
| «A render pass instance must not be active» | вложенный `beginRendering` или забытый `endRendering` при `continue` в цикле | `renderFrame` |
| Случайные значения clear, падения при старте кадра | `RenderingAttachmentInfo` для depth — временный объект, `pDepthAttachment` висит | `beginRendering` |
| imgui рисует пустоту или падает на первом кадре | `PipelineInfoMain.RenderPass` не `nullptr` либо вызван без активного rendering | `DE_Gui.cpp:84–91` |

---

## 7. Чек-лист приёмки всей задачи

1. `git grep "vk::RenderPass"` в `src/` — пусто (кроме `third_party`).
2. Ноль сообщений валидации при 60 секундах работы, включая ресайз окна и `Print Screen`-подобные
   растяжения.
3. Порядок проходов прежний: main → gui; GUI рисуется поверх сцены.
4. Ресайз не создаёт framebuffer'ов (их нет) и не пересоздаёт пайплайны.
5. FPS не изменился (ожидаемо: dynamic rendering не быстрее и не медленнее).
6. `Renderer::Renderer` не содержит слова `RenderPass`.

---

## 8. Чего не делать в этом заходе

- **Не переписывать барьеры на `vk::ImageMemoryBarrier2`/`vkCmdPipelineBarrier2`** в том же
  коммите, что и B. Фича `synchronization2` включается в A, переписывание — отдельный
  механический коммит. Иначе при первом артефакте не понять, виноват dynamic rendering
  или новые маски.
- **Не трогать синхронизацию кадра**: `SubmitInfo`, `waitStages`, семифоры, `imagesInFlightFences_`.
  Dynamic rendering к ним не относится.
- **Не делать depth-образ на кадр** (`DE_Swapchain.cpp:89` создаёт один общий на `framesInFlight`).
  Это отдельный пункт: там нужно разбираться, какая зависимость сейчас закрывает хазн
  между кадрами.
- **Не заводить мини-рендер-граф.** Два прохода руками — правильный промежуточный уровень.
- **Не держать `#ifdef DE_LEGACY_RENDERPASS`.** Два пути отрисовки разъедутся по переходам
  быстрее, чем помогут; откат — это git, а не `#ifdef`.

---

## 9. Откат

- A откатывается независимо (5 строк).
- B откатывается одним `git revert`, потому что атомарный.
- C и D откатываются независимо и не влияют на поведение.
- Если после B картинки нет, а валидация молчит: первым делом проверить (1) финальный переход
  в `PresentSrcKHR`, (2) `setViewport`/`setScissor`, (3) `loadOp` у GUI. Эти три покрывают
  девять из десяти «чёрных экранов» при переходе.
