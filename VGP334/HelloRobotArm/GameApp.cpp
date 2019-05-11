#include "GameApp.h"

#include <Graphics\Inc\Gui.h>

using namespace WOWGE;

namespace
{
	float speed{ 50.0f };
	float rotationSpeed{ 10.0f };

	Maths::Vector4 lightDirection{ 1.0f, -1.0f, 1.0f, 1.0f };

	uint32_t controlArmCounter{ 0 };
	float modelRotationSpeed{ 50.0f };

	Maths::Matrix44 initialTransform;
	Maths::Vector3 angles[3];
}

GameApp::GameApp() :
	mCameraNode(nullptr),
	mLightNode(nullptr),
	mPlaneMaterialNode(nullptr),
	mPlaneMeshNode(nullptr),
	mSkySphereMeshNode(nullptr),
	mModelNode(nullptr),
	mRasterizerStateNode(nullptr),
	mDepthMapNode(nullptr),
	mDepthMapNode2ndPass(nullptr),
	mSamplerNode(nullptr),
	mDepthSamplerNode(nullptr),
	mSkySphereSamplerNode(nullptr),
	mSceneManager(nullptr),
	mDepthMapShaderNode(nullptr),
	mShadowMapShaderNode(nullptr),
	mSkySphereShaderNode(nullptr),
	mPlaneTextureNode(nullptr),
	mSkySphereTextureNode(nullptr),
	mPlaneTransformNode(nullptr),
	mSkySphereTransformNode(nullptr)
{
	mSceneManager = new WOWGE::Graphics::SceneManager(mMatrixStack);
}

GameApp::~GameApp()
{

}

void GameApp::OnInitialize()
{
	const uint32_t w = 1920;
	const uint32_t h = 1080;

	const float widthLightCamera = w*0.2f;
	const float heightLightCamera = h*0.2f;

	mWindow.Initialize(GetInstance(), GetAppName(), w, h);

	Input::InputSystem::StaticInitialize(mWindow.GetWindowHandle());

	Graphics::GraphicsSystem::StaticInitialize(mWindow.GetWindowHandle(), false);

	Gui::Initialize(mWindow.GetWindowHandle());

	auto graphicsSystem = Graphics::GraphicsSystem::Get();

	Graphics::MeshBuilder::CreateSkySphere(mSkySphere, 200, 200);
	mSkySphereMeshBuffer.Initialize(mSkySphere);
	mSkySphereTexture.Initialize("../../Assets/Images/WoodsCompressed.jpg");

	Graphics::MeshBuilder::CreatePlane(mPlane);
	mPlaneMeshBuffer.Initialize(mPlane);
	mPlaneTexture.Initialize("../../Assets/Images/BrickSmallBrown.jpg");

	mRasterizerState.Initialize(Graphics::CullMode::None, Graphics::FillMode::Wireframe);

	///////////////////////////////////////
	// Compile and create the vertex shader
	// D3DCOMPILE_DEBUG flag improves the shader debugging experience
	mSkySphereVertexShader.Initialize(L"../../Assets/Shaders/SkySphere.fx", "VS", "vs_5_0", Graphics::Vertex::Format);
	mDepthVertexShader.Initialize(L"../../Assets/Shaders/DepthMap.fx", "VS", "vs_5_0", Graphics::Vertex::Format);
	mShadowMappingVertexShader.Initialize(L"../../Assets/Shaders/ShadowMapping.fx", "VS", "vs_5_0", Graphics::Vertex::Format);
	///////////////////////////////////////
	// Compile and create the pixel shader

	mSkySpherePixelShader.Initialize(L"../../Assets/Shaders/SkySphere.fx");
	mShadowMappingPixelShader.Initialize(L"../../Assets/Shaders/ShadowMapping.fx");

	//Robot Arm
	for (uint32_t i = 0; i < 3; ++i)
	{
		Graphics::MeshBuilder::CreateCylinder(mRobotArmMesh[i], 20, 20);
		mRobotArmMeshBuffer[i].Initialize(mRobotArmMesh[i]);
	}

	mRobotArmTexture.Initialize("../../Assets/Images/Metal.jpg");

	mCamera.Initialize(0.785f, (float)w, (float)h, 1.0f, 5000.0f);
	mCamera.InitializePerspectiveProjectionMatrix();
	mCamera.SetPosition({ 0.0f, 20.0f,-100.0f });
	mTimer.Initialize();

	mLightCamera.Initialize(0.785f, (float)w, (float)h, 25.0f, 500.0f);
	mLightCamera.InitializeOrthographicProjectionMatrix(widthLightCamera, heightLightCamera);
	mLightCamera.Pitch(30.0f);
	mLightCamera.SetPosition({ 0.0f, 50.0f,-40.0f });

	mCameraNode = mSceneManager->CreateCameraNode();
	mCameraNode->SetName("Camera");
	mCameraNode->SetCamera(&mCamera, 0);
	mCameraNode->SetCamera(&mLightCamera, 1);

	mLightNode = mSceneManager->CreateLightNode();
	mLightNode->SetName("Light");
	mLightNode->GetLight().ambient = Maths::Vector4::Black();
	mLightNode->GetLight().diffuse = Maths::Vector4::White();
	mLightNode->GetLight().direction = { -1.0f, -1.0f, 1.0f, 0.0f };
	mLightNode->GetLight().specular = Maths::Vector4::White();

	mSkySphereSamplerNode = mSceneManager->CreateSamplerNode(Graphics::Sampler::Filter::Anisotropic, Graphics::Sampler::AddressMode::Wrap);
	mSkySphereSamplerNode->SetName("SkySphereSampler");

	mSamplerNode = mSceneManager->CreateSamplerNode(Graphics::Sampler::Filter::Anisotropic, Graphics::Sampler::AddressMode::Wrap);
	mSamplerNode->SetName("Sampler");

	mDepthSamplerNode = mSceneManager->CreateSamplerNode(Graphics::Sampler::Filter::Point, Graphics::Sampler::AddressMode::Clamp, 1);
	mDepthSamplerNode->SetName("DepthSampler");

	mSkySphereTextureNode = mSceneManager->CreateTextureNode(ShaderStage::PixelShader);
	mSkySphereTextureNode->SetName("SkySphereTexture");
	mSkySphereTextureNode->SetTexture(&mSkySphereTexture);

	mPlaneTextureNode = mSceneManager->CreateTextureNode(ShaderStage::PixelShader);
	mPlaneTextureNode->SetName("PlaneTexture");
	mPlaneTextureNode->SetTexture(&mPlaneTexture);

	mRasterizerStateNode = mSceneManager->CreateRasterizerNode();
	mRasterizerStateNode->SetName("RasterizerState");
	mRasterizerStateNode->SetRasterizerStateNode(&mRasterizerState);
	mRasterizerStateNode->SetEnabled(false);

	const uint32_t depthMapSize = 4096;
	mDepthMap.Initialize(depthMapSize, depthMapSize);

	mRenderTarget = new Graphics::RenderTarget();
	mRenderTarget->Initialize(w, h, Graphics::ColorFormat::RGBA_U32);

	mDepthMapNode = mSceneManager->CreateDepthMapNode(1);
	mDepthMapNode->SetName("DepthMap");
	mDepthMapNode->SetDepthMap(&mDepthMap);

	mDepthMapNode2ndPass = mSceneManager->CreateDepthMapNode(1);
	mDepthMapNode2ndPass->SetName("DepthMap2ndPass");
	mDepthMapNode2ndPass->SetDepthMap(&mDepthMap);
	mDepthMapNode2ndPass->SetHasTexture(true);

	mSkySphereShaderNode = mSceneManager->CreateShaderNode();
	mSkySphereShaderNode->SetName("SkySphereShader");
	mSkySphereShaderNode->SetPixelShader(&mSkySpherePixelShader);
	mSkySphereShaderNode->SetVertexShader(&mSkySphereVertexShader);

	mDepthMapShaderNode = mSceneManager->CreateShaderNode();
	mDepthMapShaderNode->SetName("DepthMapShader");
	mDepthMapShaderNode->SetPixelShader(&mDepthPixelShader);
	mDepthMapShaderNode->SetVertexShader(&mDepthVertexShader);

	mShadowMapShaderNode = mSceneManager->CreateShaderNode();
	mShadowMapShaderNode->SetName("ShadowMapShader");
	mShadowMapShaderNode->SetPixelShader(&mShadowMappingPixelShader);
	mShadowMapShaderNode->SetVertexShader(&mShadowMappingVertexShader);

	mSkySphereTransformNode = mSceneManager->CreateTransformNode();
	mSkySphereTransformNode->SetName("SkySphereTransform");
	mSkySphereTransformNode->GetTransform().SetScale({ 4000.0f, 4000.0f, 4000.0f });

	mPlaneTransformNode = mSceneManager->CreateTransformNode();
	mPlaneTransformNode->SetName("Transform");
	mPlaneTransformNode->GetTransform().SetPosition({ -200.0f, 0.0f, -150.0f });
	mPlaneTransformNode->GetTransform().SetScale({ 500.0f, 500.0f, 1.0f });
	mPlaneTransformNode->GetTransform().SetRotation({ 90.0f, 0.0f, 0.0f });

	mSkySphereMeshNode = mSceneManager->CreateMeshNode();
	mSkySphereMeshNode->SetName("SkySphereMesh");
	mSkySphereMeshNode->SetMesh(&mSkySphereMeshBuffer);

	mPlaneMeshNode = mSceneManager->CreateMeshNode();
	mPlaneMeshNode->SetMesh(&mPlaneMeshBuffer);

	mPlaneMaterialNode = mSceneManager->CreateMaterialNode();
	mPlaneMaterialNode->GetMaterial().ambient = Maths::Vector4::Black();
	mPlaneMaterialNode->GetMaterial().diffuse = Maths::Vector4::White();
	mPlaneMaterialNode->GetMaterial().specular = Maths::Vector4::Gray();
	mPlaneMaterialNode->GetMaterial().emissive = Maths::Vector4::Black();
	mPlaneMaterialNode->GetMaterial().power = 1.0f;

	//Initializing Robot Arm Nodes

	mRobotMaterialNode = mSceneManager->CreateMaterialNode();
	mRobotMaterialNode->GetMaterial().ambient = Maths::Vector4::Black();
	mRobotMaterialNode->GetMaterial().diffuse = Maths::Vector4::White();
	mRobotMaterialNode->GetMaterial().specular = Maths::Vector4::Gray();
	mRobotMaterialNode->GetMaterial().emissive = Maths::Vector4::Black();
	mRobotMaterialNode->GetMaterial().power = 1.0f;


	for (uint32_t i = 0; i < 3; ++i)
	{
		mRobotArmMeshNode[i] = mSceneManager->CreateMeshNode();
		mRobotArmMeshNode[i]->SetName("RobotArmMesh");
		mRobotArmMeshNode[i]->SetMesh(&mRobotArmMeshBuffer[i]);

		mRobotArmTransformNode[i] = mSceneManager->CreateTransformNode();
		mRobotArmTransformNode[i]->SetName("RobotArmTransform");
		
		//mRobotArmTransformNode[i]->GetTransform().SetPivot({ mRobotArmTransformNode[0]->GetTransform().GetScale().x * (-0.5f), 0.0f, 0.0f });
	}

	mRobotArmTransformNode[0]->GetTransform().SetScale({ 10.0f, 20.0f, 10.0f });


	mRobotArmTransformNode[0]->GetTransform().SetPosition({0.0f, 10.0f, 0.0f});
	mRobotArmTransformNode[1]->GetTransform().SetPosition({0.0f, 10.0f, 0.0f});
	mRobotArmTransformNode[2]->GetTransform().SetPosition({0.0f, 10.0f, 0.0f});
	initialTransform = mRobotArmTransformNode[0]->GetTransform().GetTransformMatrix();

	mRobotArmTextureNode = mSceneManager->CreateTextureNode(ShaderStage::PixelShader);

	mSceneManager->GetRoot().AddChild(mCameraNode);

	mCameraNode->AddChild(mDepthMapNode);
	mDepthMapNode->AddChild(mDepthMapShaderNode);
	for (uint32_t i = 0; i < 3; ++i)
	{
		mDepthMapShaderNode->AddChild(mRobotArmTransformNode[i]);
		mRobotArmTransformNode[i]->AddChild(mRobotArmMeshNode[i]);
	}
	mRobotArmTextureNode->SetName("RobotArmTexture");
	mRobotArmTextureNode->SetTexture(&mRobotArmTexture);

	mCameraNode->AddChild(mDepthMapNode2ndPass);
	mDepthMapNode2ndPass->AddChild(mRasterizerStateNode);
	mRasterizerStateNode->AddChild(mShadowMapShaderNode);
	mShadowMapShaderNode->AddChild(mLightNode);
	mShadowMapShaderNode->AddChild(mDepthSamplerNode);
	mShadowMapShaderNode->AddChild(mSamplerNode);
	
	//SkySphere Nodes
	mDepthMapNode2ndPass->AddChild(mSkySphereShaderNode);
	mSkySphereShaderNode->AddChild(mSkySphereSamplerNode);
	mSkySphereSamplerNode->AddChild(mSkySphereTransformNode);
	mSkySphereTransformNode->AddChild(mSkySphereTextureNode);
	mSkySphereTextureNode->AddChild(mSkySphereMeshNode);

	mSamplerNode->AddChild(mPlaneTransformNode);
	mPlaneTransformNode->AddChild(mPlaneTextureNode);
	mPlaneTextureNode->AddChild(mPlaneMaterialNode);
	mPlaneMaterialNode->AddChild(mPlaneMeshNode);

	mSamplerNode->AddChild(mRobotMaterialNode);
	mRobotMaterialNode->AddChild(mRobotArmTextureNode);

	for (uint32_t i = 0; i < 3; ++i)
	{
		mRobotArmTextureNode->AddChild(mRobotArmTransformNode[i]);
	}

	GameApp::InitializeBones();
}

void GameApp::OnTerminate()
{
	mCameraNode->Terminate();
	mLightNode->Terminate();

	mSamplerNode->Terminate();
	mDepthMapNode->Terminate();

	mSceneManager->Purge();

	mSkySphereVertexShader.Terminate();
	mSkySpherePixelShader.Terminate();

	mDepthVertexShader.Terminate();

	mShadowMappingVertexShader.Terminate();
	mShadowMappingPixelShader.Terminate();

	mPlaneTexture.Terminate();
	mRobotArmTexture.Terminate();
	mSkySphereTexture.Terminate();
	mPlane.Destroy();
	mSkySphere.Destroy();
	for (uint32_t i = 0; i < 3; ++i)
	{
		mRobotArmMesh[i].Destroy();
		mRobotArmMeshBuffer[i].Terminate();
	}
	mPlaneMeshBuffer.Terminate();

	mRenderTarget->Terminate();
	mDepthMap.Terminate();
	SafeDelete(mRenderTarget);

	mSkySphereMeshBuffer.Terminate();

	mRasterizerState.Terminate();

	Gui::Terminate();
	Graphics::GraphicsSystem::StaticTerminate();
	Input::InputSystem::StaticTerminate();
	mWindow.Terminate();
}

void GameApp::OnUpdate()
{
	mTimer.Update();
	float deltaTime = mTimer.GetElapsedTime();
	float totalTime = mTimer.GetTotalTime();
	if (mWindow.ProcessMessage())
	{
		Kill();
		return;
	}

	if (GetAsyncKeyState(VK_ESCAPE))
	{
		PostQuitMessage(0);
		return;
	}

	auto inputSystem = Input::InputSystem::Get();

	inputSystem->Update();

	if (inputSystem->IsMouseDown(Input::MouseButton::RBUTTON))
	{
		auto boost = inputSystem->IsKeyDown(Input::KeyCode::LSHIFT) ? 10.0f : 1.0f;
		if (inputSystem->IsKeyDown(Input::KeyCode::W))
		{
			mCamera.Walk(speed * boost * deltaTime);
		}
		if (inputSystem->IsKeyDown(Input::KeyCode::S))
		{
			mCamera.Walk(-speed * boost * deltaTime);
		}
		if (inputSystem->IsKeyDown(Input::KeyCode::A))
		{
			mCamera.Strafe(-speed * boost * deltaTime);
		}
		if (inputSystem->IsKeyDown(Input::KeyCode::D))
		{
			mCamera.Strafe(speed * boost * deltaTime);
		}
		if (inputSystem->IsKeyDown(Input::KeyCode::Q))
		{
			mCamera.Rise(-speed * boost * deltaTime);
		}
		if (inputSystem->IsKeyDown(Input::KeyCode::E))
		{
			mCamera.Rise(speed * boost * deltaTime);
		}

		mCamera.Pitch(rotationSpeed * deltaTime * inputSystem->GetMouseMoveY()); //MOUSE MOVEMENT
		mCamera.Yaw(rotationSpeed * deltaTime * inputSystem->GetMouseMoveX()); //MOUSE MOVEMENT
	}
	if (inputSystem->IsKeyPressed(Input::KeyCode::F1))
	{
		mRasterizerStateNode->SetEnabled(!mRasterizerStateNode->GetEnabled());
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::ONE))
	{
		controlArmCounter = 0;
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::TWO))
	{

		controlArmCounter = 1;
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::THREE))
	{

		controlArmCounter = 2;
	}

	if (inputSystem->IsKeyDown(Input::KeyCode::LEFT))
	{
		if (controlArmCounter == 0)
		{
			angles[0].z += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].z += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].z += modelRotationSpeed * deltaTime;
		}
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::RIGHT))
	{
		if (controlArmCounter == 0)
		{
			angles[0].z -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].z -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].z -= modelRotationSpeed * deltaTime;
		}
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::DOWN))
	{
		if (controlArmCounter == 0)
		{
			angles[0].x -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].x -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].x -= modelRotationSpeed * deltaTime;
		}
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::UP))
	{
		if (controlArmCounter == 0)
		{
			angles[0].x += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].x += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].x += modelRotationSpeed * deltaTime;
		}
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::PGUP))
	{
		if (controlArmCounter == 0)
		{
			angles[0].y -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].y -= modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].y -= modelRotationSpeed * deltaTime;
		}
	}
	if (inputSystem->IsKeyDown(Input::KeyCode::PGDN))
	{
		if (controlArmCounter == 0)
		{
			angles[0].y += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 1)
		{
			angles[1].y += modelRotationSpeed * deltaTime;
		}
		else if (controlArmCounter == 2)
		{
			angles[2].y += modelRotationSpeed * deltaTime;
		}
	}

	mSkySphereTransformNode->GetTransform().SetPosition(mCamera.GetPosition());

	for (uint32_t i = 0; i < 3; ++i)
	{
		mRobotArmTransformNode[i]->GetTransform().SetTransformMatrix(initialTransform * mBoneWorldTransform[i]); //Fixing the bone transform with a correction matrix(initialTransform) to render boxes, as the robot arm
	}

	RotateRobotArm(controlArmCounter);
	UpdateBones();


	mCamera.UpdateViewMatrix();
	mLightCamera.UpdateViewMatrix();

	// Draw mesh

	mSceneManager->Update(deltaTime);

	auto graphicsSystem = Graphics::GraphicsSystem::Get();
	graphicsSystem->BeginRender();
	mSceneManager->Render();

 	graphicsSystem->EndRender();
}

void GameApp::InitializeBones()
{
	//Code for skeleton framework
	mBones.resize(3);
	mBoneWorldTransform.resize(3);
	for (uint32_t i = 1; i < 3; ++i)
	{
		mBones[i].parent = &mBones[i - 1];
	}
}

void GameApp::UpdateBones()
{
	mBoneWorldTransform[0] = (mBones[0].transform);

	mBoneWorldTransform[1] = (mBones[1].transform * mBones[0].transform);

	mBoneWorldTransform[2] = (mBones[2].transform * mBones[1].transform * mBones[0].transform);
}

void GameApp::RotateRobotArm(uint32_t controlArm)
{
	//Multiplication Order
	//Translating to the pivot * Rotating around that pivot * Translating Back * Actual translation of the box relative to the parent
 	mBones[0].transform = Maths::QuaternionToMatrix(Maths::ToQuaternion({ Maths::ConvertDegreeToRadian(angles[0].x), Maths::ConvertDegreeToRadian(angles[0].y), Maths::ConvertDegreeToRadian(angles[0].z) }));

	mBones[1].transform = Maths::QuaternionToMatrix(Maths::ToQuaternion({ Maths::ConvertDegreeToRadian(angles[1].x), Maths::ConvertDegreeToRadian(angles[1].y), Maths::ConvertDegreeToRadian(angles[1].z) })) * 
						  Maths::Matrix44::Translation(0.0f, mRobotArmTransformNode[0]->GetTransform().GetScale().y, 0.0f);

	mBones[2].transform = Maths::QuaternionToMatrix(Maths::ToQuaternion({ Maths::ConvertDegreeToRadian(angles[2].x), Maths::ConvertDegreeToRadian(angles[2].y), Maths::ConvertDegreeToRadian(angles[2].z) })) * 
						  Maths::Matrix44::Translation(0.0f, mRobotArmTransformNode[0]->GetTransform().GetScale().y, 0.0f);
}