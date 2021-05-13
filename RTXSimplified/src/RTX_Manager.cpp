#include "RTX_Manager.h"
#include <iostream>
namespace RTXSimplified
{
	std::shared_ptr<RTX_Manager> RTX_Manager::initialize(int _width,
		int _height, HWND _hwnd, std::string _mainShader, std::string _rayGenShader,	std::string _missShader, std::string _hitShader)
	{
		
		// Initialize class structure
		std::shared_ptr<RTX_Manager> rtn = std::make_shared<RTX_Manager>();
		rtn->self = rtn;
		rtn->initializer = std::make_shared<RTX_Initializer>();
		rtn->initializer->setRTXManager(rtn);
		rtn->initializer->setViewPortHeight(_height);
		rtn->initializer->setViewPortWidth(_width);
		rtn->bvhManager = std::make_shared<RTX_BVHmanager>();
		rtn->bvhManager->setRTXManager(rtn);
		rtn->pathTracer = std::make_shared<RTX_PathTracer>();
		rtn->pathTracer->setRTXManager(rtn);
		rtn->hwnd = _hwnd;
		rtn->height = _height;
		rtn->width = _width;
		// Store values locally
		rtn->hitShader = _hitShader;
		rtn->rayGenShader = _rayGenShader;
		rtn->missShader = _missShader;
		rtn->mainShader = _mainShader;

		// Check RTX Support
		rtn->initializer->checkRTXSupport();
		if (rtn->initializer->getRTXsupported())
		{
			std::cout << "Found compatible device." << std::endl;
		}

		//// Create AS
		//rtn->bvhManager->createAccelerationStructure();
		rtn->initializer->prepareAssetLoading();
		
		
		
		rtn->initializer->createPipeline();

		rtn->initialized = true;

		return rtn;
	}
	int RTX_Manager::addModel(Vertex _vertices[], UINT _verticesAmount)
	{
		HRESULT hr; // Error handling.

		ComPtr<ID3D12Resource> buffer;

		const UINT bufferSize = sizeof(_vertices);

		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		// Create the resource.
		hr = initializer->getRTXDevice()->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&bufferResource,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer));
		RTX_Exception::handleError(&hr, "Error allocating space for the model.");

		// Copy the triangle data to the vertex buffer.
		UINT8* vertexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // No need to read the data again.
		hr = buffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
		RTX_Exception::handleError(&hr, "Error uploading model data.");
		memcpy(vertexDataBegin, _vertices, sizeof(_vertices));
		buffer->Unmap(0, nullptr);

		// Add the model to the list
		Model model(buffer, _verticesAmount);
		models.push_back(model);

		

		return 0;
	}
	int RTX_Manager::waitForPreviousFrame()
	{
		HRESULT hr;
		// Signal and increment the fence value.
		const UINT64 fence = initializer->getFenceValue();
		hr = initializer->getCommandQueue()->Signal(initializer->getFence().Get(), fence);
		RTX_Exception::handleError(&hr, "Error signaling fence. ");
		initializer->setFenceValue(initializer->getFenceValue() + 1);

		// Wait until the previous frame is finished.
		if (initializer->getFence()->GetCompletedValue() < fence)
		{
			hr = initializer->getFence()->SetEventOnCompletion(fence, initializer->getFenceEvent());
			RTX_Exception::handleError(&hr, "Error completing frame. ");

			WaitForSingleObject(initializer->getFenceEvent(), INFINITE);
		}
		initializer->setFrameIndex(initializer->getSwapChain()->GetCurrentBackBufferIndex());

		return 0;
	}
	void RTX_Manager::onRender()
	{
		HRESULT hr;
		pathTracer->populateCommandList();
		ID3D12CommandList* commandLists[] = { initializer->getCommandList().Get() };
		initializer->getCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);

		// Present the frame.
		hr = initializer->getSwapChain()->Present(1, 0);
		RTX_Exception::handleError(&hr, "Error presenting frame. ");

		waitForPreviousFrame();
	}
	void RTX_Manager::onUpdate()
	{
		initializer->updateCameraBuffer();
	}
	int RTX_Manager::addSampleModels()
	{
		HRESULT hr; // Error handling.

				/*ADD TRIANGLE MODEL*/
		RTXSimplified::Vertex sample[] =
		{
			{{0.0f, 0.25f * 1.f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
			{{0.25f, -0.25f * 1.f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
			{{-0.25f, -0.25f * 1.f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}
		};

		ComPtr<ID3D12Resource> buffer;

		const UINT bufferSize = sizeof(sample);

		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		// Create the resource.
		hr = initializer->getRTXDevice()->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&bufferResource,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer));
		RTX_Exception::handleError(&hr, "Error allocating space for the model.");

		// Copy the triangle data to the vertex buffer.
		UINT8* vertexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // No need to read the data again.
		hr = buffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
		RTX_Exception::handleError(&hr, "Error uploading model data.");
		memcpy(vertexDataBegin, sample, sizeof(sample));
		buffer->Unmap(0, nullptr);

		// Add the model to the list
		Model model(buffer, 3);
		models.push_back(model);


		/*ADD PLANE MODEL*/

		ComPtr<ID3D12Resource> buffer2;
		Vertex planeVertices[] = {
		{{-1.5f, -.8f, 01.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}, //0 
		{{-1.5f, -.8f, -1.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // 1 
		{{01.5f, -.8f, 01.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // 2 
		{{01.5f, -.8f, 01.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // 2
		{{-1.5f, -.8f, -1.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // 1 
		{{01.5f, -.8f, -1.5f}, {1.0f, 1.0f, 1.0f, 1.0f}} // 4 
		};
		const UINT planeBufferSize = sizeof(planeVertices);
		CD3DX12_HEAP_PROPERTIES heapProperty2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource2 = CD3DX12_RESOURCE_DESC::Buffer(planeBufferSize);
		hr = initializer->getRTXDevice()->CreateCommittedResource(
			&heapProperty2, D3D12_HEAP_FLAG_NONE, &bufferResource2, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&buffer2));
		RTX_Exception::handleError(&hr, "Error allocating space for the model.");

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange2(0, 0); // We do not intend to read from this resource on the CPU. 
		hr = buffer2->Map(0, &readRange2, reinterpret_cast<void**>(&pVertexDataBegin));
		RTX_Exception::handleError(&hr, "Error uploading model data.");
		memcpy(pVertexDataBegin, planeVertices, sizeof(planeVertices));

		buffer2->Unmap(0, nullptr);

		// Add the model to the list
		Model model2(buffer2, 6);
		models.push_back(model2);


		return 0;
		return 0;
	}
	int RTX_Manager::enableShadows(std::string _shader)
	{
		self.lock()->shadowsEnabled = true;
		self.lock()->shadowShader = _shader;
		return 0;
	}
	bool RTX_Manager::getInitialized()
	{
		return initialized;
	}
	std::shared_ptr<RTX_BVHmanager> RTX_Manager::getBVHManager()
	{
		return self.lock()->bvhManager;
	}
	std::shared_ptr<RTX_PathTracer> RTX_Manager::getPathTracer()
	{
		return pathTracer;
	}
	std::shared_ptr<RTX_Initializer> RTX_Manager::getInitializer()
	{
		return self.lock()->initializer;
	}
	std::string RTX_Manager::getShadowShader()
	{
		return shadowShader;
	}
	std::string RTX_Manager::getRayGenShader()
	{
		return self.lock()->rayGenShader;
	}
	std::string RTX_Manager::getMissShader()
	{
		return self.lock()->missShader;
	}
	std::string RTX_Manager::getHitShader()
	{
		return self.lock()->hitShader;
	}
	std::string RTX_Manager::getMainShader()
	{
		return self.lock()->mainShader;
	}
	HWND RTX_Manager::getHWND()
	{
		return self.lock()->hwnd;
	}
	int RTX_Manager::getWidth()
	{
		return self.lock()->width;
	}
	int RTX_Manager::getHeight()
	{
		return self.lock()->height;
	}
	std::vector<Model> RTX_Manager::getModels()
	{
		return self.lock()->models;
	}
	bool RTX_Manager::getShadowsEnabled()
	{
		return shadowsEnabled;
	}
	void RTX_Manager::setWidth(int _value)
	{
		self.lock()->width = _value;
	}
	void RTX_Manager::setHeight(int _value)
	{
		self.lock()->height = _value;
	}
	void RTX_Manager::setHWND(HWND _hwnd)
	{
		self.lock()->hwnd = _hwnd;
	}
	Model::Model(ComPtr<ID3D12Resource> _buffer, UINT _verticesAmount)
		: buffer(_buffer), verticesAmount(_verticesAmount)
	{
	}
}