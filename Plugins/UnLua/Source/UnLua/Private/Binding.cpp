// 这段代码是UnLua框架中用于管理导出到Lua的C++类型、函数等资源的核心模块。以下是关键部分的解释：

// 1. **FExported结构体**（核心存储容器）
// ```cpp
// struct FExported
// {
//     TArray<IExportedEnum*> Enums;               // 存储导出的枚举类型
//     TArray<IExportedFunction*> Functions;      // 存储导出的全局函数
//     TMap<FString, IExportedClass*> ReflectedClasses;    // 使用UE反射系统的类
//     TMap<FString, IExportedClass*> NonReflectedClasses; // 非反射的自定义类
//     TMap<FString, TSharedPtr<ITypeInterface>> Types;   // 类型接口映射
// };
// ```
// 这是整个模块的数据中心，使用UE4容器存储各类导出资源，区分了反射类和非反射类。

// 2. **单例访问模式**
// ```cpp
// FExported* GetExported()
// {
//     static FExported Exported; // 静态变量保证唯一实例
//     return &Exported;
// }
// ```
// 通过静态变量实现单例模式，确保全局唯一的数据存储中心。

// 3. **核心导出函数
// ```cpp
// void ExportClass(IExportedClass* Class) // 类导出
// void ExportEnum(IExportedEnum* Enum)    // 枚举导出
// void ExportFunction(...)                // 函数导出
// void AddType(...)                      // 类型接口注册
// ```
// 这些函数用于将不同的C++元素注册到导出系统，后续可在Lua中访问

// 4. **反射类与非反射类的区别**
// - `ReflectedClasses`：使用UE4反射系统的类（UCLASS宏标记的类）
// - `NonReflectedClasses`：手动导出的普通C++类
// - 区分存储便于采用不同的绑定策略

// 5. **查询接口**
// ```cpp
// FindExportedClass()       // 通用类查找
// FindExportedReflectedClass()  // 反射类查找
// FindTypeInterface()       // 类型接口查找
// ```
// 这些函数在Lua与C++交互时用于快速定位对应的C++元素，是绑定系统的核心查询机制

// 6. **类型接口系统(ITypeInterface)**
// ```cpp
// TMap<FString, TSharedPtr<ITypeInterface>> Types;
// ```
// 用于自定义类型在Lua中的表现方式，处理特殊类型的转换逻辑（如智能指针、自定义容器等）

// **整体作用**：该模块作为UnLua的注册中心，统一管理所有需要暴露给Lua的C++元素。通过类型注册和查询机制，实现了：
// - C++类/枚举/函数到Lua的自动绑定
// - 反射系统与非反射类的统一管理
// - 自定义类型的特殊处理
// - 运行时高效的元素查找

// 这是UnLua实现"自动绑定"特性的核心基础模块，为后续的Lua虚拟机交互提供数据支持。


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

#include "Binding.h"

namespace UnLua
{
    struct FExported
    {
        TArray<IExportedEnum*> Enums;
        TArray<IExportedFunction*> Functions;
        TMap<FString, IExportedClass*> ReflectedClasses;
        TMap<FString, IExportedClass*> NonReflectedClasses;
        TMap<FString, TSharedPtr<ITypeInterface>> Types;
    };

    FExported* GetExported()
    {
        static FExported Exported;
        return &Exported;
    }

    void ExportClass(IExportedClass* Class)
    {
        if (Class->IsReflected())
            GetExported()->ReflectedClasses.Add(Class->GetName(), Class);
        else
            GetExported()->NonReflectedClasses.Add(Class->GetName(), Class);
    }

    void ExportEnum(IExportedEnum* Enum)
    {
        GetExported()->Enums.Add(Enum);
    }

    void ExportFunction(IExportedFunction* Function)
    {
        GetExported()->Functions.Add(Function);
    }

    void AddType(FString Name, TSharedPtr<ITypeInterface> TypeInterface)
    {
        if (!ensure(!Name.IsEmpty() && TypeInterface))
            return;
        GetExported()->Types.Add(Name, TypeInterface);
    }

    TMap<FString, IExportedClass*> GetExportedReflectedClasses()
    {
        return GetExported()->ReflectedClasses;
    }

    TMap<FString, IExportedClass*> GetExportedNonReflectedClasses()
    {
        return GetExported()->NonReflectedClasses;
    }

    TArray<IExportedEnum*> GetExportedEnums()
    {
        return GetExported()->Enums;
    }

    TArray<IExportedFunction*> GetExportedFunctions()
    {
        return GetExported()->Functions;
    }

    IExportedClass* FindExportedClass(const FString Name)
    {
        auto Class = GetExported()->ReflectedClasses.FindRef(Name);
        if (Class)
        {
            return Class;
        }
        Class = GetExported()->NonReflectedClasses.FindRef(Name);
        return Class;
    }

    IExportedClass* FindExportedReflectedClass(FString Name)
    {
        const auto Class = GetExported()->ReflectedClasses.FindRef(Name);
        return Class;
    }

    IExportedClass* FindExportedNonReflectedClass(FString Name)
    {
        const auto Class = GetExported()->NonReflectedClasses.FindRef(Name);
        return Class;
    }

    TSharedPtr<ITypeInterface> FindTypeInterface(FString Name)
    {
        return GetExported()->Types.FindRef(Name);
    }
}
