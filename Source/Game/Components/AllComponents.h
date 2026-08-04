#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Maho
{

// ═══════════════════════════════════════════
// Static mesh component
// ═══════════════════════════════════════════

struct FStaticMeshComponent
{
	std::string MeshPath;
	[[nodiscard]] bool IsValid() const { return !MeshPath.empty(); }
};

// ═══════════════════════════════════════════
// Skeleton component
// ═══════════════════════════════════════════

struct FSkeletonComponent
{
	std::string SkeletonPath;
	[[nodiscard]] bool IsValid() const { return !SkeletonPath.empty(); }
};

// ═══════════════════════════════════════════
// Animation component
// ═══════════════════════════════════════════

struct FAnimationComponent
{
	std::string AnimationClipPath;
	float Time = 0.0f;
	float Speed = 1.0f;
	bool bLoop = true;
	bool bPlaying = true;

	void Play(const std::string& Clip, float InSpeed = 1.0f, bool InLoop = true)
	{
		AnimationClipPath = Clip;
		Time = 0.0f;
		Speed = InSpeed;
		bLoop = InLoop;
		bPlaying = true;
	}

	void Stop() { bPlaying = false; }
	void Pause() { bPlaying = false; }
};

// ═══════════════════════════════════════════
// Camera component
// ═══════════════════════════════════════════

struct FCameraComponent
{
	float FOV = 60.0f;
	float NearPlane = 0.1f;
	float FarPlane = 1000.0f;
	float AspectRatio = 16.0f / 9.0f;
	bool bMainCamera = false;
	bool bOrthographic = false;
	float OrthoSize = 10.0f;

	void SetPerspective(float InFOV, float InNear, float InFar)
	{
		FOV = InFOV;
		NearPlane = InNear;
		FarPlane = InFar;
		bOrthographic = false;
	}

	void SetOrthographic(float InSize, float InNear, float InFar)
	{
		OrthoSize = InSize;
		NearPlane = InNear;
		FarPlane = InFar;
		bOrthographic = true;
	}
};

// ═══════════════════════════════════════════
// Material component
// ═══════════════════════════════════════════

struct FMaterialComponent
{
	std::string ShaderPath;

	std::unordered_map<std::string, std::vector<float>> PropertyOverrides;

	[[nodiscard]] bool IsValid() const { return !ShaderPath.empty(); }

	void SetFloat(const std::string& Name, float V) { PropertyOverrides[Name] = {V}; }
	void SetColor(const std::string& Name, float R, float G, float B, float A) { PropertyOverrides[Name] = {R, G, B, A}; }
	void SetTexture(const std::string& Name, const std::string& TexPath) { PropertyOverrides[Name] = {0.f}; SetOverridePath(Name, TexPath); }
	void SetOverridePath(const std::string& Name, const std::string& Path) { OverrideTexturePaths[Name] = Path; }
	[[nodiscard]] const std::string* GetOverridePath(const std::string& Name) const
	{
		auto It = OverrideTexturePaths.find(Name);
		return (It != OverrideTexturePaths.end()) ? &It->second : nullptr;
	}

private:
	std::unordered_map<std::string, std::string> OverrideTexturePaths;
};

// ═══════════════════════════════════════════
// Script component
// ═══════════════════════════════════════════

struct FScriptComponent
{
	std::string ScriptPath;
	bool bEnabled = true;

	std::unordered_map<std::string, std::string> Params;

	void SetParam(const std::string& Key, const std::string& Value) { Params[Key] = Value; }
	[[nodiscard]] const std::string* GetParam(const std::string& Key) const
	{
		auto It = Params.find(Key);
		return (It != Params.end()) ? &It->second : nullptr;
	}

	[[nodiscard]] bool IsValid() const { return !ScriptPath.empty(); }
};

} // namespace Maho
