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

#pragma once

//= INCLUDES ===============
#include "IComponent.h"
#include "../Math/Vector3.h"
#include "../Core/Mesh.h"
//==========================

class btBoxShape;
class MeshFilter;
class btCollisionShape;

enum ColliderShape
{
	Box,
	Capsule,
	Cylinder,
	Sphere
};

class __declspec(dllexport) Collider : public IComponent
{
public:
	Collider();
	~Collider();

	/*------------------------------------------------------------------------------
										[INTERFACE]
	------------------------------------------------------------------------------*/
	virtual void Initialize();
	virtual void Start();
	virtual void Remove();
	virtual void Update();
	virtual void Serialize();
	virtual void Deserialize();

	/*------------------------------------------------------------------------------
									[PROPERTIES]
	------------------------------------------------------------------------------*/
	Directus::Math::Vector3 GetBoundingBox();
	void SetBoundingBox(Directus::Math::Vector3 boundingBox);

	Directus::Math::Vector3 GetScale();
	void SetScale(Directus::Math::Vector3 scale);

	Directus::Math::Vector3 GetCenter();
	void SetCenter(Directus::Math::Vector3 offset);

	ColliderShape GetShapeType();
	void SetShapeType(ColliderShape type);

	btCollisionShape* GetBtCollisionShape();

private:
	ColliderShape m_shapeType;
	btCollisionShape* m_shape;
	Directus::Math::Vector3 m_boundingBox;
	Directus::Math::Vector3 m_scale; // this simply scales the bounding box
	Directus::Math::Vector3 m_center;

	/*------------------------------------------------------------------------------
										[MISC]
	------------------------------------------------------------------------------*/
	void ConstructCollisionShape();
	void SetRigidBodyCollisionShape(btCollisionShape* shape);
	Mesh* GetMeshFromAttachedMeshFilter() const;
};
