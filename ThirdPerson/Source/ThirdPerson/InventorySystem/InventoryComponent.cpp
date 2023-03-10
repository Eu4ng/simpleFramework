// Fill out your copyright notice in the Description page of Project Settings.
// Wiki for simpleFramework
// https://github.com/Eu4ng/simpleFramework/wiki

#include "InventoryComponent.h"
#include "ThirdPerson/ThirdPerson.h"
#include "ThirdPerson/Item/ItemDefinition.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UInventoryComponent::Init(int32 InventorySlots)
{
	if(InventorySlots < 0)
	{
		UE_LOG(LogInventory, Error, TEXT("InventoryComponent::Init\nInventorySlotNum < 0"));
		SetInventorySlots(0);
	}
	else
	{
		UE_LOG(LogInventory, Log, TEXT("InventoryComponent::Init\nInventorySlotNum: %d"), InventorySlots);
		SetInventorySlots(InventorySlots);
	}
}

bool UInventoryComponent::AddItem(UItemDefinition* ItemDefinition, int32& ItemCount)
{
	UE_LOG(LogInventory, Log, TEXT("\nInventoryComponent::AddItem\nItemDefinition: %s / Count: %d"), *ItemDefinition->GetName(), ItemCount);

	// 입력된 아이템 유효성 검사
	if(ItemDefinition == nullptr || ItemCount <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("InventoryComponent::AddItem\nItemCount <= 0"));
		return false;
	}

	// 동일한 아이템 존재시 채워넣기
	bool bAdded = FillSameItem(ItemDefinition, ItemCount);

	// 새로운 아이템 혹은 용량을 초과한 아이템 추가
	while(ItemCount > 0 && IsAddable())
	{
		bAdded = true;
		int32 Count;
		if(ItemCount >= ItemDefinition->MaxStack)
		{
			Count = ItemDefinition->MaxStack;
			ItemCount -= Count;
		}
		else
		{
			Count = ItemCount;
			ItemCount = 0;
		}

		int32 NewIndex = GetEmptyIndex();
		FInventoryItem NewInventoryItem(NewIndex, Count, ItemDefinition);
		Inventory.Add(NewInventoryItem);
		UE_LOG(LogInventory, Log, TEXT("Item Added To New Inventory Slot\nIndex: %d / Count: %d"), NewIndex, Count);
	}
	
	if(bAdded)
	{
		UE_LOG(LogInventory, Log, TEXT("Add Item To Inventory Complete\nItemCount Reaminder: %d"), ItemCount);
		// 이벤트 디스패처 호출
		OnUpdate.Broadcast();
	}
	else
	{
		UE_LOG(LogInventory, Log, TEXT("Inventory Is Full"));
	}
	return bAdded;
}

bool UInventoryComponent::GetItemByCount(UItemDefinition* ItemDefinition, int32 NeedCount, int32& AvailableCount)
{
	UE_LOG(LogInventory, Log, TEXT("\nInventoryComponent::GetItemByCount\nItemDefinition: %s / NeedCount: %d"), *ItemDefinition->GetName(), NeedCount);
	// 유효성 검사
	if(ItemDefinition == nullptr)
	{
		UE_LOG(LogInventory, Error, TEXT("ItemDefinition == nullptr"));
		return false;
	}
	if(NeedCount <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("NeedCount <= 0"));
		return false;
	}
	
	AvailableCount = 0;
	// 인벤토리에 동일한 아이템 검색
	TArray<int32> Indices;
	for(FInventoryItem InventoryItem : Inventory)
	{
		if(InventoryItem == ItemDefinition)
		{
			Indices.Add(InventoryItem.InventoryIndex);
		}
	}

	if(Indices.IsEmpty())
	{
		UE_LOG(LogInventory, Warning, TEXT("No %s In Inventory"), *ItemDefinition->GetName());
		return false;
	}

	Indices.Sort();

	for(int32 Index : Indices)
	{
		FInventoryItem* InventoryItem = Inventory.FindByKey(Index);
		if(NeedCount >= InventoryItem->Count)
		{
			NeedCount -= InventoryItem->Count;
			AvailableCount += InventoryItem->Count;
			Inventory.RemoveSingle(*InventoryItem);
		}
		else
		{
			InventoryItem->Count -= NeedCount;
			AvailableCount += NeedCount;
			break;
		}
	}

	UE_LOG(LogInventory, Log, TEXT("Get Item From Inventory Complete\nAvailableCount: %d"), AvailableCount);
	// 이벤트 디스패처 호출
	OnUpdate.Broadcast();
	return true;
}

bool UInventoryComponent::RemoveInventoryItemByIndex(int32 Index, int32 Count)
{
	FInventoryItem* const InventoryItem = Inventory.FindByKey(Index);
	if(InventoryItem == nullptr) { return false; }
	if(Count == 0) { return false; }
		
	if(InventoryItem->Count == Count)
	{
		Inventory.RemoveSingle(*InventoryItem);
	}
	else
	{
		InventoryItem->Count -= Count;
	}

	UE_LOG(LogInventory, Log, TEXT("Remove Item From Inventory Complete\nIndex: %d / Count: %d"), Index, Count);
	// 이벤트 디스패처 호출
	OnUpdate.Broadcast();

	return true;
}

bool UInventoryComponent::SwapItems(int32 SourceIndex, int32 DestinationIndex)
{
	bool bSucceed = false;
	// Index 유효성 검사
	if(SourceIndex < 0 || SourceIndex >= MaxInventorySlots) { return bSucceed; }
	if(DestinationIndex < 0 || DestinationIndex >= MaxInventorySlots) { return bSucceed; }

	// InventoryItem->InventoryIndex Swap
	FInventoryItem* SourceItem = FindInventoryItemByIndex(SourceIndex);
	FInventoryItem* DestinationItem = FindInventoryItemByIndex(DestinationIndex);

	if(SourceItem)
	{
		SourceItem->InventoryIndex = DestinationIndex;
		if(DestinationItem) { DestinationItem->InventoryIndex = SourceIndex; }
		bSucceed = true;
	}

	UE_LOG(LogInventory, Log, TEXT("InventoryComponent::SwapItems\n%d to %d"), SourceIndex, DestinationIndex);
	
	// 이벤트 디스패처 호출
	//OnUpdate.Broadcast();
	return bSucceed;
}

int32 UInventoryComponent::GetEmptyIndex()
{
	// 아이템이 들어있는 인벤토리 슬롯 인덱스 검색
	TArray<int32> Indices;
	for(FInventoryItem InventoryItem : Inventory)
	{
		Indices.Add(InventoryItem.InventoryIndex);
	}

	// 비어 있는 인덱스 중 가장 작은 인덱스 반환s
	for(int i = 0; i < MaxInventorySlots; i++)
	{
		if(Indices.Find(i) == INDEX_NONE)
		{
			return i;
		}
	}
	UE_LOG(LogInventory, Error, TEXT("InventoryComponent::GetEmptyIndex\nNo Empty Index. Use IsAddable() first."));
	return -1;
}

bool UInventoryComponent::FillSameItem(UItemDefinition* ItemDefinition, int32& ItemCount)
{
	// 인벤토리에 동일한 아이템 검색 후 용량이 덜 찬 아이템들만 인덱스 기록
	TArray<int32> Indices;
	for(FInventoryItem InventoryItem : Inventory)
	{
		if(InventoryItem == ItemDefinition && InventoryItem.IsAddable())
		{
			Indices.Add(InventoryItem.InventoryIndex);
		}
	}

	// 한 번이라도 채웠다면 true 반환
	if(Indices.IsEmpty()) { return false; }

	Indices.Sort();

	// 차례대로 아이템 개수 추가
	for(const int32 Index : Indices)
	{
		// 입력 아이템 소진 : ItemCount <= 0
		if(!FindInventoryItemByIndex(Index)->Add(ItemCount))
		{
			break;
		}
	}
	return true;
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

