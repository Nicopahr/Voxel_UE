// Copyright blackw 2015-2020

#include "ContainerComponent.h" // Подключение заголовочного файла компонента контейнера
#include "SandboxObject.h" // Подключение заголовочного файла объекта песочницы
#include "Net/UnrealNetwork.h" // Подключение заголовочного файла для сетевых функций Unreal
#include "SandboxLevelController.h" // Подключение заголовочного файла контроллера уровня песочницы
#include "SandboxPlayerController.h" // Подключение заголовочного файла контроллера игрока песочницы
#include <algorithm> // Подключение стандартного заголовочного файла для алгоритмов

// Функция для получения объекта по его идентификатору класса
const ASandboxObject* FContainerStack::GetObject() const {
    return ASandboxLevelController::GetDefaultSandboxObject(SandboxClassId); // Возвращает объект песочницы по идентификатору класса
}

// Конструктор компонента контейнера
UContainerComponent::UContainerComponent() {
    // Отключаем тик для компонента
    PrimaryComponentTick.bCanEverTick = false; // Отключение тика для компонента
}

// Функция, вызываемая при начале игры
void UContainerComponent::BeginPlay() {
    Super::BeginPlay(); // Вызов родительской функции BeginPlay
}

// Функция тика компонента (не используется, так как тик отключен)
void UContainerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction); // Вызов родительской функции TickComponent
}

// Проверка, пуст ли контейнер
bool UContainerComponent::IsEmpty() const {
    if (Content.Num() == 0) { // Проверка, пусто ли содержимое контейнера
        return true;
    }

    for (int Idx = 0; Idx < Content.Num(); Idx++) { // Перебор всех стеков в контейнере
        FContainerStack Stack = Content[Idx];
        if (Stack.Amount > 0) { // Проверка, есть ли объекты в стеке
            return false;
        }
    }

    return true;
}

// Установка стека объектов в указанный слот
bool UContainerComponent::SetStackDirectly(const FContainerStack& Stack, const int SlotId) {
    if (SlotId >= Content.Num()) { // Проверка, достаточно ли места в контейнере
        Content.SetNum(SlotId + 1); // Увеличение размера контейнера, если необходимо
    }

    FContainerStack* StackPtr = &Content[SlotId]; // Получение указателя на стек в указанном слоте
    if (Stack.Amount > 0) { // Проверка, есть ли объекты в стеке
        StackPtr->Amount = Stack.Amount; // Установка количества объектов в стеке
        StackPtr->SandboxClassId = Stack.SandboxClassId; // Установка идентификатора класса объекта
    } else {
        StackPtr->Clear(); // Очистка стека, если количество объектов равно нулю
    }

    MakeStats(); // Обновление статистики содержимого контейнера
    bUpdated = true; // Установка флага обновления
    return true;
}

// Добавление объекта в контейнер
bool UContainerComponent::AddObject(ASandboxObject* Obj) {
    if (Obj == nullptr) { // Проверка, не является ли объект нулевым указателем
        return false;
    }

    uint32 MaxStackSize = Obj->GetMaxStackSize(); // Получение максимального размера стека для объекта

    int FirstEmptySlot = -1; // Инициализация переменной для первого пустого слота
    bool bIsAdded = false; // Инициализация флага добавления объекта
    for (int Idx = 0; Idx < Content.Num(); Idx++) { // Перебор всех стеков в контейнере
        FContainerStack* Stack = &Content[Idx];

        // TODO: проверка максимального объема и массы инвентаря
        if (Stack->Amount > 0) { // Проверка, есть ли объекты в стеке
            if (Stack->SandboxClassId == Obj->GetSandboxClassId() && MaxStackSize > 1 && (uint64)Stack->Amount < MaxStackSize) { // Проверка, можно ли добавить объект в стек
                Stack->Amount++; // Увеличение количества объектов в стеке
                bIsAdded = true; // Установка флага добавления объекта
                break;
            }
        } else {
            if (FirstEmptySlot < 0) { // Проверка, найден ли первый пустой слот
                FirstEmptySlot = Idx; // Установка индекса первого пустого слота
                if (MaxStackSize == 1) { // Проверка, равен ли максимальный размер стека единице
                    break;
                }
            }
        }
    }

    if (!bIsAdded) { // Проверка, был ли объект добавлен
        if (FirstEmptySlot >= 0) { // Проверка, найден ли первый пустой слот
            FContainerStack* Stack = &Content[FirstEmptySlot]; // Получение указателя на стек в первом пустом слоте
            Stack->Amount = 1; // Установка количества объектов в стеке
            Stack->SandboxClassId = Obj->GetSandboxClassId(); // Установка идентификатора класса объекта
        } else {
            if (Content.Num() < MaxCapacity) { // Проверка, достаточно ли места в контейнере
                FContainerStack NewStack; // Создание нового стека
                NewStack.Amount = 1; // Установка количества объектов в новом стеке
                NewStack.SandboxClassId = Obj->GetSandboxClassId(); // Установка идентификатора класса объекта
                Content.Add(NewStack); // Добавление нового стека в контейнер
            } else {
                return false; // Возврат false, если нет места в контейнере
            }
        }
    }

    MakeStats(); // Обновление статистики содержимого контейнера
    bUpdated = true; // Установка флага обновления
    return true;
}

// Получение стека объектов в указанном слоте
FContainerStack* UContainerComponent::GetSlot(const int Slot) {
    if (!Content.IsValidIndex(Slot)) { // Проверка, является ли индекс слота допустимым
        return nullptr; // Возврат нулевого указателя, если индекс недопустим
    }

    return &Content[Slot]; // Возврат указателя на стек в указанном слоте
}

// Получение стека объектов в указанном слоте (константная версия)
const FContainerStack* UContainerComponent::GetSlot(const int Slot) const {
    if (!Content.IsValidIndex(Slot)) { // Проверка, является ли индекс слота допустимым
        return nullptr; // Возврат нулевого указателя, если индекс недопустим
    }

    return &Content[Slot]; // Возврат указателя на стек в указанном слоте
}

// Уменьшение количества объектов в контейнере
bool UContainerComponent::DecreaseObjectsInContainer(int Slot, int Num) {
    FContainerStack* Stack = GetSlot(Slot); // Получение указателя на стек в указанном слоте

    if (Stack == NULL) { // Проверка, не является ли указатель нулевым
        return false;
    }

    if (Stack->Amount > 0) { // Проверка, есть ли объекты в стеке
        Stack->Amount -= Num; // Уменьшение количества объектов в стеке
        if (Stack->Amount == 0) { // Проверка, равно ли количество объектов нулю
            Stack->Clear(); // Очистка стека
        }
    }

    MakeStats(); // Обновление статистики содержимого контейнера
    bUpdated = true; // Установка флага обновления
    return Stack->Amount > 0; // Возврат true, если в стеке остались объекты
}

// Изменение количества объектов в указанном слоте
void UContainerComponent::ChangeAmount(int Slot, int Num) {
    FContainerStack* Stack = GetSlot(Slot); // Получение указателя на стек в указанном слоте

    // TODO: проверка размера стека
    if (Stack) { // Проверка, не является ли указатель нулевым
        if (Stack->Amount > 0) { // Проверка, есть ли объекты в стеке
            Stack->Amount += Num; // Изменение количества объектов в стеке
            if (Stack->Amount == 0) { // Проверка, равно ли количество объектов нулю
                Stack->Clear(); // Очистка стека
            }

            MakeStats(); // Обновление статистики содержимого контейнера
            bUpdated = true; // Установка флага обновления
        }
    }
}

// Проверка, пуст ли указанный слот
bool UContainerComponent::IsSlotEmpty(int SlotId) const {
    const FContainerStack* Stack = GetSlot(SlotId); // Получение указателя на стек в указанном слоте
    if (Stack) { // Проверка, не является ли указатель нулевым
        return Stack->IsEmpty(); // Возврат true, если стек пуст
    }

    return true;
}

// Репликация свойств компонента
void UContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); // Вызов родительской функции GetLifetimeReplicatedProps
    DOREPLIFETIME(UContainerComponent, Content); // Репликация свойства Content
}

// Копирование содержимого контейнера в другой контейнер
void UContainerComponent::CopyTo(UContainerComponent* Target) {
    Target->Content = this->Content; // Копирование содержимого контейнера в целевой контейнер
    bUpdated = true; // Установка флага обновления
    MakeStats(); // Обновление статистики содержимого контейнера
}

// Получение всех объектов в контейнере
TArray<uint64> UContainerComponent::GetAllObjects() const {
    TArray<uint64> Result; // Создание массива для хранения идентификаторов объектов
    for (int Idx = 0; Idx < Content.Num(); Idx++) { // Перебор всех стеков в контейнере
        const FContainerStack* Stack = &Content[Idx];
        if (Stack) { // Проверка, не является ли указатель нулевым
            const ASandboxObject* Obj = Stack->GetObject(); // Получение объекта по идентификатору класса
            if (Obj) { // Проверка, не является ли указатель нулевым
                Result.Add(Obj->GetSandboxClassId()); // Добавление идентификатора класса объекта в массив
            }
        }
    }

    return Result; // Возврат массива идентификаторов объектов
}

// Проверка, являются ли два стека объектов одинаковыми
bool IsSameObject(const FContainerStack* StackSourcePtr, const FContainerStack* StackTargetPtr) {
    if (StackSourcePtr && StackTargetPtr) { // Проверка, не являются ли указатели нулевыми
        const auto* SourceObj = StackSourcePtr->GetObject(); // Получение объекта по идентификатору класса источника
        const auto* TargetObj = StackTargetPtr->GetObject(); // Получение объекта по идентификатору класса цели
        if (SourceObj && TargetObj) { // Проверка, не являются ли указатели нулевыми
            return SourceObj->GetSandboxClassId() == TargetObj->GetSandboxClassId(); // Возврат true, если идентификаторы классов объектов совпадают
        }
    }

    return false;
}

// Передача объекта по сети
void NetObjectTransfer(ASandboxPlayerController* LocalController, const ASandboxObject* RemoteObj, const UContainerComponent* RemoteContainer, const FContainerStack Stack, int32 SlotId) {
    const FString TargetContainerName = RemoteContainer->GetName(); // Получение имени целевого контейнера
    LocalController->TransferContainerStack(RemoteObj->GetSandboxNetUid(), TargetContainerName, Stack, SlotId); // Передача стека объектов по сети
}

// Передача контроллера по сети
void NetControllerTransfer(ASandboxPlayerController* LocalController, const UContainerComponent* RemoteContainer, const FContainerStack Stack, int32 SlotId) {
    const FString TargetContainerName = RemoteContainer->GetName(); // Получение имени целевого контейнера
    LocalController->TransferInventoryStack(TargetContainerName, Stack, SlotId); // Передача стека объектов по сети
}

// Перемещение стека объектов между слотами
bool UContainerComponent::SlotTransfer(int32 SlotSourceId, int32 SlotTargetId, AActor* SourceActor, UContainerComponent* SourceContainer, bool bOnlyOne) {
    UContainerComponent* TargetContainer = this; // Установка целевого контейнера
    const FContainerStack* StackSourcePtr = SourceContainer->GetSlot(SlotSourceId); // Получение указателя на стек в исходном слоте
    const FContainerStack* StackTargetPtr = TargetContainer->GetSlot(SlotTargetId); // Получение указателя на стек в целевом слоте

    bool bInternalTransfer = (TargetContainer == SourceContainer); // Проверка, является ли перемещение внутренним

    FContainerStack NewSourceStack; // Создание нового стека для исходного слота
    FContainerStack NewTargetStack; // Создание нового стека для целевого слота

    if (StackTargetPtr) { // Проверка, не является ли указатель нулевым
        NewTargetStack = *StackTargetPtr; // Копирование стека в целевой слот
    }

    if (StackSourcePtr) { // Проверка, не является ли указатель нулевым
        NewSourceStack = *StackSourcePtr; // Копирование стека в исходный слот
    }

    bool bResult = false; // Инициализация флага результата

    APawn* LocalPawn = (APawn*)TargetContainer->GetOwner(); // Получение указателя на владельца целевого контейнера
    if (LocalPawn) { // Проверка, не является ли указатель нулевым
        ASandboxPlayerController* LocalController = Cast<ASandboxPlayerController>(LocalPawn->GetController()); // Получение указателя на контроллер игрока
        if (LocalController) { // Проверка, не является ли указатель нулевым
            const FString ContainerName = GetName(); // Получение имени целевого контейнера
            const ASandboxObject* Obj = StackSourcePtr->GetObject(); // Получение объекта по идентификатору класса источника
            bool isValid = LocalController->OnContainerDropCheck(SlotTargetId, *ContainerName, Obj); // Проверка возможности перемещения объекта
            if (!isValid) { // Проверка, возможно ли перемещение объекта
                return false;
            }
        }
    }

    if (IsSameObject(StackSourcePtr, StackTargetPtr)) { // Проверка, являются ли объекты в стеках одинаковыми
        const ASandboxObject* Obj = (ASandboxObject*)StackTargetPtr->GetObject(); // Получение объекта по идентификатору класса цели
        if (StackTargetPtr->Amount < (int)Obj->MaxStackSize) { // Проверка, можно ли добавить объекты в целевой стек
            uint32 ChangeAmount = (bOnlyOne) ? 1 : StackSourcePtr->Amount; // Определение количества объектов для перемещения
            uint32 NewAmount = StackTargetPtr->Amount + ChangeAmount; // Определение нового количества объектов в целевом стеке

            if (NewAmount <= Obj->MaxStackSize) { // Проверка, не превышает ли новое количество объектов максимальный размер стека
                NewTargetStack.Amount = NewAmount; // Установка нового количества объектов в целевом стеке
                NewSourceStack.Amount -= ChangeAmount; // Уменьшение количества объектов в исходном стеке
            } else {
                int D = NewAmount - Obj->MaxStackSize; // Определение количества объектов, превышающих максимальный размер стека
                NewTargetStack.Amount = Obj->MaxStackSize; // Установка максимального количества объектов в целевом стеке
                NewSourceStack.Amount = D; // Установка количества объектов, превышающих максимальный размер стека, в исходном стеке
            }

            bResult = true; // Установка флага результата
        }
    } else {
        if (bOnlyOne) { // Проверка, перемещается ли только один объект
            if (!SourceContainer->IsSlotEmpty(SlotSourceId)) { // Проверка, не пуст ли исходный слот
                if (TargetContainer->IsSlotEmpty(SlotTargetId)) { // Проверка, пуст ли целевой слот
                    NewTargetStack = NewSourceStack; // Копирование стека в целевой слот
                    NewTargetStack.Amount = 1; // Установка количества объектов в целевом стеке
                    NewSourceStack.Amount--; // Уменьшение количества объектов в исходном стеке
                    bResult = true; // Установка флага результата
                }
            }
        } else {
            std::swap(NewTargetStack, NewSourceStack); // Обмен стеков между исходным и целевым слотами
            bResult = true; // Установка флага результата
        }
    }

    if (bResult) { // Проверка, был ли результат успешным
        if (GetNetMode() == NM_Client) { // Проверка, является ли режим сети клиентским
            ASandboxPlayerController* LocalController = Cast<ASandboxPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)); // Получение указателя на контроллер игрока
            if (LocalController) { // Проверка, не является ли указатель нулевым
                if (bInternalTransfer) { // Проверка, является ли перемещение внутренним
                    // TargetContainer->SetStackDirectly(NewTargetStack, SlotTargetId);
                    // SourceContainer->SetStackDirectly(NewSourceStack, SlotSourceId);
                }

                ASandboxObject* TargetObj = Cast<ASandboxObject>(TargetContainer->GetOwner()); // Получение указателя на объект целевого контейнера
                if (TargetObj) { // Проверка, не является ли указатель нулевым
                    NetObjectTransfer(LocalController, TargetObj, TargetContainer, NewTargetStack, SlotTargetId); // Передача объекта по сети
                }

                ASandboxObject* SourceObj = Cast<ASandboxObject>(SourceContainer->GetOwner()); // Получение указателя на объект исходного контейнера
                if (SourceObj) { // Проверка, не является ли указатель нулевым
                    NetObjectTransfer(LocalController, SourceObj, SourceContainer, NewSourceStack, SlotSourceId); // Передача объекта по сети
                }

                APawn* TargetPawn = Cast<APawn>(TargetContainer->GetOwner()); // Получение указателя на владельца целевого контейнера
                if (TargetPawn) { // Проверка, не является ли указатель нулевым
                    NetControllerTransfer(LocalController, TargetContainer, NewTargetStack, SlotTargetId); // Передача контроллера по сети
                }

                APawn* SourcePawn = Cast<APawn>(SourceContainer->GetOwner()); // Получение указателя на владельца исходного контейнера
                if (SourcePawn) { // Проверка, не является ли указатель нулевым
                    NetControllerTransfer(LocalController, SourceContainer, NewSourceStack, SlotSourceId); // Передача контроллера по сети
                }
            }
        } else {
            // только сервер
            TargetContainer->SetStackDirectly(NewTargetStack, SlotTargetId); // Установка стека в целевой слот
            SourceContainer->SetStackDirectly(NewSourceStack, SlotSourceId); // Установка стека в исходный слот
        }
    }

    bUpdated = bResult; // Установка флага обновления
    MakeStats(); // Обновление статистики содержимого контейнера
    return bResult; // Возврат результата
}

// Обработка репликации содержимого контейнера
void UContainerComponent::OnRep_Content() {
    UE_LOG(LogTemp, Warning, TEXT("OnRep_Content: %s"), *GetName()); // Логирование сообщения о репликации содержимого контейнера
    bUpdated = true; // Установка флага обновления
    MakeStats(); // Обновление статистики содержимого контейнера
}

// Проверка, было ли обновлено содержимое контейнера
bool UContainerComponent::IsUpdated() {
    return bUpdated; // Возврат флага обновления
}

// Сброс флага обновления
void UContainerComponent::ResetUpdatedFlag() {
    bUpdated = false; // Сброс флага обновления
}

// Создание статистики содержимого контейнера
void UContainerComponent::MakeStats() {
    InventoryStats.Empty(); // Очистка статистики содержимого контейнера

    for (const auto& Stack : Content) { // Перебор всех стеков в контейнере
        if (Stack.Amount > 0 && Stack.SandboxClassId > 0) { // Проверка, есть ли объекты в стеке и не равен ли идентификатор класса нулю
            auto T = InventoryStats.FindOrAdd(Stack.SandboxClassId) + Stack.Amount; // Получение или добавление идентификатора класса в статистику и увеличение количества объектов
            InventoryStats[Stack.SandboxClassId] = T; // Установка количества объектов в статистике
        }
    }
}

// Получение статистики содержимого контейнера
const TMap<uint64, uint32>& UContainerComponent::GetStats() const {
    return InventoryStats; // Возврат статистики содержимого контейнера
}

// Получение содержимого контейнера
const TArray<FContainerStack>& UContainerComponent::GetContent() {
    return Content; // Возврат содержимого контейнера
}
