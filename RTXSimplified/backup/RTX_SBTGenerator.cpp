#include "RTX_SBTGenerator.h"

namespace RTXSimplified
{
	SBTEntry::SBTEntry(std::wstring _entryPoint, std::vector<void*> _inputData)
		: entryPoint(std::move(_entryPoint)), inputData(std::move(_inputData))
	{
	}
	uint32_t RTX_SBTGenerator::calculateEntrySize(const std::vector<SBTEntry>& _entries)
	{
		size_t maximumArgs = 0; // Get the maximum number of arguments in a SBTentry.
		for (const auto& shader : _entries)
		{
			maximumArgs = max(maximumArgs, shader.inputData.size());
		}

		// Entry = program id + parameters (each 8 bytes)
		uint32_t entrySize = progIDSize + 8 * static_cast<uint32_t>(maximumArgs);
		// Allign the SBT to 16 bytes.
		entrySize = ROUND_UP(entrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

		return entrySize;
	}
	uint32_t RTX_SBTGenerator::copyShaderData(ID3D12StateObjectProperties* _RTPipeline, uint8_t* _outputData, const std::vector<SBTEntry>& _shaders, uint32_t _entrySize)
	{
		uint8_t* data = _outputData;
		for (const auto& shader : _shaders)
		{
			void* id = _RTPipeline->GetShaderIdentifier(shader.entryPoint.c_str()); // Get the ID
			if (!id) // Error check 
			{
				RTX_Exception::handleError("Unknown shader identifier.", true);
			}
			memcpy(data, id, progIDSize); // Copy the shader ID.
			memcpy(data + progIDSize, shader.inputData.data(), shader.inputData.size() * 8); // Copy the resource pointers / values.
			data += _entrySize; // Offset
		}

		// Return number of bytes written.
		return static_cast<uint32_t>(_shaders.size()) * _entrySize;;
	}
	void RTX_SBTGenerator::reset()
	{
		rayGen.clear();
		miss.clear();
		hit.clear();

		rayGenEntrySize = 0;
		missEntrySize = 0;
		hitEntrySize = 0;
		progIDSize = 0;
	}
	void RTX_SBTGenerator::addRayGenerationProgram(const std::wstring& _entryPoint, const std::vector<void*>& _inputData)
	{
		rayGen.emplace_back(SBTEntry(_entryPoint, _inputData));
	}
	void RTX_SBTGenerator::addMissProgram(const std::wstring& _entryPoint, const std::vector<void*>& _inputData)
	{
		miss.emplace_back(SBTEntry(_entryPoint, _inputData));
	}
	void RTX_SBTGenerator::addHitProgram(const std::wstring& _entryPoint, const std::vector<void*>& _inputData)
	{
		hit.emplace_back(SBTEntry(_entryPoint, _inputData));
	}
	uint32_t RTX_SBTGenerator::computeSBTSize()
	{
		progIDSize = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT; // Set the prog id size.

		// Calculate the sizes of the programs.
		rayGenEntrySize = calculateEntrySize(rayGen);
		missEntrySize = calculateEntrySize(miss);
		hitEntrySize = calculateEntrySize(hit);

		// Add all the sizes together and allign them to 256 bytes.
		uint32_t rtn = ROUND_UP(
			rayGenEntrySize * static_cast<UINT>(rayGen.size()) +
			missEntrySize * static_cast<UINT>(miss.size()) +
			hitEntrySize * static_cast<UINT>(hit.size()),
			256
		);

		return rtn;
	}
	int RTX_SBTGenerator::generate(ID3D12Resource* _sbtBuffer, ID3D12StateObjectProperties* _raytracingPipeline)
	{
		HRESULT hr; // Error handling

		// Map the sbt
		uint8_t* data;
		hr = _sbtBuffer->Map(0, nullptr, reinterpret_cast<void**>(&data));
		RTX_Exception::handleError(&hr, "Error maping the SBT.");

		// Copy the shader indentifiers and their resources.
		uint32_t offset = 0;
		offset = copyShaderData(_raytracingPipeline, data, rayGen, rayGenEntrySize);
		data += offset;
		offset = copyShaderData(_raytracingPipeline, data, miss, missEntrySize);
		data += offset;
		offset = copyShaderData(_raytracingPipeline, data, hit, hitEntrySize);
		data += offset;

		// Unmap the sbt
		_sbtBuffer->Unmap(0, nullptr);

		return 0;
	}
	UINT RTX_SBTGenerator::getHitGroupEntrySize()
	{
		return hitEntrySize;
	}
	uint32_t RTX_SBTGenerator::getHitGroupSectionSize()
	{
		return hitEntrySize * static_cast<UINT>(hit.size());
	}
	uint32_t RTX_SBTGenerator::getMissEntrySize()
	{
		return missEntrySize;
	}
	uint32_t RTX_SBTGenerator::getMissSectionSize()
	{
		return missEntrySize * static_cast<UINT>(miss.size());
	}
	uint32_t RTX_SBTGenerator::getRaygenEntrySize()
	{
		return rayGenEntrySize;
	}
	uint32_t RTX_SBTGenerator::getRaygenSectionSize()
	{
		return rayGenEntrySize * static_cast<UINT>(rayGen.size());
	}
}