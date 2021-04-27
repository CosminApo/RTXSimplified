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
	void RTX_Manager::setWidth(int _value)
	{
		width = _value;
	}
	void RTX_Manager::setHeight(int _value)
	{
		height = _value;
	}
}