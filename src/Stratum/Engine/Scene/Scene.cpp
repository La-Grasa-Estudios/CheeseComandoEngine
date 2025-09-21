#include "Scene.h"

#include "Core/UnormInt.h"
#include "Core/NormByte.h"
#include "Core/Logger.h"
#include "Core/JobManager.h"
#include "Core/EngineStats.h"
#include <Core/Time.h>

#include "Util/Globals.h"
#include "Util/CpuUtil.h"

#include "VFS/ZVFS.h"

#include "Asset/AssetModel.h"

#include "Renderer/RendererContext.h"
#include "Renderer/Buffer.h"
#include "Renderer/CopyCommandBuffer.h"

#include "Media/VideoDecode.h"

#undef min

using namespace ENGINE_NAMESPACE;

#define INIT_COMPONENT_MANAGER(manager) mCustomComponents[manager.GetTypeName()] = { false, &manager }; manager.Init(&EntityManager);

Scene::Scene()
{
	INIT_COMPONENT_MANAGER(Names);
	INIT_COMPONENT_MANAGER(Transforms);
	INIT_COMPONENT_MANAGER(Renderers);
	INIT_COMPONENT_MANAGER(SpriteRenderers);
	INIT_COMPONENT_MANAGER(SpriteAnimators);
	INIT_COMPONENT_MANAGER(GuiAnchors);
	INIT_COMPONENT_MANAGER(VideoSurfaces);
	INIT_COMPONENT_MANAGER(TextComponents);
	INIT_COMPONENT_MANAGER(TextRenderers);
	INIT_COMPONENT_MANAGER(Cameras);

	mVideoCopyCommandBuffer = new Render::CopyCommandBuffer();

	FontRegistry.LoadFont("Roboto", "fonts/Roboto-Regular.ttf");

	UI = CreateScope<SceneUI>(this);
}

Scene::~Scene()
{
	for (int i = 0; i < ECS::C_MAX_ENTITIES; i++)
	{
		if (VideoSurfaces.mAllocatedArray[i] && VideoSurfaces.mComponentArray[i].mDecoder)
		{
			VideoDecode* decode = reinterpret_cast<VideoDecode*>(VideoSurfaces.mComponentArray[i].mDecoder);
			delete decode;

			VideoSurfaces.mComponentArray[i].mDecoder = 0;
		}
	}

	EntityManager.DestroyAll();

	for (auto p : mCustomComponents)
	{
		if (std::get<0>(p.second))
			delete std::get<1>(p.second);
	}
	for (auto p : mSystems)
	{
		delete p;
	}

	delete mVideoCopyCommandBuffer;
}

void Scene::InitBindlessTable(nvrhi::IBindingLayout* bindingLayout)
{
	BindlessTable = CreateRef<Render::BindlessDescriptorTable>(bindingLayout);
	Resources = { BindlessTable.get() };
}

void Scene::UpdateSystems()
{
	UpdateTransforms();
	UpdateAnimators();
	UI->Update();



	for (int i = 0; i < mSystems.size(); i++)
	{
		if (!mSystems[i]->mInitialized)
		{
			mSystems[i]->Init(this);
			mSystems[i]->mInitialized = true;
			Time::SkipFrame = true;
		}
	}
	for (int i = 0; i < mSystems.size(); i++)
	{
		if (!mSystems[i]->mHasBeenActivated)
		{
			mSystems[i]->OnActivate(this);
			mSystems[i]->mHasBeenActivated = true;
			Time::SkipFrame = true;
		}
		mSystems[i]->Update(this);
	}
}

void Scene::PostUpdate()
{
	for (int i = 0; i < mSystems.size(); i++)
	{
		if (mSystems[i]->pEarlyUpdate)
			mSystems[i]->PostUpdate(this);
	}
	UpdateVideoPlayers();
	UpdateGuiAnchors();
	UpdateTransforms();
	for (int i = 0; i < mSystems.size(); i++)
	{
		if (!mSystems[i]->pEarlyUpdate)
			mSystems[i]->PostUpdate(this);
	}
	EntityManager.Update();
}

void Scene::RenderImGui()
{
	for (int i = 0; i < mSystems.size(); i++)
	{
		mSystems[i]->RenderImGui(this);
	}
}

void Scene::LoadModel(const std::string& path, const ECS::edict_t edict)
{

	assert(edict != ECS::C_INVALID_ENTITY);

	struct alignas(4) Vertex
	{
		glm::vec3 position;
		uint16_t texCoord[2];
		unormbyte normalX;
		unormbyte normalY;
		unormbyte normalZ;
		unormbyte tangentX;
		unormbyte tangentY;
		unormbyte tangentZ;
	};

	struct BoneVertex
	{
		Vertex vtx;
		uint8_t boneIds[4];
		normbyte4 boneWeights;
	};

	using namespace Render;

	PakStream in = ZVFS::GetFileStream(path.c_str());

	mdlheader_t header{};

	in->read((char*)&header, sizeof(mdlheader_t));

	if (header.magic != 0x4C444D43)
	{
		Z_WARN("File {} is not a valid mdl file!", path);
		return;
	}

	std::istream* pStream = in->GetStream();

	pStream->seekg(header.mesh_offset);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	uint32_t vertexOffset = 0;

	std::vector<MeshRendererComponent::MeshSubset> subSets;
	MeshRendererComponent& rendererComponent = this->Renderers.Create(edict);

	for (int i = 0; i < header.mesh_count; i++)
	{

		MeshRendererComponent::MeshSubset subset;
		mdlmesh_t mMesh{};

		pStream->read((char*)&mMesh.vertex_type, sizeof(int));
		pStream->read((char*)&mMesh.vertex_count, sizeof(int));
		pStream->read((char*)&mMesh.tris_type, sizeof(int));
		pStream->read((char*)&mMesh.tris_count, sizeof(int));
		pStream->read((char*)&mMesh.material_index, sizeof(int));

		if (header.version >= 0x02)
		{
			pStream->read((char*)&mMesh.lod_level, sizeof(int));
		}
		else
		{
			mMesh.lod_level = 0;
		}

		pStream->read((char*)&mMesh.bbmin, sizeof(glm::vec3));
		pStream->read((char*)&mMesh.bbmax, sizeof(glm::vec3));

		subset.IndexCount = mMesh.tris_count;
		subset.IndexOffset = static_cast<int>(indices.size());

		for (int i = 0; i < mMesh.tris_count; i++)
		{
			if (mMesh.tris_type == 0)
			{
				uint16_t triangle;
				pStream->read((char*)&triangle, sizeof(uint16_t));
				indices.push_back(triangle + vertexOffset);
				continue;
			}
			uint32_t triangle;
			pStream->read((char*)&triangle, sizeof(uint32_t));
			indices.push_back(triangle + vertexOffset);
		}

		for (int i = 0; i < mMesh.vertex_count; i++)
		{
			Vertex vtx{};
			float tx;
			float ty;

			pStream->read((char*)&vtx.position, sizeof(glm::vec3));
			pStream->read((char*)&tx, sizeof(float));
			pStream->read((char*)&ty, sizeof(float));
			pStream->read((char*)&vtx.normalX, sizeof(unormbyte));
			pStream->read((char*)&vtx.normalY, sizeof(unormbyte));
			pStream->read((char*)&vtx.normalZ, sizeof(unormbyte));
			pStream->read((char*)&vtx.tangentX, sizeof(unormbyte));
			pStream->read((char*)&vtx.tangentY, sizeof(unormbyte));
			pStream->read((char*)&vtx.tangentZ, sizeof(unormbyte));

			if (RendererContext::get_api() == RendererAPI::DX11 || RendererContext::get_api() == RendererAPI::DX12)
			{
				ty = 1.0f - ty;
			}

			vtx.texCoord[0] = (uint16_t)((int)abs(tx * 65535.0f) % 65535);
			vtx.texCoord[1] = (uint16_t)((int)abs(ty * 65535.0f) % 65535);

			if (mMesh.vertex_type == MDL_VERTEX_TYPE_POS_UV_NORMAL_TANGENT)
			{
				vertices.push_back(vtx);
				continue;
			}

			uint8_t ids[4];
			normbyte4 weights{};

			pStream->read((char*)ids, sizeof(ids));
			pStream->read((char*)&weights, sizeof(normbyte4));

			vertices.push_back(vtx);
		}

		vertexOffset += mMesh.vertex_count;

		subSets.push_back(subset);
	}

	rendererComponent.Subsets = subSets;

	Render::BufferDescription vertexBufferDesc{};

	vertexBufferDesc.ComputeType = BufferComputeType::RAW;
	vertexBufferDesc.Type = BufferType::VERTEX_BUFFER;

	vertexBufferDesc.Size = sizeof(Vertex) * vertices.size();
	vertexBufferDesc.pSysMem = vertices.data();

	Render::BufferDescription indexBufferDesc{};

	indexBufferDesc.ComputeType = BufferComputeType::RAW;
	//indexBufferDesc.Format = nvrhi::Format::R32_UINT;
	vertexBufferDesc.Type = BufferType::INDEX_BUFFER;

	indexBufferDesc.Size = sizeof(uint32_t) * indices.size();
	indexBufferDesc.pSysMem = indices.data();

	Buffer* vb = new Buffer(vertexBufferDesc);
	Buffer* ib = new Buffer(indexBufferDesc);

	rendererComponent.VertexBufferDescriptorIndex = Resources.CreateBufferHandle(vb);
	rendererComponent.IndexBufferDescriptorIndex = Resources.CreateBufferHandle(ib);
	
}

void Scene::RegisterCustomComponent(ECS::ComponentManager_Interface* pInterface, const std::string& name)
{
	pInterface->Init(&EntityManager);
	mCustomComponents[name] = { true, pInterface };
}

void Scene::RegisterCustomSystem(ISceneSystem* pSystem, bool initImmediately)
{
	mSystems.push_back(pSystem);

	if (initImmediately)
	{
		pSystem->Init(this);
		pSystem->mInitialized = true;
	}
}

void Scene::InitVideo(VideoSurfaceComponent& surface)
{
	surface.mDecoder = new VideoDecode(surface.Path, surface.PlayAudio ? this->AudioEngine : NULL);
	VideoDecode* decode = reinterpret_cast<VideoDecode*>(surface.mDecoder);

	auto params = decode->GetSize();

	surface.VideoResolution.x = params.width;
	surface.VideoResolution.y = params.height;

	Render::ImageDescription desc;
	desc.Width = params.width;
	desc.Height = params.height;
	desc.Format = Render::ImageFormat::RGBA8_UNORM;

	surface.TextureHandle = Resources.CreateTextureImage(desc);
	decode->Step();
}

void Scene::SwapScene(Scene* scene)
{
	NextScenePtr = scene;
}

void Scene::UpdateTransforms()
{
	Z_PROFILE_SCOPE("Scene::UpdateTransforms");

	auto& transforms = Transforms.GetEntities();

	for (auto entity : transforms)
	{
		auto& transform = Transforms.Get(entity);

		if (transform.IsDirty)
		{
			glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), transform.Position);
			model *= glm::mat4_cast(transform.Rotation);
			model = glm::scale(model, transform.Scale);
			transform.ModelMatrix = model;

			transform.IsDirty = false;
		}
	}
}

void Scene::UpdateAnimators()
{
	Z_PROFILE_SCOPE("Scene::UpdateAnimators");

	auto& animators = SpriteAnimators.GetEntities();

	for (auto entity : animators)
	{
		auto& anim = SpriteAnimators.Get(entity);

		for (auto& kp : anim.AnimationMap)
		{
			if (kp.second.PlayOnIdle)
			{
				kp.second.Accumulator += gpGlobals->deltaTime;
			}
		}

		for (auto& kp : anim.AnimationMap)
		{
			while (kp.second.Accumulator >= 1.0f / kp.second.FrameRate)
			{
				kp.second.FrameIndex += 1;
				kp.second.Accumulator -= 1.0f / kp.second.FrameRate;
			}

			if (!kp.second.IsLooping)
			{
				kp.second.FrameIndex = glm::min(kp.second.FrameIndex, (uint32_t)kp.second.rects.size() - 1);
			}
			else
			{
				kp.second.FrameIndex = kp.second.FrameIndex % kp.second.rects.size();
			}
		}

		if (anim.CurrentAnimation.empty())
			continue;

		if (auto a = anim.AnimationMap.find(anim.CurrentAnimation); a != anim.AnimationMap.end())
		{
			if (!a->second.PlayOnIdle)
			{
				a->second.Accumulator += gpGlobals->deltaTime;
			}

			if (!a->second.IsLooping && a->second.FrameIndex >= a->second.rects.size() - 1)
			{
				if (!a->second.NextState.empty())
					anim.SetState(a->second.NextState);
				else if (a->second.TransitionToDefault)
					anim.SetState(anim.DefaultAnimation);
			}
		}

	}

}

void Scene::UpdateGuiAnchors()
{
	Z_PROFILE_SCOPE("Scene::UpdateGuiAnchors");

	auto& anchors = GuiAnchors.GetEntities();

	for (auto entity : anchors)
	{

		if (!Transforms.HasComponent(entity))
			continue;

		auto& anchor = GuiAnchors.Get(entity);
		auto& transform = Transforms.Get(entity);

		glm::vec3 Position = glm::vec3(anchor.Position, 0.0f);

		if (anchor.AnchorPoint == GuiAnchorPoint::TOP)
		{
			Position = glm::vec3(anchor.Position.x, VirtualScreenSize.y - anchor.Position.y, 1.0f);
		}
		if (anchor.AnchorPoint == GuiAnchorPoint::BOTTOM)
		{
			Position = glm::vec3(anchor.Position.x, -VirtualScreenSize.y + anchor.Position.y, 1.0f);
		}

		if (anchor.AnchorPoint == GuiAnchorPoint::LEFT)
		{
			Position = glm::vec3(-VirtualScreenSize.x + anchor.Position.x, anchor.Position.y, 1.0f);
		}
		if (anchor.AnchorPoint == GuiAnchorPoint::RIGHT)
		{
			Position = glm::vec3(VirtualScreenSize.x - anchor.Position.x, anchor.Position.y, 1.0f);
		}

		if (anchor.AnchorPoint == GuiAnchorPoint::TOP_LEFT)
		{
			Position = glm::vec3(-VirtualScreenSize.x + anchor.Position.x, VirtualScreenSize.y - anchor.Position.y, 1.0f);
		}
		if (anchor.AnchorPoint == GuiAnchorPoint::TOP_RIGHT)
		{
			Position = glm::vec3(VirtualScreenSize.x - anchor.Position.x, VirtualScreenSize.y - anchor.Position.y, 1.0f);
		}

		if (anchor.AnchorPoint == GuiAnchorPoint::BOTTOM_LEFT)
		{
			Position = glm::vec3(-VirtualScreenSize.x + anchor.Position.x, -VirtualScreenSize.y + anchor.Position.y, 1.0f);
		}
		if (anchor.AnchorPoint == GuiAnchorPoint::BOTTOM_RIGHT)
		{
			Position = glm::vec3(VirtualScreenSize.x - anchor.Position.x, -VirtualScreenSize.y + anchor.Position.y, 1.0f);
		}

		transform.SetPosition(Position);
	}
}

void Scene::UpdateVideoPlayers()
{
	Z_PROFILE_SCOPE("Scene::UpdateVideoPlayers");

	auto& entities = VideoSurfaces.GetEntities();

	bool VideoUpdateRequired = false;

	for (auto entity : entities)
	{

		auto& surface = VideoSurfaces.Get(entity);

		if (surface.Path.empty())
			continue;

		if (!surface.mDecoder)
		{
			return;
		}

		VideoDecode* decode = reinterpret_cast<VideoDecode*>(surface.mDecoder);

		if (decode->Finished())
			continue;

		decode->SetLoop(surface.mShouldLoop);

		surface.mFrameAccumulator += gpGlobals->deltaTime;

		float frameTime = decode->GetFrametime() / surface.PlaybackSpeed;
		while (surface.mFrameAccumulator >= frameTime && !decode->Finished())
		{
			if (surface.mShouldPlay)
			{
				decode->Step();
			}

			surface.mFrameAccumulator -= frameTime;
			auto frame = decode->GetFrame();

			if (frame)
			{
				Render::ImageResource* pSurface = Resources.GetImageHandle(surface.TextureHandle);

				if (!VideoUpdateRequired)
					mVideoCopyCommandBuffer->Begin();

				auto cmd = mVideoCopyCommandBuffer->GetNativeCommandList();

				uint32_t count = surface.VideoResolution.x * surface.VideoResolution.y;
				auto data = frame->native()->data[0];

				mVideoCopyCommandBuffer->RequireTextureState(pSurface, Render::ResourceState::ShaderResource, Render::ResourceState::CopyDest);
				cmd->writeTexture(pSurface->Handle, 0, 0, data, surface.VideoResolution.x * 4);
				mVideoCopyCommandBuffer->RequireTextureState(pSurface, Render::ResourceState::CopyDest, Render::ResourceState::ShaderResource);
				decode->PushFrame(frame);

				VideoUpdateRequired = true;
			}
		}

	}

	for (int i = 0; i < ECS::C_MAX_ENTITIES; i++)
	{
		if (!VideoSurfaces.mAllocatedArray[i] && VideoSurfaces.mComponentArray[i].mDecoder)
		{
			VideoDecode* decode = reinterpret_cast<VideoDecode*>(VideoSurfaces.mComponentArray[i].mDecoder);
			// This is hot shit.
			// But for some reason without this the whole engine stutters to hell.
			// TO DO: Make our own abstraction of the libav library
			std::thread t([decode]() {
				delete decode;
			});
			t.detach();
			VideoSurfaces.mComponentArray[i].mDecoder = 0;
			Resources.ReleaseImage(VideoSurfaces.mComponentArray[i].TextureHandle);
		}
	}

	if (VideoUpdateRequired)
	{
		mVideoCopyCommandBuffer->End();
		mVideoCopyCommandBuffer->Submit();
		mVideoCopyCommandBuffer->TriggerWaitOnExecutionQueue(Render::CommandQueue::Graphics);
	}
	
}
