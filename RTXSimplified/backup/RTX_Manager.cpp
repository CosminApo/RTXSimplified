#include "RTX_Manager.h"
#include <iostream>
namespace RTXSimplified
{
	std::shared_ptr<RTX_Manager> RTX_Manager::initialize(std::string _rayGenShader,	std::string _missShader, std::string _hitShader)
	{
		// Initialize class structure
		std::shared_ptr<RTX_Manager> rtn = std::make_shared<RTX_Manager>();
		rtn->self = rtn;
		rtn->initializer = std::make_shared<RTX_Initializer>();
		rtn->initializer->setRTXManager(rtn);
		rtn->bvhManager = std::make_shared<RTX_BVHmanager>();
		rtn->bvhManager->setRTXManager(rtn);
		rtn->pathTracer = std::make_shared<RTX_PathTracer>();
		rtn->pathTracer->setRTXManager(rtn);

		// Store values locally
		hitShader = _hitShader;
		rayGenShader = _rayGenShader;
		missShader = _missShader;


		// Check RTX Support
		rtn->initializer->checkRTXSupport();
		if (rtn->initializer->getRTXsupported())
		{
			std::cout << "Yes it is." << std::endl;
		}

		// Create AS
		rtn->bvhManager->createAccelerationStructure();

		// Close command list for now
		rtn->initializer->getCommandList()->Close();

		// Initialize pipelioe
		rtn->initializer->createRaytracingPipeline();
	
		

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
	std::shared_ptr<RTX_BVHmanager> RTX_Manager::getBVHManager()
	{
		return bvhManager;
	}
	std::shared_ptr<RTX_Initializer> RTX_Manager::getInitializer()
	{
		return initializer;
	}
	std::string RTX_Manager::getRayGenShader()
	{
		return rayGenShader;
	}
	std::string RTX_Manager::getMissShader()
	{
		return missShader;
	}
	std::string RTX_Manager::getHitShader()
	{
		return hitShader;
	}
	HWND RTX_Manager::getHWND()
	{
		return hwnd;
	}
	int RTX_Manager::getWidth()
	{
		return width;
	}
	int RTX_Manager::getHeight()
	{
		return height;
	}
	std::vector<Model> RTX_Manager::getModels()
	{
		return models;
	}
	void RTX_Manager::setWidth(int _value)
	{
		width = _value;
	}
	void RTX_Manager::setHeight(int _value)
	{
		height = _value;
	}
	Model::Model(ComPtr<ID3D12Resource> _buffer, UINT _verticesAmount)
		: buffer(_buffer), verticesAmount(_verticesAmount)
	{
	}
}