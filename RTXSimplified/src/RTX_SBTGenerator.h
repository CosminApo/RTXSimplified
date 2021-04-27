#ifndef RTX_SBTGENERATOR_H
#define RTX_SBTGENERATOR_H

#include "d3dx12.h" // DXR
#include <d3d12.h> // DXR
#include <dxgi1_4.h> // DXR
#include <dxcapi.h> //DXR
#include <wrl.h> // Windows Runtime Library -> ComPtr
#include <memory> // Smart pointers
#include "RTX_Exception.h" // Error handling
#include <string> // strings
#include <vector> // vectors

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

namespace RTXSimplified
{

	struct SBTEntry
	{
		SBTEntry(std::wstring _entrypoint, std::vector<void*> _inputData); ///< Create a new SBT entry.

		const std::wstring entryPoint;
		const std::vector<void*> inputData;

	}; ///< Helper struct for SBT entries.

	/**
	* \brief The class responsible for creating the shader binding table.
	*
	* The SBT is where the shader resources are bound to shaders.
	* It contains a series of shaders IDs and their resource pointers.
	*/
	class RTX_SBTGenerator
	{
	private:
		std::vector<SBTEntry> rayGen; ///< Stores the SBT entry for the ray generation shader.
		std::vector<SBTEntry> miss; ///< Stores the SBT entry for the miss shader.
		std::vector<SBTEntry> hit; ///< Stores the SBT entry for the hit shader.
		uint32_t rayGenEntrySize; ///< Stores the size of one SBT entry for the ray generation shader.
		uint32_t missEntrySize; ///< Stores the size of one SBT entry for the miss shader.
		uint32_t hitEntrySize; ///< Stores the size of one SBT entry for the hit shader.
		uint32_t progIDSize; ///< Stores the size of the program identifier.
		
		uint32_t calculateEntrySize(const std::vector<SBTEntry>& _entries); ///< Calculate the total SBT entries size.
		uint32_t copyShaderData(
			ID3D12StateObjectProperties* _RTPipeline, ///< Pipeline the shaders are on.
			uint8_t* _outputData, ///< Where data is stored.
			const std::vector<SBTEntry>& _shaders, ///< Shaders to copy.
			uint32_t _entrySize ///< Size of data to copy.
		); ///< Copies the shader id in output data.

	public:

		void reset(); ///< Resets everything.
		void addRayGenerationProgram(
			const std::wstring& _entryPoint, ///< Entry point of program.
			const std::vector<void*>& _inputData ///< List of data pointers or values.
		); ///< Adds a ray generation program by name.
		void addMissProgram(
			const std::wstring& _entryPoint, ///< Entry point of program.
			const std::vector<void*>& _inputData ///< List of data pointers or values.
		); ///< Adds a miss program by name.
		void addHitProgram(
			const std::wstring& _entryPoint, ///< Entry point of program.
			const std::vector<void*>& _inputData ///< List of data pointers or values.
		); ///< Adds a hit program by name.
		uint32_t computeSBTSize(); ///< Calculates the size of the SBT.
		int generate(
			ID3D12Resource* _sbtBuffer, ///< Where to store it.
			ID3D12StateObjectProperties* _raytracingPipeline ///< Pipeline programs are on.
		); ///< Generate the SBT.

		/*GETTERS*/
		uint32_t getHitGroupEntrySize();
		uint32_t getHitGroupSectionSize();
		uint32_t getMissEntrySize();
		uint32_t getMissSectionSize();
		uint32_t getRaygenEntrySize();
		uint32_t getRaygenSectionSize();

	};
}

#endif // !RTX_SBTGENERATOR_H
