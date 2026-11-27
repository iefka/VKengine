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
| **C** | удалить `RenderPass`, framebuffer'ы, `renderPass_` из `Renderer` и `Swapchain`, `preRecord` | нет | ~10 файлов, механически |
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

**Почему `RenderTarget` при этом умирает уже в B.** Он не «мёртвый код после B» — он мешает его
написать: `beginRendering` нужны образ и вьюха, и брать их из интерфейса, единственным содержимым
которого были framebuffer и render pass, нечего. Файл удаляется в B, четыре геттера уезжают
в `Swapchain` — см. §3.6.

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
| `DE_Renderer.hpp:27–45` | `SwapchainRenderTarget` — прокси в `Swapchain` | удалить вместе с `RenderTarget` | — |
| `DE_RenderTarget.hpp:9–19` | `RenderTarget`: `getFramebuffer`, `getRenderPass`, `getExtent` | удалить файл, геттеры → в `Swapchain` (§3.6) | — |
| `DE_Renderer.hpp:85–88` | `getSwapchainRenderTarget`, `getMainRenderPass{,Ptr}` | `getSwapchainRenderTarget` удалить | `getMainRenderPass{,Ptr}` удалить |
| `DE_Renderer.hpp:114` | член `unique_ptr<RenderPass>` | — | удалить |
| `DE_IRenderPass.hpp:14–18` | `getClearValues`, `getRenderTarget`, `preRecord`, `onSwapchainRecreation` | вложения (§3.4); `getRenderTarget` удалить | `preRecord`, `onSwapchainRecreation` удалить |
| `core/DE_PassContext.hpp:10–11` | `renderPass`, `frameBuffer` | → `colorFormat`, `depthFormat` | — |
| `DE_PipelineTraits.hpp:159,212,229,252` | `setRenderPass`/`getRenderPass`/член/передача | → `PipelineRenderingCreateInfo` | — |
| `DE_RenderSystem.cpp:157`, `PointLightSystem.cpp:114`, `SkyBoxSystem.cpp:101` | `.setRenderPass(*renderPass_->getRenderPass())` | → `.setRenderingFormats(...)` | — |
| `DE_Swapchain.cpp:94`, `:103+` | framebuffer'ы | без изменений | удалить |
| `DE_Swapchain.hpp:113,115,123,133` | `getFrameBuffer`, `getRenderPass`, член, `framebuffers_` | + 4 геттера | удалить |
| `DE_Gui.cpp:39–56` | свой `RenderPass` для GUI | без изменений | удалить |
| `DE_Gui.cpp:84–91` | imgui `initInfo` с `RenderPass` | → dynamic | — |
| `DE_MainRenderPass.cpp:66–73` | clear values + `getRenderTarget` | `getClearValues` → вложения (§3.4), `getRenderTarget` удалить | — |
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

Можно вернуть и по ссылке (`const vk::PipelineRenderingCreateInfo&`) — тогда в `create()`
будет `.setPNext(&builder.getRenderingInfo())`. Функционально эквивалентно: адрес берётся
у члена `Builder`'а, время жизни то же. Форма с указателем выбрана только ради единообразия
с соседними геттерами этого класса (`getInputStateInfo`, `getMultisampleInfo` и т. д. —
все возвращают указатели, потому что их значения сразу уходят в `pNext`).

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

Контракт теперь описывает **вложения**, а не только clear-значения. Было
`std::vector<vk::ClearValue> getClearValues()` (`DE_IRenderPass.hpp:14`), где вложения вообще не
выражались: `loadOp`/`storeOp` жили в `VkRenderPass` (`DE_MainRenderPass.cpp:44–63`,
`DE_Gui.cpp:39–56`). С `vkCmdBeginRendering` их негде взять, кроме самого прохода, — поэтому
вместо вектора две структуры одинаковой формы.

```cpp
#pragma once

#include<optional>
#include<vulkan/vulkan.hpp>

namespace de {
    struct FrameData;

    struct ColorAttachment {
        vk::AttachmentLoadOp  loadOp  = vk::AttachmentLoadOp::eLoad;    // eClear | eLoad | eDontCare
        vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;  // eStore | eDontCare
        vk::ClearValue        clear{};                                  // читается только при eClear
    };

    struct DepthAttachment {
        vk::AttachmentLoadOp  loadOp  = vk::AttachmentLoadOp::eLoad;
        vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare;
        vk::ClearValue        clear{};
    };

    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        // Вложение цвета есть у любого прохода этого рендерера, поэтому без optional:
        // «нет цвета» — состояние, которого не бывает, и различать в нём нечего.
        virtual ColorAttachment colorAttachment() const { return {}; }   // eLoad + eStore

        // nullopt -> pDepthAttachment = nullptr; пайплайн такого прохода
        //            создаётся с depthAttachmentFormat = eUndefined
        virtual std::optional<DepthAttachment> depthAttachment() const { return std::nullopt; }

        virtual void record(const FrameData& frameData) = 0;
    };
}
```

**Почему структуры, а не `optional<vk::ClearValue>`.** Про вложение есть четыре вопроса: «есть ли
оно», «чем чистить», «читать старое или затирать» (`loadOp`) и «сохранять ли результат»
(`storeOp`). `optional<ClearValue>` отвечает только на первые два: `eDontCare` для `loadOp` не
выражается, `storeOp` недостижим вообще и зашивается константой в `Renderer`. Это ровно та
претензия, из-за которой отброшен вариант `bool usesDepth()` + `optional<ClearValue>
depthClearValue()`: он не различал «вложения нет» и «вложение есть, но не чистить». Тип обязан
различать состояния, а не дублировать уже выраженное.

**Почему цвет без `optional`, а глубина с ним.** Вопрос «а есть ли вложение цвета» у этого
рендерера не задаётся: кадр всегда рисуется в образ swapchain'а, и `GUIRenderPass` тоже. У глубины
же есть настоящее третье состояние — вложения нет совсем (`GUIRenderPass`; пайплайн imgui создан
с `depthAttachmentFormat = eUndefined`), и выразить его `DepthAttachment` не может, только
`nullopt`. Симметрия здесь не самоцель: одинаковой сделана *форма* описания, а не наличие
`optional`.

**Почему `clear` — не `optional<vk::ClearValue>`.** `loadOp` уже говорит, читается значение или
нет; при `eLoad`/`eDontCare` `clearValue` игнорируется и драйвером, и валидацией. Второй
`optional` внутри структуры добавит только два уровня проверки на месте использования.

**Почему не отдавать сразу `vk::RenderingAttachmentInfo`.** Технически можно, и собралось бы. Но в
этой структуре смешаны три источника, а проход владеет только одним:

| Поле | Чей | Почему не прохода |
|---|---|---|
| `loadOp`, `storeOp`, `clearValue` | прохода | это и есть политика |
| `imageView` | `Swapchain` (`images_`/`imageViews_`) | зависит от `frameData.imageIndex`, меняется каждый кадр |
| `imageLayout` | дисциплина переходов `Renderer` | следует из барьера в `beginFrame` (§3.5), а не из желания прохода |

Готовая структура требует, чтобы проход знал два чужих источника, и подпись становится
`colorAttachment(vk::ImageView colorView, vk::ImageLayout layout)` — два параметра, чтобы подставить
их в поля. Заодно в каждом проходе появляется место, где кадр ломается одним неверным `layout`, а
`pNext` в структуре, возвращаемой по значению, — висячий указатель на стек прохода без единого
предупреждения компилятора.

Конкретные поломки, которые наша форма не даёт завести:

1. вьюха закеширована в конструкторе прохода (`getColorView(0)`) — кадр уезжает не в тот образ;
   валидация ругается не на «чужой образ», а на `VUID-vkCmdDraw-None-09600` (образ уже в
   `ePresentSrcKHR`, объявлен `eColorAttachmentOptimal`);
2. `imageLayout = eGeneral` «на всякий случай» — барьер перевёл образ в `eColorAttachmentOptimal`,
   `beginRendering` объявил `eGeneral` → тот же VUID;
3. MSAA/resolve: поля `resolveMode`/`resolveImageView` начинают правиться в проходе, хотя `samples` —
   свойство `PassContext`/`Swapchain`.

Разделение простое: проход описывает **политику**, `RenderingAttachmentInfo` — уже собранное
значение, и собирает его `beginRendering`, потому что только он знает `imageIndex` и целевые layout'ы.

`MainRenderPass` (`DE_MainRenderPass.cpp:66–73`, `getClearValues` заменить):

```cpp
ColorAttachment MainRenderPass::colorAttachment() const {
    ColorAttachment att;                       // по умолчанию eLoad + eStore
    att.loadOp = vk::AttachmentLoadOp::eClear;
    att.clear  = vk::ClearValue{}.setColor(std::array<float, 4>{{0.f, 0.f, 0.f, 1.f}});
    return att;
}

std::optional<DepthAttachment> MainRenderPass::depthAttachment() const {
    DepthAttachment att;                       // по умолчанию eLoad + eDontCare
    att.loadOp = vk::AttachmentLoadOp::eClear; // storeOp не трогаем: после кадра глубина не нужна
    att.clear  = vk::ClearValue{}.setDepthStencil({1.f, 0});
    return att;
}
```

`setColor`/`setDepthStencil` возвращают ссылку на временный `vk::ClearValue`; присваивание
копирует его внутри того же выражения, поэтому здесь всё корректно. Грабли появляются, только
если тянуть ссылку дальше: `const auto& cv = vk::ClearValue{}.setDepthStencil({1.f, 0});`.

`GUIRenderPass` не переопределяет **ничего**: цвет по умолчанию `eLoad` + `eStore` — ровно то, что
нужно второму проходу, глубина `nullopt`. `onSwapchainRecreation` (`DE_Gui.cpp:126–128`) остаётся
как есть, тела `getClearValues()` (`DE_Gui.cpp:111–115`) больше нет.

Итог по значениям:

| Проход | `colorAttachment()` | `depthAttachment()` |
|---|---|---|
| `MainRenderPass` | `{eClear, eStore, {0,0,0,1}}` | `{eClear, eDontCare, {depth=1, stencil=0}}` |
| `GUIRenderPass` | дефолт `{eLoad, eStore}` | `nullopt` |

Инвариант, который эти структуры не проверяют: проход с глубиной ⇒ пайплайн создан с тем же
`depthAttachmentFormat`; `nullopt` ⇒ `eUndefined`. Это второй шов дублирования после цветовых
форматов, и лечится он только передачей владения `PipelineRenderingCreateInfo` проходу — на двух
проходах это оверинжиниринг, см. §8.

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
transitionImage(commandBuffer, swapchain_->getColorImage(imageIndex),
    vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
    vk::PipelineStageFlagBits::eColorAttachmentOutput, {},   // НЕ eTopOfPipe
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentWrite,
    vk::ImageAspectFlagBits::eColor);

transitionImage(commandBuffer, swapchain_->getDepthImage(),
    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests, {},
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    vk::ImageAspectFlagBits::eDepth);
```

**`srcStage` = `eColorAttachmentOutput`, а не `eTopOfPipe`.** `endFrame` подписывается на
`readyForRenderengSemaphores_` с `waitStages = eColorAttachmentOutput`: семафор упорядочивает
только команды этого этапа. Барьер с `srcStage = TopOfPipe` ничего не ждёт, поэтому переход
layout'а (а это запись в образ) может выполниться **до** того, как acquire завершится и
presentation engine отпустит образ: на десктопе это чаще всего прощается, на ноутбучных iGPU и
тайловых — мусор в первых кадрах и мерцание. Ровно поэтому в render pass это работало само:
неявный переход по `initialLayout` драйвер делал внутри `vkCmdBeginRenderPass`, то есть на
`COLOR_ATTACHMENT_OUTPUT`. С dynamic rendering этот переход наш, и его надо ставить на тот же
этап, что и ожидание.

С глубиной эта же логика, но по другой причине: образ глубины общий на все кадры in flight
(§8), поэтому `srcStage` должен совпадать с этапом её записей — `early`/`late fragment tests`.

`oldLayout = eUndefined` каждый кадр — законно: `Undefined` означает «предыдущее содержимое
не нужно», поэтому учёт состояния между кадрами не требуется. Ровно так же работал render pass
с `initialLayout = Undefined`.

2. `renderFrame()`, после цикла проходов и до `endFrame()`:

```cpp
transitionImage(frame.commandBuffer, swapchain_->getColorImage(frame.imageIndex),
    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite,
    vk::PipelineStageFlagBits::eBottomOfPipe, {},
    vk::ImageAspectFlagBits::eColor);
```

**Почему не `Barrier2` сразу.** `synchronization2` уже включён в A, то есть `vk::ImageMemoryBarrier2`
доступен в B без всякой подготовки, и dynamic rendering его **не требует** — `vkCmdBeginRendering`
спокойно живёт на `vkCmdPipelineBarrier`. Отложено по трём причинам:

1. **В B переходы — новый код, а не перенос.** Раньше их делал `VkRenderPass` неявно
   (`initialLayout`/`finalLayout` + `subpassDependency`), теперь они пишутся руками, и первый
   прогон — это отладка «правильные ли стадии/маски/аспекты». Если в том же коммите сменить API,
   у сломанной картинки появляются две неразличимые гипотезы: неверный барьер или неверное
   переписывание маски. Бисект между ними уже не сделать.
2. **Переписывание выглядит механическим, но таким не является.** В `ImageMemoryBarrier2` стадии
   уезжают внутрь барьера и меняют тип на `PipelineStageFlags2`/`AccessFlags2`; «1:1» отображение
   (`eTopOfPipe`, `eBottomOfPipe`, `eAllCommands`) — ровно то место, где тихо ломают
   синхронизацию, и глазами в общем диффе это не ловится.
3. **Цена откладывания нулевая, диф — 20 строк.** В B пишется один хелпер `transitionImage` на
   sync1 и четыре вызова; в отдельном коммите меняется только тело хелпера. Если что-то мигнёт,
   `git revert` этого коммита возвращает рабочее состояние, не трогая dynamic rendering.

**`beginRendering`** вместо `beginRenderPass` (`:159–184`). Параметра `target` нет: `RenderTarget`
удалён (§3.6), цель — член `swapchain_`; если обращений несколько, в начале функции удобно
`auto& sc = *swapchain_;`.

```cpp
void Renderer::beginRendering(const FrameData& frameData, IRenderPass& pass)
{
    if (!isFrameStarted_) {
        throw std::runtime_error("beginRendering called outside of a frame");
    }

    auto& sc = *swapchain_;
    const auto& cmd = frameData.commandBuffer;
    const auto extent = sc.getSwapchainExtent();

    const auto colorDesc = pass.colorAttachment();          // вложение цвета есть всегда
    const auto color = vk::RenderingAttachmentInfo{}
        .setImageView(sc.getColorView(frameData.imageIndex))
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(colorDesc.loadOp)
        .setStoreOp(colorDesc.storeOp)
        .setClearValue(colorDesc.clear);

    vk::RenderingAttachmentInfo depth{};
    const auto depthDesc = pass.depthAttachment();
    if (depthDesc) {
        depth.setImageView(sc.getDepthView())
             .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
             .setLoadOp(depthDesc->loadOp)
             .setStoreOp(depthDesc->storeOp)
             .setClearValue(depthDesc->clear);
    }

    cmd.beginRendering(vk::RenderingInfo{}
        .setRenderArea(vk::Rect2D{{0, 0}, extent})
        .setLayerCount(1)
        .setColorAttachments(color)
        .setPDepthAttachment(depthDesc ? &depth : nullptr));

    cmd.setViewport(0, vk::Viewport{}
        .setX(0.f).setY(0.f)
        .setWidth(static_cast<float>(extent.width))
        .setHeight(static_cast<float>(extent.height))
        .setMinDepth(0.f).setMaxDepth(1.f));
    cmd.setScissor(0, vk::Rect2D{{0, 0}, extent});
}
```

`color` объявлен `const auto` — это полноценная копия результата цепочки сеттеров (сеттеры
возвращают ссылку на временный объект, инициализация копирует его внутри того же выражения),
поэтому `setColorAttachments(color)` безопасен. `setPDepthAttachment(&depth)` — указатель на
локальную переменную; `beginRendering` читает её сразу, но если кто-то «сократит» до временного
объекта прямо в цепочке вызовов, получится висячий указатель. Заодно: `clear` копируется в
`RenderingAttachmentInfo` всегда, даже при `eLoad`/`eDontCare` — там он игнорируется.

**`endRenderPass`** (`:206–211`) → `cmd.endRendering();`

**Цикл** (`:195–201`):

```cpp
for (auto& pass : passes_) {
    beginRendering(frame, *pass);
    pass->record(frame);
    endRendering(frame);
}
```

`preRecord` не вызываем — ни один проход его не переопределяет.

### 3.6 `RenderTarget` — удаляется здесь же, четыре геттера уезжают в `Swapchain`

`RenderTarget` (`DE_RenderTarget.hpp:9–19`) держал ровно три вещи: `getFramebuffer`, `getRenderPass`,
`getExtent`. Первых двух после B не существует, `getExtent` — это `Swapchain::currentExtent_`.
То есть от интерфейса остаются два наследника (`SwapchainRenderTarget` в `DE_Renderer.hpp:27–45` и
`GUIRenderTarget` в `DE_Gui.hpp:21–35`), которые пробрасывают вызовы в один и тот же `Swapchain`:
слой без собственного состояния. Значит, файл удаляется **в B**, а нужные рендереру данные берутся
прямо из swapchain.

Аддитивного варианта («оживить `RenderTarget` на один коммит, удалить в C») здесь нет: чтобы
`beginRendering` собрал `RenderingAttachmentInfo`, ему нужны образ и вьюха, а держать их
в интерфейсе ради удаления следующим коммитом — это ровно тот слой, который ревизия и убирает.

**Что добавить в `Swapchain`.** Образов он сейчас не хранит (только `imageViews_`), поэтому
в конструкторе после `getSwapchainImagesKHR`:

```cpp
// DE_Swapchain.hpp, private
std::vector<vk::Image> images_;

// в конструкторе
images_ = logicalDevice.getSwapchainImagesKHR(*swapchain_);

// public
vk::Image     getColorImage(uint32_t i) const { return images_.at(i); }
vk::ImageView getColorView(uint32_t i)  const { return *imageViews_.at(i); }
vk::Image     getDepthImage()           const { return *depthImage_.image; }
vk::ImageView getDepthView()            const { return *depthImageView_; }
```

`getSwapchainExtent()` уже есть. `getFrameBuffer`/`getRenderPass`/`framebuffers_` пока остаются —
их снимет C (§4.2), это отдельная механическая правка. `vk::Image` и `vk::ImageView` не
взаимозаменяемы: барьеру нужен образ, `RenderingAttachmentInfo` — вьюха, отсюда обе пары геттеров.

**Где `Renderer` берёт цель.** Нигде — у него есть член `std::optional<Swapchain> swapchain_`.
Поэтому `beginRendering(const FrameData&, IRenderPass&)`, а внутри `sc.getColorView(imageIndex)`;
параметр `const RenderTarget& target` не появляется ни на одном шаге.

**Что должно уехать вместе с файлом** — это те места, где `RenderTarget` ещё упоминается, и без их
правки B просто не соберётся:

| Место | Что там | Куда |
|---|---|---|
| `DE_IRenderPass.hpp:3` | `#include"DE_RenderTarget.hpp"` | удалить |
| `DE_MainRenderPass.hpp:37`, `.cpp:71–73` | `getRenderTarget() const override` | удалить (`override` без базового виртуального = ошибка) |
| `DE_Gui.hpp:21–35`, `:58` | `class GUIRenderTarget`, член `renderTarget_` | удалить |
| `DE_Gui.cpp:17–27` | `GUIRenderTarget::getFramebuffer/getRenderPass/getExtent` | удалить |
| `DE_Gui.cpp:58` | `renderTarget_.rebind(renderer_.getSwapchain(), *this)` | удалить |
| `DE_Gui.cpp:117–119` | `GUIRenderPass::getRenderTarget` | удалить |
| `DE_Gui.cpp:126–128` | `onSwapchainRecreation` — внутри только `rebind` | удалить вместе с хуком |
| `DE_Renderer.hpp:41`, `.cpp:159` | `beginRenderPass(..., const RenderTarget&, clearValues)` | → `beginRendering(const FrameData&, IRenderPass&)` |
| `DE_Renderer.cpp:42,105` | `swapchainTarget_.rebind(*swapchain_)` | удалить, члена больше нет |
| `DE_Renderer.cpp:198` | `beginRenderPass(frame, pass->getRenderTarget(), ...)` | → `beginRendering(frame, *pass)` |

`MainRenderPass` продолжает брать `extent` у `Renderer` (`DE_MainRenderPass.cpp:87–90`) до коммита D
(§5) — это отдельная зависимость, к `RenderTarget` отношения не имеет.

### 3.7 imgui, `DE_Gui.cpp:84–91`

Поле называется `PipelineInfoMain.PipelineRenderingCreateInfo` и имеет тип **C-структуры**
`VkPipelineRenderingCreateInfoKHR` (`imgui_impl_vulkan.h:85–94`), а не `vk::...`. Отсюда два
отличия от кода пайплайнов:

```cpp
// формат держим в переменной: на него смотрит указатель в структуре
VkFormat colorFormat = static_cast<VkFormat>(window_.getSurfaceFormat().format);

initInfo.UseDynamicRendering = true;
initInfo.PipelineInfoMain.RenderPass = nullptr;
initInfo.PipelineInfoMain.Subpass = 0;
initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
    .sType                    = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .pNext                    = nullptr,
    .viewMask                 = 0,
    .colorAttachmentCount     = 1,
    .pColorAttachmentFormats  = &colorFormat,
    .depthAttachmentFormat    = VK_FORMAT_UNDEFINED,   // GUI рисуется без глубины
    .stencilAttachmentFormat  = VK_FORMAT_UNDEFINED,
};
```

`depthAttachmentFormat` здесь **не «забыт», а обязан быть `VK_FORMAT_UNDEFINED`**: это парная
половина решения `GUIRenderPass::depthAttachment() == nullopt` (§3.4). Обе половины проверяются
вместе при отрисовке (`VUID-vkCmdDraw-None-08971/08972/08973` — цвет, глубина, стенсиль):

| Пайплайн объявляет depth | `pDepthAttachment` в `beginRendering` | Результат |
|---|---|---|
| `eUndefined` | `nullptr` | верно (наш GUI) |
| `kDepthFormat` | образ глубины с тем же форматом | верно (наш main) |
| `eUndefined` | образ передан | ошибка валидации на первом draw |
| `kDepthFormat` | `nullptr` | ошибка валидации на первом draw |

`VkPipelineRenderingCreateInfo` зануляется при инициализации (недостающие десигнаторы = 0), а
`VK_FORMAT_UNDEFINED` — это и есть 0, так что строку можно и не писать. Писать стоит: через полгода
вопрос «а где тут глубина?» возникает гарантированно, и ответ должен быть в коде, а не в памяти.

Почему GUI обходится без глубины: пайплайн imgui создаётся с занулённым
`VkPipelineDepthStencilStateCreateInfo` (`imgui_impl_vulkan.cpp:1070–1071`), то есть
`depthTestEnable = depthWriteEnable = VK_FALSE`. Раньше у GUI был свой `VkRenderPass`
с depth-вложением `eDontCare`/`eDontCare` (`DE_Gui.cpp:39–56`) — render pass умеет говорить
«вложение есть, но не трогай»; dynamic rendering так не умеет, вложение либо приложено, либо нет.
Приложить ту же глубину (`{eLoad, eDontCare}` + `kDepthFormat` в пайплайне) имеет смысл только если
imgui начнёт с ней сравнивать — сейчас он её не читает, а это лишний барьер и лишняя пара
«формат в пайплайне ↔ формат вложения».

`colorAttachmentCount = 1` должен совпадать с тем, что реально передаётся в `RenderingInfo`
(`beginRendering` передаёт ровно один цвет, §3.5), а `MSAASamples` — с реальным числом сэмплов
образов swapchain'а (`e1`). Расхождение — снова ошибка на draw, а не на создании пайплайна.

Четыре вещи, которые здесь важны:

1. **`sType` надо выставить руками.** У `vk::PipelineRenderingCreateInfo` `sType` заполняется
   конструктором, у C-структуры — нет. Backend проверяет его ассертом:
   `PipelineRenderingCreateInfo::sType must be VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR`
   (`imgui_impl_vulkan.cpp:1106`). В Release, где ассертов нет, это происходит **тихо**:
   `ImGui_ImplVulkan_Init` смотрит на `sType` только чтобы решить, создавать ли main-пайплайн
   (`:1238`), пайплайн не создаётся — GUI просто невидим, а `vkCmdBindPipeline(VK_NULL_HANDLE)`
   ловится уже на отрисовке. Заодно `pNext` обязан быть `nullptr` (ассерт `:1107`).
2. **`setColorAttachmentFormats(...)` здесь недоступен** — это сеттер `vk::`-обёртки. Можно
   присвоить `vk::PipelineRenderingCreateInfo` целиком (у него есть оператор приведения к
   C-структуре), но тогда `pColorAttachmentFormats` будет указывать на внутренний массив
   временного объекта. Backend форматы копирует (changelog `2025-04-07`), так что это
   сработает, но полагаться на документированный deep-copy в коде инициализации не стоит —
   дешевле держать `VkFormat` в переменной, как выше.
3. **`RenderPass` обязан остаться `nullptr`.** Сейчас там `*renderPass_->getRenderPass()`
   (`:85`) — при `UseDynamicRendering = true` и ненулевом render pass backend создаст
   пайплайн под render pass, а рисовать будет в dynamic-режиме.
4. **Поля живут под `PipelineInfoMain`** — это новое расположение в 1.93 WIP
   (changelog `imgui_impl_vulkan.cpp:47`). Старые `.RenderPass`/`.MSAASamples` в `initInfo`
   просто не скомпилируются.

`ImGui_ImplVulkan_PipelineInfo::PipelineRenderingCreateInfo` существует только под
`#ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING` (`imgui_impl_vulkan.h:91–93`) — с твоим
заголовком Vulkan 1.4 макрос определён, но если кто-то соберёт проект со старыми headers,
поле исчезнет и будет ошибка компиляции, а не тихий откат на render pass.

`ImGui_ImplVulkan_RenderDrawData` вызывается внутри уже открытого rendering instance:
backend сам `vkCmdBeginRendering` не вызывает, это проверено по исходнику
(`imgui_impl_vulkan.cpp:588` — ни одного вызова). То есть `GUIRenderPass::record` остаётся
как есть, а обёртку даёт `Renderer`.

### 3.8 Проверка B

- приложение запускается, картинка на 1200×1000 визуально та же;
- ноль ошибок валидации;
- ресайз окна не ломается (переходы выполняются каждый кадр заново);
- `Select-String -Path src\*.cpp,src\Systems\*.cpp -Pattern "setRenderPass\("` — пусто;
- `git grep -n "RenderTarget" -- src` — пусто (файл и оба прокси ушли в этом же коммите, §3.6);
- `git grep -n "renderPass\.begin\|beginRenderPass(" -- src` — пусто.

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
- `:15` forward-декларация `class RenderPass` и `#include "DE_RenderPass.hpp"` — удалить.

`SwapchainRenderTarget` (`:27–45`), член `swapchainTarget_` (`:104`) и
`getSwapchainRenderTarget()` (`:85`) уже удалены в B вместе с `RenderTarget` (§3.6).

### 4.4 Проходы

`RenderTarget` и оба прокси уже удалены в B (§3.6), здесь про них вспоминать нечего. Остаётся:

- `IRenderPass::preRecord`, `onSwapchainRecreation` — удалить; ни один проход не переопределяет
  первый, а второй нужен был только чтобы перепривязать `renderTarget_`;
- `MainRenderPass::onSwapchainRecreation` (`:109–112`) — удалить;
- `DE_RenderPass.hpp/cpp` — удалить файлы;
- `GUIRenderPass::getRenderPass` (`DE_Gui.cpp:130–133`), член `renderPass_`, его создание
  (`:39–56`) — удалить; `getClearValues` заменяется вложениями ещё в B (§3.4);
- `unregisterPass`, `unregisterCompute` (`DE_Renderer.hpp:73,80`) — удалить, не вызываются.

**Чего здесь нет и почему.** Соблазн завести обратно «абстракцию цели» (интерфейс с вьюхами,
чтобы когда-нибудь нарисовать в off-screen образ) стоит отложить до первого такого прохода: тогда
вернётся не интерфейс ради интерфейса, а конкретный владелец образов, и `beginRendering` получит
параметр. Пока цель одна — swapchain, и она уже член `Renderer`.

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

`beginFrame` (`:151–155`) заполняет `swapchain_->getSwapchainExtent()`; `MainRenderPass::record` берёт
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
| Сцена пропала, видно только GUI | у второго прохода `loadOp = eClear` вместо `eLoad` | `GUIRenderPass` (должен остаться дефолтный `colorAttachment()`) |
| Экран чёрный, ошибок нет | у главного прохода `storeOp = eDontCare` — результат не дошёл до present | `MainRenderPass::colorAttachment` |
| Геометрия не рисуется, ошибок нет | не выставлены viewport/scissor после `beginRendering` | `Renderer::beginRendering` |
| «A render pass instance must not be active» | вложенный `beginRendering` или забытый `endRendering` при `continue` в цикле | `renderFrame` |
| Случайные значения clear, падения при старте кадра | `RenderingAttachmentInfo` для depth — временный объект, `pDepthAttachment` висит | `beginRendering` |
| imgui рисует пустоту или падает на первом кадре | `PipelineInfoMain.RenderPass` не `nullptr` либо вызван без активного rendering | `DE_Gui.cpp:84–91` |
| GUI невидим, картинка сцены цела, ошибок нет | `PipelineRenderingCreateInfo.sType == 0` → backend не создал main-пайплайн (`imgui_impl_vulkan.cpp:1238`), пайплайн так и остался `VK_NULL_HANDLE` | `DE_Gui.cpp:84–91` |
| Валидация на первом draw GUI: «depth attachment has wrong format / not present» | пары разъехались: `depthAttachmentFormat` в пайплайне против `pDepthAttachment` в `beginRendering` (§3.7) | `DE_Gui.cpp:84–91` ↔ `Renderer::beginRendering` |
| Мусор/чёрный экран, который зависит от GPU и «иногда проходит» | `storeOp = eDontCare` у цвета в последнем проходе: содержимое образа объявлено неопределённым, а present его читает | `MainRenderPass::colorAttachment`, `ColorAttachment` по умолчанию |
| Мерцание, артефакты в первых кадрах после ресайза | переход цвета с `srcStage = eTopOfPipe` не упорядочен с acquire-семафором (`waitStages = eColorAttachmentOutput`) | `beginFrame`, §3.5 |

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
  коммите, что и B. `synchronization2` включается в A, переписывание — отдельный коммит, и он не
  механический: в B переходы появляются впервые (раньше их делал `VkRenderPass`), поэтому смена API
  в том же диффе складывает две независимые гипотезы. Подробно — §3.5.
- **Не трогать синхронизацию кадра**: `SubmitInfo`, `waitStages`, семифоры, `imagesInFlightFences_`.
  Dynamic rendering к ним не относится.
- **Не делать depth-образ на кадр** (`DE_Swapchain.cpp:89` создаёт один общий на `framesInFlight`).
  Это отдельный пункт: там нужно разбираться, какая зависимость сейчас закрывает хазн
  между кадрами.
- **Не заводить мини-рендер-граф.** Два прохода руками — правильный промежуточный уровень.
- **Не тащить в `ColorAttachment`/`DepthAttachment` `imageView`, `imageLayout` или `vk::Extent2D`.**
  Структуры описывают *политику* (что делать с вложением), а образы, вьюхи и размеры остаются
  у `Swapchain`/`Renderer`. Иначе проход начинает знать про swapchain, и `onSwapchainRecreation`
  из `DE_MainRenderPass.cpp:109–112` разъезжается с тем, что проход уже успел закешировать.
- **Не отдавать проходу владение `vk::PipelineRenderingCreateInfo`.** Это убрало бы последний шов
  «формат вложения ↔ формат пайплайна», но на два прохода цена выше выгоды (см. конец §3.4).
- **Не держать `#ifdef DE_LEGACY_RENDERPASS`.** Два пути отрисовки разъедутся по переходам
  быстрее, чем помогут; откат — это git, а не `#ifdef`.

---

## 9. Откат

- A откатывается независимо (5 строк).
- B откатывается одним `git revert`, потому что атомарный.
- C и D откатываются независимо и не влияют на поведение.
- Если после B картинки нет, а валидация молчит: первым делом проверить (1) `storeOp = eStore`
  у цвета в **последнем** проходе, (2) финальный переход в `PresentSrcKHR`, (3)
  `setViewport`/`setScissor`, (4) `loadOp`/`pDepthAttachment` у GUI. Эти четыре покрывают
  девять из десяти «чёрных экранов» при переходе; `storeOp = eDontCare` — первый по частоте,
  потому что на дискретных GPU он «работает», а на iGPU и тайловых даёт мусор.
