// 这段代码是UnLua框架中用于检测和防止Lua脚本死循环的核心模块，以下是逐层解析：

// ---

// ### **核心设计目标**
// 通过独立监控线程检测Lua脚本执行超时，防止无限循环阻塞游戏主线程。当Lua脚本执行超过预设阈值时，强制抛出错误终止执行。

// ---

// ### **核心类结构**
// ```cpp
// FDeadLoopCheck        // 死循环检测管理器
// ├─ FRunner            // 后台监控线程（继承FRunnable）
// └─ FGuard             // 执行区间守卫（RAII模式）
// ```

// ---

// ### **关键机制解析**

// #### **1. 超时阈值配置**
// ```cpp
// int32 FDeadLoopCheck::Timeout = 0; // 单位：秒
// ```
// - 静态变量控制全局超时阈值
// - 0表示禁用检测（通过`MakeGuard`方法逻辑控制）

// #### **2. 监控线程(FRunner)**
// ```cpp
// Thread = FRunnableThread::Create(...); // 创建独立线程
// ```
// - **优先级策略**：`TPri_BelowNormal`低优先级，避免影响主线程
// - **检测逻辑**：
//   ```cpp
//   while (bRunning) {
//       Sleep(1秒);
//       if (有守卫激活) {
//           超时计数器累加
//           if (超过Timeout) {
//               触发超时处理
//           }
//       }
//   }
//   ```

// #### **3. 守卫机制(FGuard)**
// ```cpp
// TUniquePtr<FGuard> guard = MakeGuard(); // RAII守卫
// ```
// - **激活时**：`GuardEnter`
//   - 原子递增`GuardCounter`（线程安全）
//   - 重置超时计数器
//   - 记录当前守卫指针
// - **析构时**：`GuardLeave`
//   - 原子递减`GuardCounter`

// #### **4. 超时处理流程**
// ```cpp
// lua_sethook(L, OnLuaLineEvent, LUA_MASKLINE, 0); // 设置行事件钩子
// ```
// 1. 超时发生时清除当前守卫
// 2. 设置Lua行事件钩子
// 3. 下次Lua执行代码行时触发：
//    ```cpp
//    luaL_error(L, "lua script exec timeout"); // 抛出致命错误
//    ```

// ---

// ### **线程安全实现**
// | 成员                | 类型                  | 线程安全措施               |
// |---------------------|-----------------------|--------------------------|
// | `GuardCounter`      | `TAtomic<int32>`     | 原子操作                 |
// | `TimeoutGuard`      | `atomic<FGuard*>`    | 原子指针操作            |
// | `TimeoutCounter`    | `TAtomic<int32>`     | 原子递增/重置           |

// ---

// ### **典型应用场景**
// ```lua
// -- Lua脚本示例
// while true do  -- 死循环
//     -- 无任何yield操作
// end
// ```
// 1. 脚本进入时创建守卫：
//    ```cpp
//    auto guard = MakeGuard(); // 开始监控
//    ```
// 2. 超时后触发行事件钩子
// 3. 抛出错误终止执行：
//    ```
//    [ERROR] lua script exec timeout
//    ```

// ---

// ### **性能影响分析**
// - **低开销检测**：秒级轮询避免高频检查
// - **按需启用**：`Timeout=0`时完全无消耗
// - **钩子触发策略**：仅在超时后设置单次行事件钩子

// ---

// ### **设计亮点**
// 1. **多线程解耦**：独立监控线程不阻塞主逻辑
// 2. **精准熔断**：利用Lua调试钩子实现安全中断
// 3. **RAII守卫模式**：自动化的监控区间管理
// 4. **原子操作体系**：无锁化的线程安全控制

// 该模块是UnLua框架的守护者级组件，有效防止脚本错误导致的引擎级冻结，尤其对开放世界游戏中玩家自定义脚本的安全运行至关重要。


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

#include "LuaDeadLoopCheck.h"
#include "HAL/RunnableThread.h"
#include "UnLuaModule.h"

namespace UnLua
{
    int32 FDeadLoopCheck::Timeout = 0;

    FDeadLoopCheck::FDeadLoopCheck(FLuaEnv* Env)
        : Env(Env)
    {
        Runner = new FRunner();
    }

    TUniquePtr<FDeadLoopCheck::FGuard> FDeadLoopCheck::MakeGuard()
    {
        if (Timeout <= 0)
            return TUniquePtr<FGuard>();
        return MakeUnique<FGuard>(this);
    }

    FDeadLoopCheck::FRunner::FRunner()
        : bRunning(true),
          GuardCounter(0),
          TimeoutCounter(0),
          TimeoutGuard(nullptr)
    {
        Thread = FRunnableThread::Create(this, TEXT("LuaDeadLoopCheck"), 0, TPri_BelowNormal);
    }

    uint32 FDeadLoopCheck::FRunner::Run()
    {
        while (bRunning)
        {
            FPlatformProcess::Sleep(1.0f);
            if (GuardCounter.GetValue() == 0)
                continue;

            TimeoutCounter.Increment();
            if (TimeoutCounter.GetValue() < Timeout)
                continue;

            const auto Guard = TimeoutGuard.load();
            if (Guard)
            {
                TimeoutGuard.store(nullptr);
                Guard->SetTimeout();
            }
        }
        return 0;
    }

    void FDeadLoopCheck::FRunner::Stop()
    {
        bRunning = false;
        TimeoutGuard.store(nullptr);
    }

    void FDeadLoopCheck::FRunner::Exit()
    {
        delete this;
    }

    void FDeadLoopCheck::FRunner::GuardEnter(FGuard* Guard)
    {
        if (GuardCounter.Increment() > 1)
            return;
        TimeoutCounter.Set(0);
        TimeoutGuard.store(Guard);
    }

    void FDeadLoopCheck::FRunner::GuardLeave()
    {
        GuardCounter.Decrement();
    }

    FDeadLoopCheck::FGuard::FGuard(FDeadLoopCheck* Owner)
        : Owner(Owner)
    {
        Owner->Runner->GuardEnter(this);
    }

    FDeadLoopCheck::FGuard::~FGuard()
    {
        Owner->Runner->GuardLeave();
    }

    void FDeadLoopCheck::FGuard::SetTimeout()
    {
        const auto L = Owner->Env->GetMainState();
        const auto Hook = lua_gethook(L);
        if (Hook == nullptr)
            lua_sethook(L, OnLuaLineEvent, LUA_MASKLINE, 0);
    }

    void FDeadLoopCheck::FGuard::OnLuaLineEvent(lua_State* L, lua_Debug* ar)
    {
        lua_sethook(L, nullptr, 0, 0);
        luaL_error(L, "lua script exec timeout");
    }
}
