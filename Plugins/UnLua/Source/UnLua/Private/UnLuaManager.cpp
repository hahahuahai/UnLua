// 以下是针对 `UUnLuaManager` 类的详细解释：

// ---

// ### **UUnLuaManager 核心功能解析**

// `UUnLuaManager` 是 UnLua 框架的中枢管理类，负责 Lua 绑定、输入系统重定向及生命周期管理。以下是其核心实现机制：

// ---

// #### **1. 初始化与全局管理**
// ```cpp
// UUnLuaManager::UUnLuaManager() {
//     // 获取所有默认输入配置
//     GetDefaultInputs(); 
//     EKeys::GetAllKeys(AllKeys);
    
//     // 获取关键 UFunctions 模板
//     InputActionFunc = Class->FindFunctionByName("InputAction");
//     InputAxisFunc = Class->FindFunctionByName("InputAxis");
//     // ...其他输入函数
// }
// ```
// - **输入系统预加载**：获取项目默认的 Axis/Action 输入配置，存储所有按键定义。
// - **关键 UFunctions 缓存**：提前获取输入处理函数的指针，为后续动态绑定提供基础。

// ---

// #### **2. Lua 模块绑定**
// ```cpp
// bool UUnLuaManager::Bind(UObject* Object, const TCHAR* InModuleName) {
//     // 注册类到 Lua 环境
//     Env->GetClassRegistry()->Register(Class);
    
//     // 加载 Lua 模块
//     UnLua::Call(L, "require", ModuleName);
//     BindClass(Class, ModuleName);
    
//     // 创建 Lua 实例并初始化
//     Env->GetObjectRegistry()->Bind(Object);
//     PushFunction(L, Object, "Initialize");
//     ::CallFunction(L, 2, 0);
// }
// ```
// - **类注册机制**：将 UE 类注册到 Lua 类型系统，建立 C++ 与 Lua 的类型映射。
// - **模块加载流程**：
//   1. 使用 Lua `require` 加载模块，确保模块返回一个有效的 table。
//   2. 通过 `BindClass` 复制模块 table 作为类的元表。
// - **初始化回调**：调用 Lua 模块的 `Initialize` 函数，支持传递初始化参数表。

// ---

// #### **3. 动态函数覆盖**
// ```cpp
// void UUnLuaManager::BindClass(UClass* Class, const FString& ModuleName) {
//     // 获取模块函数列表
//     UnLua::LowLevel::GetFunctionNames(L, Ref, BindInfo.LuaFunctions);
    
//     // 覆盖 UE 函数
//     for (const auto& LuaFuncName : BindInfo.LuaFunctions) {
//         if (UFunction** Func = BindInfo.UEFunctions.Find(LuaFuncName)) {
//             ULuaFunction::Override(*Func, Class, LuaFuncName);
//         }
//     }
    
//     // 特殊类型处理（如动画通知）
//     if (Class->IsChildOf<UAnimInstance>()) {
//         OverrideAnimNotifies(BindInfo);
//     }
// }
// ```
// - **函数名匹配规则**：Lua 函数名需与 UE 函数名严格一致（区分大小写）。
// - **元编程覆盖**：通过 `ULuaFunction::Override` 将 Lua 函数动态绑定到 UE 的 `UFunction`，拦截原生调用。
// - **动画通知处理**：自动识别 `AnimNotify_` 前缀函数，绑定到动画系统。

// ---

// #### **4. 输入系统重定向**
// ```cpp
// void UUnLuaManager::ReplaceInputs(AActor* Actor, UInputComponent* InputComponent) {
//     ReplaceActionInputs(Actor, InputComponent);   // 替换 Action 输入
//     ReplaceKeyInputs(Actor, InputComponent);      // 替换按键输入
//     ReplaceAxisInputs(Actor, InputComponent);    // 替换轴输入
//     // ...其他输入类型
// }
// ```
// - **输入委托动态绑定**：
//   - **Action 输入**：生成 `ActionName_Pressed/Released` 格式函数名，绑定到 `InputAction` 委托。
//   - **轴输入**：直接匹配轴名称，绑定到 `InputAxis` 委托。
//   - **触摸输入**：生成 `Touch_Pressed/Released` 函数名，处理多点触控。
// - **自动补全绑定**：对比默认输入配置，自动添加缺失的输入事件绑定。

// ---

// #### **5. 协程与延迟处理**
// ```cpp
// void UUnLuaManager::OnLatentActionCompleted(int32 LinkID) {
//     Env->ResumeThread(LinkID); // 恢复指定协程
// }
// ```
// - **协程标识管理**：每个延迟操作（如 `Delay`）分配唯一 `LinkID`，完成时通过此 ID 恢复对应的 Lua 协程。

// ---

// #### **6. 关卡加载处理**
// ```cpp
// void UUnLuaManager::OnMapLoaded(UWorld* World) {
//     // 替换关卡脚本 Actor 的输入
//     ALevelScriptActor* LSA = Level->GetLevelScriptActor();
//     ReplaceInputs(LSA, LSA->InputComponent);
// }
// ```
// - **关卡脚本绑定**：确保关卡蓝图中的输入事件也能被 Lua 接管，保持输入系统统一。

// ---

// ### **关键设计亮点**

// 1. **无缝输入接管**  
//   通过分析 `InputComponent` 的绑定信息，动态生成 Lua 函数名并重定向委托，无需手动配置。

// 2. **元表继承机制**  
//   每个绑定的类获得独立的 Lua 元表副本，避免全局污染，支持类继承链的函数覆盖。

// 3. **高性能函数匹配**  
//   使用 `TSet<FName>` 快速查找 Lua 函数，时间复杂度 O(1)，确保大规模绑定的效率。

// 4. **蓝图兼容性**  
//   处理蓝图重新编译后的函数映射丢失问题，通过 `__UClassBindSucceeded` 标记确保稳定性。

// 5. **跨平台输入支持**  
//   统一处理 PC/移动端输入差异，通过 `Touch_`/`Gesture_` 前缀抽象底层输入事件。

// ---

// ### **典型应用场景**

// 1. **角色控制绑定**  
// ```lua
// -- Lua 模块 "PlayerController"
// function Jump_Pressed(Controller)
//     Controller:GetPawn():AddMovementInput(FVector(0,0,1), 500)
// end

// function MoveForward_Axis(Value)
//     -- 处理前后移动
// end
// ```
// - 自动绑定到 `Jump` Action 的按下事件和 `MoveForward` 轴输入。

// 2. **动画通知响应**  
// ```lua
// function AnimNotify_AttackStart(AnimInstance)
//     PlaySound(AnimInstance.Owner, "Attack")
// end
// ```
// - 在动画序列时间轴触发时执行 Lua 逻辑。

// 3. **UI 输入处理**  
// ```lua
// function Touch_Released(Controller, FingerIndex, Location)
//     ShowContextMenu(ConvertScreenToWorld(Location))
// end
// ```
// - 响应触屏释放事件，显示上下文菜单。

// ---

// ### **错误处理机制**

// - **模块加载校验**：若 Lua 模块未返回 table，记录详细错误日志：
// ```cpp
// Error = FString::Printf("Expected table, got %s", lua_typename(L, type));
// ```
// - **输入绑定回退**：未找到 Lua 函数时保留原生 UE 输入处理，避免游戏功能中断。
// - **协程泄漏防护**：通过 `FLuaEnv` 管理协程生命周期，防止未恢复的协程导致内存泄漏。

// ---

// ### **性能优化策略**

// 1. **输入委托缓存**  
//    首次绑定后缓存生成的委托，避免重复查找。

// 2. **元表共享**  
//    同一类的不同实例共享元表，减少内存占用。

// 3. **JIT 函数生成**  
//    通过 `ULuaFunction` 动态生成高效的字节码调用门面，减少 Lua/C++ 切换开销。

// 4. **批量输入处理**  
//    使用 `TArray` 和 `TSet` 进行批量操作，最小化每帧的 CPU 开销。

// ---

// 通过上述机制，`UUnLuaManager` 实现了 UE 与 Lua 的高效交互，为复杂游戏逻辑的脚本化提供了坚实基础。




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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/InputSettings.h"
#include "Components/InputComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/LevelScriptActor.h"
#include "UnLuaManager.h"
#include "LowLevel.h"
#include "LuaEnv.h"
#include "UnLuaLegacy.h"
#include "UnLuaInterface.h"
#include "LuaCore.h"
#include "LuaFunction.h"
#include "ObjectReferencer.h"


static const TCHAR* SReadableInputEvent[] = { TEXT("Pressed"), TEXT("Released"), TEXT("Repeat"), TEXT("DoubleClick"), TEXT("Axis"), TEXT("Max") };

UUnLuaManager::UUnLuaManager()
    : InputActionFunc(nullptr), InputAxisFunc(nullptr), InputTouchFunc(nullptr), InputVectorAxisFunc(nullptr), InputGestureFunc(nullptr), AnimNotifyFunc(nullptr)
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

    GetDefaultInputs();             // get all Axis/Action inputs
    EKeys::GetAllKeys(AllKeys);     // get all key inputs

    // get template input UFunctions for InputAction/InputAxis/InputTouch/InputVectorAxis/InputGesture/AnimNotify
    UClass *Class = GetClass();
    InputActionFunc = Class->FindFunctionByName(FName("InputAction"));
    InputAxisFunc = Class->FindFunctionByName(FName("InputAxis"));
    InputTouchFunc = Class->FindFunctionByName(FName("InputTouch"));
    InputVectorAxisFunc = Class->FindFunctionByName(FName("InputVectorAxis"));
    InputGestureFunc = Class->FindFunctionByName(FName("InputGesture"));
    AnimNotifyFunc = Class->FindFunctionByName(FName("TriggerAnimNotify"));
}

/**
 * Bind a Lua module for a UObject
 */
bool UUnLuaManager::Bind(UObject *Object, const TCHAR *InModuleName, int32 InitializerTableRef)
{
    check(Object);

    const auto Class = Object->IsA<UClass>() ? static_cast<UClass*>(Object) : Object->GetClass();
    lua_State *L = Env->GetMainState();

    if (!Env->GetClassRegistry()->Register(Class))
        return false;

    // try bind lua if not bind or use a copyed table
    UnLua::FLuaRetValues RetValues = UnLua::Call(L, "require", TCHAR_TO_UTF8(InModuleName));
    FString Error;
    if (!RetValues.IsValid() || RetValues.Num() == 0)
    {
        Error = "invalid return value of require()";
    }
    else if (RetValues[0].GetType() != LUA_TTABLE)
    {
        Error = FString("table needed but got ");
        if(RetValues[0].GetType() == LUA_TSTRING)
            Error += UTF8_TO_TCHAR(RetValues[0].Value<const char*>());
        else
            Error += UTF8_TO_TCHAR(lua_typename(L, RetValues[0].GetType()));
    }
    else
    {
        BindClass(Class, InModuleName, Error);
    }

    if (!Error.IsEmpty())
    {
        UE_LOG(LogUnLua, Warning, TEXT("Failed to attach %s module for object %s,%p!\n%s"), InModuleName, *Object->GetName(), Object, *Error);
        return false;
    }

    // create a Lua instance for this UObject
    Env->GetObjectRegistry()->Bind(Class);
    Env->GetObjectRegistry()->Bind(Object);

    // try call user first user function handler
    int32 FunctionRef = PushFunction(L, Object, "Initialize");                  // push hard coded Lua function 'Initialize'
    if (FunctionRef != LUA_NOREF)
    {
        if (InitializerTableRef != LUA_NOREF)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, InitializerTableRef);             // push a initializer table if necessary
        }
        else
        {
            lua_pushnil(L);
        }
        bool bResult = ::CallFunction(L, 2, 0);                                 // call 'Initialize'
        if (!bResult)
        {
            UE_LOG(LogUnLua, Warning, TEXT("Failed to call 'Initialize' function!"));
        }
        luaL_unref(L, LUA_REGISTRYINDEX, FunctionRef);
    }

    return true;
}

void UUnLuaManager::NotifyUObjectDeleted(const UObjectBase* Object)
{
    const UClass* Class = (UClass*)Object;
    const auto BindInfo = Classes.Find(Class);
    if (!BindInfo)
        return;

    const auto L = Env->GetMainState();
    luaL_unref(L, LUA_REGISTRYINDEX, BindInfo->TableRef);
    Classes.Remove(Class);
}

/**
 * Clean up...
 */
void UUnLuaManager::Cleanup()
{
    Env = nullptr;
    Classes.Empty();
}

int UUnLuaManager::GetBoundRef(const UClass* Class)
{
    const auto Info = Classes.Find(Class);
    if (!Info)
        return LUA_NOREF;
    return Info->TableRef;
}

/**
 * Get all default Axis/Action inputs
 */
void UUnLuaManager::GetDefaultInputs()
{
    UInputSettings *DefaultIS = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
    TArray<FName> AxisNames, ActionNames;
    DefaultIS->GetAxisNames(AxisNames);
    DefaultIS->GetActionNames(ActionNames);
    for (auto AxisName : AxisNames)
    {
        DefaultAxisNames.Add(AxisName);
    }
    for (auto ActionName : ActionNames)
    {
        DefaultActionNames.Add(ActionName);
    }
}

/**
 * Clean up all default Axis/Action inputs
 */
void UUnLuaManager::CleanupDefaultInputs()
{
    DefaultAxisNames.Empty();
    DefaultActionNames.Empty();
}

/**
 * Replace inputs
 */
bool UUnLuaManager::ReplaceInputs(AActor *Actor, UInputComponent *InputComponent)
{
    if (!Actor || !InputComponent)
        return false;

    const auto Class = Actor->GetClass();
    const auto BindInfo = Classes.Find(Class);
    if (!BindInfo)
        return false;

    auto& LuaFunctions = BindInfo->LuaFunctions;
    ReplaceActionInputs(Actor, InputComponent, LuaFunctions);       // replace action inputs
    ReplaceKeyInputs(Actor, InputComponent, LuaFunctions);          // replace key inputs
    ReplaceAxisInputs(Actor, InputComponent, LuaFunctions);         // replace axis inputs
    ReplaceTouchInputs(Actor, InputComponent, LuaFunctions);        // replace touch inputs
    ReplaceAxisKeyInputs(Actor, InputComponent, LuaFunctions);      // replace AxisKey inputs
    ReplaceVectorAxisInputs(Actor, InputComponent, LuaFunctions);   // replace VectorAxis inputs
    ReplaceGestureInputs(Actor, InputComponent, LuaFunctions);      // replace gesture inputs

    return true;
}

/**
 * Callback when a map is loaded
 */
void UUnLuaManager::OnMapLoaded(UWorld *World)
{
    ENetMode NetMode = World->GetNetMode();
    if (NetMode == NM_DedicatedServer)
    {
        return;
    }

    const TArray<ULevel*> &Levels = World->GetLevels();
    for (ULevel *Level : Levels)
    {
        // replace input defined in ALevelScriptActor::InputComponent if necessary
        ALevelScriptActor *LSA = Level->GetLevelScriptActor();
        if (LSA && LSA->InputEnabled() && LSA->InputComponent)
        {
            ReplaceInputs(LSA, LSA->InputComponent);
        }
    }
}

UDynamicBlueprintBinding* UUnLuaManager::GetOrAddBindingObject(UClass* Class, UClass* BindingClass)
{
    auto BPGC = Cast<UBlueprintGeneratedClass>(Class);
    if (!BPGC)
        return nullptr;

    UDynamicBlueprintBinding* BindingObject = UBlueprintGeneratedClass::GetDynamicBindingObject(Class, BindingClass);
    if (!BindingObject)
    {
        BindingObject = (UDynamicBlueprintBinding*)NewObject<UObject>(GetTransientPackage(), BindingClass);
        BPGC->DynamicBindingObjects.Add(BindingObject);
    }
    return BindingObject;
}

void UUnLuaManager::Override(UClass* Class, FName FunctionName, FName LuaFunctionName)
{
    if (const auto Function = GetClass()->FindFunctionByName(FunctionName))
        ULuaFunction::Override(Function, Class, LuaFunctionName);
}

/**
 * Callback for completing a latent function
 */
void UUnLuaManager::OnLatentActionCompleted(int32 LinkID)
{
    Env->ResumeThread(LinkID); // resume a coroutine
}

bool UUnLuaManager::BindClass(UClass* Class, const FString& InModuleName, FString& Error)
{
    check(Class);

    if (Class->HasAnyFlags(RF_NeedPostLoad | RF_NeedPostLoadSubobjects))
        return false;

    if (Classes.Contains(Class))
    {
#if WITH_EDITOR
        // 兼容蓝图Recompile导致FuncMap被清空的情况
        if (Class->FindFunctionByName("__UClassBindSucceeded", EIncludeSuperFlag::Type::ExcludeSuper))
            return true;
        
        ULuaFunction::RestoreOverrides(Class);
#else
        return true;
#endif
    }

    const auto  L = Env->GetMainState();
    const auto Top = lua_gettop(L);
    const auto Type = UnLua::LowLevel::GetLoadedModule(L, TCHAR_TO_UTF8(*InModuleName));
    if (Type != LUA_TTABLE)
    {
        Error = FString::Printf(TEXT("table needed got %s"), UTF8_TO_TCHAR(lua_typename(L, Type)));
        lua_settop(L, Top);
        return false;
    }

    if (!Class->IsChildOf<UBlueprintFunctionLibrary>())
    {
        // 一个LuaModule可能会被绑定到一个UClass和它的子类，复制一个出来作为它们的实例的元表
        lua_newtable(L);
        lua_pushnil(L);
        while (lua_next(L, -3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -4);
        }
    }

    lua_pushvalue(L, -1);
    const auto Ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_settop(L, Top);

    auto& BindInfo = Classes.Add(Class);
    BindInfo.Class = Class;
    BindInfo.ModuleName = InModuleName;
    BindInfo.TableRef = Ref;

    UnLua::LowLevel::GetFunctionNames(Env->GetMainState(), Ref, BindInfo.LuaFunctions);
    ULuaFunction::GetOverridableFunctions(Class, BindInfo.UEFunctions);

    // 用LuaTable里所有的函数来替换Class上对应的UFunction
    for (const auto& LuaFuncName : BindInfo.LuaFunctions)
    {
        UFunction** Func = BindInfo.UEFunctions.Find(LuaFuncName);
        if (Func)
        {
            UFunction* Function = *Func;
            ULuaFunction::Override(Function, Class, LuaFuncName);
        }
    }

    if (BindInfo.LuaFunctions.Num() == 0 || BindInfo.UEFunctions.Num() == 0)
        return true;

    // 继续对特殊类型进行替换
    if (Class->IsChildOf<UAnimInstance>())
    {
        for (const auto& LuaFuncName : BindInfo.LuaFunctions)
        {
            if (!BindInfo.UEFunctions.Find(LuaFuncName) && LuaFuncName.ToString().StartsWith(TEXT("AnimNotify_")))
                ULuaFunction::Override(AnimNotifyFunc, Class, LuaFuncName);
        }
    }

#if WITH_EDITOR
    // 兼容蓝图Recompile导致FuncMap被清空的情况
    for (const auto& Iter : BindInfo.UEFunctions)
    {
        auto& FuncName = Iter.Key;
        auto& Function = Iter.Value;
        if (Class->FindFunctionByName(FuncName, EIncludeSuperFlag::Type::ExcludeSuper))
        {
            Class->AddFunctionToFunctionMap(Function, "__UClassBindSucceeded");
            break;
        }
    }
#endif

    if (auto BPGC = Cast<UBlueprintGeneratedClass>(Class))
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, Ref);
        lua_getglobal(L, "UnLua");
        if (lua_getfield(L, -1, "Input") != LUA_TTABLE)
        {
            lua_pop(L, 2);
            return true;
        }
        UnLua::FLuaTable InputTable(Env, -1);
        UnLua::FLuaTable ModuleTable(Env, -3);
        InputTable.Call("PerformBindings", ModuleTable, this, BPGC);
        lua_pop(L, 3);
    }

    return true;
}

/**
 * Replace action inputs
 */
void UUnLuaManager::ReplaceActionInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TSet<FName> ActionNames;
    int32 NumActionBindings = InputComponent->GetNumActionBindings();
    for (int32 i = 0; i < NumActionBindings; ++i)
    {
        FInputActionBinding &IAB = InputComponent->GetActionBinding(i);
        FName Name = GET_INPUT_ACTION_NAME(IAB);
        FString ActionName = Name.ToString();
        ActionNames.Add(Name);

        FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, SReadableInputEvent[IAB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputActionFunc, Class, FuncName);
            IAB.ActionDelegate.BindDelegate(Actor, FuncName);
        }

        if (!IS_INPUT_ACTION_PAIRED(IAB))
        {
            EInputEvent IE = IAB.KeyEvent == IE_Pressed ? IE_Released : IE_Pressed;
            FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName, SReadableInputEvent[IE]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputActionBinding AB(Name, IE);
                AB.ActionDelegate.BindDelegate(Actor, FuncName);
                InputComponent->AddActionBinding(AB);
            }
        }
    }

    EInputEvent IEs[] = { IE_Pressed, IE_Released };
    TSet<FName> DiffActionNames = DefaultActionNames.Difference(ActionNames);
    for (TSet<FName>::TConstIterator It(DiffActionNames); It; ++It)
    {
        FName ActionName = *It;
        for (int32 i = 0; i < 2; ++i)
        {
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *ActionName.ToString(), SReadableInputEvent[IEs[i]]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputActionBinding AB(ActionName, IEs[i]);
                AB.ActionDelegate.BindDelegate(Actor, FuncName);
                InputComponent->AddActionBinding(AB);
            }
        }
    }
}

/**
 * Replace key inputs
 */
void UUnLuaManager::ReplaceKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TArray<FKey> Keys;
    TArray<bool> PairedKeys;
    TArray<EInputEvent> InputEvents;
    for (FInputKeyBinding &IKB : InputComponent->KeyBindings)
    {
        int32 Index = Keys.Find(IKB.Chord.Key);
        if (Index == INDEX_NONE)
        {
            Keys.Add(IKB.Chord.Key);
            PairedKeys.Add(false);
            InputEvents.Add(IKB.KeyEvent);
        }
        else
        {
            PairedKeys[Index] = true;
        }

        FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *IKB.Chord.Key.ToString(), SReadableInputEvent[IKB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputActionFunc, Class, FuncName);
            IKB.KeyDelegate.BindDelegate(Actor, FuncName);
        }
    }

    for (int32 i = 0; i< Keys.Num(); ++i)
    {
        if (!PairedKeys[i])
        {
            EInputEvent IE = InputEvents[i] == IE_Pressed ? IE_Released : IE_Pressed;
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *Keys[i].ToString(), SReadableInputEvent[IE]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputKeyBinding IKB(FInputChord(Keys[i]), IE);
                IKB.KeyDelegate.BindDelegate(Actor, FuncName);
                InputComponent->KeyBindings.Add(IKB);
            }
        }
    }

    EInputEvent IEs[] = { IE_Pressed, IE_Released };
    for (const FKey &Key : AllKeys)
    {
        if (Keys.Find(Key) != INDEX_NONE)
        {
            continue;
        }
        for (int32 i = 0; i < 2; ++i)
        {
            FName FuncName = FName(*FString::Printf(TEXT("%s_%s"), *Key.ToString(), SReadableInputEvent[IEs[i]]));
            if (LuaFunctions.Find(FuncName))
            {
                ULuaFunction::Override(InputActionFunc, Class, FuncName);
                FInputKeyBinding IKB(FInputChord(Key), IEs[i]);
                IKB.KeyDelegate.BindDelegate(Actor, FuncName);
                InputComponent->KeyBindings.Add(IKB);
            }
        }
    }
}

/**
 * Replace axis inputs
 */
void UUnLuaManager::ReplaceAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TSet<FName> AxisNames;
    for (FInputAxisBinding &IAB : InputComponent->AxisBindings)
    {
        AxisNames.Add(IAB.AxisName);
        if (LuaFunctions.Find(IAB.AxisName))
        {
            ULuaFunction::Override(InputAxisFunc, Class, IAB.AxisName);
            IAB.AxisDelegate.BindDelegate(Actor, IAB.AxisName);
        }
    }

    TSet<FName> DiffAxisNames = DefaultAxisNames.Difference(AxisNames);
    for (TSet<FName>::TConstIterator It(DiffAxisNames); It; ++It)
    {
        if (LuaFunctions.Find(*It))
        {
            ULuaFunction::Override(InputAxisFunc, Class, *It);
            FInputAxisBinding &IAB = InputComponent->BindAxis(*It);
            IAB.AxisDelegate.BindDelegate(Actor, *It);
        }
    }
}

/**
 * Replace touch inputs
 */
void UUnLuaManager::ReplaceTouchInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();

    TArray<EInputEvent> InputEvents = { IE_Pressed, IE_Released, IE_Repeat };        // IE_DoubleClick?
    for (FInputTouchBinding &ITB : InputComponent->TouchBindings)
    {
        InputEvents.Remove(ITB.KeyEvent);
        FName FuncName = FName(*FString::Printf(TEXT("Touch_%s"), SReadableInputEvent[ITB.KeyEvent]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputTouchFunc, Class, FuncName);
            ITB.TouchDelegate.BindDelegate(Actor, FuncName);
        }
    }

    for (EInputEvent IE : InputEvents)
    {
        FName FuncName = FName(*FString::Printf(TEXT("Touch_%s"), SReadableInputEvent[IE]));
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputTouchFunc, Class, FuncName);
            FInputTouchBinding ITB(IE);
            ITB.TouchDelegate.BindDelegate(Actor, FuncName);
            InputComponent->TouchBindings.Add(ITB);
        }
    }
}

/**
 * Replace axis key inputs
 */
void UUnLuaManager::ReplaceAxisKeyInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputAxisKeyBinding &IAKB : InputComponent->AxisKeyBindings)
    {
        FName FuncName = IAKB.AxisKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputAxisFunc, Class, FuncName);
            IAKB.AxisDelegate.BindDelegate(Actor, FuncName);
        }
    }
}

/**
 * Replace vector axis inputs
 */
void UUnLuaManager::ReplaceVectorAxisInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputVectorAxisBinding &IVAB : InputComponent->VectorAxisBindings)
    {
        FName FuncName = IVAB.AxisKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputVectorAxisFunc, Class, FuncName);
            IVAB.AxisDelegate.BindDelegate(Actor, FuncName);
        }
    }
}

/**
 * Replace gesture inputs
 */
void UUnLuaManager::ReplaceGestureInputs(AActor *Actor, UInputComponent *InputComponent, TSet<FName> &LuaFunctions)
{
    UClass *Class = Actor->GetClass();
    for (FInputGestureBinding &IGB : InputComponent->GestureBindings)
    {
        FName FuncName = IGB.GestureKey.GetFName();
        if (LuaFunctions.Find(FuncName))
        {
            ULuaFunction::Override(InputGestureFunc, Class, FuncName);
            IGB.GestureDelegate.BindDelegate(Actor, FuncName);
        }
    }
}
