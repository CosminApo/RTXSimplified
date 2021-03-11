#include "RTX_Initializer.h"
#include "RTX_Exception.h"



namespace RTXSimplified
{
	int RTX_Initializer::createDevice()
	{
		HRESULT hr; // Error handling
		UINT dxgiFactoryFlags = 0; // Stores flags for creating factory
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG; // Enable additional debug layers
		ComPtr<IDXGIFactory4> factory; // Interface for creating DXGI objects
		ComPtr<IDXGIAdapter1> hardwareAdapter; // Represents a GPU

		hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)); // Create the factory
		RTX_Exception::handleError(&hr, "Failed to create factory"); // Error handling
		
		getAdapter(factory.Get(), &hardwareAdapter); // Get the GPU

		hr = D3D12CreateDevice(			// Creates the device:
			hardwareAdapter.Get(),		// GPU to use
			D3D_FEATURE_LEVEL_12_1,		// Minimum feature level
			IID_PPV_ARGS(&rtxDevice)	// Where to store output
		);
		RTX_Exception::handleError(&hr, "Failed to create device"); // Error handling

		return 0;
	}

	int RTX_Initializer::getAdapter(IDXGIFactory2* _factory, IDXGIAdapter1** _adapter)
	{
		ComPtr<IDXGIAdapter1> adapter; // Store the adapter temp
		*_adapter = nullptr; // Set the other adapter to null if nothing found

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) // Loop through all the adapters found
		{
			DXGI_ADAPTER_DESC1 desc; // Get the description for each adaptor
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // Don't select the Basic Render Driver adapter.
			{
				continue;
			}
			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*_adapter = adapter.Detach(); // Detach the adapter for cleanup
		return 0;
	}

	int RTX_Initializer::checkRTXSupport()
	{
		createDevice();
		HRESULT hr; // Error handling
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 checker = {}; // query for checking RT, render passes and shader resource view support
		hr = rtxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &checker, sizeof(checker));
		RTX_Exception::handleError(&hr, "Failed to query device"); // Error handling


		if (checker.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		{
			// HARDWARE NOT SUPPORTED
		}
		else
		{
			rtxSupported = true;
		}
		return 0;
	}
	bool RTX_Initializer::getRTXsupported()
	{
		return rtxSupported;
	}
}