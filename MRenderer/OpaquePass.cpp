#include "ResourceStore.h"
#include "OpaquePass.h"

#include <iostream>

void OpaquePass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;

    m_RenderContext.BCBuffer.mView = context.view;
    m_RenderContext.BCBuffer.mProj = context.proj;
    m_RenderContext.BCBuffer.mVP = context.viewProj;

    



    for (size_t index : GetQueue())
    {
        const auto& item = frame.renderItems[index];

        m_RenderContext.BCBuffer.mWorld = item.world;


        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
        const auto* indexBuffers = m_RenderContext.indexBuffers;
        const auto* indexcounts = m_RenderContext.indexcounts;

        if (vertexBuffers && indexBuffers && item.mesh.IsValid())
        {
            const UINT bufferIndex = item.mesh.id;
            if (bufferIndex < vertexBuffers->size() && bufferIndex < indexBuffers->size())
            {
                ID3D11Buffer* vb = (*vertexBuffers)[bufferIndex].Get();
                ID3D11Buffer* ib = (*indexBuffers)[bufferIndex].Get();
                UINT32 icount = (*indexcounts)[bufferIndex];
                if (vb && ib)
                {
                    const UINT stride = sizeof(RenderData::Vertex);
                    const UINT offset = 0;
                    g_pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
                    g_pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
                    g_pDXDC->DrawIndexed(icount, 0, 0);

                }
            }
        }


        ClearBackBuffer(COLOR(0, 0, 1, 1));


        Flip();



        //아래 세팅하는건 함수로 빼두자
        //UpdateDynamicBuffer(g_pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));
        //g_pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());

        
    }
}

