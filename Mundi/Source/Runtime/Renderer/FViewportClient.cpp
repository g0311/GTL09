#include "pch.h"
#include "FViewportClient.h"
#include "FViewport.h"
#include "CameraComponent.h"
#include "CameraActor.h"
#include "World.h"
#include "PlayerController.h"
#include "Picking.h"
#include "SelectionManager.h"
#include "PlayerCameraManager.h"
#include "Gizmo/GizmoActor.h"
#include "RenderManager.h"
#include "RenderSettings.h"
#include "EditorEngine.h"
#include "PrimitiveComponent.h"
#include "Clipboard/ClipboardManager.h"
#include "InputManager.h"
#include "InputMappingContext.h"
#include "InputMappingSubsystem.h"

FVector FViewportClient::CameraAddPosition{};

FViewportClient::FViewportClient()
{
	ViewportType = EViewportType::Perspective;
	// 직교 뷰별 기본 카메라 설정
	Camera = NewObject<ACameraActor>();
	SetupCameraMode();

	// 입력 컨텍스트 설정
	SetupInputContext();
}

FViewportClient::~FViewportClient()
{
}

void FViewportClient::Tick(float DeltaTime)
{
	if (PerspectiveCameraInput)
	{
		Camera->ProcessEditorCameraInput(DeltaTime);
	}
	MouseWheel(DeltaTime);

	// 기즈모 상호작용 처리 (매 프레임)
	if (World && World->GetGizmoActor() && Camera && Viewport)
	{
		FVector2D MousePos = UInputManager::GetInstance().GetMousePosition();

		// 전역 마우스 위치를 뷰포트 로컬 좌표로 변환
		float LocalX = MousePos.X - Viewport->GetStartX();
		float LocalY = MousePos.Y - Viewport->GetStartY();

		World->GetGizmoActor()->ProcessGizmoInteraction(Camera, Viewport, LocalX, LocalY);
	}

	static UClipboardManager* ClipboardManager = NewObject<UClipboardManager>();

	// 키보드 입력 처리 (Ctrl+C/V)
	if (World)
	{
		UInputManager& InputManager = UInputManager::GetInstance();
		USelectionManager* SelectionManager = World->GetSelectionManager();

		// Ctrl 키가 눌려있는지 확인
		bool bIsCtrlDown = InputManager.IsKeyDown(VK_CONTROL);

		// Ctrl + C: 복사
		if (bIsCtrlDown && InputManager.IsKeyPressed('C'))
		{
			if (SelectionManager)
			{
				// 선택된 Actor가 있으면 Actor 복사
				if (SelectionManager->IsActorMode() && SelectionManager->GetSelectedActor())
				{
					ClipboardManager->CopyActor(SelectionManager->GetSelectedActor());
				}
			}
		}
		// Ctrl + V: 붙여넣기
		else if (bIsCtrlDown && InputManager.IsKeyPressed('V'))
		{
			// Actor 붙여넣기
			if (ClipboardManager->HasCopiedActor())
			{
				// 오프셋 적용 (1미터씩 이동)
				FVector Offset(1.0f, 1.0f, 1.0f);
				AActor* NewActor = ClipboardManager->PasteActorToWorld(World, Offset);

				if (NewActor && SelectionManager)
				{
					// 새로 생성된 Actor 선택
					SelectionManager->ClearSelection();
					SelectionManager->SelectActor(NewActor);
				}
			}
		}
	}
}

void FViewportClient::Draw(FViewport* Viewport)
{
	if (!Viewport || !World) return;

	UCameraComponent* RenderCameraComponent = nullptr;

	// PIE 모드 (Eject 아님): PlayerController의 카메라 컴포넌트 우선 사용
	if (World->bPie && !World->bPIEEjected)
	{
		APlayerController* PC = World->GetPlayerController();
		if (PC)
		{
			RenderCameraComponent = PC->GetActiveCameraComponent();
		}

		// 폴백: PlayerController나 카메라가 없으면 뷰포트 카메라 사용
		if (!RenderCameraComponent)
		{
			//UE_LOG("[FViewportClient::Draw]: PIE 모드이지만 PlayerController 카메라가 없습니다. 뷰포트 카메라를 사용합니다.");
			if (Camera)
			{
				Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
				RenderCameraComponent = Camera->GetCameraComponent();
			}
		}
	}
	else
	{
		// 에디터 모드 또는 PIE Ejected 모드: 에디터 카메라 사용
		if (!Camera)
		{
			//UE_LOG("[FViewportClient::Draw]: 에디터 카메라가 없습니다.");
			return;
		}

		// 뷰 타입에 따라 카메라 설정
		switch (ViewportType)
		{
		case EViewportType::Perspective:
		{
			Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
			break;
		}
		default: // 모든 Orthographic 케이스
		{
			Camera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
			SetupCameraMode();
			break;
		}
		}

		RenderCameraComponent = Camera->GetCameraComponent();
	}

	// 렌더링 수행
	URenderer* Renderer = URenderManager::GetInstance().GetRenderer();
	if (Renderer && RenderCameraComponent)
	{
		World->GetRenderSettings().SetViewModeIndex(ViewModeIndex);
		Renderer->RenderSceneForView(World, RenderCameraComponent, Viewport);
	}
}

void FViewportClient::SetupCameraMode()
{
	switch (ViewportType)
	{
	case EViewportType::Perspective:

		Camera->SetActorLocation(PerspectiveCameraPosition);
		Camera->SetRotationFromEulerAngles(PerspectiveCameraRotation);
		Camera->GetCameraComponent()->SetFOV(PerspectiveCameraFov);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	case EViewportType::Orthographic_Top:

		Camera->SetActorLocation({ CameraAddPosition.X, CameraAddPosition.Y, 1000 });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, 90, 0 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	case EViewportType::Orthographic_Bottom:

		Camera->SetActorLocation({ CameraAddPosition.X, CameraAddPosition.Y, -1000 });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, -90, 0 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	case EViewportType::Orthographic_Left:
		Camera->SetActorLocation({ CameraAddPosition.X, 1000 , CameraAddPosition.Z });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, 0, -90 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	case EViewportType::Orthographic_Right:
		Camera->SetActorLocation({ CameraAddPosition.X, -1000, CameraAddPosition.Z });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, 0, 90 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;

	case EViewportType::Orthographic_Front:
		Camera->SetActorLocation({ -1000 , CameraAddPosition.Y, CameraAddPosition.Z });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, 0, 0 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	case EViewportType::Orthographic_Back:
		Camera->SetActorLocation({ 1000 , CameraAddPosition.Y, CameraAddPosition.Z });
		Camera->SetActorRotation(FQuat::MakeFromEulerZYX({ 0, 0, 180 }));
		Camera->GetCameraComponent()->SetFOV(100);
		Camera->GetCameraComponent()->SetClipPlanes(0.1f, 1000.0f);
		break;
	}
}

void FViewportClient::MouseWheel(float DeltaSeconds)
{
	if (!Camera) return;

	UCameraComponent* CameraComponent = Camera->GetCameraComponent();
	if (!CameraComponent) return;
	float WheelDelta = UInputManager::GetInstance().GetMouseWheelDelta();

	float zoomFactor = CameraComponent->GetZoomFactor();
	zoomFactor *= (1.0f - WheelDelta * DeltaSeconds * 100.0f);

	CameraComponent->SetZoomFactor(zoomFactor);
}

void FViewportClient::SetupInputContext()
{
	// 입력 컨텍스트 생성
	ViewportInputContext = NewObject<UInputMappingContext>();

	// ==================== 입력 매핑 ====================
	// 좌클릭 액션 (피킹, PIE 커서 캡처)
	ViewportInputContext->MapActionMouse("Select", LeftButton, true);

	// 우클릭 액션 (카메라 컨트롤)
	ViewportInputContext->MapActionMouse("CameraControl", RightButton, true); // bConsume=true (포커스된 뷰포트만 받음)

	// ESC 키 액션 (PIE 커서 해제)
	ViewportInputContext->MapAction("ReleaseCursor", VK_ESCAPE, false, false, false, true);

	// F8 키 액션 (PIE Eject - 에디터 카메라로 전환)
	ViewportInputContext->MapAction("PIEEject", VK_F8, false, false, false, true);

	// 마우스 축 매핑
	ViewportInputContext->MapAxisMouse("CameraYaw", EInputAxisSource::MouseX, 1.0f);
	ViewportInputContext->MapAxisMouse("CameraPitch", EInputAxisSource::MouseY, 1.0f);

	// ==================== Delegate 바인딩 ====================
	// 좌클릭 누름
	ViewportInputContext->BindActionPressed("Select", [this]()
	{
		if (!World) return;

		// PIE 모드 (Eject 아님): 커서 캡처
		if (World->bPie && !World->bPIEEjected)
		{
			UInputManager::GetInstance().SetCursorVisible(false);
			UInputManager::GetInstance().LockCursor();
			return; // 게임 모드에서는 피킹 안 함
		}

		// 에디터 모드 또는 PIE Ejected 모드: 피킹 시스템
		if (!World->GetGizmoActor()) return;

		if (World->GetGizmoActor()->GetbIsHovering())
		{
			return;
		}

		// 마우스 위치 가져오기
		FVector2D MousePos = UInputManager::GetInstance().GetMousePosition();

		Camera->SetWorld(World);
		UPrimitiveComponent* PickedComponent = URenderManager::GetInstance().GetRenderer()->GetPrimitiveCollided(
			static_cast<int>(MousePos.X),
			static_cast<int>(MousePos.Y)
		);

		if (PickedComponent)
		{
			UE_LOG("[ViewportClient Picking] World: %p, SelectionManager: %p, bPie: %d, bPIEEjected: %d",
				World, World->GetSelectionManager(), World->bPie, World->bPIEEjected);
			World->GetSelectionManager()->SelectComponent(PickedComponent);
		}
		else
		{
			World->GetSelectionManager()->ClearSelection();
		}
	});

	// 좌클릭 해제
	// (기즈모는 Tick에서 InputManager를 직접 확인하여 드래그 종료를 감지함)

	// ESC 키 누름 (PIE 커서 해제)
	ViewportInputContext->BindActionPressed("ReleaseCursor", [this]()
	{
		if (World && World->bPie && !World->bPIEEjected)
		{
			UInputManager::GetInstance().SetCursorVisible(true);
			UInputManager::GetInstance().ReleaseCursor();
		}
	});

	// F8 키 누름 (PIE Eject 토글)
	ViewportInputContext->BindActionPressed("PIEEject", [this]()
	{
		if (World && World->bPie)
		{
			World->bPIEEjected = !World->bPIEEjected;

			if (World->bPIEEjected)
			{
				EnterPIEEjectMode();
			}
			else
			{
				ExitPIEEjectMode();
			}
		}
	});

	// 우클릭 누름
	ViewportInputContext->BindActionPressed("CameraControl", [this]()
	{
		// PIE 모드 (Eject 아님): 우클릭 카메라 비활성화
		if (World && World->bPie && !World->bPIEEjected)
		{
			return;
		}

		// 에디터 모드 또는 PIE Ejected 모드: 우클릭 카메라 활성화
		bIsMouseRightButtonDown = true;
		if (ViewportType == EViewportType::Perspective)
		{
			PerspectiveCameraInput = true;
		}

		UInputManager::GetInstance().SetCursorVisible(false);
		UInputManager::GetInstance().LockCursor();
	});

	// 우클릭 뗌
	ViewportInputContext->BindActionReleased("CameraControl", [this]()
	{
		// PIE 모드 (Eject 아님): 처리 안 함
		if (World && World->bPie && !World->bPIEEjected)
		{
			return;
		}

		// 에디터 모드 또는 PIE Ejected 모드: 카메라 비활성화 및 커서 복원
		bIsMouseRightButtonDown = false;
		PerspectiveCameraInput = false;

		UInputManager::GetInstance().SetCursorVisible(true);
		UInputManager::GetInstance().ReleaseCursor();
	});

	// 마우스 이동 (직교 투영 카메라)
	ViewportInputContext->BindAxis("CameraYaw", [this](float Value)
	{
		if (!bIsMouseRightButtonDown || !Camera) return;
		if (ViewportType == EViewportType::Perspective) return; // 원근은 Tick에서 처리

		// 직교 투영 카메라 이동
		const float basePixelToWorld = 0.05f;
		float zoom = Camera->GetCameraComponent()->GetZoomFactor();
		zoom = (zoom <= 0.f) ? 1.f : zoom;
		const float pixelToWorld = basePixelToWorld * zoom;

		const FVector right = Camera->GetRight();
		CameraAddPosition = CameraAddPosition - right * (Value * pixelToWorld);
		SetupCameraMode();
	});

	ViewportInputContext->BindAxis("CameraPitch", [this](float Value)
	{
		if (!bIsMouseRightButtonDown || !Camera) return;
		if (ViewportType == EViewportType::Perspective) return;

		// 직교 투영 카메라 이동
		const float basePixelToWorld = 0.05f;
		float zoom = Camera->GetCameraComponent()->GetZoomFactor();
		zoom = (zoom <= 0.f) ? 1.f : zoom;
		const float pixelToWorld = basePixelToWorld * zoom;

		const FVector up = Camera->GetUp();
		CameraAddPosition = CameraAddPosition + up * (Value * pixelToWorld);
		SetupCameraMode();
	});

	// InputContext는 초기에는 등록하지 않음 (클릭 시 포커스 받을 때 등록)
}

void FViewportClient::OnFocusGained()
{
	if (!ViewportInputContext) return;

	// InputContext 활성화 (PIE/에디터 모두)
	UInputMappingSubsystem::Get().AddMappingContext(ViewportInputContext, 100);

	// PIE 모드 (Eject 아님): 커서 다시 hide/lock
	if (World && World->bPie && !World->bPIEEjected)
	{
		UInputManager::GetInstance().SetCursorVisible(false);
		UInputManager::GetInstance().LockCursor();
	}
}

void FViewportClient::OnFocusLost()
{
	if (!ViewportInputContext) return;

	// InputContext 비활성화
	UInputMappingSubsystem::Get().RemoveMappingContext(ViewportInputContext);

	// PIE 모드 (Eject 아님): 커서 해제
	if (World && World->bPie && !World->bPIEEjected)
	{
		UInputManager::GetInstance().SetCursorVisible(true);
		UInputManager::GetInstance().ReleaseCursor();
	}
}

void FViewportClient::SetWorld(UWorld* InWorld)
{
	World = InWorld;

	// PIE 종료 시 Eject 플래그 리셋
	if (InWorld && !InWorld->bPie)
	{
		if (World->bPIEEjected)
		{
			World->bPIEEjected = false;
			UE_LOG("PIE Ended - Eject mode reset");
		}
	}
}

void FViewportClient::EnterPIEEjectMode()
{
	// 커서 표시 및 해제
	UInputManager::GetInstance().SetCursorVisible(true);
	UInputManager::GetInstance().ReleaseCursor();

	// PlayerController 입력 비활성화 (에디터 카메라만 활성)
	if (World && World->GetPlayerController())
	{
		APlayerController* PC = World->GetPlayerController();
		if (PC->GetInputContext())
		{
			UInputMappingSubsystem::Get().RemoveMappingContext(PC->GetInputContext());
		}
	}

	// GizmoActor 설정
	if (World && World->GetGizmoActor())
	{
		AGizmoActor* Gizmo = World->GetGizmoActor();

		// 에디터 카메라 설정
		Gizmo->SetCameraActor(Camera);

		// 모든 컴포넌트를 PIE에서 보이도록 설정
		for (USceneComponent* Comp : Gizmo->GetSceneComponents())
		{
			if (Comp)
			{
				Comp->SetHiddenInGame(false);
			}
		}
	}

	UE_LOG("PIE Ejected - Editor camera control enabled");
}

void FViewportClient::ExitPIEEjectMode()
{
	// 커서 숨김 및 잠금
	UInputManager::GetInstance().SetCursorVisible(false);
	UInputManager::GetInstance().LockCursor();

	// PlayerController 입력 재활성화 (플레이어 컨트롤 복원)
	if (World && World->GetPlayerController())
	{
		APlayerController* PC = World->GetPlayerController();
		if (PC->GetInputContext())
		{
			UInputMappingSubsystem::Get().AddMappingContext(PC->GetInputContext(), 0);
		}
	}

	// 선택 해제
	if (World && World->GetSelectionManager())
	{
		World->GetSelectionManager()->ClearSelection();
	}

	// GizmoActor 숨김
	if (World && World->GetGizmoActor())
	{
		AGizmoActor* Gizmo = World->GetGizmoActor();

		// 모든 컴포넌트를 PIE에서 숨김
		for (USceneComponent* Comp : Gizmo->GetSceneComponents())
		{
			if (Comp)
			{
				Comp->SetHiddenInGame(true);
			}
		}
	}

	UE_LOG("PIE Resumed - Game mode");
}
