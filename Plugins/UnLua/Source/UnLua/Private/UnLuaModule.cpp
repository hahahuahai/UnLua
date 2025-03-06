// 这段代码是 UnLua 模块的核心实现，负责管理 Lua 环境的生命周期、对象绑定以及与引擎的深度集成。以下是代码的详细解释：

// ---

// ### **1. 模块生命周期管理**
// - **`StartupModule`**:
//   - **编辑器支持**: 加载 `UnLuaEditor` 模块（仅在编辑器模式下）。
//   - **注册配置**: 调用 `RegisterSettings` 注册项目设置，允许在 UE 编辑器中配置 UnLua。
//   - **控制台命令**: 创建 `FUnLuaConsoleCommands`，提供调试用的控制台命令（如 `Lua`、`UnLua` 等）。
//   - **地图加载回调**: 绑定 `PostLoadMapWithWorld`，在加载新地图时触发 Lua 环境的初始化。
//   - **默认参数集合**: 调用 `CreateDefaultParamCollection`，用于处理 Lua 函数的默认参数。
//   - **自动启动**: 根据 `AUTO_UNLUA_STARTUP` 宏决定是否自动激活 UnLua。在编辑器 Play-in-Editor (PIE) 模式下，监听 PIE 事件以动态管理 Lua 环境。

// - **`ShutdownModule`**:
//   - 卸载配置、关闭 Lua 环境，并清理所有资源。

// ---

// ### **2. Lua 环境激活与关闭**
// - **`SetActive`**:
//   - **激活时**:
//     - 监听系统错误（`OnSystemError`）和 UObject 的创建/删除事件。
//     - 创建 `EnvLocator`（Lua 环境定位器），用于管理多个 Lua 环境。
//     - 根据配置预绑定特定类（`PreBindClasses`），提升运行时性能。
//   - **关闭时**:
//     - 移除事件监听，释放 `EnvLocator`，并恢复所有被覆盖的 C++ 函数。

// ---

// ### **3. UObject 生命周期监听**
// - **`NotifyUObjectCreated`**:
//   - 当新 UObject 创建时，通过 `EnvLocator` 找到对应的 Lua 环境，并尝试绑定该对象到 Lua。
//   - 替换对象的输入处理逻辑（如 `UPlayerInput`），允许 Lua 接管输入事件。
// - **`NotifyUObjectDeleted`**:
//   - 记录对象删除事件（暂未实现具体逻辑）。

// ---

// ### **4. 错误处理与调试**
// - **`OnSystemError`**:
//   - 在系统崩溃或 `Ensure` 触发时，打印所有 Lua 环境的调用堆栈。
//   - 通过 `bPrintLuaStackOnSystemError` 配置控制是否启用此功能。
// - **控制台命令**:
//   - 提供类似 `Lua.DoString "print('Hello')"` 的命令，方便实时执行 Lua 代码。

// ---

// ### **5. 编辑器集成**
// - **PIE 事件处理**:
//   - `OnPreBeginPIE`: 启动 PIE 时激活 UnLua。
//   - `OnPostPIEStarted`: 在 PIE 世界中触发地图加载回调。
//   - `OnEndPIE`: 结束 PIE 时关闭 UnLua。
// - **配置管理**:
//   - 在 UE 编辑器的 **Project Settings -> Plugins -> UnLua** 中暴露配置项（如死循环检测、悬空引用检查）。

// ---

// ### **6. 关键组件与设计**
// - **`EnvLocator`**:
//   - 负责定位不同 UObject 对应的 Lua 环境（如不同场景、子系统）。
//   - 支持动态创建和管理多个 Lua 环境，避免全局状态冲突。
// - **预绑定类（`PreBindClasses`）**:
//   - 在模块启动时提前绑定配置的类，减少运行时开销。
//   - 通过遍历所有 `UClass`，匹配 `PreBindClasses` 列表中的类路径。
// - **热重载（`HotReload`）**:
//   - 调用 `EnvLocator->HotReload()` 实现 Lua 代码的热更新，无需重启游戏。

// ---

// ### **7. 代码示例场景**
// - **绑定自定义 Actor 到 Lua**:
//   ```cpp
//   // C++ 类
//   class AMyActor : public AActor, public IUnLuaInterface {
//       // 实现 IUnLuaInterface
//       virtual FString GetModuleName() override { return "MyActor"; }
//   };

//   // Lua 代码
//   local MyActor = UE.UClass.Load("/Game/MyActor.MyActor_C")
//   local actor = World:SpawnActor(MyActor)
//   actor:Initialize() -- Lua 实现的初始化逻辑
//   ```
//   - UnLua 会自动绑定 `AMyActor` 到 Lua，并调用 `GetModuleName` 获取对应的 Lua 模块。

// - **处理输入事件**:
//   ```lua
//   -- Lua 模块 "InputHandler"
//   function OnKeyPressed(Key)
//       print("Key pressed:", Key)
//   end
//   ```
//   - UnLua 替换 `UPlayerInput` 的输入处理，将事件转发到 Lua。

// ---

// ### **8. 配置参数解析**
// - **`UUnLuaSettings`**:
//   - **`bPrintLuaStackOnSystemError`**: 系统错误时是否打印 Lua 堆栈。
//   - **`DeadLoopCheck`**: 检测 Lua 死循环的超时时间（毫秒）。
//   - **`DanglingCheck`**: 是否启用悬空引用检查（防止 Lua 引用已销毁的 UObject）。
//   - **`EnvLocatorClass`**: 自定义 Lua 环境定位器类（支持扩展）。

// ---

// ### **总结**
// 这段代码是 UnLua 框架的核心模块，实现了以下功能：
// 1. **模块生命周期管理**: 启动/关闭时的资源分配与清理。
// 2. **Lua 环境动态管理**: 通过 `EnvLocator` 支持多环境、热重载。
// 3. **UObject 绑定**: 监听对象创建，自动绑定到 Lua。
// 4. **错误处理**: 系统级错误捕获与 Lua 堆栈打印。
// 5. **编辑器集成**: PIE 模式支持、可视化配置。
// 6. **性能优化**: 预绑定类、死循环检测。

// 通过这些机制，UnLua 实现了高效、灵活的 C++/Lua 互操作，深度融入 UE 引擎的工作流。


// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleManager.h"
#endif

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#include "UnLuaConsoleCommands.h"
#endif

#include "Engine/World.h"
#include "UnLuaModule.h"
#include "DefaultParamCollection.h"
#include "GameDelegates.h"
#include "LuaEnvLocator.h"
#include "LuaOverrides.h"
#include "UnLuaDebugBase.h"
#include "UnLuaInterface.h"
#include "UnLuaSettings.h"
#include "GameFramework/PlayerController.h"
#include "Registries/ClassRegistry.h"
#include "Registries/EnumRegistry.h"

#define LOCTEXT_NAMESPACE "FUnLuaModule"

namespace UnLua
{
    class FUnLuaModule : public IUnLuaModule,
                         public FUObjectArray::FUObjectCreateListener,
                         public FUObjectArray::FUObjectDeleteListener
    {
    public:
        virtual void StartupModule() override
        {
#if WITH_EDITOR
            FModuleManager::Get().LoadModule(TEXT("UnLuaEditor"));
#endif

            RegisterSettings();

#if ALLOW_CONSOLE
            ConsoleCommands = MakeUnique<FUnLuaConsoleCommands>(this);
#endif

            FCoreUObjectDelegates::PostLoadMapWithWorld.AddRaw(this, &FUnLuaModule::PostLoadMapWithWorld);

            CreateDefaultParamCollection();

#if AUTO_UNLUA_STARTUP
#if WITH_EDITOR
            if (!IsRunningGame())
            {
                FEditorDelegates::PreBeginPIE.AddRaw(this, &FUnLuaModule::OnPreBeginPIE);
                FEditorDelegates::PostPIEStarted.AddRaw(this, &FUnLuaModule::OnPostPIEStarted);
                FEditorDelegates::EndPIE.AddRaw(this, &FUnLuaModule::OnEndPIE);
                FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FUnLuaModule::OnEndPlayMap);
            }

            if (IsRunningGame() || IsRunningDedicatedServer())
#endif
                SetActive(true);
#endif
        }

        virtual void ShutdownModule() override
        {
            UnregisterSettings();
            SetActive(false);
        }

        virtual bool IsActive() override
        {
            return bIsActive;
        }

        virtual void SetActive(const bool bActive) override
        {
            if (bIsActive == bActive)
                return;

            if (bActive)
            {
                OnHandleSystemErrorHandle = FCoreDelegates::OnHandleSystemError.AddRaw(this, &FUnLuaModule::OnSystemError);
                OnHandleSystemEnsureHandle = FCoreDelegates::OnHandleSystemEnsure.AddRaw(this, &FUnLuaModule::OnSystemError);
                GUObjectArray.AddUObjectCreateListener(this);
                GUObjectArray.AddUObjectDeleteListener(this);

                const auto& Settings = *GetMutableDefault<UUnLuaSettings>();
                const auto EnvLocatorClass = *Settings.EnvLocatorClass == nullptr ? ULuaEnvLocator::StaticClass() : *Settings.EnvLocatorClass;
                EnvLocator = NewObject<ULuaEnvLocator>(GetTransientPackage(), EnvLocatorClass);
                EnvLocator->AddToRoot();
                FDeadLoopCheck::Timeout = Settings.DeadLoopCheck;
                FDanglingCheck::Enabled = Settings.DanglingCheck;

                for (const auto Class : TObjectRange<UClass>())
                {
                    for (const auto& ClassPath : Settings.PreBindClasses)
                    {
                        if (!ClassPath.IsValid())
                            continue;

                        const auto TargetClass = ClassPath.ResolveClass();
                        if (!TargetClass)
                            continue;

                        if (Class->IsChildOf(TargetClass))
                        {
                            const auto Env = EnvLocator->Locate(Class);
                            Env->TryBind(Class);
                            break;
                        }
                    }
                }
            }
            else
            {
                FCoreDelegates::OnHandleSystemError.Remove(OnHandleSystemErrorHandle);
                FCoreDelegates::OnHandleSystemEnsure.Remove(OnHandleSystemEnsureHandle);
                GUObjectArray.RemoveUObjectCreateListener(this);
                GUObjectArray.RemoveUObjectDeleteListener(this);
                EnvLocator->Reset();
                EnvLocator->RemoveFromRoot();
                EnvLocator = nullptr;
                FLuaOverrides::Get().RestoreAll();
            }

            bIsActive = bActive;
        }

        virtual FLuaEnv* GetEnv(UObject* Object) override
        {
            if (!bIsActive)
                return nullptr;
            return EnvLocator->Locate(Object);
        }

        virtual void HotReload() override
        {
            if (!bIsActive)
                return;
            EnvLocator->HotReload();
        }

    private:
        virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override
        {
            // UE_LOG(LogTemp, Log, TEXT("NotifyUObjectCreated : %p"), ObjectBase);
            if (!bIsActive)
                return;

            UObject* Object = (UObject*)ObjectBase;

            const auto Env = EnvLocator->Locate(Object);
            // UE_LOG(LogTemp, Log, TEXT("Locate %s for %s"), *Env->GetName(), *ObjectBase->GetFName().ToString());
            Env->TryBind(Object);
            Env->TryReplaceInputs(Object);
        }

        virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override
        {
            // UE_LOG(LogTemp, Log, TEXT("NotifyUObjectDeleted : %p"), Object);
        }

        virtual void OnUObjectArrayShutdown() override
        {
            if (!bIsActive)
                return;

            GUObjectArray.RemoveUObjectCreateListener(this);
            GUObjectArray.RemoveUObjectDeleteListener(this);

            bIsActive = false;
        }

        void OnSystemError() const
        {
            if (!bPrintLuaStackOnSystemError)
                return;

            if (!IsInGameThread())
                return;

            for (auto& Pair : FLuaEnv::GetAll())
            {
                if (!Pair.Key || !Pair.Value)
                    continue;

                UE_LOG(LogUnLua, Log, TEXT("%s:"), *Pair.Value->GetName())
                PrintCallStack(Pair.Key);
                UE_LOG(LogUnLua, Log, TEXT(""))
            }

            if (GLog)
                GLog->Flush();
        }

#if WITH_EDITOR

        void OnPreBeginPIE(bool bIsSimulating)
        {
            SetActive(true);
        }

        void OnPostPIEStarted(bool bIsSimulating)
        {
            UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
            if (EditorEngine)
                PostLoadMapWithWorld(EditorEngine->PlayWorld);
        }

        void OnEndPIE(bool bIsSimulating)
        {
            // SetActive(false);
        }

        void OnEndPlayMap()
        {
            SetActive(false);
        }

#endif

        void RegisterSettings()
        {
#if WITH_EDITOR
            ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
            if (!SettingsModule)
                return;

            const auto Section = SettingsModule->RegisterSettings("Project", "Plugins", "UnLua",
                                                                  LOCTEXT("UnLuaEditorSettingsName", "UnLua"),
                                                                  LOCTEXT("UnLuaEditorSettingsDescription", "UnLua Runtime Settings"),
                                                                  GetMutableDefault<UUnLuaSettings>());
            Section->OnModified().BindRaw(this, &FUnLuaModule::OnSettingsModified);
#endif

#if ENGINE_MAJOR_VERSION >=5 && !WITH_EDITOR
            // UE5下打包后没有从{PROJECT}/Config/DefaultUnLua.ini加载，这里强制刷新一下
            FString UnLuaIni = TEXT("UnLua");
            GConfig->LoadGlobalIniFile(UnLuaIni, *UnLuaIni, nullptr, true);
            UUnLuaSettings::StaticClass()->GetDefaultObject()->ReloadConfig();
#endif

            auto& Settings = *GetDefault<UUnLuaSettings>();
            bPrintLuaStackOnSystemError = Settings.bPrintLuaStackOnSystemError;
        }

        void UnregisterSettings()
        {
#if WITH_EDITOR
            ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
            if (SettingsModule)
                SettingsModule->UnregisterSettings("Project", "Plugins", "UnLua");
#endif
        }

        bool OnSettingsModified()
        {
            auto& Settings = *GetDefault<UUnLuaSettings>();
            bPrintLuaStackOnSystemError = Settings.bPrintLuaStackOnSystemError;
            return true;
        }

        void PostLoadMapWithWorld(UWorld* World) const
        {
            if (!World || !bIsActive)
                return;

            const auto Env = EnvLocator->Locate(World);
            if (!Env)
                return;

            const auto Manager = Env->GetManager();
            if (!Manager)
                return;

            Manager->OnMapLoaded(World);
        }

        bool bIsActive = false;
        bool bPrintLuaStackOnSystemError = false;
        ULuaEnvLocator* EnvLocator = nullptr;
        FDelegateHandle OnHandleSystemErrorHandle;
        FDelegateHandle OnHandleSystemEnsureHandle;
#if ALLOW_CONSOLE
        TUniquePtr<FUnLuaConsoleCommands> ConsoleCommands;
#endif
    };
}

IMPLEMENT_MODULE(UnLua::FUnLuaModule, UnLua)

#undef LOCTEXT_NAMESPACE
