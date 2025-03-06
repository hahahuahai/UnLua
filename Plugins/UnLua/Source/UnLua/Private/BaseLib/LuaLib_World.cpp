// 这段代码是 UnLua 框架中用于扩展 UE 的 `UWorld` 类功能的核心模块，主要实现通过 Lua 脚本生成 Actor 的能力。以下是各部分的详细解析：

// ---

// ### **1. Actor 生成函数**

// #### **UWorld_SpawnActor**
// ```cpp
// static int32 UWorld_SpawnActor(lua_State* L)
// ```
// - **功能**：完整的 Actor 生成接口，支持 10 个参数
// - **参数列表**：
//   1. `World`：目标世界
//   2. `Class`：生成的 Actor 类
//   3. `Transform`：初始变换（可选）
//   4. `CollisionHandling`：碰撞处理方式（可选）
//   5. `Owner`：拥有者 Actor（可选）
//   6. `Instigator`：触发者 Actor（可选）
//   7. `LuaModule`：关联的 Lua 模块名（可选）
//   8. `InitializationTable`：初始化参数表（可选）
//   9. `Level`：生成层级（可选）
//   10. `Name`：Actor 名称（可选）

// - **关键逻辑**：
//   - 使用 `FScopedLuaDynamicBinding` 临时绑定 Lua 模块
//   - 调用 `World->SpawnActor` 生成 Actor
//   - 支持通过 Lua 表传递初始化参数

// #### **UWorld_SpawnActorEx**
// ```cpp
// static int32 UWorld_SpawnActorEx(lua_State* L)
// ```
// - **功能**：简化版生成接口，参数更集中
// - **参数列表**：
//   1. `World`：目标世界
//   2. `Class`：生成的 Actor 类
//   3. `Transform`：初始变换（可选）
//   4. `InitializationTable`：初始化参数表（可选）
//   5. `LuaModule`：关联的 Lua 模块名（可选）
//   6. `SpawnParameters`：高级生成参数（可选）

// ---

// ### **2. FActorSpawnParameters 的 Lua 绑定**
// ```cpp
// BEGIN_EXPORT_CLASS(FActorSpawnParameters)
//     ADD_PROPERTY(Name)
//     ADD_PROPERTY(Template)
//     //... 其他属性和方法
// END_EXPORT_CLASS()
// ```
// - **作用**：将 UE 的 `FActorSpawnParameters` 结构体暴露给 Lua
// - **支持属性**：
//   - `Name`：Actor 名称
//   - `Owner`：拥有者
//   - `Instigator`：触发者
//   - 碰撞处理标志位等
// - **版本控制**：通过 `ENGINE_MAJOR_VERSION` 处理不同引擎版本差异

// ---

// ### **3. 类型导出宏**
// #### **DEFINE_TYPE**
// ```cpp
// DEFINE_TYPE(ESpawnActorCollisionHandlingMethod)
// ```
// - **作用**：将 UE 枚举类型暴露到 Lua
// - **使用场景**：在 Lua 中可以直接访问碰撞处理方式枚举值
//   ```lua
//   ESpawnActorCollisionHandlingMethod.AlwaysSpawn
//   ```

// #### **BEGIN_EXPORT_REFLECTED_CLASS**
// ```cpp
// BEGIN_EXPORT_REFLECTED_CLASS(UWorld)
//     ADD_LIB(UWorldLib)
//     ADD_FUNCTION(GetTimeSeconds)
// END_EXPORT_CLASS()
// ```
// - **功能**：扩展 `UWorld` 类的 Lua 接口
// - **新增方法**：
//   - `SpawnActor` / `SpawnActorEx`：通过 `UWorldLib` 添加
//   - `GetTimeSeconds`：原生方法直接绑定

// ---

// ### **4. 错误处理与安全校验**
// ```cpp
// if (!World) return luaL_error(L, "invalid world");
// if (!Class) return luaL_error(L, "invalid actor class");
// ```
// - **参数校验**：严格检查前两个参数的有效性
// - **类型安全**：使用 `Cast<>` 确保对象类型正确
// - **Lua 栈保护**：通过 `luaL_ref` 管理临时表的引用

// ---

// ### **5. 动态绑定机制**
// ```cpp
// FScopedLuaDynamicBinding Binding(L, Class, ModuleName, TableRef);
// ```
// - **作用**：在生成 Actor 时临时绑定 Lua 逻辑
// - **生命周期**：作用域结束时自动解绑
// - **典型应用**：
//   - 覆盖 Actor 的初始化逻辑
//   - 动态注入 Lua 组件

// ---

// ### **6. 参数处理技巧**
// #### **可选参数处理**
// ```cpp
// if (NumParams > 2) { /* 处理 Transform */ }
// if (NumParams > 3) { /* 处理碰撞方式 */ }
// ```
// - **灵活参数**：支持从 Lua 传递不同数量的参数
// - **默认值机制**：未传递参数使用 UE 默认值

// #### **Lua 表初始化**
// ```lua
// World:SpawnActor(Class, Transform, {
//     Color = {R=1,G=0,B=0},
//     Health = 100
// })
// ```
// - **表解析**：通过 `lua_type(L, 8) == LUA_TTABLE` 检测并处理初始化表

// ---

// ### **7. 性能优化点**
// 1. **减少 Lua/C++ 交互**：批量处理参数而非逐字段获取
// 2. **引用计数管理**：使用 `LUA_REGISTRYINDEX` 避免重复创建对象
// 3. **类型缓存**：通过 `DEFINE_TYPE` 预生成类型信息

// ---

// ### **使用示例**
// #### **Lua 中生成 Actor**
// ```lua
// local World = GWorld:GetWorld()
// local Class = UE.UClass.Load("/Game/Blueprints/BP_Enemy.BP_Enemy_C")
// local Transform = UE.FTransform(UE.FVector(0,0,100))

// -- 简单生成
// local Enemy = World:SpawnActor(Class, Transform)

// -- 完整参数
// local Boss = World:SpawnActor(
//     Class, 
//     Transform, 
//     UE.ESpawnActorCollisionHandlingMethod.AlwaysSpawn,
//     Player, 
//     nil, 
//     "NPC.BossAI", 
//     {Health=5000, Level=5}
// )
// ```

// ---

// ### **总结**
// 这段代码实现了以下核心功能：
// 1. **灵活生成接口**：支持多种参数组合的 Actor 生成
// 2. **类型安全绑定**：严格校验并转换 Lua/C++ 对象
// 3. **动态逻辑注入**：通过临时绑定实现 Lua 逻辑覆盖
// 4. **跨版本兼容**：处理不同 UE 引擎版本的差异

// 通过这套机制，开发者可以在 Lua 脚本中以接近原生 C++ 的性能操作 UE 的 Actor 生成系统，同时享受 Lua 的灵活性和热重载优势。


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

#include "UnLuaEx.h"
#include "LuaCore.h"
#include "LuaDynamicBinding.h"
#include "Engine/World.h"

/**
 * Spawn an actor.
 * for example:
 * World:SpawnActor(
 *  WeaponClass, InitialTransform, ESpawnActorCollisionHandlingMethod.AlwaysSpawn,
 *  OwnerActor, Instigator, "Weapon.AK47_C", WeaponColor, ULevel, Name
 * )
 * the last four parameters "Weapon.AK47_C", 'WeaponColor', ULevel and Name are optional.
 * see programming guide for detail.
 */
static int32 UWorld_SpawnActor(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
        return luaL_error(L, "invalid parameters");

    UWorld* World = Cast<UWorld>(UnLua::GetUObject(L, 1));
    if (!World)
        return luaL_error(L, "invalid world");

    UClass* Class = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (!Class)
        return luaL_error(L, "invalid actor class");

    FTransform Transform;
    if (NumParams > 2)
    {
        FTransform* TransformPtr = (FTransform*)GetCppInstanceFast(L, 3);
        if (TransformPtr)
        {
            Transform = *TransformPtr;
        }
    }

    FActorSpawnParameters SpawnParameters;
    if (NumParams > 3)
    {
        uint8 CollisionHandlingOverride = (uint8)lua_tointeger(L, 4);
        SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)CollisionHandlingOverride;
    }
    if (NumParams > 4)
    {
        AActor* Owner = Cast<AActor>(UnLua::GetUObject(L, 5));
        check(!Owner || (Owner && World == Owner->GetWorld()));
        SpawnParameters.Owner = Owner;
    }
    if (NumParams > 5)
    {
        AActor* Actor = Cast<AActor>(UnLua::GetUObject(L, 6));
        if (Actor)
        {
            APawn* Instigator = Cast<APawn>(Actor);
            if (!Instigator)
            {
                Instigator = Actor->GetInstigator();
            }
            SpawnParameters.Instigator = Instigator;
        }
    }

    if (NumParams > 8)
    {
        ULevel* Level = Cast<ULevel>(UnLua::GetUObject(L, 9));
        if (Level)
        {
            SpawnParameters.OverrideLevel = Level;
        }
    }

    if (NumParams > 9)
    {
        SpawnParameters.Name = FName(lua_tostring(L, 10));
    }

    {
        const char* ModuleName = NumParams > 6 ? lua_tostring(L, 7) : nullptr;
        int32 TableRef = LUA_NOREF;
        if (NumParams > 7 && lua_type(L, 8) == LUA_TTABLE)
        {
            lua_pushvalue(L, 8);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
        AActor* NewActor = World->SpawnActor(Class, &Transform, SpawnParameters);
        UnLua::PushUObject(L, NewActor);
    }

    return 1;
}

/**
 * World:SpawnActorEx(
 *  WeaponClass, InitialTransform, WeaponColor, "Weapon.AK47_C", ActorSpawnParameters
 * )
 */
static int32 UWorld_SpawnActorEx(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams < 2)
        return luaL_error(L, "invalid parameters");

    UWorld* World = Cast<UWorld>(UnLua::GetUObject(L, 1));
    if (!World)
        return luaL_error(L, "invalid world");

    UClass* Class = Cast<UClass>(UnLua::GetUObject(L, 2));
    if (!Class)
        return luaL_error(L, "invalid class");

    FTransform Transform;
    if (NumParams > 2)
    {
        FTransform* TransformPtr = (FTransform*)GetCppInstanceFast(L, 3);
        if (TransformPtr)
        {
            Transform = *TransformPtr;
        }
    }

    {
        int32 TableRef = LUA_NOREF;
        if (NumParams > 3 && lua_type(L, 4) == LUA_TTABLE)
        {
            lua_pushvalue(L, 4);
            TableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        const char* ModuleName = NumParams > 4 ? lua_tostring(L, 5) : nullptr;
        FActorSpawnParameters SpawnParameters;
        if (NumParams > 5)
        {
            FActorSpawnParameters* ActorSpawnParametersPtr = (FActorSpawnParameters*)GetCppInstanceFast(L, 6);
            if (ActorSpawnParametersPtr)
            {
                SpawnParameters = *ActorSpawnParametersPtr;
            }
        }
        FScopedLuaDynamicBinding Binding(L, Class, UTF8_TO_TCHAR(ModuleName), TableRef);
        AActor* NewActor = World->SpawnActor(Class, &Transform, SpawnParameters);
        UnLua::PushUObject(L, NewActor);
    }

    return 1;
}

DEFINE_TYPE(ESpawnActorCollisionHandlingMethod)

DEFINE_TYPE(EObjectFlags)

DEFINE_TYPE(FActorSpawnParameters::ESpawnActorNameMode)

BEGIN_EXPORT_CLASS(FActorSpawnParameters)
    ADD_PROPERTY(Name)
    ADD_PROPERTY(Template)
    ADD_PROPERTY(Owner)
    ADD_PROPERTY(Instigator)
    ADD_PROPERTY(OverrideLevel)
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    ADD_PROPERTY(OverridePackage)
    ADD_PROPERTY(OverrideParentComponent)
    ADD_PROPERTY(OverrideActorGuid)
#endif
#endif
    ADD_PROPERTY(SpawnCollisionHandlingOverride)
    ADD_FUNCTION(IsRemoteOwned)
    ADD_BITFIELD_BOOL_PROPERTY(bNoFail)
    ADD_BITFIELD_BOOL_PROPERTY(bDeferConstruction)
    ADD_BITFIELD_BOOL_PROPERTY(bAllowDuringConstructionScript)
#if WITH_EDITOR
    ADD_BITFIELD_BOOL_PROPERTY(bTemporaryEditorActor)
    ADD_BITFIELD_BOOL_PROPERTY(bHideFromSceneOutliner)
#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
    ADD_BITFIELD_BOOL_PROPERTY(bCreateActorPackage)
#endif
    ADD_PROPERTY(NameMode)
    ADD_PROPERTY(ObjectFlags)
#endif
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(FActorSpawnParameters)

static const luaL_Reg UWorldLib[] =
{
    {"SpawnActor", UWorld_SpawnActor},
    {"SpawnActorEx", UWorld_SpawnActorEx},
    {nullptr, nullptr}
};

BEGIN_EXPORT_REFLECTED_CLASS(UWorld)
    ADD_LIB(UWorldLib)
    ADD_FUNCTION(GetTimeSeconds)
END_EXPORT_CLASS()

IMPLEMENT_EXPORTED_CLASS(UWorld)
