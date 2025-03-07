// 这段代码是UnLua框架中实现UE类型系统与Lua交互的核心模块，主要功能是为Lua提供对Unreal Engine原生类型系统的无缝访问。以下是分层解析：

// ---

// ### **核心架构图示**
// ```mermaid
// graph TD
//     A[UE命名空间] --> B[元表系统]
//     A --> C[类型动态加载]
//     A --> D[对象创建]
//     B --> B1(__index元方法)
//     C --> C1(反射类型加载)
//     C --> C2(枚举类型处理)
//     D --> D1(NewObject封装)
//     D --> D2(动态绑定作用域)
// ```

// ---

// ### **核心功能模块**

// #### **1. UE全局命名空间**
// - **注册入口**：`UELib::Open()`创建全局表"UE"
// - **元表机制**：设置`__index`元方法实现动态类型发现
// - **兼容处理**：
//   - `WITH_UE4_NAMESPACE`宏控制创建"UE4"别名
//   - 全局环境注入函数实现向下兼容

// #### **2. 动态类型加载**
// ```cpp
// static int UE_Index(lua_State* L)
// ```
// - **名称前缀解析**：
//   - `U/A/F`开头：反射类加载（`FClassRegistry::LoadReflectedType`）
//   - `E`开头：枚举类型加载（`FEnumRegistry::Register`）
// - **蓝图类型拦截**：
//   - 警告日志提示使用`LoadClass/LoadObject`
//   - 防止原生接口误用蓝图资产

// #### **3. 对象创建系统**
// ```cpp
// static int32 Global_NewObject(lua_State *L)
// ```
// - **参数解析**：
//   - 参数1：UClass类型
//   - 参数2：Outer对象
//   - 参数3：对象名称
//   - 参数4：Lua模块名（动态绑定）
// - **安全创建**：
//   - 使用`FScopedLuaDynamicBinding`确保作用域绑定
//   - 跨版本构造兼容（`StaticConstructObject_Internal`）

// #### **4. 核心API注册**
// ```lua
// UE_Functions = {
//     {"LoadObject", UObject_Load},
//     {"LoadClass", UClass_Load},
//     {"NewObject", Global_NewObject}
// }
// ```
// - **三大利器**：
//   - **LoadObject**：异步加载UObject资产
//   - **LoadClass**：获取UClass引用
//   - **NewObject**：运行时动态创建实例

// ---

// ### **关键实现机制**

// #### **动态类型发现流程**
// ```mermaid
// sequenceDiagram
//     Lua->>UE表: 访问ClassX
//     UE表->>UE_Index: 触发__index
//     alt 已导出非反射类
//         UE_Index->>ExportedNonReflectedClass: 注册类型
//     else 原生类型
//         UE_Index->>FClassRegistry: 加载反射信息
//         UE_Index->>Lua栈: 设置元表
//     else 蓝图类型
//         UE_Index->>日志系统: 输出警告
//     end
//     UE_Index-->>Lua: 返回类型引用
// ```

// #### **对象创建流程**
// ```mermaid
// flowchart TB
//     Start[Lua调用UE.NewObject] --> CheckParams{参数检查}
//     CheckParams -->|有效| GetClass[获取UClass]
//     CheckParams -->|无效| Error[日志报错]
//     GetClass --> CreateScope[进入动态绑定作用域]
//     CreateScope --> NativeCreate[调用StaticConstructObject_Internal]
//     NativeCreate -->|成功| Push[返回UObject到Lua]
//     NativeCreate -->|失败| CreateError[创建失败日志]
// ```

// ---

// ### **性能优化策略**

// | 优化点                | 实现方式                          | 效果提升                  |
// |-----------------------|-----------------------------------|--------------------------|
// | 延迟类型注册          | 按需加载反射信息                  | 减少启动耗时             |
// | 元表复用              | 已注册类型直接返回现有元表        | 避免重复初始化           |
// | 动态绑定作用域        | RAII模式控制绑定生命周期          | 防止元表污染             |
// | 名称前缀预判          | 首字母快速判断类型类别            | 加速类型匹配             |

// ---

// ### **错误处理机制**

// | 错误场景              | 检测方式                          | 处理策略                  |
// |-----------------------|-----------------------------------|--------------------------|
// | 无效UClass参数        | Cast<UClass>失败                  | 日志警告+返回nil         |
// | 蓝图类型误用          | 反射类型IsNative()检查            | 日志引导正确API使用      |
// | 异步加载未完成        | 对象有效性检查                    | 延迟到后续帧处理         |
// | 内存分配失败          | StaticConstructObject返回nullptr  | Lua栈返回nil+错误日志     |

// ---

// ### **设计亮点分析**

// 1. **无缝类型系统集成**：通过元表机制实现UE类型在Lua中的自然映射
// 2. **双模式类型加载**：
//    - **静态导出**：预注册的非反射类快速访问
//    - **动态反射**：运行时按需加载反射信息
// 3. **安全创建协议**：通过动态绑定作用域确保对象与Lua模块正确关联
// 4. **跨版本兼容**：处理不同引擎版本的API差异（如StaticConstructObject）
// 5. **智能类型推断**：基于命名约定的快速类型分类

// ---

// ### **典型使用示例**

// #### **Lua中创建Actor子类**
// ```lua
// local MyActorClass = UE.AActor -- 触发UE_Index加载AActor
// local actor = UE.NewObject(MyActorClass, world, "MyActor")
// actor:SetActorLocation(FVector(0,0,100))
// ```

// #### **动态加载资源**
// ```lua
// local TextureClass = UE.LoadClass("/Game/Textures/BaseTexture")
// local tex = UE.LoadObject(TextureClass, nil, "/Game/Textures/Concrete")
// ```

// #### **自定义类型绑定**
// ```cpp
// // C++导出非反射类
// class FMyStruct { ... };
// // Lua中访问
// local spec = UE.FMyStruct.new()
// ```

// ---

// ### **扩展性设计**

// 1. **自定义类型注册**：
//    - 通过`SetTableForClass`将新类型注入UE表
//    - 兼容静态导出与运行时注册两种方式

// 2. **模块化加载器**：
//    - `LoadObject/LoadClass`可扩展支持自定义加载策略
//    - 结合UnLua的模块定位器实现热重载

// 3. **元表继承系统**：
//    - 自动处理UObject继承链的元表链接
//    - 支持重写父类方法实现Lua特有行为

// 该实现是UnLua框架的UE类型系统门户，通过巧妙的元表机制和动态加载策略，在保持原生开发体验的同时，为Lua脚本提供了完整的UE类型操作能力，极大提升了脚本开发的灵活性和效率。



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

#include "UELib.h"
#include "Binding.h"
#include "Registries/ClassRegistry.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "LuaEnv.h"
#include "Registries/EnumRegistry.h"

static const char* REGISTRY_KEY = "UnLua_UELib";
static const char* NAMESPACE_NAME = "UE";

static int UE_Index(lua_State* L)
{
    const int32 Type = lua_type(L, 2);
    if (Type != LUA_TSTRING)
        return 0;

    const char* Name = lua_tostring(L, 2);
    const auto Exported = UnLua::FindExportedNonReflectedClass(Name);
    if (Exported)
    {
        Exported->Register(L);
        lua_rawget(L, 1);
        return 1;
    }

    const char Prefix = Name[0];
    const auto& Env = UnLua::FLuaEnv::FindEnvChecked(L);
    if (Prefix == 'U' || Prefix == 'A' || Prefix == 'F')
    {
        const auto ReflectedType = UnLua::FClassRegistry::LoadReflectedType(Name + 1);
        if (!ReflectedType)
            return 0;

        if (ReflectedType->IsNative())
        {
            if (auto Struct = Cast<UStruct>(ReflectedType))
                Env.GetClassRegistry()->Register(Struct);
        }
        else
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to load a blueprint type %s with UE namespace, use UE.UClass.Load or UE.UObject.Load instead."), UTF8_TO_TCHAR(Name));
            return 0;
        }
    }
    else if (Prefix == 'E')
    {
        const auto ReflectedType = UnLua::FClassRegistry::LoadReflectedType(Name);
        if (!ReflectedType)
            return 0;

        if (ReflectedType->IsNative())
        {
            if (auto Enum = Cast<UEnum>(ReflectedType))
                Env.GetEnumRegistry()->Register(Enum);
        }
        else
        {
            UE_LOG(LogUnLua, Warning, TEXT("attempt to load a blueprint enum %s with UE namespace, use UE.UObject.Load instead."), UTF8_TO_TCHAR(Name));
            return 0;
        }
    }

    lua_rawget(L, 1);
    return 1;
}

extern int32 UObject_Load(lua_State *L);
extern int32 UClass_Load(lua_State *L);
static int32 Global_NewObject(lua_State *L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 1)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UClass *Class = Cast<UClass>(UnLua::GetUObject(L, 1));
    if (!Class)
    {
        UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid class!"), ANSI_TO_TCHAR(__FUNCTION__));
        return 0;
    }

    UObject* Outer = UnLua::GetUObject(L, 2);
    if (!Outer)
        Outer = GetTransientPackage();

    FName Name = NumParams > 2 ? FName(lua_tostring(L, 3)) : NAME_None;
    //EObjectFlags Flags = NumParams > 3 ? EObjectFlags(lua_tointeger(L, 4)) : RF_NoFlags;

    {
        const char *ModuleName = NumParams > 3 ? lua_tostring(L, 4) : nullptr;
        int32 TableRef = LUA_NOREF;
        if (NumParams > 4 && lua_type(L, 5) == LUA_TTABLE)
        {
            lua_pushvalue(L, 5);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION < 26
        UObject* Object = StaticConstructObject_Internal(Class, Outer, Name);
#else
        FStaticConstructObjectParameters ObjParams(Class);
        ObjParams.Outer = Outer;
        ObjParams.Name = Name;
        UObject* Object = StaticConstructObject_Internal(ObjParams);
#endif
        if (Object)
        {
            UnLua::PushUObject(L, Object);
        }
        else
        {
            UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Failed to new object for class %s!"), ANSI_TO_TCHAR(__FUNCTION__), *Class->GetName());
            return 0;
        }
    }

    return 1;
}

static constexpr luaL_Reg UE_Functions[] = {
    {"LoadObject", UObject_Load},
    {"LoadClass", UClass_Load},
    {"NewObject", Global_NewObject},
    {NULL, NULL}
};

int UnLua::UELib::Open(lua_State* L)
{
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, UE_Index);
    lua_rawset(L, -3);

    lua_pushvalue(L, -1);
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_pushstring(L, REGISTRY_KEY);
    lua_rawset(L, LUA_REGISTRYINDEX);

    luaL_setfuncs(L, UE_Functions, 0);
    lua_setglobal(L, NAMESPACE_NAME);

    // global access for legacy support
    lua_getglobal(L, LUA_GNAME);
    luaL_setfuncs(L, UE_Functions, 0);
    lua_pop(L, 1);

#if WITH_UE4_NAMESPACE == 1
    // 兼容UE4访问
    lua_getglobal(L, NAMESPACE_NAME);
    lua_setglobal(L, "UE4");
#elif WITH_UE4_NAMESPACE == 0
    // 兼容无UE4全局访问
    lua_getglobal(L, LUA_GNAME);
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_getglobal(L, NAMESPACE_NAME);
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
#endif

    return 1;
}

void UnLua::UELib::SetTableForClass(lua_State* L, const char* Name)
{
    lua_getglobal(L, NAMESPACE_NAME);
    lua_pushstring(L, Name);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}
