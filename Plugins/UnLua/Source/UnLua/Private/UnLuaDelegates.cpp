// 这段代码是UnLua框架的委托系统核心，定义了框架的关键事件回调接口。以下是各委托的作用及使用场景解析：

// ### 核心委托列表
// ```cpp
// FOnLuaStateCreated OnLuaStateCreated;            // Lua虚拟机创建完成
// FOnLuaContextInitialized OnLuaContextInitialized; // Lua上下文初始化完成
// FOnLuaContextCleanup OnPreLuaContextCleanup;     // Lua上下文清理前
// FOnLuaContextCleanup OnPostLuaContextCleanup;    // Lua上下文清理后
// FOnPreStaticallyExport OnPreStaticallyExport;    // 静态导出前预处理
// FOnObjectBinded OnObjectBinded;                  // 对象绑定完成
// FOnObjectUnbinded OnObjectUnbinded;              // 对象解绑完成
// FGenericLuaDelegate HotfixLua;                   // Lua热修复入口
// FGenericLuaDelegate ReportLuaCallError;          // Lua调用错误报告
// FGenericLuaDelegate ConfigureLuaGC;             // GC配置入口
// FCustomLuaFileLoader CustomLoadLuaFile;          // 自定义Lua加载器
// ```

// ### 典型使用场景
// #### 1. 环境初始化监控
// ```cpp
// // 监控Lua虚拟机创建
// FUnLuaDelegates::OnLuaStateCreated.AddLambda([](lua_State* L){
//     UE_LOG(LogTemp, Log, TEXT("Lua VM Created!"));
// });

// // 上下文初始化后加载基础模块
// FUnLuaDelegates::OnLuaContextInitialized.AddLambda([](){
//     GetLuaEnv()->DoString("require 'Core.Base'"); 
// });
// ```

// #### 2. 资源热重载
// ```cpp
// // 实现Lua文件动态加载
// FUnLuaDelegates::CustomLoadLuaFile.BindLambda(
//     [](const FString& FilePath, TArray<uint8>& Data, FString& ChunkName){
//         if (DownloadFromCDN(FilePath, Data)) {
//             ChunkName = FPaths::GetBaseFilename(FilePath);
//             return true;
//         }
//         return false;
//     });
// ```

// #### 3. 对象生命周期追踪
// ```cpp
// // 记录所有绑定对象
// FUnLuaDelegates::OnObjectBinded.AddStatic(
//     [](UObject* Obj, const FString& ModuleName){
//         GObjectTracker.Add(Obj, ModuleName); 
//     });
// ```

// ### 委托触发时机
// | 委托名称                | 触发阶段                          | 典型操作                     |
// |-------------------------|-----------------------------------|-----------------------------|
// | OnLuaStateCreated       | 完成lua_newstate后               | 注入基础库/修改GC参数       |
// | OnLuaContextInitialized | 注册完所有内置类型后              | 加载游戏逻辑模块            |
// | OnPreLuaContextCleanup  | lua_close调用前                   | 保存游戏状态                |
// | OnPostLuaContextCleanup | lua_close调用后                   | 释放自定义资源              |
// | OnPreStaticallyExport   | 静态导出C++类型前                 | 添加自定义导出类型          |
// | OnObjectBinded          | 成功绑定UObject到Lua后            | 对象缓存/初始化逻辑         |
// | HotfixLua               | 执行HotReload命令时               | 热更逻辑替换                |

// ### 重要参数说明
// #### FGenericLuaDelegate
// ```cpp
// DECLARE_DELEGATE_RetVal_OneParam(bool, FGenericLuaDelegate, lua_State*);
// ```
// - **输入**：当前Lua虚拟机指针
// - **返回**：bool表示是否已处理
// - **典型应用**：
//   ```cpp
//   // 自定义错误处理
//   FUnLuaDelegates::ReportLuaCallError.BindStatic(
//       [](lua_State* L) -> bool {
//           CollectErrorStack(L); 
//           return true; // 拦截默认错误处理
//       });
//   ```

// #### FCustomLuaFileLoader
// ```cpp
// DECLARE_DELEGATE_RetVal_ThreeParams(bool, FCustomLuaFileLoader, const FUnLuaModule&, const FString&, TArray<uint8>&, FString&);
// ```
// - **参数**：
//   - const FUnLuaModule&: UnLua模块引用
//   - const FString&: 请求的Lua路径
//   - TArray<uint8>&: 输出数据缓冲区
//   - FString&: 输出块名称
// - **应用场景**：
//   - 加密Lua脚本解密
//   - 从网络资源加载脚本
//   - 多版本热更资源选择

// ### 安全使用建议
// 1. **生命周期管理**
// ```cpp
// // 正确做法：在类析构时取消绑定
// MyClass::~MyClass() {
//     FUnLuaDelegates::OnObjectBinded.Remove(Handle);
// }
// ```

// 2. **线程安全**
// ```cpp
// // 异步线程中需转到GameThread
// AsyncTask(ENamedThreads::GameThread, [](){
//     FUnLuaDelegates::OnLuaStateCreated.Broadcast(L);
// });
// ```

// 3. **性能注意**
// 避免在委托中：
// - 执行重型计算
// - 同步加载资源
// - 无限等待操作

// 该委托系统为UnLua提供了高度可扩展性，使用者可通过注册回调深度定制框架行为，但需注意合理管理委托绑定关系以确保系统稳定性。


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

#include "UnLuaDelegates.h"

FUnLuaDelegates::FOnLuaStateCreated FUnLuaDelegates::OnLuaStateCreated;
FUnLuaDelegates::FOnLuaContextInitialized FUnLuaDelegates::OnLuaContextInitialized;
FUnLuaDelegates::FOnLuaContextCleanup FUnLuaDelegates::OnPreLuaContextCleanup;
FUnLuaDelegates::FOnLuaContextCleanup FUnLuaDelegates::OnPostLuaContextCleanup;
FUnLuaDelegates::FOnPreStaticallyExport FUnLuaDelegates::OnPreStaticallyExport;
FUnLuaDelegates::FOnObjectBinded FUnLuaDelegates::OnObjectBinded;
FUnLuaDelegates::FOnObjectUnbinded FUnLuaDelegates::OnObjectUnbinded;
FUnLuaDelegates::FGenericLuaDelegate FUnLuaDelegates::HotfixLua;
FUnLuaDelegates::FGenericLuaDelegate FUnLuaDelegates::ReportLuaCallError;
FUnLuaDelegates::FGenericLuaDelegate FUnLuaDelegates::ConfigureLuaGC;
FUnLuaDelegates::FCustomLuaFileLoader FUnLuaDelegates::CustomLoadLuaFile;
