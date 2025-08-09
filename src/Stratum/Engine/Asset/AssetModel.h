#pragma once

#include "znmsp.h"
#include "glm/ext.hpp"

BEGIN_ENGINE

constexpr int MDL_BONE_TYPE_ANIM = 1; // Animation joints have an extra Matrix4x4 attached to the model data

constexpr int MDL_VERTEX_TYPE_POS_UV_NORMAL = 0;
constexpr int MDL_VERTEX_TYPE_POS_UV_NORMAL_TANGENT = 1;
constexpr int MDL_VERTEX_TYPE_ANIMATED = 2;

struct mdlkeyposition_t
{
	glm::vec3 position;
	float timeStamp;
};

struct mdlkeyrotation_t
{
	glm::quat rotation;
	float timeStamp;
};

struct mdlanimchannel_t
{
	int name_length;
	char name[128];
	int joint_id;
	int positions_count;
	int rotations_count;
};

struct mdlanim_t
{
	float duration;
	float timescale;
	int channel_count;
};

struct mdlbone_t
{
	int type;
	int id;
	int parent_length; // 0 if bone is a leaf
	char parent[128];
	int name_length;
	char name[128];
};

struct mdlmesh_t
{
	int vertex_type;
	int vertex_count;
	int tris_type; // 0 = uint16_t, 1 = uint32_t
	int tris_count;
	int material_index;
	int lod_level;

	glm::vec3 bbmin; // Corner of the mesh bounding box
	glm::vec3 bbmax; // The other corner
};

struct mdlheader_t
{
	int magic; // "CMDL"
	int version;
	char checksum[64]; // Self explanatory
	glm::vec3 bbmin; // Corner of the model bounding box
	glm::vec3 bbmax; // The other corner

	// Joint data
	int bone_count;
	int bone_offset;

	// Num physics box
	int physbox_count;
	int physbox_offset;

	// Num animations
	int anim_count;
	int anim_offset;

	// Materials 
	int material_count;
	int material_offset;

	// Meshes mdlmesh_t
	int mesh_count;
	int mesh_offset;

	// Bounding box hierarchy data
	int bvh_offset;

	int future[32];
};

END_ENGINE