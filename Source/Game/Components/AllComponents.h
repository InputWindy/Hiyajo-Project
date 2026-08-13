#pragma once

#include <Core/Extension/World/Components/ScriptComponent.h>

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Maho
{

constexpr std::size_t ECSMaxPropertyOverrides = 16;
constexpr std::size_t ECSMaxTextureOverrides = 8;

// ═══════════════════════════════════════════
// Static mesh component
// ═══════════════════════════════════════════

struct FStaticMeshComponent
{
	char MeshPath[ECSComponentAssetPathMax] = {};

	[[nodiscard]] bool IsValid() const { return MeshPath[0] != '\0'; }
};

static_assert(std::is_trivially_copyable_v<FStaticMeshComponent>, "FStaticMeshComponent must be trivially copyable");

// ═══════════════════════════════════════════
// Skeleton component
// ═══════════════════════════════════════════

struct FSkeletonComponent
{
	char SkeletonPath[ECSComponentAssetPathMax] = {};

	[[nodiscard]] bool IsValid() const { return SkeletonPath[0] != '\0'; }
};

static_assert(std::is_trivially_copyable_v<FSkeletonComponent>, "FSkeletonComponent must be trivially copyable");

// ═══════════════════════════════════════════
// Animation component
// ═══════════════════════════════════════════

struct FAnimationComponent
{
	char AnimationClipPath[ECSComponentAssetPathMax] = {};
	float Time = 0.0f;
	float Speed = 1.0f;
	bool bLoop = true;
	bool bPlaying = true;

	void Play(const char* Clip, float InSpeed = 1.0f, bool InLoop = true)
	{
		if (Clip)
		{
			std::strncpy(AnimationClipPath, Clip, ECSComponentAssetPathMax - 1);
			AnimationClipPath[ECSComponentAssetPathMax - 1] = '\0';
		}
		Time = 0.0f;
		Speed = InSpeed;
		bLoop = InLoop;
		bPlaying = true;
	}

	void Stop() { bPlaying = false; }
	void Pause() { bPlaying = false; }
};

static_assert(std::is_trivially_copyable_v<FAnimationComponent>, "FAnimationComponent must be trivially copyable");

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

static_assert(std::is_trivially_copyable_v<FCameraComponent>, "FCameraComponent must be trivially copyable");

// ═══════════════════════════════════════════
// Material component
// ═══════════════════════════════════════════

struct FPropertyOverride
{
	char Name[ECSComponentNameMax] = {};
	float Data[4] = {};
	std::uint32_t DataSize = 0; // 1–4
};

struct FTextureOverride
{
	char Name[ECSComponentNameMax] = {};
	char Path[ECSComponentAssetPathMax] = {};
};

struct FMaterialComponent
{
	char ShaderPath[ECSComponentAssetPathMax] = {};

	FPropertyOverride Overrides[ECSMaxPropertyOverrides] = {};
	std::uint32_t OverrideCount = 0;

	FTextureOverride TextureOverrides[ECSMaxTextureOverrides] = {};
	std::uint32_t TextureOverrideCount = 0;

	[[nodiscard]] bool IsValid() const { return ShaderPath[0] != '\0'; }

	void SetFloat(const char* Name, float V)
	{
		std::uint32_t Idx = FindOrAddOverride(Name);
		if (Idx < ECSMaxPropertyOverrides)
		{
			Overrides[Idx].Data[0] = V;
			Overrides[Idx].DataSize = 1;
		}
	}

	void SetColor(const char* Name, float R, float G, float B, float A)
	{
		std::uint32_t Idx = FindOrAddOverride(Name);
		if (Idx < ECSMaxPropertyOverrides)
		{
			Overrides[Idx].Data[0] = R;
			Overrides[Idx].Data[1] = G;
			Overrides[Idx].Data[2] = B;
			Overrides[Idx].Data[3] = A;
			Overrides[Idx].DataSize = 4;
		}
	}

	void SetTexture(const char* Name, const char* TexPath)
	{
		SetFloat(Name, 0.0f);
		SetOverridePath(Name, TexPath);
	}

	void SetOverridePath(const char* Name, const char* Path)
	{
		if (!Name || !Path) return;
		std::uint32_t Idx = FindOrAddTexOverride(Name);
		if (Idx < ECSMaxTextureOverrides)
		{
			std::strncpy(TextureOverrides[Idx].Path, Path, ECSComponentAssetPathMax - 1);
			TextureOverrides[Idx].Path[ECSComponentAssetPathMax - 1] = '\0';
		}
	}

	[[nodiscard]] const char* GetOverridePath(const char* Name) const
	{
		if (!Name) return nullptr;
		for (std::uint32_t I = 0; I < TextureOverrideCount; ++I)
		{
			if (std::strcmp(TextureOverrides[I].Name, Name) == 0)
				return TextureOverrides[I].Path;
		}
		return nullptr;
	}

	[[nodiscard]] const FPropertyOverride* FindOverride(const char* Name) const
	{
		if (!Name) return nullptr;
		for (std::uint32_t I = 0; I < OverrideCount; ++I)
		{
			if (std::strcmp(Overrides[I].Name, Name) == 0)
				return &Overrides[I];
		}
		return nullptr;
	}

private:
	std::uint32_t FindOrAddOverride(const char* Name)
	{
		for (std::uint32_t I = 0; I < OverrideCount; ++I)
		{
			if (std::strcmp(Overrides[I].Name, Name) == 0)
				return I;
		}
		if (OverrideCount >= ECSMaxPropertyOverrides) return ECSMaxPropertyOverrides;
		std::uint32_t Idx = OverrideCount++;
		std::strncpy(Overrides[Idx].Name, Name, ECSComponentNameMax - 1);
		Overrides[Idx].Name[ECSComponentNameMax - 1] = '\0';
		return Idx;
	}

	std::uint32_t FindOrAddTexOverride(const char* Name)
	{
		for (std::uint32_t I = 0; I < TextureOverrideCount; ++I)
		{
			if (std::strcmp(TextureOverrides[I].Name, Name) == 0)
				return I;
		}
		if (TextureOverrideCount >= ECSMaxTextureOverrides) return ECSMaxTextureOverrides;
		std::uint32_t Idx = TextureOverrideCount++;
		std::strncpy(TextureOverrides[Idx].Name, Name, ECSComponentNameMax - 1);
		TextureOverrides[Idx].Name[ECSComponentNameMax - 1] = '\0';
		return Idx;
	}
};

static_assert(std::is_trivially_copyable_v<FMaterialComponent>, "FMaterialComponent must be trivially copyable");

// ═══════════════════════════════════════════
// Tag components (zero-size markers)
// ═══════════════════════════════════════════

/** Marks the engine-owned main camera entity (hidden from level outliner). */
struct FMainCameraTag
{
};

} // namespace Maho
