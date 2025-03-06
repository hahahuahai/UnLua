// 这段代码是 UnLua 框架中 `ULuaFunction` 类的实现，负责将 Unreal Engine 的 `UFunction` 动态覆盖为 Lua 函数。以下是关键部分的逐步解释：

// ---

// ### **1. 执行函数定义**
// ```cpp
// DEFINE_FUNCTION(ULuaFunction::execCallLua) {
//     ULuaFunction* LuaFunction = Cast<ULuaFunction>(Stack.CurrentNativeFunction);
//     FLuaEnv* Env = IUnLuaModule::Get().GetEnv(Context);
//     Env->GetFunctionRegistry()->Invoke(LuaFunction, Context, Stack, RESULT_PARAM);
// }
// ```
// - **功能**：当被覆盖的 C++ 函数被调用时，此函数作为入口点
// - **流程**：
//   1. 获取当前函数对象 `ULuaFunction`
//   2. 通过 `FLuaEnv` 获取对应的 Lua 环境
//   3. 调用函数注册表的 `Invoke` 方法执行 Lua 逻辑

// ---

// ### **2. 函数标识与获取**
// ```cpp
// ULuaFunction* ULuaFunction::Get(UFunction* Function) {
//     // 检查 Script 数据的魔法头和指针
//     if (FPlatformMemory::Memcmp(Data, ScriptMagicHeader, ScriptMagicHeaderSize) == 0)
//         return FPlatformMemory::ReadUnaligned<ULuaFunction*>(Data + ScriptMagicHeaderSize);
// }
// ```
// - **魔法头验证**：`EX_StringConst, 'L','U','A',0,EX_UInt64Const`
// - **作用**：通过嵌入的特殊头标识被 Lua 覆盖的函数
// - **数据布局**：`[魔法头(6字节)][ULuaFunction*指针(8字节)]`

// ---

// ### **3. 覆盖能力判断**
// ```cpp
// bool ULuaFunction::IsOverridable(const UFunction* Function) {
//     return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || 
//            (Function->FunctionFlags & (FUNC_Native|FUNC_Event|FUNC_Net)) == (FUNC_Native|FUNC_Event);
// }
// ```
// - **可覆盖条件**：
//   - 标记为 `BlueprintEvent` 的蓝图可覆盖函数
//   - 原生事件函数（`Native + Event` 组合）

// ---

// ### **4. 函数覆盖流程**
// ```cpp
// void ULuaFunction::Override(UFunction* Function, UClass* Class, bool bAddNew) {
//     // 复制原始函数创建 Overridden 副本
//     Overridden = StaticDuplicateObject(Function, GetOuter(), *DestName);
//     Overridden->SetNativeFunc(Function->GetNativeFunc());
    
//     // 设置新函数属性
//     SetNativeFunc(execCallLua); // 指向Lua执行入口
//     Function->Script.AddUninitialized(...); // 注入魔法头和指针
// }
// ```
// - **关键步骤**：
//   1. **复制原始函数**：创建 `Overridden` 副本保留原始实现
//   2. **重定向调用**：设置新函数的 `NativeFunc` 为 `execCallLua`
//   3. **注入标识数据**：在 `Script` 属性写入魔法头和自身指针

// ---

// ### **5. 状态管理**
// ```cpp
// void ULuaFunction::SetActive(bool bActive) {
//     if (bActive) {
//         // 激活覆盖
//         Function->SetNativeFunc(&execScriptCallLua);
//         Function->Script = ...; // 注入数据
//     } else {
//         // 恢复原函数
//         Function->Script = Overridden->Script;
//         Function->SetNativeFunc(Overridden->GetNativeFunc());
//     }
// }
// ```
// - **激活时**：将原函数的执行入口改为 `execScriptCallLua` 并注入标识
// - **停用时**：恢复原函数的字节码和原生函数指针

// ---

// ### **6. 生命周期与恢复**
// ```cpp
// void ULuaFunction::Restore() {
//     if (bAdded) {
//         // 移除新增函数
//         OverriddenClass->RemoveFunctionFromFunctionMap(this);
//     } else {
//         // 恢复原函数数据
//         Old->Script = Script;
//         Old->SetNativeFunc(Overridden->GetNativeFunc());
//     }
// }
// ```
// - **新增函数**：直接从类函数映射中删除
// - **覆盖函数**：还原字节码和函数指针

// ---

// ### **7. 参数处理与绑定**
// ```cpp
// void ULuaFunction::Initialize() {
//     Desc = MakeShared<FFunctionDesc>(this, nullptr);
// }
// ```
// - **FFunctionDesc**：
//   - 解析函数的参数列表、返回类型
//   - 生成 Lua 与 C++ 间的类型转换逻辑
//   - 管理参数压栈/弹栈操作

// ---

// ### **8. 动态函数注册**
// ```cpp
// void ULuaFunction::Bind() {
//     if (bAdded) {
//         SetNativeFunc(execCallLua);
//         Class->AddNativeFunction(*GetName(), &ULuaFunction::execCallLua);
//     }
// }
// ```
// - **新增函数**：直接绑定到 Lua 执行入口
// - **覆盖函数**：通过修改原函数的 `NativeFunc` 实现透明替换

// ---

// ### **关键设计思想**
// 1. **透明替换**  
//    通过修改 `UFunction` 的 `NativeFunc` 和 `Script` 数据，在不破坏 UE 反射系统的前提下实现覆盖。

// 2. **状态双备份**  
//    `Overridden` 成员保留原始函数数据，确保可逆操作。

// 3. **标识嵌入**  
//    魔法头机制快速区分原生函数与 Lua 覆盖函数。

// 4. **分层管理**  
//    `FLuaOverrides` 全局管理所有覆盖关系，支持批量操作。

// ---

// ### **执行流程示例**
// 1. **蓝图调用 `MyFunc()`**
// 2. **UE 查找 `UFunction` 并执行其 `NativeFunc`**
// 3. **若被覆盖，执行 `execCallLua`**
// 4. **Lua 环境查找对应的函数并执行**
// 5. **如需调用原逻辑，通过 `Super.MyFunc()` 访问 `Overridden` 函数**

// ---

// ### **错误处理机制**
// - **环境检查**：执行前验证 `FLuaEnv` 有效性，避免 PIE 结束后的崩溃
// - **标志校验**：`IsOverridable` 过滤不可覆盖函数
// - **缓存清理**：`Restore` 移除所有相关引用，防止野指针

// ---

// ### **性能优化**
// - **函数描述缓存**：`FFunctionDesc` 预处理参数信息，减少运行时开销
// - **直接指针访问**：通过魔法头快速定位 `ULuaFunction`，避免字符串查找
// - **条件编译**：根据 UE 版本调整底层函数绑定方式

// ---

// 通过这种精细的 UFunction 操作，UnLua 实现了高效的 C++/Lua 互操作，为游戏逻辑的热重载和脚本化提供了基础设施保障。

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


#include "LuaFunction.h"
#include "LuaOverrides.h"
#include "LuaOverridesClass.h"
#include "UnLuaModule.h"
#include "ReflectionUtils/PropertyDesc.h"
#include "Misc/EngineVersionComparison.h"

static constexpr uint8 ScriptMagicHeader[] = {EX_StringConst, 'L', 'U', 'A', '\0', EX_UInt64Const};
static constexpr size_t ScriptMagicHeaderSize = sizeof ScriptMagicHeader;

DEFINE_FUNCTION(ULuaFunction::execCallLua)
{
    const auto LuaFunction = Cast<ULuaFunction>(Stack.CurrentNativeFunction);
    const auto Env = IUnLuaModule::Get().GetEnv(Context);
    if (!Env)
    {
        // PIE 结束时可能已经没有Lua环境了
        return;
    }
    Env->GetFunctionRegistry()->Invoke(LuaFunction, Context, Stack, RESULT_PARAM);
}

DEFINE_FUNCTION(ULuaFunction::execScriptCallLua)
{
    const auto LuaFunction = Get(Stack.CurrentNativeFunction);
    if (!LuaFunction)
        return;
    const auto Env = IUnLuaModule::Get().GetEnv(Context);
    if (!Env)
    {
        // PIE 结束时可能已经没有Lua环境了
        return;
    }
    Env->GetFunctionRegistry()->Invoke(LuaFunction, Context, Stack, RESULT_PARAM);
}

ULuaFunction* ULuaFunction::Get(UFunction* Function)
{
    if (!Function)
        return nullptr;

    const auto LuaFunction = Cast<ULuaFunction>(Function);
    if (LuaFunction)
        return LuaFunction;

    if (Function->Script.Num() < ScriptMagicHeaderSize + sizeof(ULuaFunction*))
        return nullptr;

    const auto Data = Function->Script.GetData();
    if (!Data)
        return nullptr;

    if (FPlatformMemory::Memcmp(Data, ScriptMagicHeader, ScriptMagicHeaderSize) != 0)
        return nullptr;

    return FPlatformMemory::ReadUnaligned<ULuaFunction*>(Data + ScriptMagicHeaderSize);
}

bool ULuaFunction::IsOverridable(const UFunction* Function)
{
    static constexpr uint32 FlagMask = FUNC_Native | FUNC_Event | FUNC_Net;
    static constexpr uint32 FlagResult = FUNC_Native | FUNC_Event;
    return Function->HasAnyFunctionFlags(FUNC_BlueprintEvent) || (Function->FunctionFlags & FlagMask) == FlagResult;
}

bool ULuaFunction::Override(UFunction* Function, UClass* Outer, FName NewName)
{
    UnLua::FLuaOverrides::Get().Override(Function, Outer, NewName);
    return true;
}

void ULuaFunction::RestoreOverrides(UClass* Class)
{
    UnLua::FLuaOverrides::Get().Restore(Class);
}

void ULuaFunction::SuspendOverrides(UClass* Class)
{
    UnLua::FLuaOverrides::Get().Suspend(Class);
}

void ULuaFunction::ResumeOverrides(UClass* Class)
{
    UnLua::FLuaOverrides::Get().Resume(Class);
}

void ULuaFunction::GetOverridableFunctions(UClass* Class, TMap<FName, UFunction*>& Functions)
{
    if (!Class)
        return;

    // all 'BlueprintEvent'
    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::IncludeInterfaces); It; ++It)
    {
        UFunction* Function = *It;
        if (!IsOverridable(Function))
            continue;
        FName FuncName = Function->GetFName();
        UFunction** FuncPtr = Functions.Find(FuncName);
        if (!FuncPtr)
            Functions.Add(FuncName, Function);
    }

    // all 'RepNotifyFunc'
    for (int32 i = 0; i < Class->ClassReps.Num(); ++i)
    {
        FProperty* Property = Class->ClassReps[i].Property;
        if (!Property->HasAnyPropertyFlags(CPF_RepNotify))
            continue;
        UFunction* Function = Class->FindFunctionByName(Property->RepNotifyFunc);
        if (!Function)
            continue;
        UFunction** FuncPtr = Functions.Find(Property->RepNotifyFunc);
        if (!FuncPtr)
            Functions.Add(Property->RepNotifyFunc, Function);
    }
}

void ULuaFunction::Initialize()
{
    Desc = MakeShared<FFunctionDesc>(this, nullptr);
}

void ULuaFunction::Override(UFunction* Function, UClass* Class, bool bAddNew)
{
    check(Function && Class && !From.IsValid());

#if WITH_METADATA
    UMetaData::CopyMetadata(Function, this);
#endif

    bActivated = false;
    bAdded = bAddNew;
    From = Function;

    if (Function->GetNativeFunc() == execScriptCallLua)
    {
        // 目标UFunction可能已经被覆写过了
        const auto LuaFunction = Get(Function);
        Overridden = LuaFunction->GetOverridden();
        check(Overridden);
    }
    else
    {
        const auto DestName = FString::Printf(TEXT("%s__Overridden"), *Function->GetName());
        if (Function->HasAnyFunctionFlags(FUNC_Native))
            GetOuterUClass()->AddNativeFunction(*DestName, *Function->GetNativeFunc());
        Overridden = static_cast<UFunction*>(StaticDuplicateObject(Function, GetOuter(), *DestName));
        Overridden->ClearInternalFlags(EInternalObjectFlags::Native);
        Overridden->StaticLink(true);
        Overridden->SetNativeFunc(Function->GetNativeFunc());
    }

    SetActive(true);
}

void ULuaFunction::Restore()
{
    if (bAdded)
    {
        if (const auto OverriddenClass = Cast<ULuaOverridesClass>(GetOuter())->GetOwner())
            OverriddenClass->RemoveFunctionFromFunctionMap(this);
    }
    else
    {
        const auto Old = From.Get();
        if (!Old)
            return;
        Old->Script = Script;
        Old->SetNativeFunc(Overridden->GetNativeFunc());
        Old->GetOuterUClass()->AddNativeFunction(*Old->GetName(), Overridden->GetNativeFunc());
        Old->FunctionFlags = Overridden->FunctionFlags;
    }
}

UClass* ULuaFunction::GetOverriddenUClass() const
{
    const auto OverridesClass = Cast<ULuaOverridesClass>(GetOuter());
    return OverridesClass ? OverridesClass->GetOwner() : nullptr;
}

void ULuaFunction::SetActive(const bool bActive)
{
    if (bActivated == bActive)
        return;

    const auto Function = From.Get();
    if (!Function)
        return;

    const auto Class = Cast<ULuaOverridesClass>(GetOuter())->GetOwner();
    if (!Class)
        return;
    
    if (bActive)
    {
        if (bAdded)
        {
            check(!Class->FindFunctionByName(GetFName(), EIncludeSuperFlag::ExcludeSuper));
            SetSuperStruct(Function);
            FunctionFlags |= FUNC_Native;
            ClearInternalFlags(EInternalObjectFlags::Native);
            SetNativeFunc(execCallLua);

            Class->AddFunctionToFunctionMap(this, *GetName());
            if (Function->HasAnyFunctionFlags(FUNC_Native))
                Class->AddNativeFunction(*GetName(), &ULuaFunction::execCallLua);
        }
        else
        {
            SetSuperStruct(Function->GetSuperStruct());
            Script = Function->Script;
            Children = Function->Children;
            ChildProperties = Function->ChildProperties;
            PropertyLink = Function->PropertyLink;

            Function->FunctionFlags |= FUNC_Native;
            Function->SetNativeFunc(&execScriptCallLua);
            Function->GetOuterUClass()->AddNativeFunction(*Function->GetName(), &execScriptCallLua);
            Function->Script.Empty();
            Function->Script.AddUninitialized(ScriptMagicHeaderSize + sizeof(ULuaFunction*));
            const auto Data = Function->Script.GetData();
            FPlatformMemory::Memcpy(Data, ScriptMagicHeader, ScriptMagicHeaderSize);
            FPlatformMemory::WriteUnaligned<ULuaFunction*>(Data + ScriptMagicHeaderSize, this);
        }
    }
    else
    {
        if (bAdded)
        {
            Class->RemoveFunctionFromFunctionMap(this);
        }
        else
        {
            Children = nullptr;
            ChildProperties = nullptr;

            Function->Script = Script;
            Function->SetNativeFunc(Overridden->GetNativeFunc());
            Function->GetOuterUClass()->AddNativeFunction(*Function->GetName(), Overridden->GetNativeFunc());
            Function->FunctionFlags = Overridden->FunctionFlags;
        }
    }
    
    bActivated = bActive;
}

void ULuaFunction::FinishDestroy()
{
    if (bActivated && !bAdded)
    {
        Children = nullptr;
        ChildProperties = nullptr;
    }
    UFunction::FinishDestroy();
}

UFunction* ULuaFunction::GetOverridden() const
{
    return Overridden;
}

void ULuaFunction::Bind()
{
    if (From.IsValid())
    {
        if (bAdded)
            SetNativeFunc(execCallLua);
        else
            SetNativeFunc(Overridden->GetNativeFunc());
    }
    else
    {
#if UE_VERSION_NEWER_THAN(5, 2, 1)
        Super::Bind();
#else
        SetNativeFunc(ProcessInternal);
#endif
    }
}
