#ifndef  RTX_PIPELINE_H
#define RTX_PIPELINE_H

#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <dxcapi.h> //DXR
#include <wrl.h> // Windows Runtime Library -> ComPtr
#include <memory> // Smart pointers
#include "RTX_Exception.h" // Error handling
#include <string> // strings
#include <vector> // vectors
#include <unordered_set> // shader export list
#include <tuple> // root sig
#include <xhash> // shader export list
#include "RTX_SBTGenerator.h" // SBTs 


using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces


namespace RTXSimplified
{
	/*FORWARD DECLARES*/
	class RTX_Manager;


	/**
	* \brief The class responsible for creating the raytracing pipeline. 
	* 
	* This creates a default raytracing pipeline containing 
	* the compiled version of the shaders provided and their signatures.
	*/
	class RTX_Pipeline
	{
	private:

		struct Library
		{
			Library(IDxcBlob* _lib, const std::vector<std::wstring>& _symbols); ///< Generates a new library.
			Library(const Library& _source);	///< Constructor used to avoid scope errors.

			IDxcBlob* lib;	///< Stores the library.
			const std::vector<std::wstring>& symbols; ///< Stores the exported symbols.
			std::vector<D3D12_EXPORT_DESC> exports;	///< Storest the exports.
			D3D12_DXIL_LIBRARY_DESC libDesc; ///< Stores the library descriptors.
		}; ///< Struct used to store the libraries.

		struct RootSignatureGenerator
		{
			int addHeapRangesParameter(const std::vector<D3D12_DESCRIPTOR_RANGE>& _ranges); ///< Adds a set heap range descriptors as param.
			int addHeapRangesParameter(std::vector<std::tuple<
				UINT, // base shader register
				UINT, // number of descriptors
				UINT, // register space
				D3D12_DESCRIPTOR_RANGE_TYPE, // range type
				UINT  // offset from start
				>> _ranges); ///< Adds a set of heap ranges as a param.
			int addRootParameter(
				D3D12_ROOT_PARAMETER_TYPE _type, // type
				UINT _shaderRegister = 0,		 // base shader register
				UINT _registerSpace = 0,		 // register spce
				UINT _numRootConstants = 1		 // constants
			); ///< Add a root parameter to the shader.
			ID3D12RootSignature* generate(ID3D12Device* _device, bool _local); ///< Generates the root signature.
		private:
			std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> ranges; ///< Heap range descriptors.
			std::vector<D3D12_ROOT_PARAMETER> parameters; ///< Root parameters descriptors.
			std::vector<UINT> rangeLocations;	///< Range array in ranges.

			enum
			{
				RSC_BASE_SHADER_REGISTER = 0,
				RSC_NUM_DESCRIPTORS = 1,
				RSC_REGISTER_SPACE = 2,
				RSC_RANGE_TYPE = 3,
				RSC_OFFSET_IN_DESCRIPTORS_FROM_TABLE_START = 4
			}; ///< Enum used for adding heap ranges params.
		}; ///< Struct used to generate root signatures for the shader libraries.

		struct HitGroup
		{

			HitGroup(std::wstring _hitGroupName, std::wstring _closestHit, std::wstring _anyHit = L"", std::wstring _intersection = L"");
			HitGroup(const HitGroup& _source);

			std::wstring hitGroupName;		///< Stores the name of the hit group.
			std::wstring closestHitSymbol;	///< Stores the symbol for the closest ray hit.
			std::wstring anyHitSymbol;		///< Stores the symbol for any ray hit.
			std::wstring intersectionSymbol;///< Stores the symbol for the ray intersection.
			D3D12_HIT_GROUP_DESC desc = {};	///< Descriptor for hit groups.
		}; ///< Struct used to store hitgroups.

		struct RootSignatureAssociation
		{
			RootSignatureAssociation(ID3D12RootSignature* _rootSig, const std::vector<std::wstring>& _symbols);
			RootSignatureAssociation(const RootSignatureAssociation& _source);

			ID3D12RootSignature* rootSignature;		
			ID3D12RootSignature* rootSignaturePointer;
			std::vector<std::wstring> symbols;
			std::vector<LPCWSTR> symbolPointers;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
		}; ///< Struct for associating shaders with root signatures.
		
		UINT maxAttributeSizeInBytes = 0; ///< Max space for attributes.
		UINT maxRecursionDepth = 0; ///< Max levels of recursion.
		UINT maxPayLoadSizeInBytes = 0;	///< Max size of each payload.
		std::vector<Library> libraries = {}; ///< Stores all the libraries.
		std::vector<HitGroup> hitgroups = {}; ///< Stores all the hitgroups.
		std::vector<RootSignatureAssociation> rootSigAssociations = {}; ///< Stores all the RootSignatureAssociations.
		IDxcCompiler* compiler = nullptr;	 ///< Compiler used to create a shader library.
		IDxcLibrary* library = nullptr;		 ///< Library used to create a shader library.
		IDxcIncludeHandler* includeHandler;	 ///< Handler used to create a shader library.
		ComPtr<IDxcBlob> rayGenLibrary; ///< Stores the ray generation shader library.
		ComPtr<IDxcBlob> hitLibrary;	///< Stores the hit shader library.
		ComPtr<IDxcBlob> missLibrary;	///< Stores the miss shader library.
		ID3D12RootSignature* defaultGlobalSignature; ///< Stores the default empty global signature.
		ID3D12RootSignature* defaultLocalSignature; ///< Stores the default empty local signature.
		std::shared_ptr<RTX_Manager> rtxManager; ///< Stores a reference to the RTX manager class.
		ComPtr<ID3D12RootSignature> rayGenSignature; ///< Signature for the ray generation shader.
		ComPtr<ID3D12RootSignature> hitSignature; ///< Signature for the hit shader.
		ComPtr<ID3D12RootSignature> missSignature; ///< Signature for the miss shader.
		ComPtr<ID3D12DescriptorHeap> srvUavHeap; ///< Stores the descriptor for the srv/uav/cbv heap.
		RTX_SBTGenerator SBTGenerator; ///< Helps generate SBTs.
		ComPtr<ID3D12Resource> sbtStorage; ///< Stores the SBT.

		IDxcBlob* compileShaderLib(std::string _shaderFile); ///< Compiles the shader library.
		std::wstring stringToWstring(std::string _s); ///< Converts an std::string to a std::wstring.
		void addLibrary(IDxcBlob* _library, const std::vector<std::wstring>& _symbols); ///< Adds a library to the pipeline. 
		ComPtr<ID3D12RootSignature> createRayGenSignature(); ///< Creates the signature for the ray generation shader.
		ComPtr<ID3D12RootSignature> createMissSignature(); ///< Creates the signature for the ray miss shader.
		ComPtr<ID3D12RootSignature> createHitSignature(); ///< Creates the signature for the ray hit shader.
		int buildShaderExportList(std::vector<std::wstring>& _exportedSymbols); ///< Creats the shader export symbol list.
		int addRootSignatureAssociation(
			ID3D12RootSignature* _rootSig, ///< Signature of the shader.
			const std::vector<std::wstring>& _symbols ///< Symbols associated
		); ///< Adds a new root signature association.
		ID3D12DescriptorHeap* createDescriptorHeap(
			uint32_t _count, ///< Number of descriptors.
			D3D12_DESCRIPTOR_HEAP_TYPE _type, ///< Type of descriptors.
			bool _shaderVisible ///< Is the shader visibe.
		); ///< Creates a new shader descriptor heap.
		ID3D12Resource* createBuffer(ID3D12Device* _device, uint64_t _size, D3D12_RESOURCE_FLAGS _flags,
			D3D12_RESOURCE_STATES _initState, const D3D12_HEAP_PROPERTIES& _heapProps); ///< Creates a buffer based on the device properties, data properties and ctrl flags.

	public:
		int addHitGroup(
			const std::wstring& _hitGroupName, ///< Name of the hit group.
			const std::wstring& _closestHitSymbol, ///< Symbol for the closest hit.
			const std::wstring& _anyHitSymbol = L"", ///< Symbol for any hit.
			const std::wstring& _intersectionSymbol = L"" ///< Symbol for hte intersection.
		); ///< Adds a new hit group.
		int addRootSignatureAssociation(); ///< Adds the root signature association for ray gen, hit and miss shaders.

		int createDefaultRootSignature(); ///< Creates default empty global and local root signatures.
		int createShaderLibraries();	  ///< Creates the shader libraries.
		int addLibraries(); ///< Adds the raygen,  hit and miss libaries.
		int createShaderSignatures(); ///< Creates the shader signatures.
		ID3D12StateObject* generate(); ///< Generates the pipeline.
		int createShaderResourceHeap(); ///< Creates shader resource heap.
		int createShaderBindingTable(); ///< Creates shader binding table.

		/*SETTERS*/
		void setRTXManager(std::shared_ptr<RTX_Manager> _rtxManager);
		void setMaxPayloadSize(UINT _value);
		void setMaxAttributeSize(UINT _value);
		void setMaxRecusionDepth(UINT _value);
	};
}



#endif // ! RTX_PIPELINE_H
