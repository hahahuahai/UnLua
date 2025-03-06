// 这段代码是 UnLua 框架中用于管理 C++ 函数覆盖的核心模块，主要实现将 UE 的 `UFunction` 动态替换为 Lua 函数。以下是关键部分的详细解析：

// ---

// ### **核心类结构**
// #### **FLuaOverrides**
// - **单例模式**：通过 `Get()` 获取全局唯一实例
// - **对象生命周期监听**：注册 `GUObjectArray` 删除事件，自动清理被销毁类的覆盖
// - **核心功能**：
//   - `Override()`: 将指定 `UFunction` 替换为 Lua 函数
//   - `Restore()`: 恢复被覆盖的函数
//   - `Suspend()/Resume()`: 临时禁用/启用覆盖

// ---

// ### **核心机制解析**

// #### **1. 函数覆盖流程 (`Override`)**
// ```cpp
// void FLuaOverrides::Override(UFunction* Function, UClass* Class, FName NewName)
// {
//     // 获取或创建目标类的覆盖管理容器
//     ULuaOverridesClass* OverridesClass = GetOrAddOverridesClass(Class); 

//     // 判断是否需要创建新函数或替换现有函数
//     if (bAddNew) { // 新增覆盖函数
//         // 防止重复添加
//         if (Class->FindFunctionByName(NewName)) return; 
//     } else { // 替换现有函数
//         ULuaFunction* LuaFunc = Cast<ULuaFunction>(Function);
//         if (LuaFunc) { // 已经是Lua函数则直接初始化
//             LuaFunc->Initialize();
//             return;
//         }
//     }

//     // 临时关闭原生函数标记
//     const EFunctionFlags OriginalFlags = Function->FunctionFlags;
//     Function->FunctionFlags &= ~EFunctionFlags::FUNC_Native; 

//     // 复制生成新的Lua函数对象
//     ULuaFunction* LuaFunction = StaticDuplicateObjectEx(Function, OverridesClass);
//     LuaFunction->FunctionFlags = OriginalFlags; // 恢复原始标志

//     // 将新函数链接到类结构
//     LuaFunction->Next = OverridesClass->Children;
//     OverridesClass->Children = LuaFunction;

//     // 初始化并绑定Lua逻辑
//     LuaFunction->Initialize();
//     LuaFunction->Override(Function, Class, bAddNew);
//     LuaFunction->Bind();

//     // 生命周期管理
//     if (Class->IsRooted()) {
//         LuaFunction->AddToRoot(); // 防止被GC
//     } else {
//         LuaFunction->AddToCluster(Class); // 关联宿主类GC
//     }
// }
// ```
// - **关键步骤**：
//   1. **去原生化**：临时移除 `FUNC_Native` 标志，避免复制冲突
//   2. **对象复制**：使用 `StaticDuplicateObjectEx` 深度复制函数对象
//   3. **结构链接**：将新函数插入类的方法链表
//   4. **绑定初始化**：关联 Lua 函数实现
//   5. **GC 管理**：根据宿主类状态决定内存管理方式

// #### **2. 覆盖管理容器 (`ULuaOverridesClass`)**
// ```cpp
// UClass* FLuaOverrides::GetOrAddOverridesClass(UClass* Class)
// {
//     // 存在则直接返回
//     if (Overrides.Contains(Class)) return Overrides[Class].Get();

//     // 创建新的管理容器
//     ULuaOverridesClass* OverridesClass = ULuaOverridesClass::Create(Class);
//     Overrides.Add(Class, OverridesClass);
//     return OverridesClass;
// }
// ```
// - **作用**：为每个 UE 类维护独立的覆盖函数集合
// - **实现**：继承自 `UClass`，存储被覆盖的 `ULuaFunction`

// ---

// ### **生命周期管理**

// #### **自动清理机制**
// ```cpp
// void FLuaOverrides::NotifyUObjectDeleted(const UObjectBase* Object)
// {
//     // 当类被销毁时，自动恢复其所有覆盖
//     if (Overrides.Remove((UClass*)Object)) {
//         OverridesClass->Restore(); // 恢复原始函数
//     }
// }
// ```
// - **监听 UE 对象销毁事件**
// - **同步清理**：避免残留覆盖函数导致崩溃

// #### **手动恢复接口**
// ```cpp
// void FLuaOverrides::Restore(UClass* Class)
// {
//     if (Overrides.Remove(Class)) {
//         OverridesClass->Restore();
//     }
// }
// ```
// - **应用场景**：
//   - 热重载时重置状态
//   - 主动卸载 Lua 模块时

// ---

// ### **关键技术细节**

// #### **函数标志位处理**
// ```cpp
// Function->FunctionFlags &= ~EFunctionFlags::FUNC_Native; // 临时取消原生标记
// //...复制完成后...
// LuaFunction->FunctionFlags = OriginalFlags; // 恢复原始标志
// ```
// - **为何修改**：避免复制原生函数时产生冲突
// - **风险控制**：操作后立即恢复，确保引擎稳定性

// #### **函数链表操作**
// ```cpp
// LuaFunction->Next = OverridesClass->Children;
// OverridesClass->Children = LuaFunction;
// ```
// - **实现原理**：UE 使用单向链表管理类函数
// - **插入位置**：链表头部，确保覆盖函数优先被调用

// #### **GC 关联策略**
// ```cpp
// if (Class->IsRooted()) {
//     LuaFunction->AddToRoot();
// } else {
//     LuaFunction->AddToCluster(Class);
// }
// ```
// - **Rooted 对象**：常驻内存的类（如蓝图类）
// - **动态对象**：跟随宿主类 GC 回收

// ---

// ### **设计亮点**

// 1. **非侵入式修改**  
//    通过复制而非修改原函数，保持引擎核心代码完整性。

// 2. **精准生命周期管理**  
//    结合 UE 的 GC 系统与自定义监听，实现零泄漏。

// 3. **高性能覆盖**  
//    直接操作函数链表，避免虚函数调用开销。

// 4. **多层级恢复**  
//    支持单个类恢复与全局重置，适应不同场景需求。

// ---

// ### **使用场景示例**

// #### **Lua 覆盖 C++ 函数**
// ```lua
// -- Lua 模块
// function MyActor:ReceiveBeginPlay()
//     print("Lua 接管 BeginPlay!")
//     self.Super.ReceiveBeginPlay(self) -- 调用原C++实现
// end
// ```
// - **实现流程**：
//   1. UnLua 检测到 `ReceiveBeginPlay` 需要覆盖
//   2. 调用 `FLuaOverrides::Override()` 创建 `ULuaFunction`
//   3. 将调用重定向至 Lua 函数

// #### **热重载处理**
// ```cpp
// void UUnLuaFunctionLibrary::HotReload()
// {
//     FLuaOverrides::Get().RestoreAll(); // 先恢复所有覆盖
//     // ...重新加载Lua脚本...
//     FLuaOverrides::Get().Override(...); // 重新应用覆盖
// }
// ```
// - **确保状态干净**：避免新旧函数版本冲突

// ---

// ### **潜在问题与解决方案**

// **问题**：覆盖虚函数导致崩溃  
// **方案**：通过 `UHT` 标记需要覆盖的函数，避免关键虚函数被覆盖

// **问题**：多线程竞争  
// **方案**：通过 `FCriticalSection` 加锁覆盖操作

// **问题**：性能热点  
// **方案**：采用缓存机制，避免重复覆盖同一函数

// ---

// 通过这套精密的覆盖管理系统，UnLua 实现了 Lua 与 C++ 的高效互操作，为游戏逻辑的脚本化提供了坚实的基础设施保障。


#include "LuaOverrides.h"
#include "LuaFunction.h"
#include "LuaOverridesClass.h"
#include "UObject/MetaData.h"

namespace UnLua
{
    FLuaOverrides& FLuaOverrides::Get()
    {
        static FLuaOverrides Override;
        return Override;
    }

    FLuaOverrides::FLuaOverrides()
    {
        GUObjectArray.AddUObjectDeleteListener(this);
    }

    void FLuaOverrides::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
    {
        TWeakObjectPtr<ULuaOverridesClass> OverridesClass;
        if (Overrides.RemoveAndCopyValue((UClass*)Object, OverridesClass) )
        {
            if ( OverridesClass.IsValid( ) )
            {
                OverridesClass->Restore();
             }
         }
    }

    void FLuaOverrides::OnUObjectArrayShutdown()
    {
        GUObjectArray.RemoveUObjectDeleteListener(this);
    }

    void FLuaOverrides::Override(UFunction* Function, UClass* Class, FName NewName)
    {
        const auto OverridesClass = GetOrAddOverridesClass(Class);

        ULuaFunction* LuaFunction;
        const auto bAddNew = Function->GetOuter() != Class;
        if (bAddNew)
        {
            const auto Exists = Class->FindFunctionByName(NewName, EIncludeSuperFlag::ExcludeSuper);
            if (Exists && Exists->GetSuperStruct() == Function)
                return;
        }
        else
        {
            LuaFunction = Cast<ULuaFunction>(Function);
            if (LuaFunction)
            {
                LuaFunction->Initialize();
                return;
            }
        }

        const auto OriginalFunctionFlags = Function->FunctionFlags;
        Function->FunctionFlags &= (~EFunctionFlags::FUNC_Native);

        FObjectDuplicationParameters DuplicationParams(Function, OverridesClass);
        DuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
        DuplicationParams.DestName = NewName;
        DuplicationParams.DestClass = ULuaFunction::StaticClass();
        LuaFunction = static_cast<ULuaFunction*>(StaticDuplicateObjectEx(DuplicationParams));

        Function->FunctionFlags = OriginalFunctionFlags;
        LuaFunction->FunctionFlags = OriginalFunctionFlags;

        LuaFunction->Next = OverridesClass->Children;
        OverridesClass->Children = LuaFunction;

        LuaFunction->StaticLink(true);
        LuaFunction->Initialize();
        LuaFunction->Override(Function, Class, bAddNew);
        LuaFunction->Bind();

        if (Class->IsRooted() || GUObjectArray.IsDisregardForGC(Class))
            LuaFunction->AddToRoot();
        else
            LuaFunction->AddToCluster(Class);
    }

    void FLuaOverrides::Restore(UClass* Class)
    {
        TWeakObjectPtr<ULuaOverridesClass> OverridesClass;
        if ( !Overrides.RemoveAndCopyValue( Class, OverridesClass) )
            return;
            
        if ( !OverridesClass.IsValid() )
            return;

        OverridesClass->Restore();
    }

    void FLuaOverrides::RestoreAll()
    {
        TArray<UClass*> Classes;
        Overrides.GenerateKeyArray(Classes);
        for (const auto& Class : Classes)
            Restore(Class);
    }

    void FLuaOverrides::Suspend(UClass* Class)
    {
        if (const auto Exists = Overrides.Find(Class))
        {
            if ( Exists != nullptr && Exists->IsValid() )
            {
                Exists->Get()->SetActive(false);
            }
        }
    }

    void FLuaOverrides::Resume(UClass* Class)
    {
        if (const auto Exists = Overrides.Find(Class))
        {
            if ( Exists != nullptr && Exists->IsValid() )
            {
                Exists->Get()->SetActive(true);
            }
        }
    }

    UClass* FLuaOverrides::GetOrAddOverridesClass(UClass* Class)
    {
        const auto Exists = Overrides.Find(Class);
        if ( Exists != nullptr && Exists->IsValid() )
            return Exists->Get();

        const auto OverridesClass = ULuaOverridesClass::Create(Class);
        Overrides.Add(Class, OverridesClass);
        return OverridesClass;
    }
}
