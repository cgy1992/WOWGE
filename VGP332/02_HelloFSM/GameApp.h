#ifndef INCLUDED_GAMEAPP_H
#define INCLUDED_GAMEAPP_H

#include <Core/Inc/Core.h>
#include <Graphics/Inc/Graphics.h>
#include <Input/Inc/Input.h>

#include <External\ImGui\Inc\imgui.h>

#include "IronManAgent.h"

class GameApp: public WOWGE::Core::Application
{
public:
	GameApp();
	~GameApp() override;

private:
	void OnInitialize() override;
	void OnTerminate() override;
	void OnUpdate() override;

	void UpdateIMGUI(float deltaTime);

	WOWGE::Core::Window mWindow;
	WOWGE::Core::Timer mTimer;

	WOWGE::Graphics::Mesh mSkySphere;
	WOWGE::Graphics::Texture mSkySphereTexture;
	WOWGE::Graphics::MeshBuffer mSkySphereMeshBuffer;

	WOWGE::Graphics::Mesh mPlane;
	WOWGE::Graphics::Texture mPlaneTexture;
	WOWGE::Graphics::MeshBuffer mPlaneMeshBuffer;

	WOWGE::Graphics::VertexShader mDepthVertexShader;
	WOWGE::Graphics::PixelShader mDepthPixelShader;

	WOWGE::Graphics::VertexShader mSkySphereVertexShader;
	WOWGE::Graphics::PixelShader mSkySpherePixelShader;

	WOWGE::Graphics::VertexShader mShadowMappingVertexShader;
	WOWGE::Graphics::PixelShader mShadowMappingPixelShader;

	WOWGE::Graphics::Camera mCamera;
	WOWGE::Graphics::Camera mLightCamera;

	WOWGE::Graphics::DepthMap mDepthMap;

	WOWGE::Graphics::RasterizerState mRasterizerState;

	WOWGE::Graphics::RenderTarget* mRenderTarget;

	struct HasTextureBuffer
	{
		bool hasDiffuseTextures{ false };
		bool hasNormalTextures{ false };
		bool hasSpecularTextures{ false };
		bool hasDisplacementTextures{ false };
	};

	std::vector<WOWGE::Graphics::ConstantBufferNode<HasTextureBuffer>*> mHasTexturesNode;

	//SceneStuff
	WOWGE::Graphics::CameraNode* mCameraNode;
	WOWGE::Graphics::LightNode* mLightNode;
	WOWGE::Graphics::MaterialNode* mPlaneMaterialNode;
	WOWGE::Graphics::MeshNode* mPlaneMeshNode;
	WOWGE::Graphics::MeshNode* mSkySphereMeshNode;
	WOWGE::Graphics::ModelNode* mIronManModelNode;
	WOWGE::Graphics::ModelNode* mRockModelNode;
	WOWGE::Graphics::ModelNode* mTRexModelNode;
	WOWGE::Graphics::ModelNode* mMansionModelNode;
	WOWGE::Graphics::RasterizerStateNode* mRasterizerStateNode;
	WOWGE::Graphics::DepthMapNode* mDepthMapNode;
	WOWGE::Graphics::DepthMapNode* mDepthMapNode2ndPass;
	WOWGE::Graphics::SamplerNode* mSamplerNode;
	WOWGE::Graphics::SamplerNode* mDepthSamplerNode;
	WOWGE::Graphics::SamplerNode* mSkySphereSamplerNode;
	WOWGE::Graphics::SceneManager* mSceneManager;
	WOWGE::Graphics::ShaderNode* mDepthMapShaderNode;
	WOWGE::Graphics::ShaderNode* mShadowMapShaderNode;
	WOWGE::Graphics::ShaderNode* mSkySphereShaderNode;
	WOWGE::Graphics::TextureNode* mPlaneTextureNode;
	WOWGE::Graphics::TextureNode* mSkySphereTextureNode;
	WOWGE::Graphics::TransformNode* mPlaneTransformNode;
	WOWGE::Graphics::TransformNode* mSkySphereTransformNode;

	WOWGE::Graphics::MatrixStack mMatrixStack;

	//For FSM
	IronManAgent ironManAgent;
};

//Homework
//int rows, int columns;
//int startX, int startY, int startZ;
//int endX, int endY, int endZ;

#endif // !INCLUDED_GAMEAPP_H