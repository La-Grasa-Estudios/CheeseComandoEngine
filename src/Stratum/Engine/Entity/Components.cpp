#include "Components.h"
#include "ClassMetadata.h"

using namespace ENGINE_NAMESPACE;

void TransformComponent::SetWorldPosition(const glm::vec3& position)
{
	glm::vec3 localPosition = glm::inverse(InverseBindMatrix) * glm::vec4(position, 1.0f);
	SetPosition(localPosition);
}

void TransformComponent::SetPosition(const glm::vec3& position)
{
	Position = position;
	IsDirty = true;
}

void TransformComponent::SetScale(const glm::vec3& scale)
{
	Scale = scale;
	IsDirty = true;
}

void TransformComponent::SetRotation(const glm::quat& rotation)
{
	Rotation = rotation;
	IsDirty = true;
}

glm::vec3 TransformComponent::GetWorldSpacePosition() const
{
	return InverseBindMatrix * glm::vec4(Position, 1.0f);
}

// Entity mappings for EDF if you don't declare this they won't show in level editor

DECLARE_COMPONENT(NameComponent, name)
DECLARE_COMPONENT(TransformComponent, transform)
DECLARE_COMPONENT(MeshRendererComponent, mesh_renderer)
DECLARE_COMPONENT(DynamicMeshLoaderComponent, dynamic_mesh)
DECLARE_COMPONENT(SpriteRendererComponent, sprite_renderer)
DECLARE_COMPONENT(SpriteRendererLoaderComponent, sprite_loader)
DECLARE_COMPONENT(SpriteAnimator, sprite_animator)
DECLARE_COMPONENT(GuiAnchorComponent, gui_anchor)
DECLARE_COMPONENT(VideoSurfaceComponent, video_surface)
DECLARE_COMPONENT(TextComponent, text_component)
DECLARE_COMPONENT(TextRendererComponent, text_renderer)

// Field definitions for EDF (Entity Definition File) used by the engine to load serialized scenes

DECLARE_COMPONENT_FIELD(NameComponent, Name)

DECLARE_COMPONENT_FIELD(TransformComponent, Parent)
DECLARE_COMPONENT_FIELD(TransformComponent, Position)
DECLARE_COMPONENT_FIELD(TransformComponent, Scale)
DECLARE_COMPONENT_FIELD(TransformComponent, Rotation)

DECLARE_COMPONENT_FIELD(MeshRendererComponent, CastShadows)
DECLARE_COMPONENT_FIELD(DynamicMeshLoaderComponent, Path)

DECLARE_COMPONENT_FIELD(SpriteRendererComponent, RenderLayer)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, Center)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, Rotation)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, SpriteColor)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, CameraLayer)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, FlipX)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, FlipY)
DECLARE_COMPONENT_FIELD(SpriteRendererComponent, UseNearestTextureFilter)
DECLARE_COMPONENT_FIELD(SpriteRendererLoaderComponent, RectSize)
DECLARE_COMPONENT_FIELD(SpriteRendererLoaderComponent, RectPosition)
DECLARE_COMPONENT_FIELD(SpriteRendererLoaderComponent, SpritePath)

DECLARE_COMPONENT_FIELD(SpriteAnimator, CurrentAnimation)
DECLARE_COMPONENT_FIELD(SpriteAnimator, DefaultAnimation)

DECLARE_COMPONENT_FIELD(GuiAnchorComponent, Position)
DECLARE_COMPONENT_FIELD(GuiAnchorComponent, AnchorPoint)

DECLARE_COMPONENT_FIELD(VideoSurfaceComponent, Path)
DECLARE_COMPONENT_FIELD(VideoSurfaceComponent, PlayAudio)

DECLARE_COMPONENT_FIELD(TextComponent, Font)
DECLARE_COMPONENT_FIELD(TextComponent, Text)
DECLARE_COMPONENT_FIELD(TextComponent, FontSize)

DECLARE_COMPONENT_FIELD(TextRendererComponent, Enabled)
DECLARE_COMPONENT_FIELD(TextRendererComponent, CameraLayer)
DECLARE_COMPONENT_FIELD(TextRendererComponent, Alignment)
DECLARE_COMPONENT_FIELD(TextRendererComponent, RenderLayer)
DECLARE_COMPONENT_FIELD(TextRendererComponent, Color)
