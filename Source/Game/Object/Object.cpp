// Project Object.cpp — implements non-inline FObjectRef, UObject, and FSoftObjectPath methods.

#include "Game/Object/Object.h"
#include "Game/Object/Package.h"
#include "Game/Object/SoftObjectPath.h"
#include "Game/System/GC/GCSystem.h"

namespace Maho
{

// ── FObjectRef ──────────────────────────────────────────────

FObjectRef::FObjectRef(UObject* InObject)
	: Object(InObject)
{
	if (Object)
	{
		Object->AddRef();
	}
}

FObjectRef::FObjectRef(const FObjectRef& Other)
	: Object(Other.Object)
{
	if (Object)
	{
		Object->AddRef();
	}
}

FObjectRef::FObjectRef(FObjectRef&& Other) noexcept
	: Object(Other.Object)
{
	Other.Object = nullptr;
}

FObjectRef::~FObjectRef()
{
	Reset();
}

FObjectRef& FObjectRef::operator=(const FObjectRef& Other)
{
	if (this != &Other)
	{
		Reset();
		Object = Other.Object;
		if (Object)
		{
			Object->AddRef();
		}
	}
	return *this;
}

FObjectRef& FObjectRef::operator=(FObjectRef&& Other) noexcept
{
	if (this != &Other)
	{
		Reset();
		Object = Other.Object;
		Other.Object = nullptr;
	}
	return *this;
}

bool FObjectRef::IsValid() const
{
	return Object && !Object->IsPendingKill();
}

std::uint32_t FObjectRef::GetRefCount() const
{
	return Object ? Object->GetRefCount() : 0;
}

FObjectRef FObjectRef::Wrap(UObject* InObject)
{
	return FObjectRef(InObject);
}

void FObjectRef::Reset()
{
	if (Object)
	{
		Object->ReleaseRef();
		Object = nullptr;
	}
}

UObject* FObjectRef::Get() const
{
	return IsValid() ? Object : nullptr;
}

// ── UObject ─────────────────────────────────────────────────

UObject::UObject(UPackage* InOuter, std::string InObjectName)
	: ObjectName(std::move(InObjectName))
	, Outer(static_cast<UObject*>(InOuter))
{
}

std::string UObject::GetPathName() const
{
	const UObject* Current = this;
	std::string Result;
	while (Current)
	{
		if (!Result.empty())
		{
			Result.insert(0, "/");
		}
		Result.insert(0, Current->ObjectName);
		const UObject* OuterObj = Current->Outer.GetRaw();
		if (!OuterObj || OuterObj == Current)
		{
			break;
		}
		Current = OuterObj;
	}
	return Result;
}

FObjectRef UObject::GetPackage() const
{
	const UObject* Current = this;
	while (Current)
	{
		const UObject* OuterObj = Current->Outer.GetRaw();
		if (!OuterObj || OuterObj == Current)
		{
			break;
		}
		Current = OuterObj;
	}
	return FObjectRef::Wrap(const_cast<UObject*>(Current));
}

bool UObject::CallFunction(
	std::string_view Name,
	const FPropertyValue* Args,
	std::size_t ArgCount,
	FPropertyValue* OutReturn)
{
	const FObjectType* Type = FObjectTypeRegistry::Get().FindType("UObject");
	if (!Type)
	{
		return false;
	}
	const FFunction* Func = Type->FindFunctionInHierarchy(Name);
	if (!Func || !Func->Invoke)
	{
		return false;
	}
	Func->Invoke(this, Args, ArgCount, OutReturn);
	return true;
}

bool UObject::GetPropertyValue(std::string_view Name, FPropertyValue& OutValue) const
{
	const FObjectType* Type = FObjectTypeRegistry::Get().FindType("UObject");
	if (!Type)
	{
		return false;
	}
	const FProperty* Prop = Type->FindPropertyInHierarchy(Name);
	if (!Prop || !Prop->Getter)
	{
		return false;
	}
	return Prop->Getter(const_cast<UObject*>(this), OutValue);
}

bool UObject::SetPropertyValue(std::string_view Name, const FPropertyValue& Value)
{
	const FObjectType* Type = FObjectTypeRegistry::Get().FindType("UObject");
	if (!Type)
	{
		return false;
	}
	const FProperty* Prop = Type->FindPropertyInHierarchy(Name);
	if (!Prop || !Prop->Setter)
	{
		return false;
	}
	return Prop->Setter(this, Value);
}

// ── FSoftObjectPath ─────────────────────────────────────────

FSoftObjectPath FSoftObjectPath::FromObject(const UObject& Object)
{
	return FSoftObjectPath(Object.GetPathName());
}

bool FSoftObjectPath::TrySetPath(const std::string& PathString)
{
	if (PathString.empty())
	{
		return false;
	}

	// Format: [AssetClass']PackageName.AssetName[:SubPath]
	std::string Remaining = PathString;

	// Check for AssetClass prefix (e.g. "UTexture2D'Package.Asset'")
	auto QuotePos = Remaining.find('\'');
	if (QuotePos != std::string::npos)
	{
		AssetClass = Remaining.substr(0, QuotePos);
		Remaining = Remaining.substr(QuotePos + 1);
		// Strip trailing quote after the package.asset part
		auto EndQuote = Remaining.rfind('\'');
		if (EndQuote != std::string::npos)
		{
			Remaining = Remaining.substr(0, EndQuote);
		}
	}

	// Split package and asset on '.'
	auto DotPos = Remaining.find('.');
	if (DotPos == std::string::npos)
	{
		return false;
	}
	PackageName = Remaining.substr(0, DotPos);
	Remaining = Remaining.substr(DotPos + 1);

	// Check for subpath
	auto ColonPos = Remaining.find(':');
	if (ColonPos != std::string::npos)
	{
		AssetName = Remaining.substr(0, ColonPos);
		SubPath = Remaining.substr(ColonPos + 1);
	}
	else
	{
		AssetName = Remaining;
	}

	return true;
}

std::string FSoftObjectPath::GetAssetPathString() const
{
	return PackageName + "." + AssetName;
}

std::string FSoftObjectPath::ToString() const
{
	std::string Result;
	if (!AssetClass.empty())
	{
		Result += AssetClass + "'";
	}
	Result += PackageName + "." + AssetName;
	if (!SubPath.empty())
	{
		Result += ":" + SubPath;
	}
	if (!AssetClass.empty())
	{
		Result += "'";
	}
	return Result;
}

std::string FSoftObjectPath::ToStringWithoutClass() const
{
	std::string Result = PackageName + "." + AssetName;
	if (!SubPath.empty())
	{
		Result += ":" + SubPath;
	}
	return Result;
}

FObjectRef FSoftObjectPath::Resolve() const
{
	if (!IsValid())
	{
		return FObjectRef();
	}
	// Defer to FResourceSystem for resolution
	return TryLoad();
}

FObjectRef FSoftObjectPath::TryLoad() const
{
	if (!IsValid())
	{
		return FObjectRef();
	}
	// Stub: actual loading is done by FResourceSystem
	return FObjectRef();
}

} // namespace Maho
