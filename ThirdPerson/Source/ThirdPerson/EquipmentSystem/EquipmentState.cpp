﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentState.h"

//////////////////////////////////////////////////////////////////////
// Equipment State
UEquipmentState::UEquipmentState()
{
	if(GetOuter() == nullptr){ UE_LOG(LogEquipment, Fatal, TEXT("UEquipmentState::UEquipmentState() > You must use NewObject<UEquipmentState>(Outer)")) }
	EquipmentComponent = Cast<UEquipmentComponent>(GetOuter());
}

void UEquipmentState::SwapToSelectedEquipment(FGameplayTag Slot)
{
	SelectedSlot = Slot;
	if(SelectedSlot.MatchesTag(GetMainTag()))
	{
		SwapToMainEquipment();
	}
	else
	{
		SwapToSubEquipment();
	}
}

//////////////////////////////////////////////////////////////////////
// Main Equipment
void UMainState::SwapToMainEquipment()
{
	// 현재 주 장비와 선택된 주 장비 부착 위치 교체
	if(CurrentSlot.MatchesTagExact(GetPrimarySlot()))
	{
		SwapSlots(CurrentSlot, SelectedSlot);
	}
	else if(SelectedSlot.MatchesTagExact(GetPrimarySlot()))
	{
		RestoreEquipmentToSlot(GetPrimarySlot());
		RestoreEquipmentToSlot(CurrentSlot);
	}
	else
	{
		RestoreEquipmentToSlot(CurrentSlot);
		SwapSlots(GetPrimarySlot(), SelectedSlot);
	}
	
	// 선택된 장비 슬롯으로 CurrentSlot 변경
	CurrentSlot = SelectedSlot;
}

void UMainState::SwapToSubEquipment()
{
	// 현재 들고 있는 주 장비 비활성화 후, 선택된 서브 장비를 PrimarySlot에 부착
	EquipmentComponent->SetEquipmentDisabled(true, CurrentSlot);
	EquipmentComponent->MoveEquipmentToSlot(SelectedSlot, GetPrimarySlot());

	// 선택된 장비 슬롯으로 SubState의 CurrentSlot 변경
	GetSubState()->CurrentSlot = SelectedSlot;

	// SubState로 상태 변경
	ChangeState(GetSubState());
}

//////////////////////////////////////////////////////////////////////
// Sub Equipment

void USubState::SwapToMainEquipment()
{
	ReturnToMainState();

	// Todo 플레이어 설정에 따라 원래 들고 있던 주 장비로만 교체되도록 설정할 수 있는 기능 만들어도 좋을듯
	// 원래 들고 있던 주 장비가 아닌 선택한 주 장비로 교체
	GetMainState()->SwapToSelectedEquipment(SelectedSlot);
}

void USubState::SwapToSubEquipment()
{
	// 새로운 서브 장비를 PrimarySlot에 부착
	EquipmentComponent->MoveEquipmentToSlot(SelectedSlot, GetPrimarySlot());

	CurrentSlot = SelectedSlot;
}

void USubState::SwapToSelectedEquipment(FGameplayTag Slot)
{
	// 현재 서브 장비 원위치
	RestoreEquipmentToSlot(CurrentSlot);
	Super::SwapToSelectedEquipment(Slot);
}

void USubState::ReturnToMainState()
{
	// 원래 들고 있던 주 장비 활성화
	EquipmentComponent->SetEquipmentDisabled(false, GetMainState()->CurrentSlot);
	CurrentSlot = FGameplayTag::EmptyTag;
	
	// MainState로 상태 변경
	ChangeState(GetMainState());
}