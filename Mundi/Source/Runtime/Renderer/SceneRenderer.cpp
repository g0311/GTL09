﻿#include "pch.h"
#include "SceneRenderer.h"

// FSceneRenderer가 사용하는 모든 헤더 포함
#include "World.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Renderer.h"
#include "RHIDevice.h"
#include "PrimitiveComponent.h"
#include "DecalComponent.h"
#include "StaticMeshActor.h"
#include "Grid/GridActor.h"
#include "Gizmo/GizmoActor.h"
#include "RenderSettings.h"
#include "Occlusion.h"
#include "Frustum.h"
#include "WorldPartitionManager.h"
#include "BVHierarchy.h"
#include "SelectionManager.h"
#include "StaticMeshComponent.h"
#include "DecalStatManager.h"
#include "BillboardComponent.h"
#include "TextRenderComponent.h"
#include "OBB.h"
#include "BoundingSphere.h"
#include "HeightFogComponent.h"
#include "Gizmo/GizmoArrowComponent.h"
#include "Gizmo/GizmoRotateComponent.h"
#include "Gizmo/GizmoScaleComponent.h"
#include "DirectionalLightComponent.h"
#include "AmbientLightComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "SwapGuard.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "Shader.h"
#include "ResourceManager.h"
#include "TileLightCuller.h"
#include "LineComponent.h"
#include "LightStats.h"
#include "ShadowStats.h"
#include "PlatformTime.h"

FSceneRenderer::FSceneRenderer(UWorld* InWorld, FSceneView* InView, URenderer* InOwnerRenderer)
	: World(InWorld)
	, View(InView) // 전달받은 FSceneView 저장
	, OwnerRenderer(InOwnerRenderer)
	, RHIDevice(InOwnerRenderer->GetRHIDevice())
{
	//OcclusionCPU = std::make_unique<FOcclusionCullingManagerCPU>();

	// 타일 라이트 컬러 초기화
	TileLightCuller = std::make_unique<FTileLightCuller>();
	uint32 TileSize = World->GetRenderSettings().GetTileSize();
	TileLightCuller->Initialize(RHIDevice, TileSize);

	// 라인 수집 시작
	OwnerRenderer->BeginLineBatch();
}

FSceneRenderer::~FSceneRenderer()
{
}

//====================================================================================
// 메인 렌더 함수
//====================================================================================
void FSceneRenderer::Render()
{
	if (!IsValid()) return;

	// 뷰(View) 준비: 행렬, 절두체 등 프레임에 필요한 기본 데이터 계산
	PrepareView();
	// 렌더링할 대상 수집 (Cull + Gather)
	GatherVisibleProxies();

	TIME_PROFILE(ShadowMapPass)
	RenderShadowMaps();
	TIME_PROFILE_END(ShadowMapPass)
	
	// ViewMode에 따라 렌더링 경로 결정
	if (View->ViewMode == EViewModeIndex::VMI_Lit ||
		View->ViewMode == EViewModeIndex::VMI_Lit_Gouraud ||
		View->ViewMode == EViewModeIndex::VMI_Lit_Lambert ||
		View->ViewMode == EViewModeIndex::VMI_Lit_Phong)
	{
		GWorld->GetLightManager()->UpdateLightBuffer(RHIDevice);	//라이트 구조체 버퍼 업데이트, 바인딩
		PerformTileLightCulling();	// 타일 기반 라이트 컬링 수행
		RenderLitPath();
		RenderPostProcessingPasses();	// 후처리 체인 실행
		RenderTileCullingDebug();	// 타일 컬링 디버그 시각화 draw
	}
	else if (View->ViewMode == EViewModeIndex::VMI_Unlit)
	{
		RenderLitPath();	// Unlit 모드는 조명 없이 렌더링
	}
	else if (View->ViewMode == EViewModeIndex::VMI_WorldNormal)
	{
		RenderLitPath();	// World Normal 시각화 모드
	}
	else if (View->ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		RenderWireframePath();
	}
	else if (View->ViewMode == EViewModeIndex::VMI_SceneDepth)
	{
		RenderSceneDepthPath();
	}

	//그리드와 디버그용 Primitive는 Post Processing 적용하지 않음.
	RenderEditorPrimitivesPass();	// 빌보드, 기타 화살표 출력 (상호작용, 피킹 O)
	RenderDebugPass();	//  그리드, 선택한 물체의 경계 출력 (상호작용, 피킹 X)
	
	// 오버레이(Overlay) Primitive 렌더링
	RenderOverayEditorPrimitivesPass();	// 기즈모 출력

	// FXAA 등 화면에서 최종 이미지 품질을 위해 적용되는 효과를 적용
	ApplyScreenEffectsPass();

	// 최종적으로 Scene에 그려진 텍스쳐를 Back 버퍼에 그힌다
	CompositeToBackBuffer();
}

//====================================================================================
// Render Path 함수 구현
//====================================================================================

void FSceneRenderer::RenderLitPath()
{
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithId);

	// Base Pass
	RenderOpaquePass(View->ViewMode);
	RenderDecalPass();
}

void FSceneRenderer::RenderWireframePath()
{
	// 깊이 버퍼 초기화 후 ID만 그리기
	RHIDevice->RSSetState(ERasterizerMode::Solid);
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneIdTarget);
	RenderOpaquePass(EViewModeIndex::VMI_Unlit);

	// Wireframe으로 그리기
	RHIDevice->ClearDepthBuffer(1.0f, 0);
	RHIDevice->RSSetState(ERasterizerMode::Wireframe);
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTarget);
	RenderOpaquePass(EViewModeIndex::VMI_Unlit);

	// 상태 복구
	RHIDevice->RSSetState(ERasterizerMode::Solid);
}

void FSceneRenderer::RenderSceneDepthPath()
{
	// ✅ 디버그: SceneRTV 전환 전 viewport 확인
	D3D11_VIEWPORT vpBefore;
	UINT numVP = 1;
	RHIDevice->GetDeviceContext()->RSGetViewports(&numVP, &vpBefore);
	UE_LOG("[RenderSceneDepthPath] BEFORE OMSetRenderTargets(Scene): Viewport(%.1f x %.1f) at (%.1f, %.1f)",
		vpBefore.Width, vpBefore.Height, vpBefore.TopLeftX, vpBefore.TopLeftY);

	// 1. Scene RTV와 Depth Buffer Clear
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithId);

	// ✅ 디버그: SceneRTV 전환 후 viewport 확인
	D3D11_VIEWPORT vpAfter;
	RHIDevice->GetDeviceContext()->RSGetViewports(&numVP, &vpAfter);
	UE_LOG("[RenderSceneDepthPath] AFTER OMSetRenderTargets(Scene): Viewport(%.1f x %.1f) at (%.1f, %.1f)",
		vpAfter.Width, vpAfter.Height, vpAfter.TopLeftX, vpAfter.TopLeftY);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	RHIDevice->GetDeviceContext()->ClearRenderTargetView(RHIDevice->GetCurrentTargetRTV(), ClearColor);
	RHIDevice->ClearDepthBuffer(1.0f, 0);

	// 2. Base Pass - Scene에 메시 그리기
	RenderOpaquePass(EViewModeIndex::VMI_Unlit);

	// ✅ 디버그: BackBuffer 전환 전 viewport 확인
	RHIDevice->GetDeviceContext()->RSGetViewports(&numVP, &vpBefore);
	UE_LOG("[RenderSceneDepthPath] BEFORE OMSetRenderTargets(BackBuffer): Viewport(%.1f x %.1f)",
		vpBefore.Width, vpBefore.Height);

	// 3. BackBuffer Clear
	RHIDevice->OMSetRenderTargets(ERTVMode::BackBufferWithoutDepth);
	RHIDevice->GetDeviceContext()->ClearRenderTargetView(RHIDevice->GetBackBufferRTV(), ClearColor);

	// ✅ 디버그: BackBuffer 전환 후 viewport 확인
	RHIDevice->GetDeviceContext()->RSGetViewports(&numVP, &vpAfter);
	UE_LOG("[RenderSceneDepthPath] AFTER OMSetRenderTargets(BackBuffer): Viewport(%.1f x %.1f)",
		vpAfter.Width, vpAfter.Height);

	// 4. SceneDepth Post 프로세싱 처리
	RenderSceneDepthPostProcess();

}

//====================================================================================
// 그림자맵 구현
//====================================================================================

void FSceneRenderer::RenderShadowMaps()
{
	FLightManager* LightManager = GWorld->GetLightManager();
	if (!LightManager) return;

	// 2. 그림자 캐스터(Caster) 메시 수집
	TArray<FMeshBatchElement> ShadowMeshBatches;
	for (UMeshComponent* MeshComponent : Proxies.Meshes)
	{
		if (MeshComponent && MeshComponent->IsCastShadows() && MeshComponent->IsVisible())
		{
			MeshComponent->CollectMeshBatches(ShadowMeshBatches, View);
		}
	}

	// NOTE: 카메라 오버라이드 기능을 항상 활성화 하기 위해서 그림자를 그릴 곳이 없어도 함수 실행
	//if (ShadowMeshBatches.IsEmpty()) return;

	// 섀도우 맵을 DSV로 사용하기 전에 SRV 슬롯에서 해제
	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(8, 2, nullSRVs); // 슬롯 8과 9 해제

	// 뷰 설정 복구용 데이터
	FMatrix InvView = View->ViewMatrix.InverseAffine();
	FMatrix InvProjection;
	if (View->ProjectionMode == ECameraProjectionMode::Perspective) { InvProjection = View->ProjectionMatrix.InversePerspectiveProjection(); }
	else { InvProjection = View->ProjectionMatrix.InverseOrthographicProjection(); }
	ViewProjBufferType OriginViewProjBuffer = ViewProjBufferType(View->ViewMatrix, View->ProjectionMatrix, InvView, InvProjection);

	D3D11_VIEWPORT OriginVP;
	UINT NumViewports = 1;
	RHIDevice->GetDeviceContext()->RSGetViewports(&NumViewports, &OriginVP);

	// 4. 상수 정의
	const FMatrix BiasMatrix( // 클립 공간 -> UV 공간 변환
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);

	// 1.2. 2D 섀도우 요청 수집
	TArray<FShadowRenderRequest> Requests2D;
	TArray<FShadowRenderRequest> RequestsCube;
	for (UDirectionalLightComponent* Light : LightManager->GetDirectionalLightList())
	{
		Light->GetShadowRenderRequests(View, Requests2D);
		// IsOverrideCameraLightPerspective 임시 구현
		if (Light->IsOverrideCameraLightPerspective())
		{
			OriginViewProjBuffer.View = Requests2D[Requests2D.Num() - 1].ViewMatrix;
			OriginViewProjBuffer.Proj = Requests2D[Requests2D.Num() - 1].ProjectionMatrix;
			OriginViewProjBuffer.InvView = OriginViewProjBuffer.View.Inverse();
			OriginViewProjBuffer.InvProj = OriginViewProjBuffer.Proj.Inverse();
		}
	}
	for (USpotLightComponent* Light : LightManager->GetSpotLightList())
	{
		//Light->CalculateWarpMatrix(OwnerRenderer, View->Camera, View->Viewport);
		Light->GetShadowRenderRequests(View, Requests2D);
		// IsOverrideCameraLightPerspective 임시 구현
		if (Light->IsOverrideCameraLightPerspective())
		{
			OriginViewProjBuffer.View = Requests2D[Requests2D.Num() - 1].ViewMatrix;
			OriginViewProjBuffer.Proj = Requests2D[Requests2D.Num() - 1].ProjectionMatrix;
			OriginViewProjBuffer.InvView = OriginViewProjBuffer.View.Inverse();
			OriginViewProjBuffer.InvProj = OriginViewProjBuffer.Proj.Inverse();
		}
	}
	for (UPointLightComponent* Light : LightManager->GetPointLightList())
	{
		Light->GetShadowRenderRequests(View, RequestsCube); // OriginalSubViewIndex(0~5) 채워짐
		// IsOverrideCameraLightPerspective 임시 구현
		if (Light->IsOverrideCameraLightPerspective())
		{
			int32 CamNum = std::clamp((int)Light->GetOverrideCameraLightNum(), 0, 5);
			OriginViewProjBuffer.View = RequestsCube[RequestsCube.Num() - 6 + CamNum].ViewMatrix;
			OriginViewProjBuffer.Proj = RequestsCube[RequestsCube.Num() - 6 + CamNum].ProjectionMatrix;
			OriginViewProjBuffer.InvView = OriginViewProjBuffer.View.Inverse();
			OriginViewProjBuffer.InvProj = OriginViewProjBuffer.Proj.Inverse();
		}
	}

	// SF_Shadows와 관련 없이 IsOverrideCameraLightPerspective 를 사용하기 위해서 밑에서 처리
	if (!World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Shadows))
	{
		LightManager->ClearAllRenderTargetView(RHIDevice);
		return;
	}

	// 2D 아틀라스 할당
	LightManager->AllocateAtlasRegions2D(Requests2D);
	// 2.2. 큐브맵 슬라이스 할당 (Allocate only)
	LightManager->AllocateAtlasCubeSlices(RequestsCube); // FLightManager가 RequestsCube의 AssignedSliceIndex와 Size 업데이트

	// --- 1단계: 2D 아틀라스 렌더링 (Spot + Directional) - RTV만 사용 ---
	{
		ID3D11RenderTargetView* AtlasRTV2D = LightManager->GetShadowAtlasRTV2D();
		float AtlasTotalSize2D = (float)LightManager->GetShadowAtlasSize2D();
		if (AtlasRTV2D && AtlasTotalSize2D > 0)
		{
			ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
			RHIDevice->GetDeviceContext()->PSSetShaderResources(9, 1, NullSRV);
			
			ID3D11DepthStencilView* DSV2D = LightManager->GetShadowDepthDSV2D();
			float ClearColor[] = {1.0f, 1.0f, 0.0f, 0.0f}; // R=depth, G=depth^2
			RHIDevice->OMSetCustomRenderTargets(1, &AtlasRTV2D, DSV2D);
			RHIDevice->GetDeviceContext()->ClearRenderTargetView(AtlasRTV2D, ClearColor);
			if (DSV2D) RHIDevice->GetDeviceContext()->ClearDepthStencilView(DSV2D, D3D11_CLEAR_DEPTH, 1.0f, 0);

			RHIDevice->RSSetState(ERasterizerMode::Shadows);

			for (FShadowRenderRequest& Request : Requests2D)
			{
				// 뷰포트 설정
				D3D11_VIEWPORT ShadowVP = { Request.AtlasViewportOffset.X, Request.AtlasViewportOffset.Y, static_cast<FLOAT>(Request.Size), static_cast<FLOAT>(Request.Size), 0.0f, 1.0f };
				RHIDevice->GetDeviceContext()->RSSetViewports(1, &ShadowVP);

				// 뎁스 패스 렌더링
				RenderShadowDepthPass(Request, ShadowMeshBatches);

				FShadowMapData Data;
				if (Request.Size > 0) // 렌더링 성공
				{
					Data.ShadowViewProjMatrix = Request.ViewMatrix * Request.ProjectionMatrix * BiasMatrix;
					Data.SampleCount = Request.SampleCount;
					Data.AtlasScaleOffset = Request.AtlasScaleOffset;
					Data.WorldPosition = Request.WorldLocation;
					Data.Radius = Request.Radius;
				}
				// 렌더링 실패 시(Size==0) 빈 데이터(기본값) 전달
				LightManager->SetShadowMapData(Request.LightOwner, Request.SubViewIndex, Data);
			}
			ID3D11RenderTargetView* NullRTV[1] = { nullptr };
			RHIDevice->OMSetCustomRenderTargets(1, NullRTV, nullptr);
		}
	}

	// --- 2단계: 큐브맵 아틀라스 렌더링 (Point) ---
	{
		uint32 AtlasSizeCube = LightManager->GetShadowCubeArraySize();
		uint32 MaxCubeSlices = LightManager->GetShadowCubeArrayCount(); // MaxCubeSlices는 FLightManager에서 가져옴
		if (AtlasSizeCube > 0 && MaxCubeSlices > 0)
		{
			// 2.1. RHI 상태 설정 (큐브맵)
			RHIDevice->RSSetState(ERasterizerMode::Shadows);

			// 큐브맵은 항상 1:1 종횡비의 전체 뷰포트 사용
			D3D11_VIEWPORT ShadowVP = { 0.0f, 0.0f, (float)AtlasSizeCube, (float)AtlasSizeCube, 0.0f, 1.0f };
			RHIDevice->GetDeviceContext()->RSSetViewports(1, &ShadowVP);

			// 이제 RequestsCube 배열을 직접 순회
			for (FShadowRenderRequest& Request : RequestsCube) // 레퍼런스 유지
			{
				// 슬라이스 할당 실패는 FLightManager::Allocate... 함수가 처리 (Size=0 설정)
				if (Request.Size == 0) // 할당 실패한 요청 건너뛰기
				{
					// 실패 시 FLightManager에 알림 (선택적이지만 안전)
					LightManager->SetShadowCubeMapData(Request.LightOwner, -1);
					continue;
				}
				else
				{
					//Data.ShadowViewProjMatrix = Request.ViewMatrix * Request.ProjectionMatrix * BiasMatrix;
					LightManager->SetShadowCubeMapData(Request.LightOwner, Request.AssignedSliceIndex);
				}

				// 할당된 슬라이스 인덱스와 원본 면 인덱스 사용
				int32 SliceIndex = Request.AssignedSliceIndex;   // FLightManager가 할당한 값
				int32 FaceIndex = Request.SubViewIndex; // 원본 면 인덱스

			// 2.3. 면 렌더링 (RTV 사용)
				ID3D11RenderTargetView* FaceRTV = LightManager->GetShadowCubeFaceRTV(SliceIndex, FaceIndex);
				ID3D11DepthStencilView* DSVCube = LightManager->GetShadowDepthDSVCube();
				if (FaceRTV)
				{
					float ClearColor[] = {1.0f, 1.0f, 0.0f, 0.0f};
					RHIDevice->OMSetCustomRenderTargets(1, &FaceRTV, DSVCube);
					RHIDevice->GetDeviceContext()->ClearRenderTargetView(FaceRTV, ClearColor);
					if (DSVCube) RHIDevice->GetDeviceContext()->ClearDepthStencilView(DSVCube, D3D11_CLEAR_DEPTH, 1.0f, 0);
					RenderShadowDepthPass(Request, ShadowMeshBatches);
				}
			}
		}
	}

	// --- 3. RHI 상태 복구 ---
	RHIDevice->RSSetState(ERasterizerMode::Solid);
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->OMSetCustomRenderTargets(1, &nullRTV, nullptr);
	//RHIDevice->RSSetViewport(); // 메인 뷰포트로 복구
	// 4. 저장해둔 'OriginVP'로 뷰포트를 복구합니다. (이때는 주소(&)가 필요 없음)
	RHIDevice->GetDeviceContext()->RSSetViewports(1, &OriginVP);
	
	// ViewProjBufferType 복구 (라이트 시점 Override 일 경우 마지막 라이트 시점으로 설정됨)
	RHIDevice->SetAndUpdateConstantBuffer(ViewProjBufferType(OriginViewProjBuffer));
}

void FSceneRenderer::RenderShadowDepthPass(FShadowRenderRequest& ShadowRequest, const TArray<FMeshBatchElement>& InShadowBatches)
{
	// 1. 뎁스 전용 셰이더 로드
	UShader* DepthVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Shadows/DepthOnly_VS.hlsl");
	if (!DepthVS || !DepthVS->GetVertexShader()) return;

	FShaderVariant* ShaderVariantVS = DepthVS->GetOrCompileShaderVariant(RHIDevice->GetDevice());
	if (!ShaderVariantVS) return;

	UShader* DepthPS = UResourceManager::GetInstance().Load<UShader>("Shaders/Shadows/DepthOnly_PS.hlsl");
	if (!DepthPS || !DepthPS->GetPixelShader()) return;

	FShaderVariant* ShaderVariantPS = DepthPS->GetOrCompileShaderVariant(RHIDevice->GetDevice());
	if (!ShaderVariantPS) return;

	// 2. 파이프라인 설정 (RTV를 위해 항상 PS 필요)
	RHIDevice->GetDeviceContext()->IASetInputLayout(ShaderVariantVS->InputLayout);
	RHIDevice->GetDeviceContext()->VSSetShader(ShaderVariantVS->VertexShader, nullptr, 0);
	RHIDevice->GetDeviceContext()->PSSetShader(ShaderVariantPS->PixelShader, nullptr, 0);

	// 3. 라이트의 View-Projection 행렬을 메인 ViewProj 버퍼에 설정
	FMatrix WorldLocation = {};
	WorldLocation.VRows[0] = FVector4(ShadowRequest.WorldLocation.X, ShadowRequest.WorldLocation.Y, ShadowRequest.WorldLocation.Z, ShadowRequest.Radius);
	ViewProjBufferType ViewProjBuffer = ViewProjBufferType(ShadowRequest.ViewMatrix, ShadowRequest.ProjectionMatrix, WorldLocation, FMatrix::Identity());	// NOTE: 그림자 맵 셰이더에는 역행렬이 필요 없으므로 Identity를 전달함
	RHIDevice->SetAndUpdateConstantBuffer(ViewProjBufferType(ViewProjBuffer));

	// 4. (DrawMeshBatches와 유사하게) 배치 순회하며 그리기
	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11Buffer* CurrentIndexBuffer = nullptr;
	UINT CurrentVertexStride = 0;
	D3D11_PRIMITIVE_TOPOLOGY CurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

	for (const FMeshBatchElement& Batch : InShadowBatches)
	{
		// 셰이더/픽셀 상태 변경 불필요

		// IA 상태 변경
		if (Batch.VertexBuffer != CurrentVertexBuffer ||
			Batch.IndexBuffer != CurrentIndexBuffer ||
			Batch.VertexStride != CurrentVertexStride ||
			Batch.PrimitiveTopology != CurrentTopology)
		{
			UINT Stride = Batch.VertexStride;
			UINT Offset = 0;
			RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 1, &Batch.VertexBuffer, &Stride, &Offset);
			RHIDevice->GetDeviceContext()->IASetIndexBuffer(Batch.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(Batch.PrimitiveTopology);

			CurrentVertexBuffer = Batch.VertexBuffer;
			CurrentIndexBuffer = Batch.IndexBuffer;
			CurrentVertexStride = Batch.VertexStride;
			CurrentTopology = Batch.PrimitiveTopology;
		}

		// 오브젝트별 World 행렬 설정 (VS에서 필요)
		RHIDevice->SetAndUpdateConstantBuffer(ModelBufferType(Batch.WorldMatrix, Batch.WorldMatrix.InverseAffine().Transpose()));

		// 드로우 콜
		RHIDevice->GetDeviceContext()->DrawIndexed(Batch.IndexCount, Batch.StartIndex, Batch.BaseVertexIndex);
	}
}


//====================================================================================
// Private 헬퍼 함수 구현
//====================================================================================

bool FSceneRenderer::IsValid() const
{
	return World && View && OwnerRenderer && RHIDevice;
}

void FSceneRenderer::PrepareView()
{
	OwnerRenderer->SetCurrentViewportSize(View->ViewRect.Width(), View->ViewRect.Height());

	// FSceneRenderer 멤버 변수(View->ViewMatrix, View->ProjectionMatrix)를 채우는 대신
	// FSceneView의 멤버를 직접 사용합니다. (예: RenderOpaquePass에서 View->ViewMatrix 사용)

	// 뷰포트 크기 설정
	D3D11_VIEWPORT Vp = {};
	Vp.TopLeftX = (float)View->ViewRect.MinX;
	Vp.TopLeftY = (float)View->ViewRect.MinY;
	Vp.Width = (float)View->ViewRect.Width();
	Vp.Height = (float)View->ViewRect.Height();
	Vp.MinDepth = 0.0f;
	Vp.MaxDepth = 1.0f;
	RHIDevice->GetDeviceContext()->RSSetViewports(1, &Vp);

	// 뷰포트 상수 버퍼 설정 (View->ViewRect, RHIDevice 크기 정보 사용)
	FViewportConstants ViewConstData;
	// 1. 뷰포트 정보 채우기
	ViewConstData.ViewportRect.X = Vp.TopLeftX;
	ViewConstData.ViewportRect.Y = Vp.TopLeftY;
	ViewConstData.ViewportRect.Z = Vp.Width;
	ViewConstData.ViewportRect.W = Vp.Height;
	// 2. 전체 화면(렌더 타겟) 크기 정보 채우기
	ViewConstData.ScreenSize.X = static_cast<float>(RHIDevice->GetViewportWidth());
	ViewConstData.ScreenSize.Y = static_cast<float>(RHIDevice->GetViewportHeight());
	ViewConstData.ScreenSize.Z = 1.0f / RHIDevice->GetViewportWidth();
	ViewConstData.ScreenSize.W = 1.0f / RHIDevice->GetViewportHeight();
	RHIDevice->SetAndUpdateConstantBuffer((FViewportConstants)ViewConstData);

	// 공통 상수 버퍼 설정 (View, Projection 등) - 루프 전에 한 번만
	FVector CameraPos = View->ViewLocation;

	FMatrix InvView = View->ViewMatrix.InverseAffine();
	FMatrix InvProjection;
	if (View->ProjectionMode == ECameraProjectionMode::Perspective)
	{
		InvProjection = View->ProjectionMatrix.InversePerspectiveProjection();
	}
	else
	{
		InvProjection = View->ProjectionMatrix.InverseOrthographicProjection();
	}

	ViewProjBufferType ViewProjBuffer = ViewProjBufferType(View->ViewMatrix, View->ProjectionMatrix, InvView, InvProjection);

	RHIDevice->SetAndUpdateConstantBuffer(ViewProjBufferType(ViewProjBuffer));
	RHIDevice->SetAndUpdateConstantBuffer(CameraBufferType(CameraPos, 0.0f));
}

void FSceneRenderer::GatherVisibleProxies()
{
	// NOTE: 일단 컴포넌트 단위와 데칼 관련 이슈 해결까지 컬링 무시
	//// 절두체 컬링 수행 -> 결과가 멤버 변수 PotentiallyVisibleActors에 저장됨
	//PerformFrustumCulling();

	const bool bDrawStaticMeshes = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_StaticMeshes);
	const bool bDrawDecals = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Decals);
	const bool bDrawFog = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Fog);
	const bool bDrawLight = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Lighting);
	const bool bUseAntiAliasing = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_FXAA);
	const bool bUseBillboard = World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Billboard);

	// Helper lambda to collect components from an actor
	auto CollectComponentsFromActor = [&](AActor* Actor, bool bIsEditorActor)
		{
			if (!Actor || !Actor->IsActorVisible())
			{
				return;
			}

			for (USceneComponent* Component : Actor->GetSceneComponents())
			{
				if (!Component || !Component->IsVisible())
				{
					continue;
				}

				// 엔진 에디터 액터 컴포넌트
				if (bIsEditorActor)
				{
					if (UGizmoArrowComponent* GizmoComponent = Cast<UGizmoArrowComponent>(Component))
					{
						Proxies.OverlayPrimitives.Add(GizmoComponent);
					}
					else if (ULineComponent* LineComponent = Cast<ULineComponent>(Component))
					{
						Proxies.EditorLines.Add(LineComponent);
					}

					continue;
				}

				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component); PrimitiveComponent)
				{
					// 에디터 보조 컴포넌트 (빌보드 등)
					if (!PrimitiveComponent->IsEditable())
					{
						Proxies.EditorPrimitives.Add(PrimitiveComponent);
						continue;
					}

					// 일반 컴포넌트
					if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(PrimitiveComponent))
					{
						bool bShouldAdd = true;

						// 메시 타입이 '스태틱 메시'인 경우에만 ShowFlag를 검사하여 추가 여부를 결정
						if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
						{
							bShouldAdd = bDrawStaticMeshes;
						}
						// else if (USkeletalMeshComponent* SkeletalMeshComponent = ...)
						// {
						//     bShouldAdd = bDrawSkeletalMeshes;
						// }

						if (bShouldAdd)
						{
							Proxies.Meshes.Add(MeshComponent);
						}
					}
					else if (UBillboardComponent* BillboardComponent = Cast<UBillboardComponent>(PrimitiveComponent); BillboardComponent && bUseBillboard)
					{
						Proxies.Billboards.Add(BillboardComponent);
					}
					else if (UDecalComponent* DecalComponent = Cast<UDecalComponent>(PrimitiveComponent); DecalComponent && bDrawDecals)
					{
						Proxies.Decals.Add(DecalComponent);
					}
				}
				else
				{
					if (UHeightFogComponent* FogComponent = Cast<UHeightFogComponent>(Component); FogComponent && bDrawFog)
					{
						SceneGlobals.Fogs.Add(FogComponent);
					}

					else if (UDirectionalLightComponent* LightComponent = Cast<UDirectionalLightComponent>(Component); LightComponent && bDrawLight)
					{
						SceneGlobals.DirectionalLights.Add(LightComponent);
					}

					else if (UAmbientLightComponent* LightComponent = Cast<UAmbientLightComponent>(Component); LightComponent && bDrawLight)
					{
						SceneGlobals.AmbientLights.Add(LightComponent);
					}

					else if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(Component); LightComponent && bDrawLight)
					{
						if (USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(LightComponent); SpotLightComponent)
						{
							SceneLocals.SpotLights.Add(SpotLightComponent);
						}
						else
						{
							SceneLocals.PointLights.Add(LightComponent);
						}
					}
				}
			}
		};

	// Collect from Editor Actors (Gizmo, Grid, etc.)
	for (AActor* EditorActor : World->GetEditorActors())
	{
		CollectComponentsFromActor(EditorActor, true);
	}

	// Collect from Level Actors (including their Gizmo components)
	for (AActor* Actor : World->GetActors())
	{
		CollectComponentsFromActor(Actor, false);
	}

	// 라이트 통계 업데이트
	FLightStats LightStats;
	LightStats.TotalPointLights = SceneLocals.PointLights.Num();
	LightStats.TotalSpotLights = SceneLocals.SpotLights.Num();
	LightStats.TotalDirectionalLights = SceneGlobals.DirectionalLights.Num();
	LightStats.TotalAmbientLights = SceneGlobals.AmbientLights.Num();
	LightStats.CalculateTotal();
	FLightStatManager::GetInstance().UpdateStats(LightStats);

	// 쉐도우 통계 업데이트
	FShadowStats ShadowStats;
	for (UPointLightComponent* Light : SceneLocals.PointLights)
	{
		if (Light && Light->IsCastShadows())
		{
			ShadowStats.ShadowCastingPointLights++;
		}
	}
	for (USpotLightComponent* Light : SceneLocals.SpotLights)
	{
		if (Light && Light->IsCastShadows())
		{
			ShadowStats.ShadowCastingSpotLights++;
		}
	}
	for (UDirectionalLightComponent* Light : SceneGlobals.DirectionalLights)
	{
		if (Light && Light->IsCastShadows())
		{
			ShadowStats.ShadowCastingDirectionalLights++;
		}
	}

	// 쉐도우 맵 아틀라스 정보
	FLightManager* LightManager = World->GetLightManager();
	if (LightManager)
	{
		ShadowStats.ShadowAtlas2DSize = static_cast<uint32>(LightManager->GetShadowAtlasSize2D());
		ShadowStats.ShadowAtlasCubeSize = LightManager->GetShadowCubeArraySize();
		ShadowStats.ShadowCubeArrayCount = LightManager->GetShadowCubeArrayCount();
		ShadowStats.Calculate2DAtlasMemory();
		ShadowStats.CalculateCubeAtlasMemory();
	}

	ShadowStats.CalculateTotal();
	FShadowStatManager::GetInstance().UpdateStats(ShadowStats);
}

void FSceneRenderer::PerformTileLightCulling()
{
	if (!TileLightCuller)
		return;

	// ShowFlag 확인
	URenderSettings& RenderSettings = World->GetRenderSettings();
	bool bTileCullingEnabled = RenderSettings.IsShowFlagEnabled(EEngineShowFlags::SF_TileCulling);

	// 뷰포트 크기 가져오기
	UINT ViewportWidth = static_cast<UINT>(View->ViewRect.Width());
	UINT ViewportHeight = static_cast<UINT>(View->ViewRect.Height());

	// 타일 컬링이 활성화된 경우에만 컬링 수행
	if (bTileCullingEnabled)
	{
		// PointLight와 SpotLight 정보 수집
		TArray<FPointLightInfo>& PointLights = GWorld->GetLightManager()->GetPointLightInfoList();
		TArray<FSpotLightInfo>& SpotLights = GWorld->GetLightManager()->GetSpotLightInfoList();

		// 타일 컬링 수행
		TileLightCuller->CullLights(
			PointLights,
			SpotLights,
			View->ViewMatrix,
			View->ProjectionMatrix,
			View->ZNear,
			View->ZFar,
			ViewportWidth,
			ViewportHeight
		);

		// 통계를 전역 매니저에 업데이트
		FTileCullingStatManager::GetInstance().UpdateStats(TileLightCuller->GetStats());
	}

	// 타일 컬링 상수 버퍼 업데이트
	uint32 TileSize = RenderSettings.GetTileSize();
	FTileCullingBufferType TileCullingBuffer;
	TileCullingBuffer.TileSize = TileSize;
	TileCullingBuffer.TileCountX = (ViewportWidth + TileSize - 1) / TileSize;
	TileCullingBuffer.TileCountY = (ViewportHeight + TileSize - 1) / TileSize;
	TileCullingBuffer.bUseTileCulling = bTileCullingEnabled ? 1 : 0;  // ShowFlag에 따라 설정
	TileCullingBuffer.ViewportStartX = View->ViewRect.MinX;  // ShowFlag에 따라 설정
	TileCullingBuffer.ViewportStartY = View->ViewRect.MinY;  // ShowFlag에 따라 설정

	RHIDevice->SetAndUpdateConstantBuffer(TileCullingBuffer);

	// Structured Buffer SRV를 t2 슬롯에 바인딩 (타일 컬링 활성화 시에만)
	if (bTileCullingEnabled)
	{
		ID3D11ShaderResourceView* TileLightIndexSRV = TileLightCuller->GetLightIndexBufferSRV();
		if (TileLightIndexSRV)
		{
			RHIDevice->GetDeviceContext()->PSSetShaderResources(2, 1, &TileLightIndexSRV);
		}
	}
}

void FSceneRenderer::PerformFrustumCulling()
{
	PotentiallyVisibleComponents.clear();	// 할 필요 없는데 명목적으로 초기화

	// Todo: 프로스텀 컬링 수행, 추후 프로스텀 컬링이 컴포넌트 단위로 변경되면 적용

	//World->GetPartitionManager()->FrustumQuery(ViewFrustum)

	//for (AActor* Actor : World->GetActors())
	//{
	//	if (!Actor || Actor->GetActorHiddenInEditor()) continue;

	//	// 절두체 컬링을 통과한 액터만 목록에 추가
	//	if (ViewFrustum.Intersects(Actor->GetBounds()))
	//	{
	//		PotentiallyVisibleActors.Add(Actor);
	//	}
	//}
}

void FSceneRenderer::RenderOpaquePass(EViewModeIndex InRenderViewMode)
{
	TArray<FShaderMacro> ShaderMacros;
	FString ShaderPath = "Shaders/Materials/UberLit.hlsl";
	bool bNeedsShaderOverride = true; // 뷰 모드가 셰이더를 강제하는지 여부

	switch (InRenderViewMode)
	{
	case EViewModeIndex::VMI_Lit:
	case EViewModeIndex::VMI_Lit_Phong:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_PHONG", "1" });
		break;
	case EViewModeIndex::VMI_Lit_Gouraud:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_GOURAUD", "1" });
		break;
	case EViewModeIndex::VMI_Lit_Lambert:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_LAMBERT", "1" });
		break;
	case EViewModeIndex::VMI_Unlit:
		// 매크로 없음 (Unlit)
		break;
	case EViewModeIndex::VMI_WorldNormal:
		ShaderMacros.push_back(FShaderMacro{ "VIEWMODE_WORLD_NORMAL", "1" });
		break;
	default:
		// 셰이더를 강제하지 않는 모드는 여기서 처리 가능
		bNeedsShaderOverride = false;
		break;
	}

	// 그림자 AA 설정
	URenderSettings& RenderSettings = GWorld->GetRenderSettings();
	if (RenderSettings.IsShowFlagEnabled(EEngineShowFlags::SF_ShadowAntiAliasing))
	{
		EShadowAATechnique Technique = RenderSettings.GetShadowAATechnique();
		if (Technique == EShadowAATechnique::PCF)
		{
			ShaderMacros.Add(FShaderMacro("SHADOW_AA_TECHNIQUE", "1")); // 1 = PCF
		}
		else if (Technique == EShadowAATechnique::VSM)
		{
			ShaderMacros.Add(FShaderMacro("SHADOW_AA_TECHNIQUE", "2")); // 2 = VSM
		}
	}
	else
	{
		ShaderMacros.Add(FShaderMacro("SHADOW_AA_TECHNIQUE", "0")); // 0 = Hard Shadow (AA 끔)
	}

	// ViewMode에 맞는 셰이더 로드 (셰이더 오버라이드가 필요한 경우에만)
	FShaderVariant* ShaderVariant = nullptr;
	UShader* ViewModeShader = nullptr;
	if (bNeedsShaderOverride)
	{
		ViewModeShader = UResourceManager::GetInstance().Load<UShader>(ShaderPath);
		ShaderVariant = ViewModeShader->GetOrCompileShaderVariant(RHIDevice->GetDevice(), ShaderMacros);
		if (!ShaderVariant)
		{
			// 필요시 기본 셰이더로 대체하거나 렌더링 중단
			UE_LOG("RenderOpaquePass: Failed to load ViewMode shader: %s", ShaderPath.c_str());
			return;
		}
	}

	// --- 1. 수집 (Collect) ---
	MeshBatchElements.Empty();
	for (UMeshComponent* MeshComponent : Proxies.Meshes)
	{
		MeshComponent->CollectMeshBatches(MeshBatchElements, View);
	}

	// --- UMeshComponent 셰이더 오버라이드 ---
	if (bNeedsShaderOverride && ShaderVariant)
	{
		// 수집된 UMeshComponent 배치 요소의 셰이더를 ViewModeShader로 강제 변경
		for (FMeshBatchElement& BatchElement : MeshBatchElements)
		{
			BatchElement.VertexShader = ShaderVariant->VertexShader;
			BatchElement.PixelShader = ShaderVariant->PixelShader;
			BatchElement.InputLayout = ShaderVariant->InputLayout;
		}
	}

	for (UBillboardComponent* BillboardComponent : Proxies.Billboards)
	{
		BillboardComponent->CollectMeshBatches(MeshBatchElements, View);
	}

	for (UTextRenderComponent* TextRenderComponent : Proxies.Texts)
	{
		// TODO: UTextRenderComponent도 CollectMeshBatches를 통해 FMeshBatchElement를 생성하도록 구현
		//TextRenderComponent->CollectMeshBatches(MeshBatchElements, View);
	}

	// --- 2. 정렬 (Sort) ---
	MeshBatchElements.Sort();

	// --- 3. 그리기 (Draw) ---
	DrawMeshBatches(MeshBatchElements, true);
}

void FSceneRenderer::RenderDecalPass()
{
	if (Proxies.Decals.empty())
		return;

	// WorldNormal 모드에서는 Decal 렌더링 스킵
	if (View->ViewMode == EViewModeIndex::VMI_WorldNormal)
		return;

	UWorldPartitionManager* Partition = World->GetPartitionManager();
	if (!Partition)
		return;

	const FBVHierarchy* BVH = Partition->GetBVH();
	if (!BVH)
		return;

	FDecalStatManager::GetInstance().AddTotalDecalCount(Proxies.Decals.Num());	// TODO: 추후 월드 컴포넌트 추가/삭제 이벤트에서 데칼 컴포넌트의 개수만 추적하도록 수정 필요
	FDecalStatManager::GetInstance().AddVisibleDecalCount(Proxies.Decals.Num());	// 그릴 Decal 개수 수집

	// ViewMode에 따라 조명 모델 매크로 설정
	TArray<FShaderMacro> ShaderMacros;
	FString ShaderPath = "Shaders/Effects/Decal.hlsl";

	switch (View->ViewMode)
	{
	case EViewModeIndex::VMI_Lit_Phong:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_PHONG", "1" });
		break;
	case EViewModeIndex::VMI_Lit_Gouraud:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_GOURAUD", "1" });
		break;
	case EViewModeIndex::VMI_Lit_Lambert:
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_LAMBERT", "1" });
		break;
	case EViewModeIndex::VMI_Lit:
		// 기본 Lit 모드는 Phong 사용
		ShaderMacros.push_back(FShaderMacro{ "LIGHTING_MODEL_PHONG", "1" });
		break;
	case EViewModeIndex::VMI_Unlit:
		// 매크로 없음 (Unlit)
		break;
	default:
		// 기타 ViewMode는 매크로 없음
		break;
	}

	// ViewMode에 따른 Decal 셰이더 로드
	UShader* DecalShader = UResourceManager::GetInstance().Load<UShader>(ShaderPath, ShaderMacros);
	FShaderVariant* ShaderVariant = DecalShader->GetOrCompileShaderVariant(RHIDevice->GetDevice(), ShaderMacros);
	if (!DecalShader)
	{
		UE_LOG("RenderDecalPass: Failed to load Decal shader with ViewMode macros!");
		return;
	}

	// 데칼 렌더 설정
	RHIDevice->RSSetState(ERasterizerMode::Decal);
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly); // 깊이 쓰기 OFF
	RHIDevice->OMSetBlendState(true);

	for (UDecalComponent* Decal : Proxies.Decals)
	{
		if (!Decal || !Decal->GetDecalTexture())
		{
			continue;
		}

		// Decal이 그려질 Primitives
		TArray<UPrimitiveComponent*> TargetPrimitives;

		// 1. Decal의 World AABB와 충돌한 모든 StaticMeshComponent 쿼리
		const FOBB DecalOBB = Decal->GetWorldOBB();
		TArray<UStaticMeshComponent*> IntersectedStaticMeshComponents = BVH->QueryIntersectedComponents(DecalOBB);

		// 2. 충돌한 모든 visible Actor의 PrimitiveComponent를 TargetPrimitives에 추가
		// Actor에 기본으로 붙어있는 TextRenderComponent, BoundingBoxComponent는 decal 적용 안되게 하기 위해,
		// 임시로 PrimitiveComponent가 아닌 UStaticMeshComponent를 받도록 함
		for (UStaticMeshComponent* SMC : IntersectedStaticMeshComponents)
		{
			// 기즈모에 데칼 입히면 안되므로 에디팅이 안되는 Component는 데칼 그리지 않음
			if (!SMC || !SMC->IsEditable())
				continue;
			
			AActor* Owner = SMC->GetOwner();
			if (!Owner || !Owner->IsActorVisible())
				continue;

			FDecalStatManager::GetInstance().IncrementAffectedMeshCount();
			TargetPrimitives.push_back(SMC);
		}

		// --- 데칼 렌더 시간 측정 시작 ---
		auto CpuTimeStart = std::chrono::high_resolution_clock::now();

		// 데칼 전용 상수 버퍼 설정
		const FMatrix DecalMatrix = Decal->GetDecalProjectionMatrix();
		RHIDevice->SetAndUpdateConstantBuffer(DecalBufferType(DecalMatrix, Decal->GetOpacity()));

		// 3. TargetPrimitive 순회하며 수집 후 렌더링
		MeshBatchElements.Empty();
		for (UPrimitiveComponent* Target : TargetPrimitives)
		{
			Target->CollectMeshBatches(MeshBatchElements, View);
		}
		for (FMeshBatchElement& BatchElement : MeshBatchElements)
		{
			BatchElement.InstanceShaderResourceView = Decal->GetDecalTexture()->GetShaderResourceView();
			BatchElement.Material = Decal->GetMaterial(0);
			BatchElement.InputLayout = ShaderVariant->InputLayout;
			BatchElement.VertexShader = ShaderVariant->VertexShader;
			BatchElement.PixelShader = ShaderVariant->PixelShader;
			BatchElement.VertexStride = sizeof(FVertexDynamic);
		}
		DrawMeshBatches(MeshBatchElements, true);

		// --- 데칼 렌더 시간 측정 종료 및 결과 저장 ---
		auto CpuTimeEnd = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> CpuTimeMs = CpuTimeEnd - CpuTimeStart;
		FDecalStatManager::GetInstance().GetDecalPassTimeSlot() += CpuTimeMs.count(); // CPU 소요 시간 저장
	}

	// 상태 복구
	RHIDevice->RSSetState(ERasterizerMode::Solid);
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::LessEqual);
	RHIDevice->OMSetBlendState(false);
}

void FSceneRenderer::RenderPostProcessingPasses()
{
	UHeightFogComponent* FogComponent = nullptr;
	if (0 < SceneGlobals.Fogs.Num())
	{
		FogComponent = SceneGlobals.Fogs[0];
	}

	if (!FogComponent)
	{
		return;
	}

	// Swap 가드 객체 생성: 스왑을 수행하고, 소멸 시 0번 슬롯부터 2개의 SRV를 자동 해제하도록 설정
	FSwapGuard SwapGuard(RHIDevice, 0, 2);

	// 렌더 타겟 설정
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	// Depth State: Depth Test/Write 모두 OFF
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	// 쉐이더 설정
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* HeightFogPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/HeightFog_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !HeightFogPS || !HeightFogPS->GetPixelShader())
	{
		UE_LOG("HeightFog용 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(FullScreenTriangleVS, HeightFogPS);

	// 텍스쳐 관련 설정
	ID3D11ShaderResourceView* DepthSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth);
	ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	ID3D11SamplerState* PointClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);
	ID3D11SamplerState* LinearClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);

	if (!DepthSRV || !SceneSRV || !PointClampSamplerState || !LinearClampSamplerState)
	{
		UE_LOG("Depth SRV / Scene SRV / PointClamp Sampler / LinearClamp Sampler is null!\n");
		return;
	}

	ID3D11ShaderResourceView* srvs[2] = { DepthSRV, SceneSRV };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, srvs);

	ID3D11SamplerState* Samplers[2] = { LinearClampSamplerState, PointClampSamplerState };
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, Samplers);

	// 상수 버퍼 업데이트
	ECameraProjectionMode ProjectionMode = View->ProjectionMode;
	//RHIDevice->UpdatePostProcessCB(ZNear, ZFar, ProjectionMode == ECameraProjectionMode::Orthographic);
	RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->ZNear, View->ZFar, ProjectionMode == ECameraProjectionMode::Orthographic));
	UHeightFogComponent* F = FogComponent;
	//RHIDevice->UpdateFogCB(F->GetFogDensity(), F->GetFogHeightFalloff(), F->GetStartDistance(), F->GetFogCutoffDistance(), F->GetFogInscatteringColor()->ToFVector4(), F->GetFogMaxOpacity(), F->GetFogHeight());
	RHIDevice->SetAndUpdateConstantBuffer(FogBufferType(F->GetFogDensity(), F->GetFogHeightFalloff(), F->GetStartDistance(), F->GetFogCutoffDistance(), F->GetFogInscatteringColor().ToFVector4(), F->GetFogMaxOpacity(), F->GetFogHeight()));

	// Draw
	RHIDevice->DrawFullScreenQuad();

	// 모든 작업이 성공적으로 끝났으므로 Commit 호출
	// 이제 소멸자는 버퍼 스왑을 되돌리지 않고, SRV 해제 작업만 수행함
	SwapGuard.Commit();
}

void FSceneRenderer::RenderSceneDepthPostProcess()
{
	// Swap 가드 객체 생성: 스왑을 수행하고, 소멸 시 0번 슬롯부터 1개의 SRV를 자동 해제하도록 설정
	FSwapGuard SwapGuard(RHIDevice, 0, 1);

	// 렌더 타겟 설정 (Depth 없이 BackBuffer에만 그리기)
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	// Depth State: Depth Test/Write 모두 OFF
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	// 쉐이더 설정
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* SceneDepthPS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/SceneDepth_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !SceneDepthPS || !SceneDepthPS->GetPixelShader())
	{
		UE_LOG("HeightFog용 셰이더 없음!\n");
		return;
	}
	RHIDevice->PrepareShader(FullScreenTriangleVS, SceneDepthPS);

	// 텍스쳐 관련 설정
	ID3D11ShaderResourceView* DepthSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth);
	if (!DepthSRV)
	{
		UE_LOG("Depth SRV is null!\n");
		return;
	}

	ID3D11SamplerState* SamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);
	if (!SamplerState)
	{
		UE_LOG("PointClamp Sampler is null!\n");
		return;
	}

	// Shader Resource 바인딩 (슬롯 확인!)
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &DepthSRV);  // t0
	RHIDevice->GetDeviceContext()->PSSetSamplers(1, 1, &SamplerState);

	// 상수 버퍼 업데이트
	ECameraProjectionMode ProjectionMode = View->ProjectionMode;
	//RHIDevice->UpdatePostProcessCB(ZNear, ZFar, ProjectionMode == ECameraProjectionMode::Orthographic);
	RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->ZNear, View->ZFar, ProjectionMode == ECameraProjectionMode::Orthographic));

	// Draw
	RHIDevice->DrawFullScreenQuad();

	// 모든 작업이 성공적으로 끝났으므로 Commit 호출
	// 이제 소멸자는 버퍼 스왑을 되돌리지 않고, SRV 해제 작업만 수행함
	SwapGuard.Commit();
}

void FSceneRenderer::RenderTileCullingDebug()
{
	// SF_TileCullingDebug가 비활성화되어 있으면 아무것도 하지 않음
	if (!World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_TileCullingDebug))
	{
		return;
	}

	// Swap 가드 객체 생성: 스왑을 수행하고, 소멸 시 SRV를 자동 해제하도록 설정
	// t0 (SceneColorSource), t2 (TileLightIndices) 사용
	FSwapGuard SwapGuard(RHIDevice, 0, 1);

	// 렌더 타겟 설정 (Depth 없이 SceneColor에 블렌딩)
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	// Depth State: Depth Test/Write 모두 OFF
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	// 셰이더 설정
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* TileDebugPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/TileDebugVisualization_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !TileDebugPS || !TileDebugPS->GetPixelShader())
	{
		UE_LOG("TileDebugVisualization 셰이더 없음!\n");
		return;
	}
	RHIDevice->PrepareShader(FullScreenTriangleVS, TileDebugPS);

	// 텍스처 관련 설정
	ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	ID3D11SamplerState* SamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	if (!SceneSRV || !SamplerState)
	{
		UE_LOG("TileDebugVisualization: Scene SRV or Sampler is null!\n");
		return;
	}

	// t0: 원본 씬 텍스처
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &SceneSRV);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &SamplerState);

	// t2: 타일 라이트 인덱스 버퍼 (이미 PerformTileLightCulling에서 바인딩됨)
	// 별도 바인딩 불필요, 유지됨

	// b11: 타일 컬링 상수 버퍼 (이미 PerformTileLightCulling에서 설정됨)
	// 별도 업데이트 불필요, 유지됨

	// 전체 화면 쿼드 그리기
	RHIDevice->DrawFullScreenQuad();

	// 모든 작업이 성공적으로 끝났으므로 Commit 호출
	SwapGuard.Commit();
}

// 빌보드, 에디터 화살표 그리기 (상호 작용, 피킹 O)
void FSceneRenderer::RenderEditorPrimitivesPass()
{
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithId);
	for (UPrimitiveComponent* GizmoComp : Proxies.EditorPrimitives)
	{
		GizmoComp->CollectMeshBatches(MeshBatchElements, View);
	}
	DrawMeshBatches(MeshBatchElements, true);
}

// 경계, 외곽선 등 표시 (상호 작용, 피킹 X)
void FSceneRenderer::RenderDebugPass()
{
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTarget);

	// 그리드 라인 수집
	for (ULineComponent* LineComponent : Proxies.EditorLines)
	{
		if (GWorld->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
		{
			LineComponent->CollectLineBatches(OwnerRenderer);
		}
	}

	// 선택된 액터의 디버그 볼륨 렌더링
	for (AActor* SelectedActor : World->GetSelectionManager()->GetSelectedActors())
	{
		for (USceneComponent* Component : SelectedActor->GetSceneComponents())
		{
			// 모든 컴포넌트에서 RenderDebugVolume 호출
			// 각 컴포넌트는 필요한 경우 override하여 디버그 시각화 제공
			Component->RenderDebugVolume(OwnerRenderer);
		}
	}

	// Debug draw (BVH, Octree 등)
	if (World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_BVHDebug) && World->GetPartitionManager())
	{
		if (FBVHierarchy* BVH = World->GetPartitionManager()->GetBVH())
		{
			BVH->DebugDraw(OwnerRenderer); // DebugDraw가 LineBatcher를 직접 받도록 수정 필요
		}
	}

	// 수집된 라인을 출력하고 정리
	OwnerRenderer->EndLineBatch(FMatrix::Identity());
}

void FSceneRenderer::RenderOverayEditorPrimitivesPass()
{
	// 후처리된 최종 이미지 위에 원본 씬의 뎁스 버퍼를 사용하여 3D 오버레이를 렌더링합니다.
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithId);

	// 뎁스 버퍼를 Clear하고 LessEqual로 그리기 때문에 오버레이로 표시되는데
	// 오버레이 끼리는 깊이 테스트가 가능함
	RHIDevice->ClearDepthBuffer(1.0f, 0);

	for (UPrimitiveComponent* GizmoComp : Proxies.OverlayPrimitives)
	{
		GizmoComp->CollectMeshBatches(MeshBatchElements, View);
	}

	// 수집된 배치를 그립니다.
	DrawMeshBatches(MeshBatchElements, true);
}

// 수집한 Batch 그리기
void FSceneRenderer::DrawMeshBatches(TArray<FMeshBatchElement>& InMeshBatches, bool bClearListAfterDraw)
{
	if (InMeshBatches.IsEmpty()) return;

	// RHI 상태 초기 설정 (Opaque Pass 기본값)
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::LessEqual); // 깊이 쓰기 ON

	// PS 리소스 초기화
	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRVs);
	ID3D11SamplerState* nullSamplers[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, nullSamplers);
	FPixelConstBufferType DefaultPixelConst{};
	RHIDevice->SetAndUpdateConstantBuffer(DefaultPixelConst);

	// 현재 GPU 상태 캐싱용 변수 (UStaticMesh* 대신 실제 GPU 리소스로 변경)
	ID3D11VertexShader* CurrentVertexShader = nullptr;
	ID3D11PixelShader* CurrentPixelShader = nullptr;
	UMaterialInterface* CurrentMaterial = nullptr;
	ID3D11ShaderResourceView* CurrentInstanceSRV = nullptr; // [추가] Instance SRV 캐시
	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11Buffer* CurrentIndexBuffer = nullptr;
	UINT CurrentVertexStride = 0;
	D3D11_PRIMITIVE_TOPOLOGY CurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

	// 기본 샘플러 미리 가져오기 (루프 내 반복 호출 방지)
	ID3D11SamplerState* DefaultSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::Default);
	// Shadow PCF용 샘플러 추가
	ID3D11SamplerState* ShadowSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::Shadow);
	ID3D11SamplerState* VSMSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::VSM);

	// 정렬된 리스트 순회
	for (const FMeshBatchElement& Batch : InMeshBatches)
	{
		// --- 필수 요소 유효성 검사 ---
		if (!Batch.VertexShader || !Batch.PixelShader || !Batch.VertexBuffer || !Batch.IndexBuffer || Batch.VertexStride == 0)
		{
			// 셰이더나 버퍼, 스트라이드 정보가 없으면 그릴 수 없음
			//UE_LOG("[%s] 머티리얼에 셰이더가 컴파일에 실패했거나 없습니다!", Batch.Material->GetFilePath().c_str());	// NOTE: 로그가 매 프레임 떠서 셰이더 컴파일 에러 로그를 볼 수 없어서 주석 처리
			continue;
		}

		// 1. 셰이더 상태 변경
		if (Batch.VertexShader != CurrentVertexShader || Batch.PixelShader != CurrentPixelShader)
		{
			RHIDevice->GetDeviceContext()->IASetInputLayout(Batch.InputLayout);
			RHIDevice->GetDeviceContext()->VSSetShader(Batch.VertexShader, nullptr, 0);

			RHIDevice->GetDeviceContext()->PSSetShader(Batch.PixelShader, nullptr, 0);

			CurrentVertexShader = Batch.VertexShader;
			CurrentPixelShader = Batch.PixelShader;
		}

		// --- 2. 픽셀 상태 (텍스처, 샘플러, 재질CBuffer) 변경 (캐싱됨) ---
		//
		// 'Material' 또는 'Instance SRV' 둘 중 하나라도 바뀌면
		// 모든 픽셀 리소스를 다시 바인딩해야 합니다.
		if (Batch.Material != CurrentMaterial || Batch.InstanceShaderResourceView != CurrentInstanceSRV)
		{
			ID3D11ShaderResourceView* DiffuseTextureSRV = nullptr; // t0
			ID3D11ShaderResourceView* NormalTextureSRV = nullptr;  // t1
			FPixelConstBufferType PixelConst{};

			if (Batch.Material)
			{
				PixelConst.Material = Batch.Material->GetMaterialInfo();
				PixelConst.bHasMaterial = true;
			}
			else
			{
				FMaterialInfo DefaultMaterialInfo;
				PixelConst.Material = DefaultMaterialInfo;
				PixelConst.bHasMaterial = false;
				PixelConst.bHasDiffuseTexture = false;
				PixelConst.bHasNormalTexture = false;
			}

			// 1순위: 인스턴스 텍스처 (빌보드)
			if (Batch.InstanceShaderResourceView)
			{
				DiffuseTextureSRV = Batch.InstanceShaderResourceView;
				PixelConst.bHasDiffuseTexture = true;
				PixelConst.bHasNormalTexture = false;
			}
			// 2순위: 머티리얼 텍스처 (스태틱 메시)
			else if (Batch.Material)
			{
				const FMaterialInfo& MaterialInfo = Batch.Material->GetMaterialInfo();
				if (!MaterialInfo.DiffuseTextureFileName.empty())
				{
					if (UTexture* TextureData = Batch.Material->GetTexture(EMaterialTextureSlot::Diffuse))
					{
						DiffuseTextureSRV = TextureData->GetShaderResourceView();
						PixelConst.bHasDiffuseTexture = (DiffuseTextureSRV != nullptr);
					}
				}
				if (!MaterialInfo.NormalTextureFileName.empty())
				{
					if (UTexture* TextureData = Batch.Material->GetTexture(EMaterialTextureSlot::Normal))
					{
						NormalTextureSRV = TextureData->GetShaderResourceView();
						PixelConst.bHasNormalTexture = (NormalTextureSRV != nullptr);
					}
				}
			}			// --- RHI 상태 업데이트 ---
			// 1. 텍스처(SRV) 바인딩
			ID3D11ShaderResourceView* Srvs[2] = { DiffuseTextureSRV, NormalTextureSRV };
			RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);

			// 2. 샘플러 바인딩
			ID3D11SamplerState* Samplers[4] = { DefaultSampler, DefaultSampler, ShadowSampler, VSMSampler };
			RHIDevice->GetDeviceContext()->PSSetSamplers(0, 4, Samplers);			

			// 3. 재질 CBuffer 바인딩
			RHIDevice->SetAndUpdateConstantBuffer(PixelConst);

			// --- 캐시 업데이트 ---
			CurrentMaterial = Batch.Material;
			CurrentInstanceSRV = Batch.InstanceShaderResourceView;
		}

		// 3. IA (Input Assembler) 상태 변경
		if (Batch.VertexBuffer != CurrentVertexBuffer ||
			Batch.IndexBuffer != CurrentIndexBuffer ||
			Batch.VertexStride != CurrentVertexStride ||
			Batch.PrimitiveTopology != CurrentTopology)
		{
			UINT Stride = Batch.VertexStride;
			UINT Offset = 0;

			// Vertex/Index 버퍼 바인딩
			RHIDevice->GetDeviceContext()->IASetVertexBuffers(0, 1, &Batch.VertexBuffer, &Stride, &Offset);
			RHIDevice->GetDeviceContext()->IASetIndexBuffer(Batch.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

			// 토폴로지 설정 (이전 코드의 5번에서 이동하여 최적화)
			RHIDevice->GetDeviceContext()->IASetPrimitiveTopology(Batch.PrimitiveTopology);

			// 현재 IA 상태 캐싱
			CurrentVertexBuffer = Batch.VertexBuffer;
			CurrentIndexBuffer = Batch.IndexBuffer;
			CurrentVertexStride = Batch.VertexStride;
			CurrentTopology = Batch.PrimitiveTopology;
		}

		// 4. 오브젝트별 상수 버퍼 설정 (매번 변경)
		RHIDevice->SetAndUpdateConstantBuffer(ModelBufferType(Batch.WorldMatrix, Batch.WorldMatrix.InverseAffine().Transpose()));
		RHIDevice->SetAndUpdateConstantBuffer(ColorBufferType(Batch.InstanceColor, Batch.ObjectID));

		// 5. 드로우 콜 실행
		RHIDevice->GetDeviceContext()->DrawIndexed(Batch.IndexCount, Batch.StartIndex, Batch.BaseVertexIndex);
	}

	// 루프 종료 후 리스트 비우기 (옵션)
	if (bClearListAfterDraw)
	{
		InMeshBatches.Empty();
	}
}

void FSceneRenderer::ApplyScreenEffectsPass()
{
	if (!World->GetRenderSettings().IsShowFlagEnabled(EEngineShowFlags::SF_FXAA))
	{
		return;
	}

	// Swap 가드 객체 생성: 스왑을 수행하고, 소멸 시 0번 슬롯부터 1개의 SRV를 자동 해제하도록 설정
	FSwapGuard SwapGuard(RHIDevice, 0, 1);

	// 렌더 타겟 설정
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	// 텍스쳐 관련 설정
	ID3D11ShaderResourceView* SourceSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	ID3D11SamplerState* SamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	if (!SourceSRV || !SamplerState)
	{
		UE_LOG("PointClamp Sampler is null!\n");
		return;
	}

	// Shader Resource 바인딩 (슬롯 확인!)
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &SourceSRV);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &SamplerState);

	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* CopyTexturePS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/FXAA_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !CopyTexturePS || !CopyTexturePS->GetPixelShader())
	{
		UE_LOG("FXAA 셰이더 없음!\n");
		return;
	}

	//RHIDevice->UpdateFXAACB(
	//FVector2D(static_cast<float>(RHIDevice->GetViewportWidth()), static_cast<float>(RHIDevice->GetViewportHeight())),
	//	FVector2D(1.0f / static_cast<float>(RHIDevice->GetViewportWidth()), 1.0f / static_cast<float>(RHIDevice->GetViewportHeight())),
	//	0.0833f,
	//	0.166f,
	//	1.0f,	// 0.75 가 기본값이지만 효과 강조를 위해 1로 설정
	//	12
		//);
	// FXAA 파라미터를 RenderSettings에서 가져옴
	URenderSettings& RenderSettings = World->GetRenderSettings();

	RHIDevice->SetAndUpdateConstantBuffer(FXAABufferType(
		FVector2D(static_cast<float>(RHIDevice->GetViewportWidth()), static_cast<float>(RHIDevice->GetViewportHeight())),
		FVector2D(1.0f / static_cast<float>(RHIDevice->GetViewportWidth()), 1.0f / static_cast<float>(RHIDevice->GetViewportHeight())),
		RenderSettings.GetFXAAEdgeThresholdMin(),
		RenderSettings.GetFXAAEdgeThresholdMax(),
		RenderSettings.GetFXAAQualitySubPix(),
		RenderSettings.GetFXAAQualityIterations()));

	RHIDevice->PrepareShader(FullScreenTriangleVS, CopyTexturePS);

	RHIDevice->DrawFullScreenQuad();

	// 모든 작업이 성공적으로 끝났으므로 Commit 호출
	// 이제 소멸자는 버퍼 스왑을 되돌리지 않고, SRV 해제 작업만 수행함
	SwapGuard.Commit();
}

// 최종 결과물의 실제 BackBuffer에 그리는 함수
void FSceneRenderer::CompositeToBackBuffer()
{
	// 1. 최종 결과물을 Source로 만들기 위해 스왑하고, 작업 후 SRV 슬롯 0을 자동 해제하는 가드 생성
	FSwapGuard SwapGuard(RHIDevice, 0, 1);

	// 2. 렌더 타겟을 백버퍼로 설정 (깊이 버퍼 없음)
	RHIDevice->OMSetRenderTargets(ERTVMode::BackBufferWithoutDepth);

	// 3. 텍스처 및 샘플러 설정
	// 이제 RHI_SRV_Index가 아닌, 현재 상태에 맞는 Source SRV를 직접 가져옴
	ID3D11ShaderResourceView* SourceSRV = RHIDevice->GetCurrentSourceSRV();
	ID3D11SamplerState* SamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	if (!SourceSRV || !SamplerState)
	{
		UE_LOG("CompositeToBackBuffer에 필요한 리소스 없음!\n");
		return; // 가드가 자동으로 스왑을 되돌리고 SRV를 해제해줌
	}

	// 4. 셰이더 리소스 바인딩
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &SourceSRV);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &SamplerState);

	// 5. 셰이더 준비
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* BlitPS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/Blit_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !BlitPS || !BlitPS->GetPixelShader())
	{
		UE_LOG("Blit용 셰이더 없음!\n");
		return; // 가드가 자동으로 스왑을 되돌리고 SRV를 해제해줌
	}
	RHIDevice->PrepareShader(FullScreenTriangleVS, BlitPS);

	// 6. 그리기
	RHIDevice->DrawFullScreenQuad();

	// 7. 모든 작업이 성공했으므로 Commit
	SwapGuard.Commit();
}