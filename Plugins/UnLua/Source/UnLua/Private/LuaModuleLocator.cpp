// **代码解释与分析：**

// 这两段代码属于 UnLua 框架中的模块定位器，用于确定与特定 Unreal Engine 对象关联的 Lua 模块名称。以下是核心逻辑的详细说明：

// ---

// ### **1. `ULuaModuleLocator` 类**
// **功能**：通过 `UnLuaInterface` 接口获取 Lua 模块名。

// **逻辑流程**：
// 1. **确定目标对象**：
//    - 如果传入对象是 **类默认对象 (CDO)** 或原型对象，直接使用它。
//    - 否则，获取对象的类并提取其 CDO。
   
// 2. **有效性检查**：
//    - 若 CDO 未初始化，返回空字符串。
//    - 检查 CDO 是否实现 `UnLuaInterface` 接口，未实现则返回空。

// 3. **调用接口**：
//    - 通过 `IUnLuaInterface::GetModuleName` 获取模块名，允许开发者自定义模块映射。

// **示例**：
// - 若 `MyActor` 类实现了 `UnLuaInterface` 并返回 `"MyGame.MyActor"`，则 Lua 模块路径为 `MyGame/MyActor.lua`。

// ---

// ### **2. `ULuaModuleLocator_ByPackage` 类**
// **功能**：根据对象所属包的路径自动生成模块名。

// **逻辑流程**：
// 1. **获取类信息**：
//    - 确定对象对应的类，若为原生类（C++ 类），直接使用类名。
   
// 2. **包路径处理**：
//    - 获取对象的最外层包名，将路径分隔符 `/` 替换为 `.`。
//    - **路径截断**：查找第一个 `/` 的位置，截断前面的根目录（如 `/Game`），保留后续路径。

// 3. **缓存机制**：
//    - 使用类名作为键缓存生成的模块名，避免重复计算。

// **路径转换示例**：
// - **输入路径**：`/Game/Characters/Hero`
// - **替换分隔符** → `".Game.Characters.Hero"`
// - **截断处理**（假设第一个 `/` 在位置 0）→ `"Game.Characters.Hero"`

// **潜在问题**：
// - **路径处理错误**：代码中在替换分隔符后尝试查找 `/`，导致 `Find` 失败，应直接在原始路径处理。
// - **修正逻辑**：应在替换前处理路径，例如：
//   ```cpp
//   FString OriginalPath = Object->GetOutermost()->GetName();
//   int32 FirstSlashPos = OriginalPath.Find(TEXT("/"));
//   if (FirstSlashPos != INDEX_NONE)
//       OriginalPath = OriginalPath.RightChop(FirstSlashPos + 1);
//   FString ModuleName = OriginalPath.Replace(TEXT("/"), TEXT("."));
//   ```

// ---

// ### **对比与用途**
// - **`ULuaModuleLocator`**：
//   - **优点**：灵活，允许开发者通过接口自定义模块名。
//   - **适用场景**：需特定模块映射或动态生成名称的情况。
  
// - **`ULuaModuleLocator_ByPackage`**：
//   - **优点**：自动化，基于项目结构生成模块名，减少配置。
//   - **适用场景**：项目结构清晰，希望模块与 UE 目录结构一致时。

// ---

// ### **关键代码修复建议**
// **`ULuaModuleLocator_ByPackage` 路径处理修正**：
// ```cpp
// FString ModuleName = Object->GetOutermost()->GetName();
// // 处理根目录截断
// int32 FirstSlashPos = ModuleName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1);
// if (FirstSlashPos != INDEX_NONE)
// {
//     ModuleName = ModuleName.RightChop(FirstSlashPos + 1);
// }
// ModuleName = ModuleName.Replace(TEXT("/"), TEXT("."));
// ```
// **说明**：
// - 先处理原始路径，截断根目录（如 `/Game`），再替换分隔符，确保生成正确的模块名。

// ---

// ### **总结**
// 这两个类提供了不同策略的模块定位机制，`ULuaModuleLocator` 依赖接口实现灵活映射，而 `ULuaModuleLocator_ByPackage` 通过包路径自动生成名称。后者存在路径处理逻辑缺陷，需修正以确保正确性。开发者可根据项目需求选择合适的定位器，或基于此实现自定义逻辑。


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

#include "LuaModuleLocator.h"
#include "UnLuaInterface.h"

FString ULuaModuleLocator::Locate(const UObject* Object)
{
    const UObject* CDO;
    if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
    {
        CDO = Object;
    }
    else
    {
        const auto Class = Cast<UClass>(Object);
        CDO = Class ? Class->GetDefaultObject() : Object->GetClass()->GetDefaultObject();
    }

    if (CDO->HasAnyFlags(RF_NeedInitialization))
    {
        // CDO还没有初始化完成
        return "";
    }

    if (!CDO->GetClass()->ImplementsInterface(UUnLuaInterface::StaticClass()))
    {
        return "";
    }

    return IUnLuaInterface::Execute_GetModuleName(CDO);
}

FString ULuaModuleLocator_ByPackage::Locate(const UObject* Object)
{
    const auto Class = Object->IsA<UClass>() ? static_cast<const UClass*>(Object) : Object->GetClass();
    const auto Key = Class->GetFName();
    const auto Cached = Cache.Find(Key);
    if (Cached)
        return *Cached;

    FString ModuleName;
    if (Class->IsNative())
    {
        ModuleName = Class->GetName();
    }
    else
    {
        ModuleName = Object->GetOutermost()->GetName();
        const auto ChopCount = ModuleName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 1) + 1;
        ModuleName = ModuleName.Replace(TEXT("/"), TEXT(".")).RightChop(ChopCount);
    }
    Cache.Add(Key, ModuleName);
    return ModuleName;
}
