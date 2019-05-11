//====================================================================================================
// Constant Buffers
//====================================================================================================

#include "ConstantBuffers.fx"

cbuffer TransformBuffer : register(b3)
{
    matrix world;
}

cbuffer HasTextureBuffer : register(b4)
{
    vector hasTextures;
}

cbuffer BoneBuffer : register(b5)
{
    matrix boneTransforms[200];
}

static matrix Identity =
{
    1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);
Texture2D displacementMap : register(t3);

SamplerState textureSampler : register(s0);

//====================================================================================================
// Structs
//====================================================================================================

struct VS_INPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
    int4 blendIndices : BLENDINDICES;
    float4 blendWeight : BLENDWEIGHT;
};

//----------------------------------------------------------------------------------------------------

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float3 dirToLight : TEXCOORD0;
    float3 dirToView : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
};

matrix GetBoneTransform(int4 indices, float4 weights)
{
    if (length(weights) <= 0.0f)
        return Identity;

    matrix transform;
    transform = boneTransforms[indices[0]] * weights[0];
    transform += boneTransforms[indices[1]] * weights[1];
    transform += boneTransforms[indices[2]] * weights[2];
    transform += boneTransforms[indices[3]] * weights[3];
    return transform;
}

//====================================================================================================
// Vertex Shader
//====================================================================================================

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;

    float displacement = hasTextures.w == 1.0f ? displacementMap.SampleLevel(textureSampler, input.texCoord, 0).x : 0.0f;

	float4 position = input.position + (float4(input.normal, 0.0f) * 0.01f * displacement);

    float4x4 wv = mul(world, cameraView[0]);
    float4x4 wvp = mul(wv, cameraProjection[0]);

    matrix boneTransform = GetBoneTransform(input.blendIndices, input.blendWeight);
    float4 posBone = position;
    float4 posLocal = mul(posBone, boneTransform) + (float4(input.normal, 0.0f) * displacement * 0.1f);
    float4 posWorld = mul(posLocal, world);
    float4 posProj = mul(posLocal, wvp);

    float4x4 wvLight = mul(world, cameraView[1]);
    float4x4 wvpLight = mul(wvLight, cameraProjection[1]);
	
    float3 normalBone = input.normal;
    float3 normalLocal = mul(float4(normalBone, 0.0f), boneTransform);
    float3 tangent = mul(float4(input.tangent, 0.0f), world).xyz;
    float3 binormal = cross(normalLocal, tangent);
	
    float3 dirToLight = -normalize(lightDirection.xyz); //For diffuse lighting
    float3 dirToView = normalize(cameraPosition[0].xyz - posWorld.xyz); //For specular lighting

    output.position = posProj;
    output.normal = mul(float4(normalLocal, 0.0f), world).xyz;
    output.tangent = tangent;
    output.binormal = binormal;
    output.dirToLight = dirToLight;
    output.dirToView = dirToView;
    output.texCoord = input.texCoord;
	
    return output;
}

//====================================================================================================
// Pixel Shader
//====================================================================================================

float4 PS(VS_OUTPUT input) : SV_Target
{
	// Sample normal from normal map
    float4 normalColor = hasTextures.y == 1.0f ? normalMap.Sample(textureSampler, input.texCoord) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 sampledNormal = float3((2.0f * normalColor.xy) - 1.0f, normalColor.z);

	// Transform to world space
    float3 t = normalize(input.tangent);
    float3 b = normalize(input.binormal);
    float3 n = normalize(input.normal);
    float3x3 tbn = float3x3(t, b, n);
    float3 normal = mul(sampledNormal, tbn);

	//Re-Normalize normals
    float3 dirToLight = normalize(input.dirToLight);
    float3 dirToView = normalize(input.dirToView);
	
	// Ambient
    float4 ambient = lightAmbient * materialAmbient;
	
	// Diffuse
    float d = hasTextures.y == 1.0f ? saturate(dot(dirToLight, normal)) : saturate(dot(dirToLight, n));
    float4 diffuse = d * lightDiffuse * materialDiffuse;

	// Specular
    float3 h = normalize((dirToLight + dirToView));
    float s = hasTextures.y == 1.0f ? saturate(dot(h, normal)) : saturate(dot(h, n));
    float4 specular = pow(s, materialPower) * lightSpecular * materialSpecular;

    float4 diffuseColor = hasTextures.x == 1.0f ? diffuseMap.Sample(textureSampler, input.texCoord) : float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 specularColor = hasTextures.z == 1.0f ? specularMap.Sample(textureSampler, input.texCoord).rrrr : float4(1.0f, 1.0f, 1.0f, 1.0f);

	//return float4(1.0f, 0.0f, 0.0f, 1.0f);
    return ((ambient + diffuse) * diffuseColor + (specular * specularColor));
}