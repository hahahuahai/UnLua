// 这段代码是 UnLua 框架中的 `FClassDesc` 类实现，负责管理 Unreal Engine 中 UStruct 和 UClass 的元数据，为 Lua 绑定提供支持。以下是其核心机制的分步解析：

// ---

// ### **1. 类描述初始化**
// ```cpp
// FClassDesc::FClassDesc(...) : Struct(InStruct), ... {
//     bIsScriptStruct = InStruct->IsA(UScriptStruct::StaticClass());
//     bIsClass = InStruct->IsA(UClass::StaticClass());
//     bIsInterface = ...; // 判断是否为接口类
//     Size = Struct->GetStructureSize(); // 类/结构体内存大小
//     UserdataPadding = CalcUserdataPadding(Alignment); // 内存对齐计算
// }
// ```
// - **类型识别**：区分脚本结构体、类、接口等类型
// - **内存布局**：计算 Lua userdata 的内存对齐填充（应对 UE 的 C++ 与 Lua 内存模型差异）

// ---

// ### **2. 字段注册流程**
// ```cpp
// TSharedPtr<FFieldDesc> FClassDesc::RegisterField(FName FieldName, ...) {
//     // 1. 检查字段缓存
//     if (TSharedPtr<FFieldDesc>* Existing = Fields.Find(FieldName))
//         return *Existing;

//     // 2. 查找属性或函数
//     FProperty* Prop = Struct->FindPropertyByName(FieldName);
//     UFunction* Func = (!Prop) ? AsClass()->FindFunctionByName(FieldName) : nullptr;

//     // 3. 处理蓝图生成属性名
//     if (!Prop && bIsScriptStruct && !Struct->IsNative()) {
//         // 剥离 GUID 后缀如 "_12345678"
//         FString DisplayName = StripGuidSuffix(FieldName.ToString());
//         Prop = FindPropertyByDisplayName(DisplayName);
//     }

//     // 4. 确定字段所属的外部类
//     UStruct* Outer = GetPropertyOuter(Prop) ?: Func->GetOuter();
//     if (Outer != Struct) {
//         // 递归注册外部类字段
//         return OuterClass->RegisterField(FieldName, this);
//     }

//     // 5. 创建字段描述
//     TSharedPtr<FFieldDesc> Desc = MakeShared<FFieldDesc>();
//     if (Prop) {
//         Desc->FieldIndex = Properties.Add(FPropertyDesc::Create(Prop)) + 1;
//     } else {
//         Desc->FieldIndex = -(Functions.Add(FFunctionDesc::Create(Func)) + 1);
//     }
//     Fields.Add(FieldName, Desc);
// }
// ```
// - **蓝图属性处理**：自动剥离自动生成的 GUID 后缀（如 `MyVar_92AE3B41`）
// - **递归注册**：支持嵌套类/结构体的字段查找
// - **索引编码**：正数为属性索引，负数为函数索引

// ---

// ### **3. 继承链管理**
// ```cpp
// void FClassDesc::GetInheritanceChain(TArray<FClassDesc*>& Chain) {
//     Chain.Add(this);
//     Chain.Append(SuperClasses); // 包含所有父类描述
// }
// ```
// - **应用场景**：遍历类继承体系以查找覆盖的方法或属性
// - **性能优化**：缓存继承链避免重复计算

// ---

// ### **4. 动态加载/卸载**
// ```cpp
// void FClassDesc::Load() {
//     if (!Struct.IsValid()) {
//         Struct = LoadObject<UStruct>(*ClassName);
//         // 初始化内存布局数据...
//     }
// }

// void FClassDesc::UnLoad() {
//     Fields.Empty(); // 释放所有字段描述
//     Properties.Empty();
//     Functions.Empty();
//     Struct.Reset();
// }
// ```
// - **懒加载**：首次访问时加载 UStruct 对象
// - **资源释放**：显式卸载避免内存泄漏

// ---

// ### **5. 内存对齐处理**
// ```cpp
// UserdataPadding = CalcUserdataPadding(Alignment);
// ```
// - **计算方式**：
//   ```cpp
//   Padding = (HeaderSize % Alignment == 0) ? 0 : (Alignment - HeaderSize % Alignment);
//   ```
// - **作用**：确保 Lua userdata 的内存布局与 UE C++ 对象对齐

// ---

// ### **6. 默认参数处理**
// ```cpp
// FunctionCollection = GDefaultParamCollection.Find(*ClassName);
// ```
// - **数据源**：`GDefaultParamCollection` 存储预生成的函数默认参数
// - **绑定时机**：在类描述初始化时关联，加速运行时参数解析

// ---

// ### **关键设计亮点**
// 1. **蓝图兼容性**  
//    自动处理蓝图生成的属性名，实现无缝的 Lua 访问。

// 2. **高效递归查询**  
//    通过外部类递归注册，支持复杂嵌套结构的字段访问。

// 3. **双缓冲索引**  
//    正负索引编码属性与函数，减少类型判断开销。

// 4. **内存安全**  
//    `TSharedPtr` 管理字段描述，结合显式卸载机制防止内存泄漏。

// ---

// ### **典型应用场景**
// #### **Lua 中访问 UE 属性**
// ```lua
// local player = GetPlayer()
// player.Health = 100 -- 通过 FPropertyDesc 写入属性
// ```

// #### **调用覆盖函数**
// ```lua
// UE4.Override.Actor.ReceiveBeginPlay = function(self)
//     print("Lua override called!")
// end
// ```
// - **实现原理**：`RegisterField` 找到 `ReceiveBeginPlay` 函数描述并替换为 Lua 闭包

// ---

// ### **性能优化策略**
// 1. **缓存字段描述**  
//    `Fields` 映射表避免重复查找反射信息。

// 2. **懒加载类信息**  
//    首次访问时加载 UStruct，减少启动时间。

// 3. **内存预对齐**  
//    计算 userdata 填充提升内存访问效率。

// ---

// ### **潜在问题与解决方案**
// | 问题 | 解决方案 |
// |------|---------|
// | **蓝图属性名变化** | 定期扫描并更新 `Fields` 缓存 |
// | **循环依赖** | 递归深度限制 + 循环检测 |
// | **跨模块加载** | 使用 `LoadObject` 全路径加载 |

// ---

// ### **总结**
// `FClassDesc` 是 UnLua 框架的核心元数据管理系统，通过高效的类型识别、字段注册和内存管理，为 Lua 脚本与 Unreal Engine 的深度交互提供了坚实基础。其设计充分考虑了 UE 反射系统的特性，特别是在处理蓝图生成内容、内存对齐和跨类查询等方面展现出优秀的工程实践。


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

#include "UnLuaCompatibility.h"
#include "ClassDesc.h"
#include "FieldDesc.h"
#include "PropertyDesc.h"
#include "FunctionDesc.h"
#include "LuaCore.h"
#include "DefaultParamCollection.h"
#include "LowLevel.h"
#include "LuaOverridesClass.h"
#include "UnLuaModule.h"

/**
 * Class descriptor constructor
 */
FClassDesc::FClassDesc(UnLua::FLuaEnv* Env, UStruct* InStruct, const FString& InName)
    : Struct(InStruct), ClassName(InName), UserdataPadding(0), Size(0), Env(Env), FunctionCollection(nullptr)
{
    RawStructPtr = InStruct;
    bIsScriptStruct = InStruct->IsA(UScriptStruct::StaticClass());
    bIsClass = InStruct->IsA(UClass::StaticClass());
    bIsInterface = bIsClass && static_cast<UClass*>(InStruct)->HasAnyClassFlags(CLASS_Interface) && InStruct != UInterface::StaticClass();
    bIsNative = InStruct->IsNative();

    if (bIsClass)
    {
        Size = Struct->GetStructureSize();
        FunctionCollection = GDefaultParamCollection.Find(*ClassName);
    }
    else if (bIsScriptStruct)
    {
        UScriptStruct* ScriptStruct = AsScriptStruct();
        UScriptStruct::ICppStructOps* CppStructOps = ScriptStruct->GetCppStructOps();
        int32 Alignment = CppStructOps ? CppStructOps->GetAlignment() : ScriptStruct->GetMinAlignment();
        Size = CppStructOps ? CppStructOps->GetSize() : ScriptStruct->GetStructureSize();
        UserdataPadding = CalcUserdataPadding(Alignment); // calculate padding size for userdata
    }
}

/**
 * Register a field of this class
 */
TSharedPtr<FFieldDesc> FClassDesc::RegisterField(FName FieldName, FClassDesc* QueryClass)
{
    Load();

    TSharedPtr<FFieldDesc> FieldDesc = nullptr;
    TSharedPtr<FFieldDesc>* FieldDescPtr = Fields.Find(FieldName);
    if (FieldDescPtr)
    {
        FieldDesc = *FieldDescPtr;
        return FieldDesc;
    }

    // a property or a function ?
    FProperty* Property = Struct->FindPropertyByName(FieldName);
    UFunction* Function = (!Property && bIsClass) ? AsClass()->FindFunctionByName(FieldName) : nullptr;
    bool bValid = Property || Function;
    if (!bValid && bIsScriptStruct && !Struct->IsNative())
    {
        FString FieldNameStr = FieldName.ToString();
        const int32 GuidStrLen = 32;
        const int32 MinimalPostfixlen = GuidStrLen + 3;
        for (TFieldIterator<FProperty> PropertyIt(Struct.Get(), EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); PropertyIt; ++PropertyIt)
        {
            FString DisplayName = (*PropertyIt)->GetName();
            if (DisplayName.Len() > MinimalPostfixlen)
            {
                DisplayName = DisplayName.LeftChop(GuidStrLen + 1);
                int32 FirstCharToRemove = INDEX_NONE;
                if (DisplayName.FindLastChar(TCHAR('_'), FirstCharToRemove))
                {
                    DisplayName = DisplayName.Mid(0, FirstCharToRemove);
                }
            }

            if (DisplayName == FieldNameStr)
            {
                Property = *PropertyIt;
                break;
            }
        }

        bValid = Property != nullptr;
    }
    if (!bValid)
        return nullptr;

    UStruct* OuterStruct;
    if (Property)
    {
        OuterStruct = Cast<UStruct>(GetPropertyOuter(Property));
    }
    else
    {
        OuterStruct = Cast<UStruct>(Function->GetOuter());
        if (const auto OverridesClass = Cast<ULuaOverridesClass>(OuterStruct))
            OuterStruct = OverridesClass->GetOwner();
    }

    if (!OuterStruct)
        return nullptr;

    if (OuterStruct != Struct)
    {
        FClassDesc* OuterClass = Env->GetClassRegistry()->RegisterReflectedType(OuterStruct);
        check(OuterClass);
        return OuterClass->RegisterField(FieldName, QueryClass);
    }

    // create new Field descriptor
    FieldDesc = MakeShared<FFieldDesc>();
    FieldDesc->QueryClass = QueryClass;
    FieldDesc->OuterClass = this;
    Fields.Add(FieldName, FieldDesc);
    if (Property)
    {
        TSharedPtr<FPropertyDesc> Ptr(FPropertyDesc::Create(Property));
        FieldDesc->FieldIndex = Properties.Add(Ptr); // index of property descriptor
        ++FieldDesc->FieldIndex;
    }
    else
    {
        check(Function);
        FParameterCollection* DefaultParams = FunctionCollection ? FunctionCollection->Functions.Find(FieldName) : nullptr;
        FieldDesc->FieldIndex = Functions.Add(MakeShared<FFunctionDesc>(Function, DefaultParams)); // index of function descriptor
        ++FieldDesc->FieldIndex;
        FieldDesc->FieldIndex = -FieldDesc->FieldIndex;
    }
    return FieldDesc;
}

void FClassDesc::GetInheritanceChain(TArray<FClassDesc*>& DescChain)
{
    DescChain.Add(this);
    DescChain.Append(SuperClasses);
}

void FClassDesc::Load()
{
    if (Struct.IsValid())
    {
        return;
    }

    if (GIsGarbageCollecting)
    {
        return;
    }

    UnLoad();

    FString Name = (ClassName[0] == 'U' || ClassName[0] == 'A' || ClassName[0] == 'F') ? ClassName.RightChop(1) : ClassName;
    UStruct* Found = FindFirstObject<UStruct>(*Name);
    if (!Found)
        Found = LoadObject<UStruct>(nullptr, *Name);

    Struct = Found;
    RawStructPtr = Found;
}

void FClassDesc::UnLoad()
{
    Fields.Empty();
    Properties.Empty();
    Functions.Empty();

    Struct.Reset();
    RawStructPtr = nullptr;
}
