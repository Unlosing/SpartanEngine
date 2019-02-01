/*
Copyright(c) 2016-2019 Panos Karabelas

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

//= INCLUDES =====================
#include "RHI_Pipeline.h"
#include "RHI_Implementation.h"
#include "RHI_Sampler.h"
#include "RHI_Device.h"
#include "RHI_RenderTexture.h"
#include "RHI_VertexBuffer.h"
#include "RHI_IndexBuffer.h"
#include "RHI_Texture.h"
#include "RHI_Shader.h"
#include "RHI_ConstantBuffer.h"
#include "RHI_InputLayout.h"
#include "..\Logging\Log.h"
#include "../Profiling/Profiler.h"
//================================

//= NAMESPACES ================
using namespace std;
using namespace Directus::Math;
//=============================

namespace Directus
{
	RHI_Pipeline::RHI_Pipeline(Context* context, shared_ptr<RHI_Device> rhiDevice)
	{
		m_rhiDevice	= rhiDevice;
		m_profiler	= context->GetSubsystem<Profiler>();
		Reset();
	}

	bool RHI_Pipeline::DrawIndexed(unsigned int indexCount, unsigned int indexOffset, unsigned int vertexOffset)
	{
		bool bindResult = Bind();
		m_rhiDevice->DrawIndexed(indexCount, indexOffset, vertexOffset);
		m_profiler->m_rhiDrawCalls++;
		return bindResult;
	}

	bool RHI_Pipeline::Draw(unsigned int vertexCount)
	{
		bool bindResult = Bind();
		m_rhiDevice->Draw(vertexCount);
		m_profiler->m_rhiDrawCalls++;
		return bindResult;
	}

	void RHI_Pipeline::SetShader(shared_ptr<RHI_Shader>& shader)
	{
		SetVertexShader(shader);
		SetPixelShader(shader);	
	}

	bool RHI_Pipeline::SetVertexShader(shared_ptr<RHI_Shader>& shader)
	{
		if (!shader)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		if (shader->HasVertexShader() && m_boundVertexShaderID != shader->RHI_GetID())
		{
			SetInputLayout(shader->GetInputLayout()); // TODO: this has to be done outside of this function 
			m_vertexShader			= shader;
			m_boundVertexShaderID	= m_vertexShader->RHI_GetID();
			m_vertexShaderDirty		= true;
		}

		return true;
	}

	bool RHI_Pipeline::SetPixelShader(shared_ptr<RHI_Shader>& shader)
	{
		if (!shader)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		if (shader->HasPixelShader() && m_boundPixelShaderID != shader->RHI_GetID())
		{
			m_pixelShader			= shader;
			m_boundPixelShaderID	= m_pixelShader->RHI_GetID();
			m_pixelShaderDirty		= true;
		}

		return true;
	}

	bool RHI_Pipeline::SetIndexBuffer(const shared_ptr<RHI_IndexBuffer>& indexBuffer)
	{
		if (!indexBuffer)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		m_indexBuffer		= indexBuffer;
		m_indexBufferDirty	= true;

		return true;
	}

	bool RHI_Pipeline::SetVertexBuffer(const shared_ptr<RHI_VertexBuffer>& vertexBuffer)
	{
		if (!vertexBuffer)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		m_vertexBuffer		= vertexBuffer;
		m_vertexBufferDirty = true;

		return true;
	}

	bool RHI_Pipeline::SetSampler(const shared_ptr<RHI_Sampler>& sampler)
	{
		if (!sampler)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		m_samplers.emplace_back(sampler->GetBuffer());
		m_samplersDirty = true;

		return true;
	}

	bool RHI_Pipeline::SetTexture(const shared_ptr<RHI_RenderTexture>& texture)
	{
		// allow for null texture to be bound so we can maintain slot order
		m_textures.emplace_back(texture ? texture->GetShaderResource() : nullptr);
		m_texturesDirty = true;

		return true;
	}

	bool RHI_Pipeline::SetTexture(const shared_ptr<RHI_Texture>& texture)
	{
		// allow for null texture to be bound so we can maintain slot order
		m_textures.emplace_back(texture ? texture->GetShaderResource() : nullptr);
		m_texturesDirty = true;

		return true;
	}

	bool RHI_Pipeline::SetTexture(const RHI_Texture* texture)
	{
		// allow for null texture to be bound so we can maintain slot order
		m_textures.emplace_back(texture ? texture->GetShaderResource() : nullptr);
		m_texturesDirty = true;

		return true;
	}

	bool RHI_Pipeline::SetRenderTarget(const shared_ptr<RHI_RenderTexture>& renderTarget, void* depthStencilView /*= nullptr*/, bool clear /*= false*/)
	{
		if (!renderTarget)
			return false;

		m_renderTargetViews.clear();
		m_renderTargetViews.emplace_back(renderTarget->GetRenderTargetView());
		m_depthStencil			= depthStencilView;
		m_renderTargetsClear	= clear;
		m_renderTargetsDirty	= true;

		return true;
	}

	bool RHI_Pipeline::SetRenderTarget(void* renderTargetView, void* depthStencilView /*= nullptr*/, bool clear /*= false*/)
	{
		if (!renderTargetView)
			return false;

		m_renderTargetViews.clear();
		m_renderTargetViews.emplace_back(renderTargetView);

		m_depthStencil			= depthStencilView;
		m_renderTargetsClear	= clear;
		m_renderTargetsDirty	= true;

		return true;
	}

	bool RHI_Pipeline::SetRenderTarget(const vector<void*>& renderTargetViews, void* depthStencilView /*= nullptr*/, bool clear /*= false*/)
	{
		if (renderTargetViews.empty())
			return false;

		m_renderTargetViews.clear();
		for (const auto& renderTarget : renderTargetViews)
		{
			if (!renderTarget)
				continue;

			m_renderTargetViews.emplace_back(renderTarget);
		}

		m_depthStencil			= depthStencilView;
		m_renderTargetsClear	= clear;
		m_renderTargetsDirty	= true;

		return true;
	}

	bool RHI_Pipeline::SetConstantBuffer(const shared_ptr<RHI_ConstantBuffer>& constantBuffer, unsigned int slot, Buffer_Scope scope)
	{
		if (!constantBuffer)
		{
			LOG_WARNING("Invalid parameter");
			return false;
		}

		m_constantBuffers.emplace_back(constantBuffer->GetBuffer(), slot, scope);
		m_constantBufferDirty = true;

		return true;
	}

	void RHI_Pipeline::SetPrimitiveTopology(PrimitiveTopology_Mode primitiveTopology)
	{
		if (m_primitiveTopology == primitiveTopology)
			return;
	
		m_primitiveTopology			= primitiveTopology;
		m_primitiveTopologyDirty	= true;
	}

	bool RHI_Pipeline::SetInputLayout(const shared_ptr<RHI_InputLayout>& inputLayout)
	{
		if (m_inputLayout == inputLayout->GetInputLayout())
			return false;

		m_inputLayout		= inputLayout->GetInputLayout();
		m_inputLayoutBuffer	= inputLayout->GetBuffer();
		m_inputLayoutDirty	= true;

		return true;
	}

	void RHI_Pipeline::SetCullMode(Cull_Mode cullMode)
	{
		if (m_cullMode == cullMode)
			return;

		m_cullMode		= cullMode;
		m_cullModeDirty = true;
	}

	void RHI_Pipeline::SetFillMode(Fill_Mode fillMode)
	{
		if (m_fillMode == fillMode)
			return;

		m_fillMode		= fillMode;
		m_fillModeDirty = true;
	}

	void RHI_Pipeline::SetAlphaBlending(bool enabled)
	{
		if (m_alphaBlending == enabled)
			return;

		m_alphaBlending			= enabled;
		m_alphaBlendingDirty	= true;
	}

	void RHI_Pipeline::SetViewport(const RHI_Viewport& viewport)
	{
		if (viewport == m_viewport)
			return;

		m_viewport		= viewport;
		m_viewportDirty = true;
	}

	bool RHI_Pipeline::Bind()
	{
		if (!m_rhiDevice)
		{
			LOG_ERROR("Invalid RHI_Device");
			return false;
		}

		// Render Targets
		if (m_renderTargetsDirty)
		{
			if (m_renderTargetViews.empty())
			{
				LOG_ERROR("Invalid render target(s)");
				return false;
			}
			m_rhiDevice->Set_DepthEnabled(m_depthStencil != nullptr);
			m_rhiDevice->Set_RenderTargets((unsigned int)m_renderTargetViews.size(), &m_renderTargetViews[0], m_depthStencil);
			m_profiler->m_rhiBindingsRenderTarget++;

			if (m_renderTargetsClear)
			{
				for (const auto& renderTargetView : m_renderTargetViews)
				{
					m_rhiDevice->ClearRenderTarget(renderTargetView, Vector4(0, 0, 0, 0));
				}

				if (m_depthStencil)
				{
					float depth = m_viewport.GetMaxDepth();
					#if REVERSE_Z == 1
					depth = 1.0f - depth;
					#endif

					m_rhiDevice->ClearDepthStencil(m_depthStencil, Clear_Depth, depth, 0);
				}
			}

			m_renderTargetsClear = false;
			m_renderTargetsDirty = false;
		}

		// Textures
		if (m_texturesDirty)
		{
			unsigned int startSlot = 0;
			unsigned int textureCount = (unsigned int)m_textures.size();
			void* const* textures = textureCount != 0 ? &m_textures[0] : nullptr;

			m_rhiDevice->Set_Textures(startSlot, textureCount, textures);
			m_profiler->m_rhiBindingsTexture++;

			m_textures.clear();
			m_texturesDirty = false;
		}

		// Samplers
		if (m_samplersDirty)
		{
			unsigned int startSlot		= 0;
			unsigned int samplerCount	= (unsigned int)m_samplers.size();
			void* const* samplers		= samplerCount != 0 ? &m_samplers[0] : nullptr;

			m_rhiDevice->Set_Samplers(startSlot, samplerCount, samplers);
			m_profiler->m_rhiBindingsSampler++;

			m_samplers.clear();
			m_samplersDirty = false;
		}

		// Constant buffers
		if (m_constantBufferDirty)
		{
			for (const auto& constantBuffer : m_constantBuffers)
			{
				m_rhiDevice->Set_ConstantBuffers(constantBuffer.slot, 1, constantBuffer.scope, (void*const*)&constantBuffer.buffer);
				m_profiler->m_rhiBindingsBufferConstant += (constantBuffer.scope == Buffer_Global) ? 2 : 1;
			}

			m_constantBuffers.clear();
			m_constantBufferDirty = false;
		}

		// Vertex shader
		if (m_vertexShaderDirty)
		{
			m_rhiDevice->Set_VertexShader(m_vertexShader->GetVertexShaderBuffer());
			m_profiler->m_rhiBindingsVertexShader++;
			m_vertexShaderDirty = false;
		}

		// Pixel shader
		if (m_pixelShaderDirty)
		{
			m_rhiDevice->Set_PixelShader(m_pixelShader->GetPixelShaderBuffer());
			m_profiler->m_rhiBindingsPixelShader++;
			m_pixelShaderDirty = false;
		}

		// Input layout
		if (m_inputLayoutDirty)
		{
			m_rhiDevice->Set_InputLayout(m_inputLayoutBuffer);
			m_inputLayoutDirty = false;
		}

		// Viewport
		if (m_viewportDirty)
		{
			m_rhiDevice->Set_Viewport(m_viewport);
			m_viewportDirty = false;
		}

		// Primitive topology
		if (m_primitiveTopologyDirty)
		{
			m_rhiDevice->Set_PrimitiveTopology(m_primitiveTopology);
			m_primitiveTopologyDirty = false;
		}

		// Cull mode
		if (m_cullModeDirty)
		{
			m_rhiDevice->Set_CullMode(m_cullMode);
			m_cullModeDirty = false;
		}

		// Fill mode
		if (m_fillModeDirty)
		{
			m_rhiDevice->Set_FillMode(m_fillMode);
			m_fillModeDirty = false;
		}

		// Index buffer
		bool resultIndexBuffer = false;
		if (m_indexBufferDirty)
		{
			resultIndexBuffer = m_indexBuffer->Bind();
			m_profiler->m_rhiBindingsBufferIndex++;
			m_indexBufferDirty = false;
		}

		// Vertex buffer
		bool resultVertexBuffer = false;
		if (m_vertexBufferDirty)
		{
			resultVertexBuffer = m_vertexBuffer->Bind();
			m_profiler->m_rhiBindingsBufferVertex++;
			m_vertexBufferDirty = false;
		}

		// Alpha blending
		bool resultAlphaBlending = false;
		if (m_alphaBlendingDirty)
		{
			resultAlphaBlending = m_rhiDevice->Set_AlphaBlendingEnabled(m_alphaBlending);
			m_alphaBlendingDirty = false;
		}

		return resultIndexBuffer && resultVertexBuffer && resultAlphaBlending;
	}

	void RHI_Pipeline::Reset()
	{
		vector<void*> empty(10);
		auto empty_count		= (unsigned int)empty.size();
		void* const* empty_ptr	= &empty[0];

		// Textures
		m_rhiDevice->Set_Textures(0, empty_count, empty_ptr);
		m_textures.clear();
		m_texturesDirty = false;

		// Samplers
		m_rhiDevice->Set_Samplers(0, empty_count, empty_ptr);
		m_samplers.clear();
		m_samplersDirty = false;

		// Constant buffers
		m_rhiDevice->Set_ConstantBuffers(0, empty_count, Buffer_Global, empty_ptr);
		m_constantBuffers.clear();
		m_constantBufferDirty = false;

		// Fill mode
		if (m_fillMode != Fill_Solid)
		{
			m_fillMode = Fill_Solid;
			m_rhiDevice->Set_FillMode(m_fillMode);
			m_fillModeDirty = false;
		}

		// Cull mode
		if (m_cullMode != Cull_Back)
		{
			m_cullMode = Cull_Back;
			m_rhiDevice->Set_CullMode(m_cullMode);
			m_cullModeDirty = false;
		}

		// Primitive topology
		if (m_primitiveTopology != PrimitiveTopology_TriangleList)
		{
			m_primitiveTopology = PrimitiveTopology_TriangleList;
			m_rhiDevice->Set_PrimitiveTopology(m_primitiveTopology);
			m_primitiveTopologyDirty = false;
		}

		// Alpha blending
		if (m_alphaBlending == true)
		{
			m_alphaBlending = false;
			m_rhiDevice->Set_AlphaBlendingEnabled(m_alphaBlending);
			m_alphaBlendingDirty = false;
		}
	}
}