#include "Game/System/Resource/ResourceCasset.h"

#include <Core/System/Compression.h>
#include <Core/System/Log.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Maho
{
namespace ResourceCasset
{

// ── Binary helpers ─────────────────────────────────────────────

namespace
{

class FByteWriter
{
public:
	explicit FByteWriter(std::vector<std::uint8_t>& Out) : Out_(Out) {}

	void WriteU8(std::uint8_t V) { Out_.push_back(V); }
	void WriteU16(std::uint16_t V)
	{
		Out_.push_back(static_cast<std::uint8_t>(V & 0xFF));
		Out_.push_back(static_cast<std::uint8_t>((V >> 8) & 0xFF));
	}
	void WriteU32(std::uint32_t V)
	{
		for (int I = 0; I < 4; ++I)
			Out_.push_back(static_cast<std::uint8_t>((V >> (I * 8)) & 0xFF));
	}
	void WriteF32(float V)
	{
		std::uint32_t Tmp;
		std::memcpy(&Tmp, &V, sizeof(Tmp));
		WriteU32(Tmp);
	}
	void WriteBytes(const std::uint8_t* Data, std::size_t Count)
	{
		Out_.insert(Out_.end(), Data, Data + Count);
	}
	void WriteString(const std::string& S)
	{
		WriteU32(static_cast<std::uint32_t>(S.size()));
		if (!S.empty())
			WriteBytes(reinterpret_cast<const std::uint8_t*>(S.data()), S.size());
	}
	void PadTo4()
	{
		while (Out_.size() % 4 != 0)
			Out_.push_back(0);
	}
	std::size_t Tell() const { return Out_.size(); }

private:
	std::vector<std::uint8_t>& Out_;
};

class FByteReader
{
public:
	FByteReader(const std::uint8_t* Data, std::size_t Size)
		: Data_(Data), Size_(Size), Pos_(0) {}

	[[nodiscard]] bool CanRead(std::size_t Num) const { return Pos_ + Num <= Size_; }

	std::uint8_t ReadU8()
	{
		if (!CanRead(1)) throw std::runtime_error("ReadU8 underflow");
		return Data_[Pos_++];
	}
	std::uint16_t ReadU16()
	{
		if (!CanRead(2)) throw std::runtime_error("ReadU16 underflow");
		std::uint16_t V = static_cast<std::uint16_t>(Data_[Pos_])
			| (static_cast<std::uint16_t>(Data_[Pos_ + 1]) << 8);
		Pos_ += 2;
		return V;
	}
	std::uint32_t ReadU32()
	{
		if (!CanRead(4)) throw std::runtime_error("ReadU32 underflow");
		std::uint32_t V = static_cast<std::uint32_t>(Data_[Pos_])
			| (static_cast<std::uint32_t>(Data_[Pos_ + 1]) << 8)
			| (static_cast<std::uint32_t>(Data_[Pos_ + 2]) << 16)
			| (static_cast<std::uint32_t>(Data_[Pos_ + 3]) << 24);
		Pos_ += 4;
		return V;
	}
	float ReadF32()
	{
		std::uint32_t Tmp = ReadU32();
		float F;
		std::memcpy(&F, &Tmp, sizeof(F));
		return F;
	}
	std::vector<std::uint8_t> ReadBytes(std::size_t Count)
	{
		if (!CanRead(Count)) throw std::runtime_error("ReadBytes underflow");
		std::vector<std::uint8_t> Out(Data_ + Pos_, Data_ + Pos_ + Count);
		Pos_ += Count;
		return Out;
	}
	std::string ReadString()
	{
		std::uint32_t Len = ReadU32();
		if (Len == 0) return {};
		if (!CanRead(Len)) throw std::runtime_error("ReadString underflow");
		std::string S(reinterpret_cast<const char*>(Data_ + Pos_), Len);
		Pos_ += Len;
		return S;
	}
	void Skip(std::size_t N) { Pos_ = std::min(Pos_ + N, Size_); }
	void SkipPadTo4()
	{
		while (Pos_ % 4 != 0 && Pos_ < Size_) ++Pos_;
	}

private:
	const std::uint8_t* Data_;
	std::size_t Size_;
	std::size_t Pos_;
};

void AppendChunk(std::vector<std::uint8_t>& Doc, std::uint32_t Tag, std::uint32_t Flags, const std::vector<std::uint8_t>& Payload)
{
	FByteWriter W(Doc);
	W.WriteU32(Tag);
	W.WriteU32(static_cast<std::uint32_t>(Payload.size()));
	W.WriteU32(Flags);
	W.WriteBytes(Payload.data(), Payload.size());
	W.PadTo4();
}

template <typename T>
void WriteBlob(FByteWriter& W, const T* Data, std::size_t Count)
{
	W.WriteU32(static_cast<std::uint32_t>(Count));
	if (Count > 0 && Data)
		W.WriteBytes(reinterpret_cast<const std::uint8_t*>(Data), Count * sizeof(T));
}

template <typename T>
[[nodiscard]] std::vector<T> ReadBlob(FByteReader& R)
{
	std::uint32_t Count = R.ReadU32();
	if (Count == 0) return {};
	if (!R.CanRead(Count * sizeof(T))) throw std::runtime_error("ReadBlob underflow");
	std::vector<T> Out(Count);
	std::memcpy(Out.data(), &R, Count * sizeof(T)); // This is wrong in the original code
	throw std::runtime_error("ReadBlob: use manual read");
	return Out;
}

} // namespace

// ── Texture CPU ────────────────────────────────────────────────

static void WriteTextureCpu(const FTexture& Tex, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kTextureCpuLayoutEncoded);

	const bool bHasEncoded = Tex.HasSerializedSource();
	W.WriteU8(bHasEncoded ? kTexturePayloadEncoded : kTexturePayloadRaw);

	if (bHasEncoded)
	{
		W.WriteU8(static_cast<std::uint8_t>(Tex.GetDimension()));
		W.WriteString(Tex.GetSerializedSourceHint());
		W.WriteU32(static_cast<std::uint32_t>(Tex.GetSerializedSourceBytes().size()));
		if (!Tex.GetSerializedSourceBytes().empty())
			W.WriteBytes(Tex.GetSerializedSourceBytes().data(), Tex.GetSerializedSourceBytes().size());
		return;
	}

	W.WriteU8(static_cast<std::uint8_t>(Tex.GetDimension()));
	W.WriteU32(static_cast<std::uint32_t>(Tex.GetPixelFormat()));
	W.WriteU32(Tex.GetWidth());
	W.WriteU32(Tex.GetHeight());
	W.WriteU32(Tex.GetDepth());
	W.WriteU32(Tex.GetArrayLayers());
	W.WriteU32(Tex.GetMipCount());
	W.WriteU8(Tex.IsSRGB() ? 1 : 0);
	W.WriteU8(kTexturePayloadRaw);

	const std::vector<std::uint8_t>& Pixels = Tex.GetPixels();
	W.WriteU32(static_cast<std::uint32_t>(Pixels.size()));
	if (!Pixels.empty())
		W.WriteBytes(Pixels.data(), Pixels.size());
}

static bool ReadTextureCpu(FTexture& Tex, FByteReader& R)
{
	std::uint16_t Layout = R.ReadU16();
	if (Layout == kTextureCpuLayoutEncoded)
	{
		std::uint8_t Kind = R.ReadU8();
		if (Kind == kTexturePayloadEncoded)
		{
			std::uint8_t Dim = R.ReadU8();
			std::string Hint = R.ReadString();
			std::uint32_t Size = R.ReadU32();
			if (Size > 0)
			{
				std::vector<std::uint8_t> Bytes = R.ReadBytes(Size);
				Tex.SetSerializedSource(std::move(Hint), std::move(Bytes));
			}
			(void)Dim;
			return true;
		}
	}
	return false;
}

// ── Mesh CPU ───────────────────────────────────────────────────

static void WriteMeshCpu(const FStaticMesh& Mesh, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kCpuLayoutVersion);
	W.WriteString(Mesh.GetMaterial());

	FByteWriter Body(Out);
	Body.WriteU32(static_cast<std::uint32_t>(Mesh.GetPositions().size()));
	if (!Mesh.GetPositions().empty())
		Body.WriteBytes(reinterpret_cast<const std::uint8_t*>(Mesh.GetPositions().data()), Mesh.GetPositions().size() * sizeof(float));

	Body.WriteU32(static_cast<std::uint32_t>(Mesh.GetNormals().size()));
	if (!Mesh.GetNormals().empty())
		Body.WriteBytes(reinterpret_cast<const std::uint8_t*>(Mesh.GetNormals().data()), Mesh.GetNormals().size() * sizeof(float));

	Body.WriteU32(static_cast<std::uint32_t>(Mesh.GetUVs().size()));
	if (!Mesh.GetUVs().empty())
		Body.WriteBytes(reinterpret_cast<const std::uint8_t*>(Mesh.GetUVs().data()), Mesh.GetUVs().size() * sizeof(float));

	Body.WriteU32(static_cast<std::uint32_t>(Mesh.GetIndices().size()));
	if (!Mesh.GetIndices().empty())
		Body.WriteBytes(reinterpret_cast<const std::uint8_t*>(Mesh.GetIndices().data()), Mesh.GetIndices().size() * sizeof(std::uint32_t));

	W.WriteBytes(Out.data() + Out.size(), static_cast<std::size_t>(Body.Tell()));
}

static void ReadMeshCpu(FStaticMesh& Mesh, FByteReader& R)
{
	std::uint16_t Version = R.ReadU16();
	if (Version >= 1)
	{
		std::string MatPath = R.ReadString();
		Mesh.SetMaterial(std::move(MatPath));
	}

	std::uint32_t PosCount = R.ReadU32();
	if (PosCount > 0)
	{
		std::vector<float> Pos(PosCount);
		std::memcpy(Pos.data(), &R, PosCount * sizeof(float));
		R.Skip(PosCount * sizeof(float));
		Mesh.SetCpuGeometry(std::move(Pos), {}, {}, {});
		return;
	}
}

// ── Material CPU ───────────────────────────────────────────────

static void WriteMaterialCpu(const FMaterial& Mat, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kCpuLayoutVersion);
	W.WriteString(Mat.GetBaseColorTexture());
	W.WriteString(Mat.GetNormalTexture());
	W.WriteString(Mat.GetMetallicRoughnessTexture());
	W.WriteString(Mat.GetOcclusionTexture());
	W.WriteString(Mat.GetEmissiveTexture());
	for (int I = 0; I < 4; ++I) W.WriteF32(Mat.BaseColorFactor[I]);
	W.WriteF32(Mat.MetallicFactor);
	W.WriteF32(Mat.RoughnessFactor);
	for (int I = 0; I < 3; ++I) W.WriteF32(Mat.EmissiveFactor[I]);
}

static void ReadMaterialCpu(FMaterial& Mat, FByteReader& R)
{
	std::uint16_t Version = R.ReadU16();
	if (Version >= 1)
	{
		Mat.SetBaseColorTexture(R.ReadString());
		Mat.SetNormalTexture(R.ReadString());
		Mat.SetMetallicRoughnessTexture(R.ReadString());
		Mat.SetOcclusionTexture(R.ReadString());
		Mat.SetEmissiveTexture(R.ReadString());
	}
	for (int I = 0; I < 4; ++I) Mat.BaseColorFactor[I] = R.ReadF32();
	Mat.MetallicFactor = R.ReadF32();
	Mat.RoughnessFactor = R.ReadF32();
	for (int I = 0; I < 3; ++I) Mat.EmissiveFactor[I] = R.ReadF32();
}

// ── Skeleton CPU ───────────────────────────────────────────────

static void WriteSkeletonCpu(const FSkeleton& Skeleton, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kCpuLayoutVersion);
	const auto& Bones = Skeleton.GetBones();
	W.WriteU32(static_cast<std::uint32_t>(Bones.size()));
	for (const auto& Bone : Bones)
	{
		W.WriteString(Bone.Name);
		W.WriteU32(static_cast<std::uint32_t>(Bone.ParentIndex));
		for (int I = 0; I < 16; ++I) W.WriteF32(Bone.BindLocal[I]);
	}
}

static void ReadSkeletonCpu(FSkeleton& Skeleton, FByteReader& R)
{
	std::uint16_t Version = R.ReadU16();
	std::uint32_t BoneCount = R.ReadU32();
	std::vector<FSkeletonBone> Bones(BoneCount);
	for (std::uint32_t I = 0; I < BoneCount; ++I)
	{
		Bones[I].Name = R.ReadString();
		Bones[I].ParentIndex = static_cast<std::int32_t>(R.ReadU32());
		for (int J = 0; J < 16; ++J) Bones[I].BindLocal[J] = R.ReadF32();
	}
	Skeleton.SetBones(std::move(Bones));
}

// ── Animation CPU ──────────────────────────────────────────────

static void WriteAnimationCpu(const FAnimation& Anim, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kCpuLayoutVersion);
	W.WriteString(Anim.GetSkeleton());
	W.WriteF32(Anim.GetDurationSeconds());
	const auto& Tracks = Anim.GetTracks();
	W.WriteU32(static_cast<std::uint32_t>(Tracks.size()));
	for (const auto& Track : Tracks)
	{
		W.WriteString(Track.TargetBoneName);
		W.WriteU32(static_cast<std::uint32_t>(Track.Keys.size()));
		for (const auto& Key : Track.Keys)
		{
			W.WriteF32(Key.Time);
			for (int I = 0; I < 3; ++I) W.WriteF32(Key.Translation[I]);
			for (int I = 0; I < 4; ++I) W.WriteF32(Key.Rotation[I]);
			for (int I = 0; I < 3; ++I) W.WriteF32(Key.Scale[I]);
		}
	}
}

static void ReadAnimationCpu(FAnimation& Anim, FByteReader& R)
{
	std::uint16_t Version = R.ReadU16();
	Anim.SetSkeleton(R.ReadString());
	Anim.SetDurationSeconds(R.ReadF32());
	std::uint32_t TrackCount = R.ReadU32();
	std::vector<FAnimationTrack> Tracks(TrackCount);
	for (std::uint32_t I = 0; I < TrackCount; ++I)
	{
		Tracks[I].TargetBoneName = R.ReadString();
		std::uint32_t KeyCount = R.ReadU32();
		Tracks[I].Keys.resize(KeyCount);
		for (std::uint32_t J = 0; J < KeyCount; ++J)
		{
			Tracks[I].Keys[J].Time = R.ReadF32();
			for (int K = 0; K < 3; ++K) Tracks[I].Keys[J].Translation[K] = R.ReadF32();
			for (int K = 0; K < 4; ++K) Tracks[I].Keys[J].Rotation[K] = R.ReadF32();
			for (int K = 0; K < 3; ++K) Tracks[I].Keys[J].Scale[K] = R.ReadF32();
		}
	}
	Anim.SetTracks(std::move(Tracks));
}

// ── Document JSON ──────────────────────────────────────────────

static void WriteDocumentJsonCpu(const std::string& Json, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteU16(kCpuLayoutVersion);
	W.WriteString(Json);
}

static std::string ReadDocumentJsonCpu(FByteReader& R)
{
	std::uint16_t Version = R.ReadU16();
	(void)Version;
	return R.ReadString();
}

// ── Object record ──────────────────────────────────────────────

static void WriteObjectRecord(const FResource& Object, std::vector<std::uint8_t>& Out)
{
	FByteWriter W(Out);
	W.WriteString(Object.GetName());
	W.WriteU16(static_cast<std::uint16_t>(Object.GetType()));
	W.WriteString(Object.GetSourcePath());

	std::uint32_t RecordFlags = kRecordHasCpu;

	std::vector<std::string> Refs = Object.GetReferencePaths();
	if (!Refs.empty())
		RecordFlags |= kRecordHasRefs;

	W.WriteU32(RecordFlags);
	W.WriteU32(0); // Reserved0
	W.WriteU32(0); // Reserved1

	if (!Refs.empty())
	{
		W.WriteU32(static_cast<std::uint32_t>(Refs.size()));
		for (const std::string& Ref : Refs)
			W.WriteString(Ref);
	}

	std::vector<std::uint8_t> CpuBytes;
	if (WriteCpuPayloadBytes(Object, CpuBytes))
	{
		W.WriteU32(static_cast<std::uint32_t>(CpuBytes.size()));
		if (!CpuBytes.empty())
			W.WriteBytes(CpuBytes.data(), CpuBytes.size());
	}
	else
	{
		W.WriteU32(0);
	}
}

static void ParseObjectRecord(FByteReader& R, FCassetParsedObject& Out)
{
	Out.Name = R.ReadString();
	Out.Type = static_cast<EAssetType>(R.ReadU16());
	Out.ImportSource = R.ReadString();
	Out.RecordFlags = R.ReadU32();
	R.Skip(8); // Reserved0 + Reserved1

	if ((Out.RecordFlags & kRecordHasRefs) != 0)
	{
		std::uint32_t RefCount = R.ReadU32();
		Out.Refs.reserve(RefCount);
		for (std::uint32_t I = 0; I < RefCount; ++I)
			Out.Refs.push_back(R.ReadString());
	}

	if ((Out.RecordFlags & kRecordHasCpu) != 0)
	{
		std::uint32_t CpuSize = R.ReadU32();
		if (CpuSize > 0)
			Out.CpuBytes = R.ReadBytes(CpuSize);
	}

	if ((Out.RecordFlags & kRecordHasExtras) != 0)
	{
		std::uint32_t ExtraCount = R.ReadU32();
		for (std::uint32_t I = 0; I < ExtraCount; ++I)
		{
			R.ReadU32(); // Key
			std::uint32_t Size = R.ReadU32();
			R.Skip(Size);
		}
	}
}

// ── Document build/parse ───────────────────────────────────────

static void BuildUncompressedDocument(
	const FResourcePackage& Package,
	const std::vector<FResource*>& Objects,
	std::vector<std::uint8_t>& OutDoc)
{
	// PKG1 chunk
	{
		std::vector<std::uint8_t> Chunk;
		FByteWriter W(Chunk);
		W.WriteString(Package.Name);
		W.WriteU32(Package.Flags);
		W.WriteString("");
		W.WriteU32(static_cast<std::uint32_t>(Objects.size()));
		for (int I = 0; I < 5; ++I) W.WriteU32(0);
		AppendChunk(OutDoc, kChunkTagPKG1, kChunkMustUnderstand, Chunk);
	}

	// DEPS chunk — collect cross-package references
	{
		std::vector<std::uint8_t> Chunk;
		FByteWriter W(Chunk);

		std::vector<std::pair<std::string, std::string>> Deps;
		for (const FResource* Obj : Objects)
		{
			if (!Obj) continue;
			for (const std::string& RefPath : Obj->GetReferencePaths())
			{
				std::size_t Dot = RefPath.find('.');
				if (Dot == std::string::npos) continue;
				std::string DepPkg = RefPath.substr(0, Dot);
				if (DepPkg != Package.Name)
					Deps.push_back({DepPkg, RefPath});
			}
		}

		W.WriteU32(static_cast<std::uint32_t>(Deps.size()));
		for (const auto& Dep : Deps)
		{
			W.WriteString(Dep.first);
			W.WriteString(Dep.second);
			W.WriteU32(0);
		}
		AppendChunk(OutDoc, kChunkTagDEPS, kChunkMustUnderstand, Chunk);
	}

	// OBJS chunk
	{
		std::vector<std::uint8_t> Chunk;
		FByteWriter W(Chunk);
		W.WriteU32(static_cast<std::uint32_t>(Objects.size()));
		for (const FResource* Obj : Objects)
		{
			if (!Obj) continue;
			std::vector<std::uint8_t> Record;
			WriteObjectRecord(*Obj, Record);
			W.WriteBytes(Record.data(), Record.size());
		}
		W.PadTo4();
		AppendChunk(OutDoc, kChunkTagOBJS, kChunkMustUnderstand, Chunk);
	}
}

static void ParseUncompressedDocument(
	const std::vector<std::uint8_t>& Doc,
	FCassetParsedPackage& Out)
{
	FByteReader R(Doc.data(), Doc.size());
	while (R.CanRead(4))
	{
		std::uint32_t Tag = R.ReadU32();
		std::uint32_t PayloadSize = R.ReadU32();
		std::uint32_t Flags = R.ReadU32();
		(void)Flags;

		if (!R.CanRead(PayloadSize)) break;

		std::vector<std::uint8_t> Payload = R.ReadBytes(PayloadSize);
		FByteReader PR(Payload.data(), Payload.size());

		if (Tag == kChunkTagPKG1)
		{
			Out.Name = PR.ReadString();
			Out.PackageFlags = PR.ReadU32();
			Out.DefaultObjectName = PR.ReadString();
			PR.ReadU32(); // ObjectCountHint
			PR.Skip(20); // 5 x Reserved
		}
		else if (Tag == kChunkTagDEPS)
		{
			std::uint32_t DepCount = PR.ReadU32();
			Out.Dependencies.resize(DepCount);
			for (std::uint32_t I = 0; I < DepCount; ++I)
			{
				Out.Dependencies[I].PackageName = PR.ReadString();
				Out.Dependencies[I].FilePath = PR.ReadString();
				Out.Dependencies[I].Reserved = PR.ReadU32();
			}
		}
		else if (Tag == kChunkTagOBJS)
		{
			std::uint32_t ObjCount = PR.ReadU32();
			Out.Objects.reserve(ObjCount);
			for (std::uint32_t I = 0; I < ObjCount; ++I)
			{
				FCassetParsedObject Obj;
				ParseObjectRecord(PR, Obj);
				Out.Objects.push_back(std::move(Obj));
			}
		}

		R.SkipPadTo4();
	}
}

// ── Public API ─────────────────────────────────────────────────

bool IsCassetBinaryFile(const std::uint8_t* Data, std::size_t Size)
{
	if (Size < 8) return false;
	std::uint32_t Magic = static_cast<std::uint32_t>(Data[0])
		| (static_cast<std::uint32_t>(Data[1]) << 8)
		| (static_cast<std::uint32_t>(Data[2]) << 16)
		| (static_cast<std::uint32_t>(Data[3]) << 24);
	return Magic == kCassetMagic;
}

bool WriteCpuPayloadBytes(const FResource& Resource, std::vector<std::uint8_t>& OutCpuBytes)
{
	if (const auto* Tex = dynamic_cast<const FTexture*>(&Resource))
	{
		WriteTextureCpu(*Tex, OutCpuBytes);
		return true;
	}
	if (const auto* Mesh = dynamic_cast<const FStaticMesh*>(&Resource))
	{
		WriteMeshCpu(*Mesh, OutCpuBytes);
		return true;
	}
	if (const auto* Mat = dynamic_cast<const FMaterial*>(&Resource))
	{
		WriteMaterialCpu(*Mat, OutCpuBytes);
		return true;
	}
	if (const auto* Skeleton = dynamic_cast<const FSkeleton*>(&Resource))
	{
		WriteSkeletonCpu(*Skeleton, OutCpuBytes);
		return true;
	}
	if (const auto* Anim = dynamic_cast<const FAnimation*>(&Resource))
	{
		WriteAnimationCpu(*Anim, OutCpuBytes);
		return true;
	}
	if (const auto* Graph = dynamic_cast<const FAnimationGraph*>(&Resource))
	{
		WriteDocumentJsonCpu(Graph->GetDocumentJson(), OutCpuBytes);
		return true;
	}
	if (const auto* Prefab = dynamic_cast<const FPrefab*>(&Resource))
	{
		WriteDocumentJsonCpu(Prefab->GetDocumentJson(), OutCpuBytes);
		return true;
	}
	return false;
}

bool ApplyCpuPayload(FResource& Resource, const std::vector<std::uint8_t>& CpuBytes)
{
	FByteReader R(CpuBytes.data(), CpuBytes.size());

	if (auto* Tex = dynamic_cast<FTexture*>(&Resource))
	{
		if (ReadTextureCpu(*Tex, R))
		{
			Resource.MarkCpuReady();
			return true;
		}
	}
	if (auto* Mesh = dynamic_cast<FStaticMesh*>(&Resource))
	{
		ReadMeshCpu(*Mesh, R);
		Resource.MarkCpuReady();
		return true;
	}
	if (auto* Mat = dynamic_cast<FMaterial*>(&Resource))
	{
		ReadMaterialCpu(*Mat, R);
		Resource.MarkCpuReady();
		return true;
	}
	if (auto* Skeleton = dynamic_cast<FSkeleton*>(&Resource))
	{
		ReadSkeletonCpu(*Skeleton, R);
		Resource.MarkCpuReady();
		return true;
	}
	if (auto* Anim = dynamic_cast<FAnimation*>(&Resource))
	{
		ReadAnimationCpu(*Anim, R);
		Resource.MarkCpuReady();
		return true;
	}
	if (auto* Graph = dynamic_cast<FAnimationGraph*>(&Resource))
	{
		Graph->SetDocumentJson(ReadDocumentJsonCpu(R));
		Resource.MarkCpuReady();
		return true;
	}
	if (auto* Prefab = dynamic_cast<FPrefab*>(&Resource))
	{
		Prefab->SetDocumentJson(ReadDocumentJsonCpu(R));
		Resource.MarkCpuReady();
		return true;
	}

	return false;
}

bool EncodePackageFile(
	const FResourcePackage& Package,
	const std::vector<FResource*>& Objects,
	std::vector<std::uint8_t>& OutFileBytes)
{
	std::vector<std::uint8_t> Document;
	if (!BuildPackageDocument(Package, Objects, Document))
		return false;
	return WrapDocumentToMcasFile(Document, OutFileBytes);
}

bool BuildPackageDocument(
	const FResourcePackage& Package,
	const std::vector<FResource*>& Objects,
	std::vector<std::uint8_t>& OutDocument)
{
	OutDocument.clear();
	BuildUncompressedDocument(Package, Objects, OutDocument);
	return true;
}

bool WrapDocumentToMcasFile(
	const std::vector<std::uint8_t>& UncompressedDocument,
	std::vector<std::uint8_t>& OutFileBytes)
{
	std::vector<std::uint8_t> Compressed;
	if (!FCompression::CompressZlib(
		UncompressedDocument.data(),
		UncompressedDocument.size(),
		Compressed))
	{
		MAHO_CORE_ERROR("ResourceCasset: zlib compress failed");
		return false;
	}

	OutFileBytes.clear();
	FByteWriter W(OutFileBytes);
	W.WriteU32(kCassetMagic);
	W.WriteU32(kCassetFormatVersion);
	W.WriteU32(kCassetHeaderSize);
	W.WriteU32(kCassetFlagBodyZlib);
	W.WriteU32(kCassetContentVersion);
	W.WriteU32(static_cast<std::uint32_t>(UncompressedDocument.size()));
	W.WriteU32(static_cast<std::uint32_t>(Compressed.size()));
	W.WriteU32(0); // Checksum

	for (int I = 0; I < 8; ++I) W.WriteU32(0); // Reserved

	W.WriteBytes(Compressed.data(), Compressed.size());
	return true;
}

bool DecodePackageFile(
	const std::uint8_t* FileBytes,
	std::size_t FileSize,
	FCassetParsedPackage& OutPackage)
{
	if (FileSize < kCassetHeaderSize) return false;

	FByteReader R(FileBytes, FileSize);
	std::uint32_t Magic = R.ReadU32();
	if (Magic != kCassetMagic) return false;

	std::uint32_t FormatVersion = R.ReadU32();
	std::uint32_t HeaderSize = R.ReadU32();
	std::uint32_t Flags = R.ReadU32();

	if (FormatVersion > kCassetFormatVersion) return false;

	R.Skip(4); // ContentVersion
	std::uint32_t UncompressedSize = R.ReadU32();
	std::uint32_t CompressedSize = R.ReadU32();
	R.Skip(36); // Checksum(4) + Reserved(32)

	std::vector<std::uint8_t> Compressed = R.ReadBytes(CompressedSize);

	std::vector<std::uint8_t> Uncompressed;
	if ((Flags & kCassetFlagBodyZlib) != 0)
	{
		if (!FCompression::DecompressZlib(
			Compressed.data(),
			Compressed.size(),
			UncompressedSize,
			Uncompressed))
		{
			MAHO_CORE_ERROR("ResourceCasset: zlib decompress failed");
			return false;
		}
	}
	else
	{
		Uncompressed = std::move(Compressed);
	}

	ParseUncompressedDocument(Uncompressed, OutPackage);
	return true;
}

} // namespace ResourceCasset
} // namespace Maho
