#include "RTX_PathTracer.h"
#include "RTX_Manager.h"
#include "RTX_SBTGenerator.h"


namespace RTXSimplified
{
	int RTX_PathTracer::populateCommandList()
	{
		// Reset the allocator
		rtxManager->getInitializer()->getCommandAllocator()->Reset();

		// Reset the command list
		rtxManager->getInitializer()->getCommandList()->Reset(
			rtxManager->getInitializer()->getCommandAllocator().Get(), rtxManager->getInitializer()->getPipelineState().Get());

		// Set states
		rtxManager->getInitializer()->getCommandList()->SetGraphicsRootSignature(
			rtxManager->getInitializer()->getRootSignature().Get()
		);
		CD3DX12_VIEWPORT viewPort = rtxManager->getInitializer()->getViewPort();
		rtxManager->getInitializer()->getCommandList()->RSSetViewports(
			1, &viewPort
		);
		CD3DX12_RECT scissorRect = rtxManager->getInitializer()->getScissorRect();
		rtxManager->getInitializer()->getCommandList()->RSSetScissorRects(
			1, &scissorRect
		);

		CD3DX12_RESOURCE_BARRIER trans = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getRenderTarget().Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		// Set render target
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(
			1, &trans
		);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
			rtxManager->getInitializer()->getRTVheap()->GetCPUDescriptorHandleForHeapStart(),
			rtxManager->getInitializer()->getFrameIndex(),
			rtxManager->getInitializer()->getDescriptorHeapSize()
		);
		rtxManager->getInitializer()->getCommandList()->OMSetRenderTargets(
			1, &rtvHandle, false, nullptr
		);

		// Bind heaps
		std::vector<ID3D12DescriptorHeap*> heaps = { rtxManager->getInitializer()->getPipeline()->getSrvUavHeap().Get() };
		rtxManager->getInitializer()->getCommandList()->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

		// Transition last frame to UAV for shaders to write on it.
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getOutputResource().Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(1, &transition);

		// Set up RT task
		D3D12_DISPATCH_RAYS_DESC desc = {};
		
		// Add Raygen, Hit and Miss
		uint32_t rayGenerationSectionSize = rtxManager->getInitializer()->getPipeline()->getSBTGenerator().getRaygenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress = rtxManager->getInitializer()->getPipeline()->getSBTStorage()->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSize;

		uint32_t missSectionSize = rtxManager->getInitializer()->getPipeline()->getSBTGenerator().getMissSectionSize();
		desc.MissShaderTable.StartAddress = rtxManager->getInitializer()->getPipeline()->getSBTStorage()->GetGPUVirtualAddress() + rayGenerationSectionSize;
		desc.MissShaderTable.SizeInBytes = missSectionSize;
		desc.MissShaderTable.StrideInBytes = rtxManager->getInitializer()->getPipeline()->getSBTGenerator().getMissEntrySize();

		uint32_t hitGroupsSectionSize = rtxManager->getInitializer()->getPipeline()->getSBTGenerator().getHitGroupSectionSize();
		desc.HitGroupTable.StartAddress = rtxManager->getInitializer()->getPipeline()->getSBTStorage()->GetGPUVirtualAddress() +
			rayGenerationSectionSize +
			missSectionSize;
		desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
		desc.HitGroupTable.StrideInBytes = rtxManager->getInitializer()->getPipeline()->getSBTGenerator().getHitGroupSectionSize();

		// set width and height
		desc.Width = rtxManager->getWidth();
		desc.Height = rtxManager->getHeight();

		// Bind RT pipeline
		rtxManager->getInitializer()->getCommandList()->SetPipelineState1(rtxManager->getInitializer()->getRTStateObject().Get());
		// Dispatch rays
		rtxManager->getInitializer()->getCommandList()->DispatchRays(&desc);

		// Copy the output to copy source and then to target.
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getOutputResource().Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(1, &transition);
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getRenderTarget().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST);
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(1, &transition);

		rtxManager->getInitializer()->getCommandList()->CopyResource(
			rtxManager->getInitializer()->getRenderTarget().Get(),
			rtxManager->getInitializer()->getOutputResource().Get()
		);

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getRenderTarget().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(1, &transition);

		// Set the backbuffer.
		CD3DX12_RESOURCE_BARRIER trans2 = CD3DX12_RESOURCE_BARRIER::Transition(
			rtxManager->getInitializer()->getRenderTarget().Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		rtxManager->getInitializer()->getCommandList()->ResourceBarrier(
			1, &trans2
		);

		// Close command list
		rtxManager->getInitializer()->getCommandList()->Close();


		return 0;
	}

	void RTX_PathTracer::setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager)
	{
		rtxManager = _rtxManager;
	}
}
