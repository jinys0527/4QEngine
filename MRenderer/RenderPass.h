#pragma once

#include <string_view>
#include <vector>
#include "AssetLoader.h"
#include "RenderData.h"
#include "DX11.h"
#include "Buffers.h"
#include "MathHelper.h"

// 렌더 패스의 공통 인터페이스
// 새로운 렌더 패스를 작성을 위한 부모 클래스
class RenderPass
{
public:
	RenderPass(RenderContext& context, AssetLoader& assetLoader) : m_RenderContext(context), m_AssetLoader(assetLoader) {}
	virtual ~RenderPass() = default;
	virtual std::string_view GetName() const = 0;

	void SetEnabled(bool enabled) { m_Enabled = enabled; }
	bool IsEnabled() const { return m_Enabled; }

	virtual void Setup(const RenderData::FrameData& frame);
	virtual void Execute(const RenderData::FrameData& frame) = 0;		//여기서 Draw까지 호출

	void SetBlendState(BS state);
	void SetDepthStencilState(DS state);
	void SetRasterizerState(RS state);
	void SetSamplerState();

	virtual void SetBaseCB(const RenderData::RenderItem& item);
	virtual void SetCameraCB(const RenderData::FrameData& frame);
	virtual void SetDirLight(const RenderData::FrameData& frame);
	virtual void SetVertex(const RenderData::RenderItem& item);
	virtual void DrawMesh(
		ID3D11Buffer* vb,
		ID3D11Buffer* ib,
		ID3D11InputLayout* layout,
		ID3D11VertexShader* vs,
		ID3D11PixelShader* ps,
		BOOL useSubMesh,
		UINT indexCount,
		UINT indexStart
	);
protected:
	virtual bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const;
	const std::vector<size_t>& GetQueue() const { return m_Queue; }

private:
	void BuildQueue(const RenderData::FrameData& frame);

	std::vector<size_t> m_Queue;
	bool m_Enabled = true;

protected:
	RenderContext& m_RenderContext;
	AssetLoader& m_AssetLoader;
};
