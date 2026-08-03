#pragma once

#include <cstring>
#include <string>

/**
 * Project gameplay actor base (not UObject).
 * Owned by FWorld on the game thread.
 */
class FActor
{
public:
	explicit FActor(std::string InName = "Actor")
		: Name(std::move(InName))
	{
		SetIdentityTransform();
	}

	virtual ~FActor() = default;

	FActor(const FActor&) = delete;
	FActor& operator=(const FActor&) = delete;

	virtual void Tick(float /*DeltaSeconds*/) {}

	[[nodiscard]] const std::string& GetName() const { return Name; }
	[[nodiscard]] const float* GetLocalToWorld() const { return LocalToWorld; }

	void SetIdentityTransform()
	{
		std::memset(LocalToWorld, 0, sizeof(LocalToWorld));
		LocalToWorld[0] = LocalToWorld[5] = LocalToWorld[10] = LocalToWorld[15] = 1.0f;
	}

	void SetLocalToWorld(const float InMatrix[16])
	{
		std::memcpy(LocalToWorld, InMatrix, sizeof(LocalToWorld));
	}

protected:
	std::string Name;
	float LocalToWorld[16] = {};
};
