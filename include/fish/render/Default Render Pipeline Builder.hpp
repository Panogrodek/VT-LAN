#pragma once

#include "Render Pipeline.hpp"
#include "graphics/Shader.hpp"

#include "shader source/Vertex Source.hpp"
#include "shader source/Fragment Source.hpp"

namespace fs {
	namespace fs_priv {
		// | -------------------------------------- | //
		// | Default rendering pipeline create data | //
		// | -------------------------------------- | //

		static PipelineData CreateDefaultPipeline(Shader& shader) {
			VertexBufferDescriptions bufferLayout =
			{
				{ sizeof(Vertex) }
			};

			VertexBufferAttributeLayout attributeLayout =
			{
				{
					{ ShaderDataType::Float3 },
					{ ShaderDataType::Float2 },
					{ ShaderDataType::Float4 }
				}
			};

			ColorTargetAttributeLayout targetLayout =
			{
				{ TextureFormat::RGBA_16F }, // should be float 16f - 32f
			};

			targetLayout.EnableAlphaBlending = true;

			PipelineData pipelineData;
			pipelineData.VertexShader				   = &shader.GetVertexShader();
			pipelineData.FragmentShader				   = &shader.GetFragmentShader();
			pipelineData.VertexBuffersAttributeLayouts = attributeLayout;
			pipelineData.VertexBufferDescriptions	   = bufferLayout;
			pipelineData.ColorTargetsLayouts		   = targetLayout;
			pipelineData.PrimitiveType				   = PrimitiveType::TriangleList;

			return pipelineData;
		}

		// | ------------------------------------ | //
		// | Batch rendering pipeline create data | //
		// | ------------------------------------ | //

		static PipelineData CreateBatchPipeline(Shader& shader) {
			VertexBufferDescriptions bufferLayout =
			{
				{ sizeof(BatchVertex), 1 } // buffer will be bound on slot 1
			};

			VertexBufferAttributeLayout attributeLayout =
			{
				{
					{ ShaderDataType::Float3 },
					{ ShaderDataType::Float2 },
					{ ShaderDataType::Float2 },
					{ ShaderDataType::Float4 },
					{ ShaderDataType::Int	 },
				}
			};

			attributeLayout.SetSlot(0, 1); // declares usage of layout 0 on vertex buffer slot 1

			ColorTargetAttributeLayout targetLayout =
			{
				{ TextureFormat::RGBA_16F },
			};

			targetLayout.EnableAlphaBlending = true;

			PipelineData pipelineData;
			pipelineData.VertexShader				   = &shader.GetVertexShader();
			pipelineData.FragmentShader				   = &shader.GetFragmentShader();
			pipelineData.VertexBuffersAttributeLayouts = attributeLayout;
			pipelineData.VertexBufferDescriptions	   = bufferLayout;
			pipelineData.ColorTargetsLayouts		   = targetLayout;
			pipelineData.PrimitiveType				   = PrimitiveType::TriangleList;
			
			return pipelineData;
		}

		// | ---------------------------------------------- | //
		// | Flush to screen rendering pipeline create data | //
		// | ---------------------------------------------- | //

		static PipelineData CreateFlushPipeline(Shader& shader) {
			VertexBufferDescriptions bufferLayout =
			{
				{ sizeof(glm::vec2) }
			};

			VertexBufferAttributeLayout attributeLayout =
			{
				{
					{ ShaderDataType::Float2 },
				}
			};

			ColorTargetAttributeLayout targetLayout =
			{
				{ TextureFormat::RGBA_8B },
			};

			PipelineData pipelineData;
			pipelineData.VertexShader				   = &shader.GetVertexShader();
			pipelineData.FragmentShader				   = &shader.GetFragmentShader();
			pipelineData.VertexBuffersAttributeLayouts = attributeLayout;
			pipelineData.VertexBufferDescriptions	   = bufferLayout;
			pipelineData.ColorTargetsLayouts		   = targetLayout;
			pipelineData.PrimitiveType				   = PrimitiveType::TriangleStrip;

			return pipelineData;
		}

		// | -------------------------------------------- | //
		// | Default light rendering pipeline create data | //
		// | -------------------------------------------- | //

		static PipelineData CreateDefaultLightPipeline(Shader& shader) {
			return CreateDefaultPipeline(shader);
		}

		// | ------------------------------------------ | //
		// | Batch light rendering pipeline create data | //
		// | ------------------------------------------ | //

		static PipelineData CreateBatchLightPipeline(Shader& shader) {
			return CreateBatchPipeline(shader);
		}
	}
}
