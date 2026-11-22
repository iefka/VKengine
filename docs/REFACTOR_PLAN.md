# План рефакторинга VKengine

Документ описывает поэтапный переход от текущего состояния (рабочий движок на 86 собственных
файлах, ~6.5k строк) к архитектуре, которую можно расширять без переписывания, и к современному
C++ (23 → точечно 26). Плюс отдельный раздел про модули с честной оценкой «надо/не надо».

**Окружение, под которое написан план:** Windows, MSYS2 **CLANG64** (clang++ **23.1.4**, libc++,
mingw-w64 UCRT — не MSVC STL), Vulkan SDK 1.4.357, приложение запрашивает API **1.4**,
Ninja-сборки `build-vscode-clang64` (Debug) и `build-vscode-release`, `framesInFlight = 2`,
`VK_HARD_VALIDATION` уже заведён опцией CMake.

Следствия для плана, которые учтены ниже:
- Vulkan **1.3 core** у тебя уже доступен → dynamic rendering и `synchronization2` — не расширения,
  а core-функциональность; `VK_EXT_scalar_block_layout` не нужен, `scalarBlockLayout` — core-фича 1.2
  в уже используемой цепочке `PhysicalDeviceVulkan12Features`.
- clang **23.1.4** → `#embed` (clang 19+) и весь C++23 STL (libc++) доступны; `std::meta` в мейнлайне нет.
- libc++ вместо MSVC STL означает, что «плохая» ветка развилки §4.2.1 тебя не касается.

**Три проверки на 30 секунд, которые снимают половину неопределённости в §4** (clang 23.1.4):

```bash
clang++ --version && clang++ -print-resource-dir   # что в PATH именно 23.1.4, а не второй clang от VS
find /clang64 -name "libc++.modules.json" -o -name "std.cppm" 2>/dev/null
```

- Нашёлся `libc++.modules.json` — libc++ собран с модулями, `import std;` принципиально возможен,
  спайк §4.4 имеет смысл. Ничего не нашлось — MSYS2-шный libc++ без модульных исходников,
  и вопрос модулей закрывается **до** спайка, по внешней причине, а не по твоей.
- Отдельно: если однажды заведёшь VS-пресет с toolset ClangCL — это **другой** clang, обычно
  старее 23.1.4. Модули проверяй только в Ninja-пресете, иначе получишь «не собирается» на ровном месте.
- `#embed` в 23.1.4 включён, но в C++-режиме это C23-расширение: при `-Wpedantic`/`-Werror`
  понадобится `-Wno-c23-extensions` (см. §4 про `#embed`).

---

## Статус (обновлено 2026-11-09, `main` @ `14fa095`)

**Фаза 0 закрыта, фаза 1 закрыта.** Ниже — фактическое состояние, включая отклонения от
первоначального плана: часть решений в процессе оказалась другой (и лучше), часть сознательно
перенесена, часть не сделана и вынесена в отдельный список хвостов.

### Что сделано и как именно

| Пункт плана | Фактическая реализация | Отличие от плана |
|---|---|---|
| 0: `.gitignore`, пресеты, флаги, `baseline.md` | `.editorconfig`, `CMakePresets.json`, `docs/baseline.md` (время сборки + fps) | `.clang-format` **отвергнут** (свой формат); CI и тесты не заводились; в baseline нет времени старта, счётчика `queue.submit` и CPU-времени кадра |
| 1.1 `RenderContext` | device-scoped: `const Device*`, `descriptorLayouts`, `descriptors`, `pipelinesCache` (`vk::PipelineCache`) | `extent`/`samples` **вынесены** в `PassContext` — они swapchain-scoped и меняются на ресайзе; `pipelinesCache` — handle, а не указатель на обёртку |
| 1.2 `SetSlot` + `bindSet` | `SetSlot` — словарь из 4 имён; `bindSets(cmd, layout, std::span<vk::DescriptorSet>)` **позиционный**, без индекса; `bindSet` удалён | индекс сета — свойство конкретного pipeline layout, а не глобальное понятие; литерал `0` в биндингах не остался |
| 1.2 списки слотов | `kSlots` только там, где сетов больше одного: `RenderSystem` (4), `SkyBoxSystem` (1 общий + 1 приватный через локальный `SkyBoxSlots`); `static_assert(areConsecutiveFromZero(kSlots))` | от «списка из одного элемента» и от локальных enum'ов у односетовых систем отказались — это был лишний механизм |
| 1.3 `DescriptorSetLayoutCache` | header-only, ключ `uint32_t`, концепт `Uint32EnumClass`, `ensure()` идемпотентный («тот же слот ⇒ тот же layout») | хеширование списка биндингов как ключ **отвергнуто**; кэш не владеет layout'ами и не знает про `SetSlot` |
| 1.3 `DescriptorArena` | класс написан (пул на кадр, `allocate`/`reset`, `DE_ASSERT` на индекс) | **не подключён**: `RenderContext::descriptors = nullptr`, все 7 `DescriptorPool` на месте — переезд по одному владельцу в фазах 2–4 |
| 1.4 `std::span` вместо `void*` | `GeometryPool::upload`, `Texture{..., std::span<std::byte> data, ...}` | `Buffer::copyToBuffer(const void*)` остался — см. хвосты |
| 1.5 `common.glsl` | `Shaders/general/common.glsl` (`GlobalUbo`, `PointLight`, `lights[10]`), подключён в 5 шейдеров; `MAX_LIGHTS` из `DE_FrameData.hpp` убран | `glslc` без `-I`, `DEPENDS` без `common.glsl` — см. хвосты |
| 1.6 `static_assert` на `GlobalUBO` | **не сделано** | старые assert'ы удалены вместе с `_padding`, обратно не добавлены; числа изменились (см. хвосты) |
| 1.7 `layout(scalar)` | проверка фичи + включение в `Device::createLogicalDevice`; `layout(scalar, set=…)` в `GlobalUbo`/`MeshBuffer`/`TransformBuffer`; `_padding` из C++-структур вычищены | — |
| 3.2 `vk::PipelineCache` | **сделано раньше срока**: класс + `Builder::setCache`, все системы передают кэш, `initInfo.PipelineCache` у imgui | эффект пока не измерен (нет счётчиков и `PipelineCreationFeedback`) |
| 2.2 скретч-буферы | **сделано раньше срока**: `transforms_`/`indirectCommands_` — члены `RenderSystem` | — |
| 0.3 `totalTime_` | сделано | — |

### Хвосты фазы 1 (6 правок, каждая < 30 мин)

1. **`static_assert` на раскладку** `GlobalUBO`/`pointLight` под scalar: `pointLight` = 36,
   `pointLights` на смещении 272, `numLights` на 632, `GlobalUBO` = 636 — при условии, что в
   `glm_config.hpp` не задан `GLM_FORCE_DEFAULT_ALIGNED_GENTYPES`. Ровно этот assert и поймает,
   если условие нарушится.
2. **`glslc`: `-I` и `DEPENDS`.** Сейчас `COMMAND glslc "${SHADER_FILE}"` и
   `DEPENDS "${SHADER_FILE}"`: `common.glsl` резолвится относительно каталога шейдера (работает
   случайно), а правка общего заголовка **не пересобирает ни один шейдер** — молча, без ошибки.
3. **`Buffer::copyToBuffer(std::span<const std::byte>, vk::DeviceSize offset = 0)`** вместо `void*`
   (шаблонный overload для контейнеров остаётся сахаром).
4. **Литералы `0` в биндингах compute-пассов** (`DE_ShaderTexturingPass.cpp:97`,
   `DE_RuntimeComputeGeometry.cpp:154`) → `bindSets` с массивом из одного сета.
5. **Вынести `Uint32EnumClass` + `areConsecutiveFromZero`** из `DE_DescriptorSetLayoutCache.hpp`
   в `core/DE_SlotUtils.hpp`: сейчас `RenderSystem.hpp` и `SkyBoxSystem.hpp` включают
   `unordered_map` + `DE_Descriptors.hpp` ради одной `consteval`-функции.
6. **Убрать из кэша `DE_Descriptors.hpp`**, дописать недостающие `<array>`/`<cstddef>`/`<cstdint>`
   (сейчас `std::array` в подписи живёт на транзитивных инклюдах `vulkan.hpp`).

### Что нужно снять до старта фазы 2

DoD фазы 2 («≥30% CPU, один submit») без этих чисел непроверяем, а `baseline.md` их не содержит:

| Метрика | Как |
|---|---|
| Число `queue.submit` + `waitIdle` на старте | счётчик в `Device::endSingleCommand` под `#ifdef DE_COUNTERS` |
| Время от `main()` до первого кадра | `steady_clock` вокруг `initResources()`, холодный кэш пайплайнов и тёплый |
| CPU-время кадра | замер `renderFrame()` целиком, p50/p95 по 1000 кадрам |

---

## Старт: первые три дня, по коммитам

> **Историческая часть.** Так работа начиналась; большая часть пунктов выполнена (см. «Статус»
> выше). Из отличий: `.clang-format` отвергнут — используется свой формат, добавлен только
> `.editorconfig`; CI и тесты пока не заводились.

Порядок отличается от нумерации фаз намеренно: сначала то, что нельзя сделать потом без потерь
(инструменты и замеры), и только затем код.

**День 1 — «сделать так, чтобы можно было откатывать»**

| # | Коммит | Время | Зачем именно сейчас |
|---|---|---|---|
| 1 | `git rm --cached Shaders/bin/*.spv` + `.gitignore` | 10 мин | Пока бинари в индексе, каждый `git status` — шум, а diff нечитаем |
| 2 | `CMakePresets.json` (Приложение A) + проверка `PATH` | 20 мин | Без пресетов `cmake --preset` не работает, а замеры придётся делать в «старой» сборке — числа будут несравнимы с тем, что будет после рефакторинга |
| 3 | `docs/baseline.md`: время сборки Debug/Release, инкремент, время кадра, `0` ошибок validation | 2–3 ч | Мерить **до** флагов: они изменят сборку, и «стало быстрее?» будет не с чем сравнить |
| 4 | `.clang-format` + `.editorconfig` | 30 мин | Дешевле сейчас, чем разгребать в диффах полтора месяца |

Замер из п.3 снимай **один раз и сохрани числа в файл**, а не «помню, было примерно». Первый
`-ftime-trace` тоже здесь: если `vulkan.hpp` уже сейчас занимает треть сборки, фаза 6 (модули/PCH)
получит цель, а не надежду.

Порядок 2 → 3 важен: baseline меряется в той самой сборке, которую потом будешь сравнивать, иначе
разница в секундах уйдёт в шум генератора. **Новых каталогов не заводим:** пресеты указывают
`binaryDir` на уже существующие `build-vscode-clang64` / `build-vscode-release` (Приложение A).
Плюс: не надо заново качать FetchContent-зависимости, минус: у существующего каталога уже есть
`CMakeCache.txt`, и генератор с компилятором в пресете должны **совпасть** с ним (см. ниже).

Одна ловушка MSYS2: в **PowerShell** `clang++` и `ninja` могут быть не в `PATH` (они лежат в
`C:\msys64\clang64\bin`). Либо запускай из шелла MSYS2 CLANG64, либо добавь каталог в `PATH` —
иначе `cmake --preset` упадёт на «CMAKE_CXX_COMPILER not found» уже после того, как пресет найден.

**Грабли Ninja на Windows (встречаются при первом же configure):**

- `ninja: error: failed recompaction: Permission denied` на `_deps/*-subbuild` — ninja не может
  перезаписать `.ninja_log`. Причины по частоте: (1) второй живой процесс (открытый VS Code с
  CMake Tools, забытый `ninja`/`cmake`); (2) Windows Defender сканирует только что созданный файл;
  (3) read-only атрибут после распаковки/синхронизации облаком. Лечение: закрыть процессы, удалить
  каталог сборки целиком (после ошибки кэш полузаписан), добавить исключение в Defender для
  каталога проекта и `C:\msys64`.
- Если исключение в Defender сделать нельзя — вынести `binaryDir` из дерева исходников
  (`"binaryDir": "C:/build/vkengine/clang-debug"`), чтобы сканирование не пересекалось со сборкой.
- `where.exe ninja` — убедись, что найден ninja **из MSYS2** (`C:\msys64\clang64\bin`), а не из
  Visual Studio/Strawberry Perl: смешивание сборок даёт невоспроизводимые падения.

**День 2 — «убрать известные баги по одному»**

| # | Коммит | Время |
|---|---|---|
| 4 | `totalTime = totalTime` → `totalTime_ = totalTime` (`DE_MainRenderPass.cpp`) | 15 мин |
| 5 | `throw` в `Texture.cpp:84` | 10 мин |
| 6 | `static_assert` на размер/раскладку `GlobalUbo` + `-Wuninitialized` по своим структурам | 1–2 ч |

Каждый — отдельный коммит: если картинка изменится, ты будешь знать, какой из трёх её сдвинул.

**День 3 — «сделать вывод компилятора полезным»**

| # | Коммит | Время | Тонкость |
|---|---|---|---|
| 7 | `-Wall -Wextra` только для своих таргетов (`target_compile_options`, не `add_compile_options`) | 1 ч | Сначала просто **посчитай** варнинги. Ожидай несколько сотен — это нормально, не чини в этом же коммите |
| 8 | Чистка варнингов порциями по файлу/системе | 3–4 ч | По одному коммиту на файл, иначе diff нечитаем |
| 9 | `-Wconversion -Wshadow -Wpedantic` | 1–2 ч | Уже после того, как базовые вычищены, иначе утонешь |

`-Werror` — не раньше, чем счётчик дойдёт до нуля, и не в этот заход.

**Первое изменение в движке (не RenderContext, а это):** depth image на кадр при `framesInFlight = 2`
(§3 фазы 3). Причина — оно маленькое, независимое и **измеримое**: есть `baseline.md` с временем
кадра, есть validation, есть второй кадр в полёте. Если после правки кадр ускорился — гипотеза про
сериализацию подтвердилась; если нет — ты это узнал за полдня, а не после месяца рефакторинга.

**Чего не делать в первую неделю:** не включать модули (§4), не заводить `Scene` (фаза 2), не
переписывать синхронизацию целиком, не включать `-Werror`, не трогать архитектуру «пока не поздно».
Все они выигрывают от того, что сеть безопасности уже стоит.

---

## 0. Принципы плана

1. **Движок работает после каждого этапа.** Никаких «полгода ветка не собирается». Каждый этап —
   самостоятельный PR/коммит, который заканчивается запускающимся приложением с той же картинкой.
2. **Сначала контракты, потом оптимизация, потом косметика.** Порядок фаз выбран так, чтобы
   каждая следующая опиралась на предыдущую (например, именованные сеты нужны динамическому
   рендерингу, а `Scene` — сортировке и рендер-графу).
3. **Ничего не делаем «на будущее».** Каждая задача закрывает конкретную боль из текущего кода.
4. **Инструменты раньше рефакторинга.** Варнинги, тесты и CI ставятся в фазе 0 — иначе
   последующие 30 изменений нечем проверять.
5. **Оценки — в человеко-днях (6–8 ч)** для одного разработчика, с учётом отладки. Диапазон
   «оптимистично–реалистично».

---

## 1. Замеры «до» (baseline, 2–3 часа)

Без этих чисел нельзя понять, помог ли рефакторинг. Снять один раз, записать в
`docs/baseline.md`:

| Метрика | Как мерить |
|---|---|
| Время полной сборки | `cmake --build` с нуля, секунды (Release и Debug отдельно) |
| Время инкрементальной сборки | правка одного `.cpp` в `Systems/` → rebuild |
| Из чего состоит сборка | `-ftime-trace` → ui.perfetto.dev: доля `vulkan.hpp`/`glm`/`imgui` против своих файлов (clang-only, бесплатно) |
| Время старта | от `main()` до первого кадра, мс |
| Число `queue.submit` + `waitIdle` при старте | счётчик в `Device::endSingleCommand` под `#ifdef DE_COUNTERS` |
| CPU-время кадра | `deltaTime` при фиксированной сцене, p50/p95 за 1000 кадров |
| GPU-время кадра | RenderDoc / GPU timestamp queries |
| Пик VRAM | RenderDoc, `vkGetDeviceGroupPeerMemoryFeatures`-не нужно, просто total allocation |
| Число варнингов | `-Wall -Wextra` по своим TU (после фазы 0) |

Дальше все фазы ссылаются на эти числа как на «цель».

**`-ftime-trace`: куда именно его кидать в CMake.** Флаг не глобальный и не передаётся через
переменную окружения — это опция компиляции конкретного таргета:

```cmake
option(VKENGINE_TIME_TRACE "Emit clang -ftime-trace JSON next to each object" OFF)

if(VKENGINE_TIME_TRACE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(vulkan_tutorial PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-ftime-trace>
        $<$<COMPILE_LANGUAGE:CXX>:-ftime-trace-granularity=500>)
endif()
```

- Включать **опцией, а не по умолчанию**: трейс пишет JSON на каждый TU и немного замедляет сборку.
- Где лежат файлы: рядом с объектниками — `build-vscode-clang64/CMakeFiles/vulkan_tutorial.dir/src/*.cpp.json`.
- **Нужен генератор Ninja.** С VS-генератором трейсы не появляются — ещё один
  аргумент, что замеры делаются в Ninja-пресете.
- Смотреть: один файл — `ui.perfetto.dev` (перетащить `.json`); все сразу — слить `ClangBuildAnalyzer`
  (`--all build-vscode-clang64/ trace.bin` → `--analyze trace.bin`), он же показывает суммарное время по
  заголовкам, что и нужно для решения про PCH.
- Каталог `build/` уже в `.gitignore` — трейсы в репозиторий не попадут.

---

## 2. Фазы

### Фаза 0 — Фундамент и страховка (2–3 дня)

**Цель:** чтобы дальнейшие 30 изменений можно было делать без страха.

Задачи:

1. **Ревизия мусора.**
   - Удалить `conanfile.txt` (сборка на FetchContent) и `.girignore`. Опечатка в имени означает,
     что файл никогда ни на что не влиял — его содержимое надо глазами проверить на забытые
     правила и перенести в `.gitignore`, а не просто удалить.
   - Удалить `src/push_constants.hpp` (`PushConstantsRegistry` + `pushConstants2`) — не используется,
     живой API в `DE_PushConstants.hpp`.
   - Удалить `DE_Model.hpp`/`DE_Model.cpp` под `#if MODEL` — второй, устаревший vertex-тип
     (`glm::vec3` + форматы `R32G32B32A32`, которые с ним не совпадают).
   - Удалить/заменить `#if 1`, `#if BEDROOM`, `#if BREKFAST_ROOM` — сцена будет переезжать в фазу 2.
2. **`.gitignore`** *(файл уже добавлен — осталось вычистить индекс)*. Содержимое: `build*`,
   `out/`, `.vs/`, `Shaders/bin/*.spv`, `*.obj.mesh`, `*.obj.matCache`, `CMakeUserPresets.json`.
   **Важно:** `.gitignore` не действует на уже отслеживаемые файлы, а в индексе лежат 10 `.spv`
   (`git ls-files Shaders/bin`) — их надо вынести отдельным коммитом:
   `git rm --cached Shaders/bin/*.spv`. Иначе после пересборки шейдеров `git status` будет
   показывать модификации бинарей, и `.gitignore` создаст ложное чувство порядка.
   Проверка после: `git status --ignored --short Shaders/` — бинари должны быть в `!!`.
3. **Три реальных бага** (по одному коммиту):
   - `DE_MainRenderPass.cpp`, `updateTime`: `totalTime = totalTime;` → `totalTime_ = totalTime;`
     (сейчас time-based эффекты получают 0).
   - `Texture.cpp:84`: `std::runtime_error(...)` без `throw`.
   - Неинициализированные поля: `Model` удаляем, но прогнать по всем структурам `-Wuninitialized`.
4. **Флаги компиляции** (только для своего кода, не для third_party). Тулчейн — **clang++**
   (`/utf-8` не нужен: clang по умолчанию считает исходники UTF-8, BOM/кириллица в комментариях
   не мешают):
   ```cmake
   target_compile_options(vulkan_tutorial PRIVATE
     -Wall -Wextra -Wshadow -Wconversion -Wpedantic
     -Wno-unused-parameter          # интерфейсы с неиспользуемыми аргументами
     $<$<CONFIG:Debug>:-O0 -g -fstandalone-debug>
     $<$<CONFIG:Debug>:-ftime-trace>)   # см. §1, замеры
   ```
   Нюансы под clang:
   - если компилируешь как **clang-cl** (`--driver-mode=cl`), флаги `-W*` всё равно принимаются,
     но `/W4` лучше не смешивать — оставь только `-W`-набор;
   - `-Wconversion` на `vk::Format`/`glm` может шуметь в заголовках third_party — лечится
     `-isystem` для `third_party/`, а не отключением варнинга;
   - `-fstandalone-debug` — чтобы RenderDoc/отладчик видели типы в Debug, а не «оптимизировано».
   `-Werror` — не сразу, а через 2 недели, когда счётчик упадёт до нуля.
5. **`CMakePresets.json`** под clang++:

   | preset | генератор | зачем |
   |---|---|---|
   | `clang-debug` | Ninja | основной рабочий, `binaryDir` = существующий `build-vscode-clang64` |
   | `clang-release` | Ninja | замеры и пакет, `binaryDir` = существующий `build-vscode-release` |

   Пресеты **не создают новых каталогов** — они лишь дают существующим сборкам имена, чтобы
   `cmake --preset`, CI и VS Code работали с одним и тем же кэшем. Отдельный VS-пресет не заводим:
   он означал бы третий каталог и другой clang.

   Ninja — не «потому что модно»: с ним быстрее сборка, нормально работает `-ftime-trace`,
   и он совместим с модулями (CMake + clang сканируют зависимости через `clang-scan-deps`,
   который должен быть в `PATH`).
6. **Тесты + минимальный каркас.** `FetchContent(doctest)` (header-only, встраивается в 10 строк),
   отдельный таргет `de_tests`, первые тесты на «чистые» функции:
   `RangeAllocator` (allocate/free/grow/фрагментация), `floatToHalf` (денормали, inf, nan),
   `getFormatSize`, `equirectUV`-подобная математика, `Transform::normalMatrix`.
7. **CI** (GitHub Actions, `windows-latest`): установка Vulkan SDK, `cmake --preset clang-release`,
   `ctest`. Даже один джоб ловит 80% «а у меня не собирается».
8. **`.clang-format`** + `.editorconfig` (единый стиль мешает меньше, чем кажется).

**DoD:** сборка с нуля, все тесты зелёные, CI зелёный, варнингов по своим файлам < 30.
**Риск:** низкий. Всё обратимо.

---

### Фаза 1 — Контракты между классами ✅ сделано

**Цель (достигнута):** убрать неявные договорённости — номера сетов, порядок layout'ов,
магические константы — и разделить device-scoped и pass-scoped состояние.

**Итоговая форма — это и есть договорённость, на которую опираются следующие фазы:**

1. **Два контекста, а не один.**
   ```cpp
   struct RenderContext {                  // device-scoped, создаётся один раз
       const Device* device;
       const DescriptorSetLayoutCache* descriptorLayouts;
       DescriptorArena* descriptors;       // пока nullptr, см. фазу 2–4
       vk::PipelineCache pipelinesCache{}; // null = создавать пайплайны без кэша
   };

   struct PassContext {                    // pass-scoped, меняется на ресайзе
       const RenderPass* renderPass;
       vk::Framebuffer frameBuffer{};
       vk::Extent2D extent;
       vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
   };
   ```
   Правило: контекст **передаётся вниз** в конструктор, а не хранится ссылкой/указателем
   (указатель на чужую локальную переменную — висячая ссылка на первом же рефакторе).
   `extent` допустим как параметр конструктора (статические viewport/scissor), но на кадре
   читается из `PassContext`, а не из поля системы.

2. **Слоты.** `SetSlot` — словарь имён **общих** layout'ов (`eGlobal`, `eTextures`,
   `eMaterialData`, `eTransforms`), а не индекс биндинга. Биндинг позиционный:
   `bindSets(cmd, pipelineLayout, std::span<const vk::DescriptorSet>)`, всегда с индекса 0.
   Список `kSlots` заводится только если сетов больше одного **и** он используется в двух местах
   (создание layout'ов и биндинг) — сейчас это `RenderSystem` и `SkyBoxSystem`, и там же стоит
   `static_assert(areConsecutiveFromZero(kSlots))`.

3. **Кэш layout'ов.** `DescriptorSetLayoutCache`: `ensure(slot, layout)` — идемпотентный
   («тот же слот ⇒ тот же layout», `DE_ASSERT` на конфликт) и `get(slot)`. Ключ — любой scoped
   enum с `uint32_t` (концепт `Uint32EnumClass`), хранение — `unordered_map<uint32_t, …>`.
   Кэш **не владеет** layout'ами и не знает про `SetSlot`. Приватные layout'ы систем (cubemap
   у skybox, SSBO у compute) в кэш не попадают: их числовые ключи столкнулись бы с общими.

4. **Ошибки.** `core/DE_Assert.hpp`: `DE_ASSERT` — Debug `assert`, Release
   `throw std::logic_error` с текстом условия и файлом. Два правила: не использовать в
   деструкторах и `noexcept`-функциях (Release-ветка там = `std::terminate`), и не использовать
   в горячих циклах — в Release условие всё ещё вычисляется.

5. **Шейдеры.** `Shaders/general/common.glsl` (общие структуры) + `layout(scalar)` в UBO/SSBO;
   `scalarBlockLayout` проверяется и включается в `Device::createLogicalDevice`. Паддинги
   `float _padding[3]` из C++-структур удалены — раскладка теперь совпадает с GLSL буквально.

**DoD:** в биндингах нет номеров сетов; все создания пайплайнов идут через `vk::PipelineCache`
(`git grep "vk::PipelineCache{}"` → пусто); одинаковые layout'ы не дублируются; шейдеры включают
общий заголовок; `GlobalUBO` защищён `static_assert` (пока нет — см. хвосты).

**Осталось:** шесть пунктов из «Хвостов фазы 1» выше. Ни один не блокирует фазу 2.

**Риск (по факту):** реализованный перебор layout'ов ломал биндинги дважды — двойной биндинг сета
с null в `sets[]` и рассинхрон «layout на 4 сета / биндим 5». Оба ловятся `DE_ASSERT` +
валидацией, но не глазами; отсюда и требование к `static_assert` на списках слотов.

---

### Фаза 2 — Кадр: upload и данные сцены (3–5 дней)

**Цель:** CPU перестаёт делать O(N) работы каждым кадром безусловно, а старт — перестать быть
сотней последовательных submit'ов с `waitIdle`.

Порядок ниже выбран по «выгода / риск», а не по номерам первоначального плана.

0. **Снять три числа** из таблицы «Что нужно снять до старта фазы 2» (счётчик submit'ов, время до
   первого кадра, CPU-время кадра). Без них DoD ниже непроверяем.

1. **`UploadContext`** (`src/gpu/DE_UploadContext.hpp`) — первый и самый выгодный шаг.
   Сейчас `Device::endSingleCommand` делает `submit` + `queue.waitIdle()` на **каждую** операцию:
   `MaterialBuffer::upload` — на каждый материал (плюс локальный staging-буфер и его пул),
   `Texture` — 2–3 на текстуру (staging + мипы), по одному в четырёх местах `GeometryPool`,
   в `Mesh`, в `MaterialBuffer::grow`.
   Форма: один `vk::CommandBuffer` и один fence на батч; persistent-mapped staging с линейным
   аллокатором; `cmd()` для ручных записей (мипы, барьеры); `flush()` = end + submit + wait.
   **Не в `Device`** — устройство остаётся обёрткой, батчинг это слой выше.
   Ловушки, проверенные на текущем коде:
   - staging обязан жить до сигнала fence → член контекста, а не локальная переменная
     (сейчас в `MaterialBuffer::upload` это работает только из-за `waitIdle` внутри);
   - `MaterialBuffer::grow` внутри батча: новый буфер + отложенное удаление старого после fence;
   - один `vkCmdPipelineBarrier` перед `flush()` (transfer → shader read, layout'ы образов):
     сейчас барьеров нет вовсе, и это держится на `waitIdle` + порядке сабмитов;
   - контекст не вставляет барьеры между командами пользователя: порядок внутри батча значим
     (мипы зависят от предыдущего blit'а).

2. ~~**Dirty-механизм для compute**~~ — **вычеркнуто.** `ComputeTextureTarget` (2048²) и
   `RuntimeComputeGeometry` живут в `src/Experements/` и остаются как есть: они учебные, их цена
   в кадре — осознанная. `IComputeStep` не получает `invalidate()`/`isDirty()`, `Renderer`
   не пропускает шаги. Если когда-нибудь понадобится — это отдельная задача поверх `IComputeStep`,
   интерфейс для неё ничего не блокирует.

3. **`Scene`** (`src/scene/DE_Scene.hpp`): `std::vector<GameObject>` + `idToIndex`, стабильный
   порядок между запусками, `buildDrawList()` с сортировкой по `(pipelineKey, materialIndex)`
   и dirty-флагом. Самый инвазивный пункт фазы (трогает `GameObject::Map` во всех системах) —
   делать последним.

3. **Сцена как данные** (опционально, после `Scene`): `scenes/demo.scene` + `SceneLoader`, убрать 200 строк из
   `initGameObjects`, менять сцену без пересборки.

4. **`framesInFlight`**: измерить 2 против 3, зафиксировать в конфиге. Не раньше, чем закрыт п.1.

Скретч-буферы (`transforms_`/`indirectCommands_` как члены `RenderSystem`) — **сделано в фазе 1**,
из плана вычеркнуто.

**DoD:** на старте единицы `vkQueueSubmit` вместо десятков; время до первого кадра упало
(числа из п.0); CPU-время кадра на `assets/HWY_Galleri` −≥30% от baseline.
**Риск:** средний — синхронизация upload'ов, но всё изолировано в одном классе.
**Не делать в этой фазе:** не смешивать с правками fences/semaphores кадра. Upload-путь живёт
вне кадра; смешение даёт неповторяющиеся артефакты и неделю отладки.

---

### Фаза 3 — Проходы: dynamic rendering + барьеры (5–8 дней)

**Цель:** убрать `VkRenderPass` из конструкторов систем; открыть дорогу второму проходу
(IBL-фон, пре-пасс, MSAA, деферред) без пересоздания пайплайнов.

1. **Dynamic rendering** (core Vulkan 1.3, `vk::RenderingAttachmentInfo` + `cmd.beginRendering`):
   - удалить `DE_RenderPass.{hpp,cpp}`, `DE_RenderTarget.hpp`, `onSwapchainRecreation` из
     `IRenderPass`;
   - `IRenderPass::record(const FrameInfo&, const PassTarget&)`, где `PassTarget` — вектор
     `RenderingAttachmentInfo` + extent (сегодня эту роль играет `PassContext`);
   - пайплайны создаются с `vk::PipelineRenderingCreateInfo` (форматы + samples), без render pass;
   - `Swapchain` перестаёт держать render pass и framebuffer'ы.

2. **Один API барьеров**: заменить `de::imageBarrier` и `Device::transitionImageLayout` на
   `de::Barrier`-хелпер на `vk::DependencyInfo`/`vk::ImageMemoryBarrier2`. Ты на Vulkan 1.4, то
   есть `synchronization2` и dynamic rendering — **core**, достаточно объявленной `apiVersion`.
   (Заодно чинится баг `setLevelCount(1)`, который перетирает `mipLevelCount`.)

3. **Глубина на кадр, а не на свопчейн** (можно и до dynamic rendering, отдельный маленький PR):
   `DE_Swapchain.cpp:89-94` создаёт **один** depth image/view, общий для всех framebuffer'ов,
   при `framesInFlight = 2`. Консервативная `SUBPASS_EXTERNAL → 0` зависимость
   (`DE_RenderPass.cpp:76-90`), которая сейчас закрывает sync-хазарды, делает это ценой
   сериализации: второй кадр не может писать в depth, пока первый не дошёл до
   `LATE_FRAGMENT_TESTS`. Правильно — копия depth на кадр (или на swapchain image) и узкие
   зависимости. С dynamic rendering это два читаемых барьера на кадр.

4. **Мини-рендер-граф (отдельный PR)**: проход объявляет `reads`/`writes`
   (`ImageBinding{image, layout, stage}`), граф сам строит барьеры между проходами.
   Правило: ручные барьеры остаются разрешёнными как escape hatch.

Кэш пайплайнов (`vk::PipelineCache`) — **сделано в фазе 1**: класс, `Builder::setCache`,
все системы и `initInfo.PipelineCache` у imgui. Осталось измерить эффект
(`vk::PipelineCreationFeedbackCreateInfo`, флаг `eApplicationPipelineCacheHit`) и решить,
обслуживать ли дальше: если суммарный выигрыш на старте < ~20 мс — зафиксировать и не трогать.

**DoD:** `grep -r "vk::RenderPass" src/` → пусто (кроме third_party); второй проход добавляется
без правок существующих систем; depth-картинка на кадр, кадры не сериализуются.
**Риск:** высокий — самый инвазивный этап. Отдельной веткой, RenderDoc на каждом шаге,
старый путь за `#ifdef DE_LEGACY_RENDERPASS` до конца фазы.

---

### Фаза 4 — Ресурсы и память (5–10 дней)

**Цель:** аллокации и владение перестают быть «по одному `vk::DeviceMemory` на объект».

1. **`Buffer`/`Image`: вернуть move-semantics** (сейчас `= delete` → везде `std::unique_ptr`).
   Внутри — `vk::UniqueBuffer` + `Allocation{memory, offset, size}`.
2. **Аллокатор памяти**: страницы по 64–256 МБ на memory type, bucketed free-list,
   suballocation для Image с учётом `bufferImageGranularity` (4096) и `linear/tiled` границ;
   dedicated allocation для больших картинок.
   - Вариант «сделать самому»: ценно для обучения, но 7–10 дней.
   - Вариант «взять VMA» (`vk_mem_alloc.h`, 2 файла): 1–2 дня, зрелый код.
     Рекомендую **VMA**, а свою реализацию оставить как эксперимент в `Experements/`.
3. **Отложенное удаление**: `ResourceRegistry::retire(handle)` → держим до `frame + framesInFlight`,
   потом освобождаем. Снимает вопрос «когда можно удалять текстуру/буфер».
4. **Дескрипторные хендлы** вместо `vk::DescriptorSet` в системах — уже частично в фазе 1.
5. **`Device::executeSingleCommand`** ограничить: разрешить только в дебаге/тестах, в
   проде — `UploadContext`.

**DoD:** число `vkAllocateMemory` при старте < 50 (сейчас ~сотни); в коде нет
`std::make_unique<Buffer>` вне аллокатора; тест на аллокатор с фрагментацией.
**Риск:** средний. VMA-путь — низкий.

---

### Фаза 5 — C++-модернизация и reflection (2–3 дня, россыпью по 0.5 дня)

Мелкие PR'ы, каждый независим:

| Задача | Где |
|---|---|
| `std::bit_cast` вместо `memcpy` | `Texture.cpp` (half-конверсия), `DE_Memory` |
| `std::mdspan` для пикселей (`image[y, x, c]`) | лоадеры кросс-куба, equirect |
| `std::flat_map` вместо `unordered_map` | `descriptorLayouts_`, `PushConstantDesc::descs_` |
| `constexpr`-таблицы + `std::span` | `Mesh::Vertex::getAttributes()` (сейчас `std::vector` аллокацией), `getFormatSize` + `std::unreachable()` |
| `std::to_underlying` / `using enum` | все `enum class` в битовых операциях |
| `std::source_location` + `std::stacktrace` | лог-макрос, кастомное исключение, debug callback |
| deducing this (`this auto&&`) | пары const/non-const аксессоров в `Pipeline`/`Shader` |
| naming cleanup | `Builer`→`Builder`, `detphState_`, `destenationStage`, `faceWidht`, `queuFamilyIndex`, `inverceViewMatrix` (механический коммит) |
| `#embed` (C++26) | .spv в бинарь — когда тулчейн даст (см. §4) |

**Reflection для GPU-структур — отдельный подплан:**

1. Сегодня: `static_assert`-хелпер `DE_ASSERT_LAYOUT(Type, ...)` + общий `.glsl`-include (фаза 1).
2. Сегодня: **debug-проверка через SPIRV-Reflect** при создании пайплайна — сравнить
   `offset/size/members` из SPIR-V с `offsetof` C++-структуры. Это ловит настоящие рассинхроны
   (то, чего reflection сам по себе не умеет — он не видит шейдер).
3. Когда `std::meta` появится в стабильном clang (сейчас это экспериментальная ветка/флаг
   вокруг P2996, не для продакшена): `consteval`-генератор печатает
   `layout(scalar) struct {...}` текст → CMake custom command пишет `.glsl`-заголовок
   (рядом с компиляцией шейдеров). Требование: `layout(scalar)` из фазы 1 уже включён.
4. **`#embed` — доступен тебе уже сейчас** (clang 19+, в C++-режиме это C23-расширение,
   глушится `-Wno-c23-extensions`). Вкомпилировать `.spv` в бинарь и убрать чтение с диска
   плюс `POST_BUILD copy_directory`:
   ```cpp
   // Shaders/EmbeddedShaders.hpp
   inline constexpr unsigned char kSkyboxVertSpv[] = {
   #embed "bin/skybox.vert.spv"
   };
   ```
   Единственная тонкость — порядок сборки: файл с `#embed` должен компилироваться **после**
   `compile_shaders`, иначе `bin/*.spv` ещё нет (лечится `add_dependencies` / генерацией файла
   в `CMAKE_BINARY_DIR`). Плюс: минус целый класс ошибок «шейдер не найден», быстрее старт.

**DoD:** ни одного `void*` в публичных API; ни одного `#include <vector>`-стиля `memcpy` в своих
файлах; варнинги `-Wconversion` = 0.

---

### Фаза 6 — Модули (опционально, см. §4): спайк 1–2 дня, миграция 10–15 дней

### Фаза 7 — Документация (1–2 дня, можно параллельно)

- `README.md`: как собрать (presets, зависимости), как запустить, какие сцены есть.
- `docs/ARCHITECTURE.md`: диаграмма владения
  (`Application → Renderer → IRenderPass* → Systems`, `Scene`, `UploadContext`, `DescriptorArena`,
  `GpuAllocator`), правила «кто чем владеет» и «кто что вызывает».
- `docs/adr/NNNN-*.md` — по одному ADR на крупное решение (dynamic rendering, аллокатор,
  `layout(scalar)`, модули). Шаблон в приложении B.
- `CONTRIBUTING.md`: нейминг, формат коммитов, «не добавляй новый `DescriptorPool`».

---

## 3. Сводная таблица

| Фаза | Что даёт | Оценка | Риск | Статус |
|---|---|---|---|---|
| 0. Фундамент | страховка, пресеты, baseline | 2–3 д | низкий | ✅ закрыта (CI/тесты не заводились, `.clang-format` отвергнут) |
| 1. Контракты | убирает неявные договорённости | 3–5 д | средний | ✅ закрыта, осталось 6 хвостов |
| 2. Кадр | −30% CPU, единицы submit'ов | 3–5 д | средний | → следующая; `UploadContext` в работе, dirty compute вычеркнут, осталась `Scene` |
| 3. Проходы | dynamic rendering, барьеры | 5–8 д | высокий | кэш пайплайнов уже сделан и подключён |
| 4. Ресурсы | аллокатор, отложенное удаление | 5–10 д | средний | — |
| 5. C++/reflection | чистота API, автогенерация layout | 2–3 д | низкий | `layout(scalar)` уже включён |
| 6. Модули | изоляция, скорость сборки | 11–17 д | высокий | отложены (см. §4) |
| 7. Docs | вход в проект за день | 1–2 д | низкий | этот документ — её часть |

Итого без модулей: **~25–35 дней**, с модулями — **~40–50**. Сделано: фазы 0–1
(≈6–8 дней по оценке, больше по факту — два перебора layout'ов и scalar-раскладка).
Осталось: **~14–20 дней** без модулей.

Параллелить можно: 5 и 7 идут параллельно любой фазе; 4 можно начать сразу после 2.

**Порядок, если времени мало (минимальный полезный набор, осталось):**
хвосты фазы 1 (полдня) → 2 (пп. 1–2: `UploadContext`, `Scene`) → 3 (пп. 1–3).
Это ~9–12 дней и закрывает 90% оставшихся болей.

---

## 4. Модули: анализ и вердикт

### 4.1 Что модули дают в теории

- Изоляция: макросы не протекают, `#include`-порядок перестаёт иметь значение, ODR-проблемы ловятся.
- Скорость сборки: заголовок парсится один раз в BMI, а не в каждом TU.
- Явные границы API: `export` — это документированный контракт, а не «что попалось в hpp».
- Лучше масштабируется на команду: нельзя «случайно» залезть во внутренности модуля.

### 4.2 Что мешает именно в этом проекте (по состоянию на 2026)

1. **CMake + clang.** Именованные модули работают с **Ninja** (CMake 3.28+, сканирование через
   `clang-scan-deps`, нужен в `PATH`, `CMAKE_CXX_SCAN_FOR_MODULES ON`). Но:
   - **header units (`import <header.hpp>`) CMake не поддерживает вообще** — для любой
     платформы, не только MSVC;
   - **`import std;` под clang — самая рискованная часть.** CMake-овская поддержка
     (`CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` + GUID, меняющийся между версиями) изначально
     делалась под MSVC; clang умеет собирать стандартный модуль, но **только если сборщик
     сначала соберёт `std`/`std.compat`**. Одиночный TU с `import std;` под clang+CMake
     сегодня не собирается без ручной подготовки BMIs;
   - **выбор STL для clang на Windows — это развилка** (см. 4.2.1), и она влияет на модули
     сильнее, чем версия компилятора.
2. **`vulkan.hpp` — главная проблема.** Он header-only, огромный, макро-конфигурируемый
   (`VK_USE_PLATFORM_WIN32_KHR`, `VULKAN_HPP_NO_EXCEPTIONS`, `VULKAN_HPP_TYPESAFE_CONVERSION`),
   с шаблонным `vk::StructureChain`, который зависит вообще от всех структур.
   Header unit для него CMake не даст, значит в каждом модуле будет:
   ```cpp
   module;                                  // global module fragment
   #include <vulkan/vulkan.hpp>
   export module de.memory;
   ```
   И каждый модуль **всё равно парсит `vulkan.hpp` целиком**. Скоростной выигрыш от модулей
   обнуляется, остаётся только гигиенический. Написать «свой `vk.ixx`» с ручным `export`
   нужных типов нереалистично: Vulkan-Hpp шаблонный, экспортировать придётся чуть не всё.
3. **Третьи стороны.** `imgui` — C-стиль API, макросы (`IMGUI_API`, `IMGUI_DEFINE_MATH_OPERATORS`),
   статические состояния, `.cpp` внутри таргета. `tinyobjloader` — header-only с
   `TINYOBJLOADER_IMPLEMENTATION`. `glm` — шаблонный header-only (можно header unit, но CMake
   не умеет). Всё это остаётся классическими заголовками; модули будут «островом» в середине.
4. **Сборочный скрипт.** `GLOB_RECURSE *.cpp` + `target_sources` не сочетается с модулями:
   нужен `FILE_SET CXX_MODULES`, то есть явные списки (или `file(GLOB)` для file set с
   `CONFIGURE_DEPENDS`). Плюс переход на Ninja ломает привычный F5-дебаг из VS (работает, но
   через CMake-интеграцию, а не .vcxproj).
5. **Макросы в коде.** `MAX_LIGHTS` в `DE_FrameData.hpp`, `#if MODEL`, `#if 1` — модули их не
   экспортируют, значит выпиливать всё равно (это уже в фазе 0/1, что хорошо).
6. **Экосистема.** Библиотек с модулями почти нет; ты будешь первопроходцем в отладке
   toolchain'а, а не движка.
7. **IntelliSense/отладка.** VS 2022 с модулями работает, но с известными глюками (подсветка,
   автодополнение через границы модулей); clangd слабее. Для соло-разработки это ежедневный
   налог.

### 4.2.1 STL: CLANG64 = libc++, и это в нашу пользу

MSYS2 **CLANG64** — это clang + **libc++** + libunwind поверх mingw-w64/UCRT (в отличие от
`MINGW64`, где libstdc++). Для модулей это лучшая из возможных веток:

| | clang++ + MSVC STL (не твой случай) | clang++ + libc++ (CLANG64, твой случай) |
|---|---|---|
| `import std;` | MSVC STL поставляет `std.ixx`, из-под clang собирается нетривиально | libc++ поставляет модульные исходники (`std.cppm`), путь чище |
| заголовки под `export` | MSVC-заголовки плохо переносят глобальный фрагмент | libc++ проектировался с учётом модулей |
| `<print>`, `views::enumerate`, `mdspan` | есть | есть (23.1.4) |

Единственное, что надо **проверить руками**: поставляет ли твой пакет MSYS2 модульные исходники
libc++ (`std.cppm`/`std.compat.cppm` или `-fmodules`-совместимый `module.modulemap`). Если их нет
в пакете — `import std;` не взлетит даже с правильным CMake, и это будет внешнее ограничение,
а не проблема твоего кода. Проверка занимает 5 минут в спайке.

Ещё нюанс CLANG64: `find_package(Vulkan)` найдёт **MSYS2-пакет** (headers + `libvulkan-1.dll.a`),
а LunarG SDK нужен в основном ради `glslc`. Стоит явно зафиксировать в `CMakePresets`/README,
какие заголовки реально используются: если MSYS2-хедеры и LunarG-`glslc` разъезжаются по
`VK_HEADER_VERSION`, это ловится валидацией как «несовпадение struct'ов» в самый неудобный момент.

### 4.3 Вердикт

**Не сейчас. Делать через 2–3 фазы и только после спайка с замерами.**

> **Решение по факту замеров (принято, фаза 6 отложена).** Первый `-ftime-trace` на реальном
> проекте: 41 TU, **326 с** суммарного CPU-времени на компиляцию, при этом время по TU почти
> одинаковое (10.5–14.4 с), включая файлы, вообще не связанные с Vulkan
> (`DE_MovementController.cpp` — 10.48 с). Это признак общего префикса инклюдов, а не отдельных
> тяжёлых файлов. Вывод: приоритет — PCH с `vulkan.hpp` (он работает и с header units, то есть с
> тем, что модули как раз ломают), модули — после фаз 1–4.

Аргументация: у тебя 86 своих файлов и ~6.5k строк. Выигрыш модулей на таком объёме —
это единицы секунд сборки и гигиена, а цена — отказ от header units для `vulkan.hpp`
(то есть главного источника парсинга), явные списки файлов в `FILE_SET`, рискованный
`import std;` под clang и налог на отладку/IntelliSense.
Пока архитектура сама по себе рыхлая (`Application` знает всё, сеты пронумерованы руками),
модули только **усилят** бардак: они делают границы видимыми, но не создают их. Сначала фазы
1–4 (тонкие интерфейсы, `RenderContext`, никаких макросов), потом — модули, уже по готовым швам.

**Что сделать вместо прямо сейчас (30–60 минут работы, 80% эффекта):**

```cmake
# PCH — убивает повторный парсинг vulkan.hpp/glm/imgui
target_precompile_headers(vulkan_tutorial PRIVATE
    <vulkan/vulkan.hpp>
    <memory>
    <vector>
    <string>
    <algorithm>
)
```
плюс:
- разбить таргет на `de_core` (static lib: device/gpu/pipeline/scene) и `vulkan_tutorial` (exe) —
  инкрементальная сборка перестаёт перекомпилировать всё, появляется место для `de_tests`;
- один `de_pch.hpp` вместо 20 инклюдов в каждом файле (clang PCH через `target_precompile_headers`
  работает штатно, никаких хитростей);
- **`-ftime-trace`** (у тебя clang — значит доступен бесплатно): открой трейс в
  [ui.perfetto.dev], и сразу увидишь, сколько из 40 секунд сборки — это `vulkan.hpp`, а сколько
  твои файлы. Это единственный честный способ решить, нужны ли модули/PCH вообще.
  Ожидание по опыту: 50–70% времени сборки — парсинг `vulkan.hpp` в каждом TU, и тогда PCH
  даёт больше, чем модули, за 1% усилий.

Это по духу уже «модульность»: явные зависимости, изоляция третьих сторон, тестируемое ядро.

### 4.4 Если всё-таки да — план миграции

**Спайк (1–2 дня, отдельная ветка, ничего не мержим):**

1. `CMake ≥ 3.28` (лучше свежий), `set(CMAKE_CXX_SCAN_FOR_MODULES ON)`, генератор **Ninja**,
   `clang-scan-deps` в `PATH` (лежит рядом с `clang++.exe`).
2. Один маленький модуль `de.math` (Vec/Matrix/Transform из `glm_config.hpp`).
   Стандартную библиотеку на первом шаге **не** импортируй — `module; #include <vector>` и
   посмотри, что вообще работает. Отдельным шагом потом попробуй `import std;`
   (`CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` + `CMAKE_CXX_MODULE_STD=1`) и, если не взлетит, —
   так и запиши в ADR; от этого план не рушится.
3. Внутри модуля — `module; #include <vulkan/vulkan.hpp>` (проверить, что макросы
   (`VK_USE_PLATFORM_WIN32_KHR`) приходят из `target_compile_definitions`).
4. Замерить: полная сборка, инкрементальная, работает ли IntelliSense, открывается ли
   `vk::StructureChain` из импортирующего модуля.
5. Проверить `export` шаблонов (`Pipeline<Tag>`, `PushConstant<T>`) — они экспортируются, но
   инстанцируются на стороне пользователя, это нормально.

**Порядок миграции (листья → корень), по одному модулю на PR:**

| № | Модуль | Что входит |
|---|---|---|
| 1 | `de.math` | `glm_config`, `Transform`, математические хелперы |
| 2 | `de.memory` | `Buffer`, `Image`, `GpuAllocator`, `RangeAllocator` |
| 3 | `de.gpu` | `Instance`, `Device`, `Window`, `Swapchain`, `CommandBuffers` |
| 4 | `de.pipeline` | `Shader`, `Pipeline`, `PipelineTraits`, `Descriptors` |
| 5 | `de.scene` | `Mesh`, `MeshBaker`, `GameObject`, `Material`, `Scene` |
| 6 | `de.render` | `Renderer`, `IRenderPass`, `Systems/*` |
| 7 | `de.app` | exe, `import de.render;` |

Правила на каждый модуль:

- `module;` + все `#include` третьих сторон → `export module de.X;` + `import std;` + `export { ... }`;
- внутренние детали — в `module :private;` или партиции `module de.X:detail;`;
- **не** экспортировать `vk::`-типы наружу там, где можно спрятать (иначе Vulkan протекает в API —
  хотя в движке это нормально, но лучше решать осознанно);
- в `.cpp` — `module de.X;` вместо `#include` своих заголовков;
- third-party (`imgui`, `tinyobjloader`, `glfw`, `glm`) — отдельные static-lib таргеты, подключаются
  как обычные заголовки.

**Go/no-go критерии (после спайка и после 2–3 модулей):**

- сборка стала быстрее ≥ 30% (иначе PCH решает то же дешевле);
- Ninja-конфиг стабилен (нет «то соберётся, то нет»);
- отладка в VS/RenderDoc не деградировала;
- нет блокеров типа «шаблон из модуля не инстанцируется».

Если хоть один пункт не выполняется — откат. Модули и заголовки спокойно сосуществуют, поэтому
миграция по одному модулю обратима и не требует «большого взрыва».

---

## Приложение A — CMake-сниппеты

**Presets (`CMakePresets.json`), clang++:**
```json
{ "version": 6,
  "configurePresets": [
    { "name": "clang-debug", "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-vscode-clang64",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug",
                          "CMAKE_CXX_COMPILER": "clang++",
                          "CMAKE_C_COMPILER": "clang",
                          "CMAKE_EXPORT_COMPILE_COMMANDS": "ON" } },
    { "name": "clang-release", "inherits": "clang-debug",
      "binaryDir": "${sourceDir}/build-vscode-release",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } }
  ] }
```
`binaryDir` намеренно указывает на **существующие** каталоги сборки: третий каталог — это повторная
загрузка FetchContent, отдельный кэш и два несравнимых числа времени сборки.

Если в существующем каталоге другой генератор или компилятор, `cmake --preset` скажет об этом прямо
(`generator does not match`). Посмотреть фактическое значение:

```powershell
Select-String -Path build-vscode-clang64\CMakeCache.txt -Pattern "^CMAKE_GENERATOR:|^CMAKE_CXX_COMPILER:"
```

и подставить его в пресет — не пересоздавая каталог. Пересоздавать имеет смысл только если кэш
битый или собран другим компилятором.

**Совместимость с `.vscode/` (проверено по `tasks.json`, `launch.json`, `c_cpp_properties.json`):**

- У тебя `.vscode` целиком в `.gitignore` — это осознанный выбор, но помни: `tasks.json`/`launch.json`
  тогда живут только на этой машине. А вот `CMakePresets.json` — наоборот, **обязан** быть в
  репозитории: его читает CLI и CI; `.gitignore` его не покрывает, но и `git commit -am` его не
  добавит — файл новый, значит только `git add`.

- `CMakePresets.json` читают только CLI (`cmake --preset`) и расширение **CMake Tools**.
  `tasks.json` (у тебя — `cmake --build ... --target vulkan_tutorial`), `launch.json` (LLDB) и
  `c_cpp_properties.json` (IntelliSense) пресеты **не читают вообще** — конфликта «файл против
  файла» нет.
- Реальный риск другой: если расширение CMake Tools включено, оно конфигурирует **тот же**
  `build-vscode-clang64` параллельно с задачей сборки. Симптом — тот самый
  `ninja: failed recompaction: Permission denied`. Лечится либо отказом от CMake Tools
  (`"cmake.configureOnOpen": false` в `.vscode/settings.json`), либо единым источником истины:
  в задаче вызывать `cmake --build --preset clang-debug`.
- **Обязательно перенести в пресет `"CMAKE_EXPORT_COMPILE_COMMANDS": "ON"`**: в `CMakeLists.txt`
  этой опции нет, значит она живёт только в кэше `build-vscode-clang64`. Пропадёт кэш — пропадёт
  `compile_commands.json`, а с ним молча сломается IntelliSense в `c_cpp_properties.json`
  (там конфигурация `Clang64-Debug` ссылается именно на этот файл).
- В `tasks.json` стоит `--parallel 2`. Для замера «полная сборка» это вдвое медленнее реального:
  либо меряй без `--parallel`, либо зафиксируй в `baseline.md` число потоков рядом с секундами.
- Единый источник истины достигается так: в `CMakePresets.json` добавляются `buildPresets`
  (без них `cmake --build --preset` не работает — это разные массивы), а задача сводится к
  `cmake --build --preset clang-debug`:

  ```json
  "buildPresets": [
    { "name": "clang-debug", "configurePreset": "clang-debug", "targets": ["vulkan_tutorial"] }
  ]
  ```

  Тогда каталог сборки упомянут **ровно в одном месте** — в `binaryDir` пресета. Сейчас он
  продублирован в `tasks.json` (`--build ${workspaceFolder}/build-vscode-clang64`), и при
  переименовании/добавлении конфигурации эти два файла разъезжаются молча.

- `c_cpp_properties.json`, конфигурация `"Win32"` (`compilerPath: cl.exe`, `windowsSdkVersion`) —
  мёртвая: IntelliSense берёт данные из `compile_commands.json` конфигурации `Clang64-Debug`.
  Её можно удалить, чтобы не путаться, но это не срочно.
Убедись, что `clang++`, `clang`, `clang-scan-deps` — из **одного** LLVM, иначе модули
будут падать на несовпадении версий BMI.

**Разделение таргетов (фаза 0):**
```cmake
add_library(de_core STATIC ${CORE_SOURCES})
target_include_directories(de_core PUBLIC src)
target_link_libraries(de_core PUBLIC glm::glm Vulkan::Vulkan glfw)

add_executable(vulkan_tutorial ${APP_SOURCES})
target_link_libraries(vulkan_tutorial PRIVATE de_core tinyobjloader)

add_executable(de_tests tests/main.cpp tests/test_range_allocator.cpp ...)
target_link_libraries(de_tests PRIVATE de_core doctest::doctest)
```

**Формат кода: `clang-format` не используем** (решение принято — стиль проекта устраивает,
автоформат добавил бы только churn). Следствия, которые надо помнить:
- пункт про `clang-format --dry-run -Werror` в CI вычёркивается;
- `#include"DE_X.hpp"` (без пробела — так во всех файлах) остаётся как есть, руками не чиним;
- выравнивание и переносы — на усмотрение автора, ревью на них не смотрит.

**`.editorconfig` (в репозитории все файлы LF, `.gitattributes` нет):**
```ini
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true
indent_style = space
indent_size = 4

[*.{cpp,hpp,h}]
indent_style = tab
indent_size = 4
tab_width = 4

[CMakeLists.txt]
indent_style = space
indent_size = 4

[*.{json,yml,yaml}]
indent_size = 2

[*.glsl]
indent_size = 4

[*.md]
trim_trailing_whitespace = false
```

`.editorconfig` не переформатирует существующий код — он действует только в момент правки: с этого
момента редактор ставит табы в C++, LF вместо CRLF и убирает пробелы в конце строк. То есть файлы,
которые ты и так трогаешь, потихоньку приходят в порядок, а диффы перестают содержать случайный
мусор. Единственное, что стоит править осознанно: `.glsl` и шейдеры — там `trim_trailing_whitespace`
сработает и на них (это безопасно, GLSL пробелы в конце строк не значит).

File sets для модулей (фаза 6):
```cmake
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
target_sources(de_core PUBLIC FILE_SET CXX_MODULES FILES
    src/math/de.math.ixx
    src/memory/de.memory.ixx)
```

## Приложение B — шаблон ADR

```markdown
# ADR NNNN: <решение>

Дата: <дата>
Статус: предложено | принято | заменено ADR-XXXX

## Контекст
Что болит, какие ограничения (toolchain, время, железо).

## Варианты
1. ... (плюсы/минусы, оценка)
2. ...

## Решение
Что выбрали и почему именно сейчас.

## Последствия
Что станет проще, что сложнее, как откатить, какие метрики проверим.
```

## Приложение C — чек-лист PR

- [ ] собирается в `clang-debug` и `clang-release` без новых варнингов
- [ ] запускается, сцена рендерится как до изменений (скриншот в описании)
- [ ] валидация Vulkan молчит (в Debug)
- [ ] нет новых `#include` своих заголовков в `.cpp`, если есть модуль (после фазы 6)
- [ ] нет нового `DescriptorPool`/`vk::DeviceMemory` в обход аллокатора/арены
- [ ] если тронут GPU-layout — обновлены `.glsl` и `static_assert`
- [ ] метрика из `docs/baseline.md` не ухудшилась
