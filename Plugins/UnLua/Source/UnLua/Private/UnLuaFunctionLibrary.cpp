// 这段代码是 UnLua 框架的蓝图函数库部分，提供了三个关键功能来支持 Lua 脚本的动态管理和开发工作流。以下是各函数的详细说明：

// ---

// ### **1. 获取脚本根路径 - `GetScriptRootPath()`**
// ```cpp
// FString UUnLuaFunctionLibrary::GetScriptRootPath() {
//     return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Script/"));
// }
// ```
// - **功能**  
//   返回项目内容目录下 `Script` 文件夹的绝对路径，如 `D:/Project/Content/Script/`。

// - **技术细节**  
//   - `FPaths::ProjectContentDir()` 获取引擎标准内容目录（如 `Content/`）
//   - 路径拼接确保跨平台兼容性（Windows/Mac/Linux）
//   - 转换绝对路径避免相对路径歧义

// - **典型应用场景**  
//   ```lua
//   -- Lua 中加载位于 Script/ 目录下的模块
//   local MyModule = require "MyModule"
//   ```

// ---

// ### **2. 获取文件修改时间戳 - `GetFileLastModifiedTimestamp()`**
// ```cpp
// int64 UUnLuaFunctionLibrary::GetFileLastModifiedTimestamp(FString Path) {
//     const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*Path);
//     return FileTime.GetTicks();
// }
// ```
// - **功能**  
//   返回指定文件最后修改时间的 Ticks（1 Tick = 100 纳秒）

// - **技术细节**  
//   - 使用 UE 的 `IFileManager` 接口实现跨平台文件操作
//   - `Ticks` 提供高精度时间比对（相比秒级时间戳）

// - **热重载中的使用**  
//   框架内部通过比对时间戳变化决定是否重新加载脚本：
//   ```cpp
//   if (NewTimestamp > CachedTimestamp) ReloadScript();
//   ```

// ---

// ### **3. 触发热重载 - `HotReload()`**
// ```cpp
// void UUnLuaFunctionLibrary::HotReload() {
//     IUnLuaModule::Get().HotReload();
// }
// ```
// - **功能**  
//   强制重新加载所有已绑定的 Lua 脚本，实时更新游戏逻辑。

// - **实现机制**  
//   - 遍历所有注册的 Lua 环境
//   - 清除旧脚本缓存 (`package.loaded`)
//   - 重新执行 `require` 加载最新代码

// - **开发流程集成**  
//   可在编辑器中绑定快捷键或按钮调用此函数：
//   ```cpp
//   // 蓝图节点示例
//   UUnLuaFunctionLibrary::HotReload();
//   ```

// ---

// ### **代码设计亮点**

// 1. **路径处理标准化**  
//   使用 `FPaths` 工具类确保路径格式统一，避免手动拼接错误。

// 2. **跨平台兼容性**  
//   `IFileManager` 和 `FPaths` 抽象了操作系统差异，确保功能在 Windows/Mac/Linux 一致。

// 3. **高性能时间比对**  
//   使用 `Ticks` 而非秒级时间戳，避免短时间内多次修改无法检测。

// 4. **模块化设计**  
//   通过 `IUnLuaModule` 接口解耦，框架核心逻辑与蓝图工具分离。

// ---

// ### **潜在改进方向**

// 1. **异常处理增强**  
//   ```cpp
//   // 示例：处理无效路径
//   if (!FPaths::FileExists(Path)) return -1;
//   ```

// 2. **异步热重载支持**  
//   在游戏运行时后台检测文件变化，自动触发重载。

// 3. **路径可配置化**  
//   通过项目设置允许自定义脚本目录，而非硬编码 `Script/`。

// ---

// ### **与其他模块的协作**

// 1. **UnLua 核心模块**  
//   `IUnLuaModule::HotReload()` 调用实际执行 Lua 虚拟机重置和脚本重载。

// 2. **蓝图系统**  
//   暴露为蓝图可调用函数，方便非程序员参与工作流。

// 3. **Lua 调试工具**  
//   热重载后自动重新连接调试器，保持开发状态连续性。

// ---

// 通过这三个函数，UnLua 实现了 Lua 脚本开发-测试-迭代的快速闭环，极大提升了游戏逻辑开发的效率。


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

#include "UnLua.h"
#include "UnLuaFunctionLibrary.h"
#include "UnLuaDelegates.h"
#include "UnLuaModule.h"

FString UUnLuaFunctionLibrary::GetScriptRootPath()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + TEXT("Script/"));
}

int64 UUnLuaFunctionLibrary::GetFileLastModifiedTimestamp(FString Path)
{
    const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*Path);
    return FileTime.GetTicks();
}

void UUnLuaFunctionLibrary::HotReload()
{
    IUnLuaModule::Get().HotReload();
}
