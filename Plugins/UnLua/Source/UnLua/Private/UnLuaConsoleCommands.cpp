// 这段代码是UnLua模块的控制台命令实现，为开发者提供了在运行时与Lua环境交互的调试工具。以下是关键点的分步解析：

// 1. **命令注册机制**
// - 构造函数中创建三个控制台命令：
//   - `lua.do`：执行Lua代码片段
//   - `lua.dofile`：重新加载指定Lua模块
//   - `lua.gc`：手动触发Lua垃圾回收
// - 使用UE的`FConsoleCommandWithArgsDelegate`委托绑定成员函数

// 2. **命令实现细节**
// ```cpp
// // lua.do 实现
// void Do(const TArray<FString>& Args) {
//    Env->DoString(FString::Join(Args, TEXT(" "))); 
// }

// // lua.dofile 热重载实现
// FString Chunk = FString::Printf(TEXT("
//    package.loaded["%s"] = nil 
//    collectgarbage("collect") 
//    require("%s")
// "), *Args[0]);
// ```

// 3. **安全防护措施**
// - 参数校验：检查参数数量合法性
// - 环境检查：确认Lua环境有效后才执行操作
// - 资源清理：DoFile主动清除模块缓存和垃圾

// 4. **内存管理注意**
// - 使用`CreateRaw`需确保命令对象生命周期
// - 建议在析构函数中注销控制台命令
// - GC命令直接调用LuaEnv的强制回收接口

// 5. **典型使用场景**
// ```bash
// # 执行Lua代码
// lua.do print('Hello UnLua') 

// # 热重载角色模块
// lua.dofile Game.Character.Player 

// # 手动回收Lua内存
// lua.gc
// ```

// 6. **扩展建议**
// - 添加异常捕获显示详细错误栈
// - 支持多参数文件路径处理
// - 允许GC命令指定回收模式（如分步回收）
// - 增加执行超时保护机制

// 该代码为UnLua提供了重要的开发期调试能力，通过控制台命令实现快速测试和问题排查，显著提升脚本开发效率。需要注意命令绑定的生命周期管理，避免潜在的崩溃风险。


#include "UnLuaConsoleCommands.h"

#define LOCTEXT_NAMESPACE "UnLuaConsoleCommands"

namespace UnLua
{
    FUnLuaConsoleCommands::FUnLuaConsoleCommands(IUnLuaModule* InModule)
        : DoCommand(
              TEXT("lua.do"),
              *LOCTEXT("CommandText_Do", "Runs the given string in lua env.").ToString(),
              FConsoleCommandWithArgsDelegate::CreateRaw(this, &FUnLuaConsoleCommands::Do)
          ),
          DoFileCommand(
              TEXT("lua.dofile"),
              *LOCTEXT("CommandText_DoFile", "Runs the given module path in lua env.").ToString(),
              FConsoleCommandWithArgsDelegate::CreateRaw(this, &FUnLuaConsoleCommands::DoFile)
          ),
          CollectGarbageCommand(
              TEXT("lua.gc"),
              *LOCTEXT("CommandText_CollectGarbage", "Force collect garbage in lua env.").ToString(),
              FConsoleCommandWithArgsDelegate::CreateRaw(this, &FUnLuaConsoleCommands::CollectGarbage)
          ),
          Module(InModule)
    {
    }

    void FUnLuaConsoleCommands::Do(const TArray<FString>& Args) const
    {
        if (Args.Num() == 0)
        {
            UE_LOG(LogUnLua, Log, TEXT("usage: lua.do <your code>"));
            return;
        }

        auto Env = Module->GetEnv();
        if (!Env)
        {
            UE_LOG(LogUnLua, Warning, TEXT("no available lua env found to run code."));
            return;
        }

        const auto Chunk = FString::Join(Args, TEXT(" "));
        Env->DoString(Chunk);
    }

    void FUnLuaConsoleCommands::DoFile(const TArray<FString>& Args) const
    {
        if (Args.Num() != 1)
        {
            UE_LOG(LogUnLua, Log, TEXT("usage: lua.dofile <lua.module.path>"));
            return;
        }

        auto Env = Module->GetEnv();
        if (!Env)
        {
            UE_LOG(LogUnLua, Warning, TEXT("no available lua env found to run file."));
            return;
        }

        const auto& Format = TEXT(R"(
            local name = "%s"
            package.loaded[name] = nil
            collectgarbage("collect")
            require(name)
        )");
        const auto Chunk = FString::Printf(Format, *Args[0]);
        Env->DoString(Chunk);
    }

    void FUnLuaConsoleCommands::CollectGarbage(const TArray<FString>& Args) const
    {
        auto Env = Module->GetEnv();
        if (!Env)
        {
            UE_LOG(LogUnLua, Warning, TEXT("no available lua env found to collect garbage."));
            return;
        }

        Env->GC();
    }
}

#undef LOCTEXT_NAMESPACE
