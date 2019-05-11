#include "GameApp.h"

#include <Graphics/Inc/Gui.h>

using namespace WOWGE;

namespace
{
	float speed = 1000.0f;
	float rotationSpeed = 10.0f;
}

GameApp::GameApp()
{
	
}

GameApp::~GameApp()
{

}

void GameApp::OnInitialize()
{
	mWindow.Initialize(GetInstance(), GetAppName(), 720, 720);

	Input::InputSystem::StaticInitialize(mWindow.GetWindowHandle());

	Graphics::GraphicsSystem::StaticInitialize(mWindow.GetWindowHandle(), false);

	Gui::Initialize(mWindow.GetWindowHandle());

	auto graphicsSystem = Graphics::GraphicsSystem::Get();

	//Creating a cube of unit size

	Graphics::MeshBuilder::CreateSphere(mSphereMesh, 200, 200);
	mSphereMeshBuffer.Initialize(mSphereMesh.GetVertices(), sizeof(Graphics::Vertex), mSphereMesh.GetVertexCount(), mSphereMesh.GetIndices(), mSphereMesh.GetIndexCount());

	Graphics::MeshBuilder::CreateSkySphere(mSkySphere, 1000, 1000);
	mSkySphereMeshBuffer.Initialize(mSkySphere.GetVertices(), sizeof(Graphics::Vertex), mSkySphere.GetVertexCount(), mSkySphere.GetIndices(), mSkySphere.GetIndexCount());

	////////////////////////////////
	// Create and fill constant buffer
	mConstantBuffer.Initialize();

	mLight.ambient = Maths::Vector4::White();
	mLight.diffuse = Maths::Vector4::White();
	mLight.direction = { 0.0f, 0.0f, 1.0f, 0.0f };
	mLight.specular = Maths::Vector4::Black();

	mMaterial.ambient = Maths::Vector4::White();
	mMaterial.diffuse = Maths::Vector4::White();
	mMaterial.emissive = Maths::Vector4::White();
	mMaterial.specular = Maths::Vector4::White();
	mMaterial.power = 1.0f;

	mSunMaterial.ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
	mSunMaterial.diffuse = { 0.25f, 0.25f, 0.25f, 1.0f };
	mSunMaterial.emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
	mSunMaterial.specular = { 0.125f, 0.125f, 0.125f, 1.0f };
	mSunMaterial.power = 1.0f;

	mTexture[0].Initialize("../Assets/Images/Stars.jpg");
	mTexture[1].Initialize("../Assets/Images/Planets/Mercury.jpg");
	mTexture[2].Initialize("../Assets/Images/Planets/Venus.jpg");
	mTexture[3].Initialize("../Assets/Images/Planets/Earth.jpg");
	mTexture[4].Initialize("../Assets/Images/Planets/Mars.jpg");
	mTexture[5].Initialize("../Assets/Images/Planets/Jupiter.jpg");
	mTexture[6].Initialize("../Assets/Images/Planets/Saturn.jpg");
	mTexture[7].Initialize("../Assets/Images/Planets/Uranus.jpg");
	mTexture[8].Initialize("../Assets/Images/Planets/Neptune.jpg");
	mTexture[9].Initialize("../Assets/Images/Sun.jpg");

	mAnisotropicSamplerUsingWrap.Initialize(Graphics::Sampler::Filter::Anisotropic, Graphics::Sampler::AddressMode::Wrap);

	///////////////////////////////////////
	// Compile and create the vertex shader
	// D3DCOMPILE_DEBUG flag improves the shader debugging experience

	mVertexShader.Initialize(L"../Assets/Shaders/Lighting.fx", "VS", "vs_5_0", Graphics::Vertex::Format);

	///////////////////////////////////////
	// Compile and create the pixel shader

	mPixelShader.Initialize(L"../Assets/Shaders/Lighting.fx");

	mCamera.Initialize(0.785f, 720.0f, 720.0f, 0.5f, 100000.0f);
	mCamera.InitializePerspectiveProjectionMatrix();
	mCamera.SetPosition({0.0f, 0.0f, -3000.0f});
	mTimer.Initialize();
}

void GameApp::OnTerminate()
{
	mPixelShader.Terminate();
	mVertexShader.Terminate();
	mAnisotropicSamplerUsingWrap.Terminate();

	for (uint32_t i = 0; i < 10; ++i)
	{
		mTexture[i].Terminate();
	}
	mSphereMesh.Destroy();
	mSkySphere.Destroy();

	mConstantBuffer.Terminate();
	mSphereMeshBuffer.Terminate();
	mSkySphereMeshBuffer.Terminate();
	Graphics::GraphicsSystem::StaticTerminate();
	Input::InputSystem::StaticTerminate();
	Gui::Terminate();
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

	Input::InputSystem::Get()->Update();


	auto graphicsSystem = Graphics::GraphicsSystem::Get();
	auto inputSystem = Input::InputSystem::Get();
	graphicsSystem->BeginRender();

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

	mCamera.UpdateViewMatrix();
	
	// Draw mesh

	// Bind the input layout, vertex shader, and pixel shader
	mVertexShader.Bind();
	mPixelShader.Bind();

	//THIS IS SKYSPHERE
	
	Maths::Matrix44 skySphereWorldMatrix = Maths::Matrix44::Scaling(50000.0f) * Maths::Matrix44::Translation(mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z);
	Maths::Matrix44 wvpSkySphere = skySphereWorldMatrix * mCamera.GetViewMatrix() * mCamera.GetProjectionMatrix();

	mTexture[0].BindPS(0);
	mTexture[0].BindVS(0);

	mAnisotropicSamplerUsingWrap.BindVS(0);
	mAnisotropicSamplerUsingWrap.BindPS(0);
	
	// Update matrices
	TransformData data;
	data.world = Maths::Transpose(skySphereWorldMatrix);
	data.wvp = Maths::Transpose(wvpSkySphere);
	data.cameraPosition = { mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z, 0.0f };
	data.lightDirection = mLight.direction;
	data.lightAmbient = mLight.ambient;
	data.lightDiffuse = mLight.diffuse;
	data.lightSpecular = mLight.specular;
	data.materialAmbient = mMaterial.ambient;
	data.materialDiffuse = mMaterial.diffuse;
	data.materialSpecular = mMaterial.specular;
	data.materialEmissive = mMaterial.emissive;
	data.materialPower = mMaterial.power;

	uint32_t sizeOfData = sizeof(data);

	mConstantBuffer.Set(data);
	mConstantBuffer.BindVS();
	mConstantBuffer.BindPS();

	mSkySphereMeshBuffer.Render();

	//THIS IS SUN

	Maths::Matrix44 sunWorldMatrix =
	{
		1090.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1090.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1090.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Maths::Matrix44 wvpSun = sunWorldMatrix * mCamera.GetViewMatrix() * mCamera.GetProjectionMatrix();

	mTexture[9].BindPS(0);
	mTexture[9].BindVS(0);

	// Update matrices
	TransformData sunData;
	sunData.world = Maths::Transpose(sunWorldMatrix);
	sunData.wvp = Maths::Transpose(wvpSun);
	sunData.cameraPosition = { mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z, 0.0f };
	sunData.lightDirection = mLight.direction;
	sunData.lightAmbient = mLight.ambient;
	sunData.lightDiffuse = mLight.diffuse;
	sunData.lightSpecular = mLight.specular;
	sunData.materialAmbient = mSunMaterial.ambient;
	sunData.materialDiffuse = mSunMaterial.diffuse;
	sunData.materialSpecular = mSunMaterial.specular;
	sunData.materialEmissive = mSunMaterial.emissive;
	sunData.materialPower = mSunMaterial.power;

	uint32_t sizeOfSunData = sizeof(sunData);

	mConstantBuffer.Set(sunData);
	mConstantBuffer.BindVS();
	mConstantBuffer.BindPS();

	mSphereMeshBuffer.Render();

	for (uint32_t i = 1; i < 9; ++i)
	{
		if (i == 1)
		{
			planetTranslation = {1124.1f, 0.0f, 0.0f};
			planetScaling = { 3.8f, 3.8f, 3.8f };

		}
		else if (i == 2)
		{
			planetTranslation = { 1627.0f, 0.0f, 0.0f};
			planetScaling = {9.5f, 9.5f, 9.5f};
		}
		else if (i == 3)
		{
			planetTranslation = {2041.0f, 0.0f, 0.0f};
			planetScaling = {10.0f, 10.0f, 10.0f};
		}
		else if (i == 4)
		{
			planetTranslation = {2824.0f, 0.0f, 0.0f};
			planetScaling = {5.3f, 5.3f, 5.3f};
		}
		else if (i == 5)
		{
			planetTranslation = {8330.0f, 0.0f, 0.0f};
			planetScaling = {112.0f, 112.0f, 112.0f};
		}
		else if (i == 6)
		{
			planetTranslation = {14835.0f, 0.0f, 0.0f};
			planetScaling = {94.5f, 94.5f, 94.5f};
		}
		else if (i == 7)
		{
			planetTranslation = {29255.0f, 0.0f, 0.0f};
			planetScaling = {40.0f, 40.0f, 40.0f,};
		}
		else if (i == 8)
		{
			planetTranslation = {45525.0f, 0.0f, 0.0f};
			planetScaling = {38.8f, 38.8f, 38.8f};
		}

		float hack = 5.0f;
		float speed = 10.0f;
		WOWGE::Maths::Matrix44 planetWorldMatrix =
			Maths::Matrix44::Scaling(planetScaling.x * hack, planetScaling.y * hack, planetScaling.z * hack) *
			Maths::Matrix44::Translation(planetTranslation.x, planetTranslation.y, planetTranslation.z);

		mTexture[i].BindPS(0);
		mTexture[i].BindVS(0);

		TransformData planetsData;
		planetsData.world = Maths::Transpose(planetWorldMatrix);
		planetsData.wvp = Maths::Transpose(planetWorldMatrix * mCamera.GetViewMatrix() * mCamera.GetProjectionMatrix());
		planetsData.cameraPosition = { mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z, 0.0f };
		planetsData.lightDirection = mLight.direction;
		planetsData.lightAmbient = mLight.ambient;
		planetsData.lightDiffuse = mLight.diffuse;
		planetsData.lightSpecular = mLight.specular;
		planetsData.materialAmbient = mMaterial.ambient;
		planetsData.materialDiffuse = mMaterial.diffuse;
		planetsData.materialSpecular = mMaterial.specular;
		planetsData.materialEmissive = mMaterial.emissive;
		planetsData.materialPower = mMaterial.power;

		mConstantBuffer.Set(planetsData);
		mConstantBuffer.BindVS();
		mConstantBuffer.BindPS();

		mSphereMeshBuffer.Render();
	}



	graphicsSystem->EndRender();
}