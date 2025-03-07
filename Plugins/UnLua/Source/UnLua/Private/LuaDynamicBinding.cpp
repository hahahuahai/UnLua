// 这段代码是UnLua框架中实现动态Lua绑定的核心机制，通过栈式管理实现运行时的绑定切换。以下是分层解析：

// ---

// ### **核心设计目标**
// 实现Lua绑定的**动态作用域切换**能力，允许在特定代码区间临时修改类与Lua模块的映射关系，常用于：
// - 运行时动态替换类的Lua实现
// - 嵌套绑定场景的隔离管理
// - 单元测试时的mock替换

// ---

// ### **核心类结构**
// ```cpp
// FLuaDynamicBinding            // 全局绑定状态管理器
// ├─ TStack<StackNode>          // 绑定状态栈（UE4的TArray实现栈）
// └─ FLuaDynamicBindingStackNode // 栈节点（Class+Module+Ref三元组）

// FScopedLuaDynamicBinding      // RAII作用域守卫（核心使用接口）
// ```

// ---

// ### **关键数据流解析**
// #### **1. 绑定状态三元组**
// ```cpp
// struct {
//     UClass* Class;              // 当前绑定的UClass
//     FString ModuleName;         // 对应的Lua模块路径
//     int32 InitializerTableRef;  // Lua注册表中的初始化表引用
// }
// ```

// #### **2. 全局状态栈操作**
// | 方法       | 作用                                                                 | 线程安全 |
// |------------|----------------------------------------------------------------------|----------|
// | `Push()`   | 压栈当前状态 → 更新为新绑定                                          | 非线程安全 |
// | `Pop()`    | 弹栈恢复旧状态 → 返回被弹出状态的初始化表引用                        | 非线程安全 |
// | `IsValid()`| 验证当前绑定是否有效（Class匹配 + 模块名非空）                       | 非线程安全 |

// ---

// ### **RAII守卫工作流程**
// ```cpp
// // 示例用法
// {
//     FScopedLuaDynamicBinding Guard(L, MyClass, TEXT("Game.Module"), Ref);
//     // 在此作用域内，MyClass绑定到Game.Module
// } // 作用域结束自动还原
// ```

// 1. **构造阶段**
//    - 调用`GLuaDynamicBinding.Push()`保存当前全局状态
//    - 设置新绑定参数到全局管理器

// 2. **析构阶段**
//    - 调用`GLuaDynamicBinding.Pop()`恢复之前状态
//    - 使用`luaL_unref`释放Lua初始化表的注册表引用

// ---

// ### **Lua引用管理**
// ```cpp
// luaL_unref(L, LUA_REGISTRYINDEX, InitializerTableRef);
// ```
// - **InitializerTableRef**：存储Lua侧初始化配置表的引用
// - 通过`LUA_REGISTRYINDEX`管理生命周期，避免：
//   - 野指针风险
//   - 跨作用域的意外访问
//   - 内存泄漏

// ---

// ### **典型应用场景**
// #### **动态实现替换**
// ```cpp
// // 临时将PlayerActor绑定到测试模块
// FScopedLuaDynamicBinding Guard(L, APlayerActor::StaticClass(), 
//     TEXT("Test.PlayerMock"), LUA_NOREF);
// APlayerActor* Actor = NewObject<APlayerActor>();
// // Actor会使用Test/PlayerMock.lua的实现
// ```

// #### **嵌套绑定隔离**
// ```cpp
// void ProcessNPC()
// {
//     FScopedLuaDynamicBinding Guard1(L, ANPC::StaticClass(), "AI.NPCBase", Ref1);
//     SpawnNPC(); // 使用基础AI
    
//     {
//         FScopedLuaDynamicBinding Guard2(L, ANPC::StaticClass(), "AI.BossNPC", Ref2);
//         SpawnBoss(); // 使用Boss专用AI
//     } // 恢复为NPCBase
    
//     SpawnNPC(); // 再次使用基础AI
// }
// ```

// ---

// ### **性能优化特性**
// | 特性                  | 实现方式                          | 优势                     |
// |-----------------------|-----------------------------------|--------------------------|
// | 栈式状态切换          | TArray模拟栈操作                  | O(1)时间复杂度           |
// | 引用计数清理          | 析构时自动unref                   | 无累计内存消耗           |
// | 轻量守卫对象          | 仅存储LuaState指针+有效性标记     | 低内存开销               |

// ---

// ### **设计亮点分析**
// 1. **栈式状态管理**：完美支持嵌套绑定场景
// 2. **自动引用释放**：通过RAII模式避免Lua侧资源泄漏
// 3. **动态无感切换**：业务代码无需感知绑定变化
// 4. **精准作用域控制**：通过C++生命周期管理绑定时效

// 该机制是UnLua实现**热重载**和**运行时逻辑替换**的基石，特别适用于需要动态调整游戏逻辑的开放世界或MMO类项目，能够在不重启游戏的情况下快速切换不同版本的Lua实现。



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

#include "LuaDynamicBinding.h"

FLuaDynamicBinding GLuaDynamicBinding;

bool FLuaDynamicBinding::IsValid(UClass *InClass) const
{
    //return Class && InClass->IsChildOf(Class) && ModuleName.Len() > 0;
    return Class && Class == InClass && ModuleName.Len() > 0;
}

bool FLuaDynamicBinding::Push(UClass *InClass, const TCHAR *InModuleName, int32 InInitializerTableRef)
{
    FLuaDynamicBindingStackNode StackNode;

    StackNode.Class = Class;
    StackNode.ModuleName = ModuleName;
    StackNode.InitializerTableRef = InitializerTableRef;

    Stack.Push(StackNode);

    Class = InClass;
    ModuleName = InModuleName;
    InitializerTableRef = InInitializerTableRef;

    return true;
}

int32 FLuaDynamicBinding::Pop()
{
    check(Stack.Num() > 0);

    FLuaDynamicBindingStackNode StackNode = Stack.Pop();
    int32 TableRef = InitializerTableRef;

    Class = StackNode.Class;
    ModuleName = StackNode.ModuleName;
    InitializerTableRef = StackNode.InitializerTableRef;

    return TableRef;
}

FScopedLuaDynamicBinding::FScopedLuaDynamicBinding(lua_State *InL, UClass *Class, const TCHAR *ModuleName, int32 InitializerTableRef)
    : L(InL), bValid(false)
{
    if (L)
    {
        bValid = GLuaDynamicBinding.Push(Class, ModuleName, InitializerTableRef);
    }
}

FScopedLuaDynamicBinding::~FScopedLuaDynamicBinding()
{
    if (bValid)
    {
        int32 InitializerTableRef = GLuaDynamicBinding.Pop();
        if (InitializerTableRef != LUA_NOREF)
        {
            check(L);
            luaL_unref(L, LUA_REGISTRYINDEX, InitializerTableRef);
        }
    }
}
