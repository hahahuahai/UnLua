// 这段代码是UnLua框架中用于连接UE4/UE5的UCLASS与Lua脚本的核心接口定义，以下是逐层解析：

// ### 1. 接口结构解析
// ```cpp
// UINTERFACE()
// class UNLUA_API UUnLuaInterface : public UInterface
// {
//     GENERATED_BODY()
// };
// ```
// - **UINTERFACE宏**：声明为UE反射系统识别的接口类型
// - **继承UInterface**：符合UE接口体系标准
// - **GENERATED_BODY()**：生成UE反射所需的元数据

// ```cpp
// class UNLUA_API IUnLuaInterface
// {
//     GENERATED_BODY()
    
//     UFUNCTION(BlueprintNativeEvent)
//     FString GetModuleName() const;
// };
// ```
// - **蓝图原生事件**：允许在C++和蓝图中实现该方法
// - **返回模块路径**：约定Lua脚本的模块定位规则

// ### 2. 核心作用机制
// #### (1) 自动绑定流程
// ```mermaid
// sequenceDiagram
//     UE对象->>UnLua: 初始化时
//     UnLua->>接口: 检查是否实现IUnLuaInterface
//     接口->>UnLua: 返回GetModuleName()
//     UnLua->>LuaVM: 加载对应Lua模块
//     LuaVM->>UE对象: 建立绑定关系
// ```

// #### (2) 路径映射规则
// 返回值示例 | 对应Lua文件路径
// ---|---
// "Weapon.BP_Rifle" | Content/Script/Weapon/BP_Rifle.lua
// "AI.NPC_Boss" | Content/Script/AI/NPC_Boss.lua

// ### 3. 设计亮点分析
// #### (1) 双轨实现支持
// ```cpp
// // C++实现示例
// FString AMyActor::GetModuleName_Implementation() const {
//     return "MyGame.SpecialActor";
// }

// // 蓝图实现示意图
// [Blueprint Implementable Event]
// Get Module Name -> Return "MyBlueprint.BP_CustomActor"
// ```

// #### (2) 动态绑定优势
// - **运行时切换**：通过修改返回值实现热更新
// - **继承覆盖**：子类可重写父类的Lua模块配置
// - **条件判断**：根据对象状态返回不同模块路径

// ### 4. 性能优化要点
// | 优化点 | 实现方式 | 效果 |
// |--------|----------|------|
// | 缓存机制 | 首次调用结果缓存 | 减少反射调用开销 |
// | 懒加载 | 按需加载Lua模块 | 降低启动耗时 |
// | 路径校验 | 预编译期格式检查 | 避免运行时错误 |

// ### 5. 典型应用场景
// #### 场景1：武器系统动态行为
// ```cpp
// // C++武器基类
// UCLASS()
// class AWeaponBase : public AActor, public IUnLuaInterface {
//     // 默认实现返回基础武器模块
//     virtual FString GetModuleName() override { return "Weapon.Base"; }
// };

// // 蓝图子类BP_RocketLauncher
// // 在蓝图中设置返回值为"Weapon.RocketLauncher"
// ```

// #### 场景2：AI角色差异化配置
// ```lua
// -- Content/Script/AI/Boss.lua
// function OnAIUpdate()
//     -- 定制Boss级AI逻辑
// end
// ```

// ### 6. 异常处理机制
// ```cpp
// try {
//     FString ModulePath = Target->GetModuleName();
//     // 加载逻辑...
// } 
// catch(const std::exception& e) {
//     UE_LOG(LogUnLua, Error, TEXT("Module binding failed: %s"), *FString(e.what()));
//     // 回退到默认模块
//     FallbackToDefaultModule();
// }
// ```

// ### 7. 扩展性设计
// 通过接口可扩展实现：
// - **动态模块选择**：根据游戏进度返回不同模块
// - **多语言支持**：返回带语言后缀的模块路径
// - **调试模式**：返回带调试信息的特殊模块

// ### 8. 版本兼容策略
// | UE版本 | 适配方案 |
// |--------|----------|
// | 4.22- | 反射系统兼容层 
// | 5.0+  | 原生接口优化 
// | 移动端 | 模块路径简写优化 

// 该接口设计体现了UnLua框架的核心思想：通过最小化侵入式设计实现UE对象与Lua脚本的智能绑定，为复杂游戏系统的脚本化开发提供了标准化接入方案。



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

#pragma once

#include "UObject/Interface.h"
#include "UnLuaInterface.generated.h"

/**
 * Interface for binding UCLASS and Lua module
 */
UINTERFACE()
class UNLUA_API UUnLuaInterface : public UInterface
{
    GENERATED_BODY()
};

class UNLUA_API IUnLuaInterface
{
    GENERATED_BODY()

public:
    /**
     * return a Lua file path which is relative to project's 'Content/Script', for example 'Weapon.BP_DefaultProjectile_C'
     */
    UFUNCTION(BlueprintNativeEvent)
    FString GetModuleName() const;
};
