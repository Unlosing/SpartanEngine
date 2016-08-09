/*
Copyright(c) 2016 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =================================================
#include "Collider.h"
#include "MeshFilter.h"
#include "RigidBody.h"
#include "../Core/GameObject.h"
#include "../IO/Serializer.h"
#include "../Physics/BulletPhysicsHelper.h"
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
//===========================================================

//= NAMESPACES ================
using namespace Directus::Math;
//=============================

Collider::Collider()
{
	m_shapeType = Box;
	m_shape = nullptr;
	m_boundingBox = Vector3::One;
	m_scale = Vector3::One;
	m_center = Vector3::Zero;
}

Collider::~Collider()
{
	delete m_shape;
	m_shape = nullptr;
}

//= ICOMPONENT ========================================================================
void Collider::Initialize()
{
	Mesh* mesh = GetMeshFromAttachedMeshFilter();
	if (mesh)
	{
		m_boundingBox = mesh->GetExtent();
		m_center = mesh->GetCenter();
	}

	ConstructCollisionShape();
}

void Collider::Start()
{

}

void Collider::Remove()
{
	SetRigidBodyCollisionShape(nullptr);
}

void Collider::Update()
{

}

void Collider::Serialize()
{
	Serializer::SaveInt(int(m_shapeType));
	Serializer::SaveVector3(m_boundingBox);
	Serializer::SaveVector3(m_scale);
	Serializer::SaveVector3(m_center);
}

void Collider::Deserialize()
{
	m_shapeType = ColliderShape(Serializer::LoadInt());
	m_boundingBox = Serializer::LoadVector3();
	m_scale = Serializer::LoadVector3();
	m_center = Serializer::LoadVector3();

	ConstructCollisionShape();
}

//= BOUNDING BOX =============================================
Vector3 Collider::GetBoundingBox()
{
	return m_boundingBox;
}

void Collider::SetBoundingBox(Vector3 boxSize)
{
	if (boxSize == Vector3::Zero)
		return;

	m_boundingBox = boxSize.Absolute();

	ConstructCollisionShape();
}

//= SCALE ========================================================
Vector3 Collider::GetScale()
{
	return m_scale;
}

void Collider::SetScale(Vector3 scale)
{
	m_scale = scale;

	ConstructCollisionShape();
}

//= CENTER ========================================================
Vector3 Collider::GetCenter()
{
	return m_center;
}

void Collider::SetCenter(Vector3 center)
{
	m_center = center;

	ConstructCollisionShape();
}

//= COLLISION SHAPE ================================================
ColliderShape Collider::GetShapeType()
{
	return m_shapeType;
}

void Collider::SetShapeType(ColliderShape type)
{
	m_shapeType = type;

	ConstructCollisionShape();
}

btCollisionShape* Collider::GetBtCollisionShape()
{
	return m_shape;
}

//= MISC ====================================================================
void Collider::ConstructCollisionShape()
{
	Vector3 boundingBox = m_boundingBox * m_scale;

	// delete old shape (if it exists)
	if (m_shape)
	{
		SetRigidBodyCollisionShape(nullptr);
		delete m_shape;
		m_shape = nullptr;
	}

	// Create BOX shape
	if (m_shapeType == Box)
	{
		m_shape = new btBoxShape(ToBtVector3(boundingBox));
	}

	// Create CAPSULE shape
	if (m_shapeType == Capsule)
	{
		float radius = max(boundingBox.x, boundingBox.z);
		float height = boundingBox.y;
		m_shape = new btCapsuleShape(radius, height);
	}

	// Create CYLINDER shape
	if (m_shapeType == Cylinder)
	{
		m_shape = new btCylinderShape(ToBtVector3(boundingBox));
	}

	// Create SPHERE shape
	if (m_shapeType == Sphere)
	{
		float radius = max(boundingBox.x, boundingBox.y);
		radius = max(radius, boundingBox.z);
		m_shape = new btSphereShape(radius);
	}

	SetRigidBodyCollisionShape(m_shape);
}

void Collider::SetRigidBodyCollisionShape(btCollisionShape* shape)
{
	if (!g_gameObject)
		return;

	RigidBody* rigidBody = g_gameObject->GetComponent<RigidBody>();
	if (rigidBody)
		rigidBody->SetCollisionShape(shape);
}

Mesh* Collider::GetMeshFromAttachedMeshFilter() const
{
	MeshFilter* meshFilter = g_gameObject->GetComponent<MeshFilter>();
	return meshFilter ? meshFilter->GetMesh() : nullptr;
}