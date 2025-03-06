// 这段代码是 UnLua 框架的核心库模块，主要实现 Lua 与 Unreal Engine 的深度集成，包含日志系统、热重载、对象引用管理、属性访问及旧版兼容支持。以下是对代码的详细解析：

// ---

// ### **1. 日志系统**
// - **函数注册**：将 Lua 的 `print` 函数重定向到 UE 的日志系统。
//   ```cpp
//   lua_register(L, "print", LogInfo);
//   ```
// - **日志分级**：
//   - `LogInfo`: 普通信息（`LogUnLua::Log`）
//   - `LogWarn`: 警告信息（`LogUnLua::Warning`）
//   - `LogError`: 错误信息（`LogUnLua::Error`）
// - **多参数处理**：`GetMessage` 函数将多个 Lua 参数拼接为字符串，支持自动类型转换。

// ---

// ### **2. 热重载机制**
// - **条件编译**：通过 `UNLUA_WITH_HOT_RELOAD` 宏控制热重载功能。
//   ```cpp
//   #if UNLUA_WITH_HOT_RELOAD
//   luaL_dostring(L, "_G.require = require('UnLua.HotReload').require");
//   #endif
//   ```
// - **热重载触发**：`HotReload` 函数调用 Lua 模块的热重载逻辑。
//   ```cpp
//   luaL_dostring(L, "require('UnLua.HotReload').reload()");
//   ```
// - **动态替换 `require`**：重写全局 `require` 函数，使其支持模块更新检测。

// ---

// ### **3. 对象引用管理**
// - **手动引用计数**：
//   - `Ref`: 增加对象的引用计数，防止被 GC 回收。
//     ```cpp
//     Env.GetObjectRegistry()->AddManualRef(L, Object);
//     ```
//   - `Unref`: 减少引用计数，允许 GC 回收。
//     ```cpp
//     Env.GetObjectRegistry()->RemoveManualRef(Object);
//     ```
// - **有效性检查**：`GetUObject` 检测对象是否有效，避免操作已释放对象。

// ---

// ### **4. 属性访问 (Legacy)**
// - **元方法拦截**：通过 `__index` 和 `__newindex` 元表方法拦截属性访问。
//   ```lua
//   function Index(t, k)
//       local p = mt[k]
//       if type(p) == "userdata" then
//           return GetUProperty(t, p)
//       end
//   end
//   ```
// - **C++ 属性操作**：
//   - `GetUProperty`: 读取 UE 属性值到 Lua。
//   - `SetUProperty`: 将 Lua 值写入 UE 属性。
// - **类型安全**：`CheckPropertyOwner` 确保属性属于目标对象，防止内存越界。

// ---

// ### **5. 类系统兼容层**
// - **传统类定义**：提供 `Class` 函数模拟继承。
//   ```lua
//   local MyClass = Class("ParentClass")
//   ```
// - **继承链查找**：通过 `Super` 字段实现父类方法查找。
//   ```lua
//   while super do
//       local v = rawget(super, k)
//       super = rawget(super, "Super")
//   end
//   ```

// ---

// ### **6. 模块路径管理**
// - **默认路径**：设置 Lua 模块搜索路径。
//   ```cpp
//   lua_pushstring(L, "Content/Script/?.lua;Plugins/UnLua/Content/Script/?.lua");
//   lua_setfield(L, -2, PACKAGE_PATH_KEY);
//   ```
// - **动态配置**：`SetPackagePath` 允许运行时修改模块路径。
//   ```cpp
//   void SetPackagePath(lua_State* L, const FString& Path);
//   ```

// ---

// ### **7. FText 支持**
// - **编译开关**：通过 `UNLUA_ENABLE_FTEXT` 宏控制 FText 功能。
//   ```cpp
//   #if UNLUA_ENABLE_FTEXT
//   luaL_dostring(L, "UnLua.FTextEnabled = true");
//   #endif
//   ```

// ---

// ### **8. 核心初始化流程**
// 1. **注册全局表**：创建 `UnLua` 全局表并填充功能函数。
// 2. **替换原生函数**：覆盖 `print` 以使用 UE 日志系统。
// 3. **加载子模块**：通过元表实现按需加载子模块（如 `UnLua.HotReload`）。
// 4. **旧版支持**：注入传统类系统和属性访问逻辑。

// ---

// ### **关键设计亮点**
// - **无缝日志集成**：Lua 日志直接输出到 UE 控制台，便于调试。
// - **安全热重载**：确保脚本更新后游戏状态的一致性。
// - **高效引用管理**：手动控制关键对象生命周期，避免 GC 问题。
// - **灵活路径配置**：支持多项目目录结构，便于模块化开发。
// - **渐进式兼容**：保留旧版 API 的同时提供现代化替代方案。

// ---

// ### **使用示例**
// - **Lua 日志输出**：
//   ```lua
//   print("玩家进入游戏", PlayerName)  -- 输出到 UE 日志
//   ```
// - **热重载模块**：
//   ```lua
//   UnLua.HotReload()  -- 重新加载所有 Lua 脚本
//   ```
// - **引用对象**：
//   ```lua
//   local ref = UnLua.Ref(MyActor)  -- 防止 MyActor 被 GC
//   -- ... 使用 MyActor ...
//   UnLua.Unref(MyActor)  -- 允许回收
//   ```

// ---

// ### **注意事项**
// 1. **性能敏感操作**：频繁的属性访问（如每帧）建议缓存结果。
// 2. **热重载限制**：涉及状态保存的模块需自行处理数据迁移。
// 3. **引用泄漏**：确保 `Ref` 和 `Unref` 成对调用，避免内存泄漏。
// 4. **旧版代码迁移**：逐步替换 `Class` 语法为原生 Lua 面向对象方案。

// 通过上述机制，该模块为 UE 项目提供了强大的 Lua 集成能力，显著提升了开发效率和运行时灵活性。



#include "UnLuaLib.h"
#include "LowLevel.h"
#include "LuaEnv.h"
#include "UnLuaBase.h"

namespace UnLua
{
    namespace UnLuaLib
    {
        static const char* PACKAGE_PATH_KEY = "PackagePath";

        static FString GetMessage(lua_State* L)
        {
            const auto ArgCount = lua_gettop(L);
            FString Message;
            if (!lua_checkstack(L, ArgCount))
            {
                luaL_error(L, "too many arguments, stack overflow");
                return Message;
            }
            for (int ArgIndex = 1; ArgIndex <= ArgCount; ArgIndex++)
            {
                if (ArgIndex > 1)
                    Message += "\t";
                Message += UTF8_TO_TCHAR(luaL_tolstring(L, ArgIndex, NULL));
            }
            return Message;
        }

        static int LogInfo(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Log, TEXT("%s"), *Msg);
            return 0;
        }

        static int LogWarn(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Warning, TEXT("%s"), *Msg);
            return 0;
        }

        static int LogError(lua_State* L)
        {
            const auto Msg = GetMessage(L);
            UE_LOG(LogUnLua, Error, TEXT("%s"), *Msg);
            return 0;
        }

        static int HotReload(lua_State* L)
        {
#if UNLUA_WITH_HOT_RELOAD
            if (luaL_dostring(L, "require('UnLua.HotReload').reload()") != 0)
            {
                LogError(L);
            }
#endif
            return 0;
        }

        static int Ref(lua_State* L)
        {
            const auto Object = GetUObject(L, -1);
            if (!Object)
                return luaL_error(L, "invalid UObject");

            const auto& Env = FLuaEnv::FindEnvChecked(L);
            Env.GetObjectRegistry()->AddManualRef(L, Object);
            return 1;
        }

        static int Unref(lua_State* L)
        {
            const auto Object = GetUObject(L, -1);
            if (!Object)
                return luaL_error(L, "invalid UObject");

            const auto& Env = FLuaEnv::FindEnvChecked(L);
            Env.GetObjectRegistry()->RemoveManualRef(Object);
            return 0;
        }

        static constexpr luaL_Reg UnLua_Functions[] = {
            {"Log", LogInfo},
            {"LogWarn", LogWarn},
            {"LogError", LogError},
            {"HotReload", HotReload},
            {"Ref", Ref},
            {"Unref", Unref},
            {"FTextEnabled", nullptr},
            {NULL, NULL}
        };

#pragma region Legacy Support

        int32 GetUProperty(lua_State* L)
        {
            auto Ptr = lua_touserdata(L, 2);
            if (!Ptr)
                return 0;

            auto Property = static_cast<TSharedPtr<UnLua::ITypeOps>*>(Ptr);
            if (!Property->IsValid())
                return 0;

            auto Self = GetCppInstance(L, 1);
            if (!Self)
                return 0;

            if (UnLua::LowLevel::IsReleasedPtr(Self))
                return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("attempt to read property '%s' on released object"), *(*Property)->GetName())));

            if (!LowLevel::CheckPropertyOwner(L, (*Property).Get(), Self))
                return 0;

            (*Property)->ReadValue_InContainer(L, Self, false);
            return 1;
        }

        int32 SetUProperty(lua_State* L)
        {
            auto Ptr = lua_touserdata(L, 2);
            if (!Ptr)
                return 0;

            auto Property = static_cast<TSharedPtr<UnLua::ITypeOps>*>(Ptr);
            if (!Property->IsValid())
                return 0;

            auto Self = GetCppInstance(L, 1);
            if (LowLevel::IsReleasedPtr(Self))
                return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("attempt to write property '%s' on released object"), *(*Property)->GetName())));

            if (!LowLevel::CheckPropertyOwner(L, (*Property).Get(), Self))
                return 0;

            (*Property)->WriteValue_InContainer(L, Self, 3);
            return 0;
        }

        static constexpr luaL_Reg UnLua_LegacyFunctions[] = {
            {"GetUProperty", GetUProperty},
            {"SetUProperty", SetUProperty},
            {NULL, NULL}
        };

        static void LegacySupport(lua_State* L)
        {
            static const char* Chunk = R"(
            local rawget = _G.rawget
            local rawset = _G.rawset
            local rawequal = _G.rawequal
            local type = _G.type
            local getmetatable = _G.getmetatable
            local require = _G.require

            local GetUProperty = GetUProperty
            local SetUProperty = SetUProperty

            local NotExist = {}

            local function Index(t, k)
                local mt = getmetatable(t)
                local super = mt
                while super do
                    local v = rawget(super, k)
                    if v ~= nil and not rawequal(v, NotExist) then
                        rawset(t, k, v)
                        return v
                    end
                    super = rawget(super, "Super")
                end

                local p = mt[k]
                if p ~= nil then
                    if type(p) == "userdata" then
                        return GetUProperty(t, p)
                    elseif type(p) == "function" then
                        rawset(t, k, p)
                    elseif rawequal(p, NotExist) then
                        return nil
                    end
                else
                    rawset(mt, k, NotExist)
                end

                return p
            end

            local function NewIndex(t, k, v)
                local mt = getmetatable(t)
                local p = mt[k]
                if type(p) == "userdata" then
                    return SetUProperty(t, p, v)
                end
                rawset(t, k, v)
            end

            local function Class(super_name)
                local super_class = nil
                if super_name ~= nil then
                    super_class = require(super_name)
                end

                local new_class = {}
                new_class.__index = Index
                new_class.__newindex = NewIndex
                new_class.Super = super_class

              return new_class
            end

            _G.Class = Class
            _G.GetUProperty = GetUProperty
            _G.SetUProperty = SetUProperty
            )";

            lua_register(L, "UEPrint", LogInfo);
            luaL_loadstring(L, Chunk);
            lua_newtable(L);
            lua_getglobal(L, LUA_GNAME);
            lua_setfield(L, -2, LUA_GNAME);
            luaL_setfuncs(L, UnLua_LegacyFunctions, 0);
            lua_setupvalue(L, -2, 1);
            lua_pcall(L, 0, LUA_MULTRET, 0);
            lua_getglobal(L, "Class");
            lua_setfield(L, -2, "Class");
        }

#pragma endregion

        static int LuaOpen(lua_State* L)
        {
            lua_newtable(L);
            luaL_setfuncs(L, UnLua_Functions, 0);
            lua_pushstring(L, "Content/Script/?.lua;Plugins/UnLua/Content/Script/?.lua");
            lua_setfield(L, -2, PACKAGE_PATH_KEY);
            return 1;
        }

        int Open(lua_State* L)
        {
            lua_register(L, "print", LogInfo);
            luaL_requiref(L, "UnLua", LuaOpen, 1);
            luaL_dostring(L, R"(
                setmetatable(UnLua, {
                    __index = function(t, k)
                        local ok, result = pcall(require, "UnLua." .. tostring(k))
                        if ok then
                            rawset(t, k, result)
                            return result
                        else
                            t.LogWarn(string.format("failed to load module UnLua.%s\n%s", k, result))
                        end
                    end
                })
            )");

#if UNLUA_ENABLE_FTEXT
            luaL_dostring(L, "UnLua.FTextEnabled = true");
#else
            luaL_dostring(L, "UnLua.FTextEnabled = false");
#endif

#if UNLUA_WITH_HOT_RELOAD
            luaL_dostring(L, R"(
                pcall(function() _G.require = require('UnLua.HotReload').require end)
            )");
#endif

            LegacySupport(L);
            lua_pop(L, 1);
            return 1;
        }

        FString GetPackagePath(lua_State* L)
        {
            lua_getglobal(L, "UnLua");
            checkf(lua_istable(L, -1), TEXT("UnLuaLib not registered"));
            lua_getfield(L, -1, PACKAGE_PATH_KEY);
            const auto PackagePath = lua_tostring(L, -1);
            checkf(PackagePath, TEXT("invalid PackagePath"));
            lua_pop(L, 2);
            return UTF8_TO_TCHAR(PackagePath);
        }

        void SetPackagePath(lua_State* L, const FString& PackagePath)
        {
            lua_getglobal(L, "UnLua");
            checkf(lua_istable(L, -1), TEXT("UnLuaLib not registered"));
            lua_pushstring(L, TCHAR_TO_UTF8(*PackagePath));
            lua_setfield(L, -2, PACKAGE_PATH_KEY);
            lua_pop(L, 2);
        }
    }
}
