#ifndef RTX_MANAGER_H
#define RTX_MANAGER_H

/*Classes accessible from core, managed by this*/
#include "RTX_BVHmanager.h"
#include "RTX_PathTracer.h"
#include "RTX_Initializer.h"

#include <memory> // smart pointers
#include "d3dx12.h" // DirectX12 api
#include <d3d12.h>
#include <dxcapi.h> // DXR
#include <dxgi1_2.h> // DXR
#include <windows.foundation.h> //Windows for WRL
#include <wrl.h> // Windows Runtime Library -> ComPtr
#include <vector> // std::vector
#include <DirectXMath.h> // XMMATRIX -> 4*4 matrix aligned on a 16-byte boundary 
					     //				that maps to four hardware vector registers

using Microsoft::WRL::ComPtr; ///< Smart pointer for interfaces

namespace RTXSimplified
{
	/**
	*  \brief The class that will manage all the other classes.
	*	Acts as a link between them and Core as well.
	*/
	struct Model
	{
		Model(ComPtr<ID3D12Resource> _buffer, UINT _verticesAmount); ///< Constructor.

		ComPtr<ID3D12Resource> buffer; ///< Vertices describing the geometry.
		UINT verticesAmount; ///< Number of vertices.
	}; ///< Struct to help store the models to render.
	class RTX_Manager
	{

	private:
		bool initialized; ///< Flag that checks if everything has been initialized.
		bool rtxSupported; ///< Flag that checks if RTX is supported.
		std::string rayGenShader; ///< Stores the path to the ray generation shader.
		std::string missShader;	  ///< Stores the path to the miss shader.
		std::string hitShader;	  ///< Stores the path to the hit shader.
		std::string mainShader; ///< Stores the path to the main shader.
		std::string shadowShader; ///< Stores the path to the shadow shader.
		int width, height; ///< Stores information about the output window size.
		bool shadowsEnabled; ///< Flag that checks whether real time shadows are enabled or not.

		std::shared_ptr<RTX_BVHmanager> bvhManager; ///< Class responsible for managing BVH.
		std::shared_ptr<RTX_PathTracer> pathTracer; ///< Class responsible for tracing rays path.
		std::shared_ptr<RTX_Initializer> initializer;  ///< Class responsible for initializing the library.
		HWND hwnd; ///< Handle to the output window.

		std::weak_ptr<RTX_Manager> self; ///< Smart "this" pointer.
		std::vector<Model> models; ///< Models to render.

	public:

		std::shared_ptr<RTX_Manager> initialize(
			int _width,				   ///< Width of window.
			int _height,			   ///< Height of window.
			HWND _hwnd,				   ///< Window handle.
			std::string _mainShader,   ///< Path to the main shader.
			std::string _rayGenShader, ///< Path to the ray generation shader.
			std::string _missShader,   ///< Path to the no hit shader.
			std::string _hitShader	   ///< Path to the closest hit shader. 
		); ///< Initializes the library, uses the initializer class.

		int addModel(Vertex _vertices[], UINT _verticesAmount); ///< Adds a model to be rendered.
		int waitForPreviousFrame(); ///< Wait for frame to end.
		void onRender(); ///< Handles on render events.
		void onUpdate(); ///< Handles on update events.
		int addSampleModels(); ///< Adds sample models.
		int enableShadows(std::string _shader); ///< Enables real time shadows.
		/*GETTERS*/
		bool getInitialized();
		std::shared_ptr<RTX_BVHmanager> getBVHManager();
		std::shared_ptr<RTX_PathTracer> getPathTracer();
		std::shared_ptr<RTX_Initializer> getInitializer();
		std::string getShadowShader();
		std::string getRayGenShader();
		std::string getMissShader();
		std::string getHitShader();
		std::string getMainShader();
		HWND getHWND();
		int getWidth();
		int getHeight();
		std::vector<Model> getModels();
		bool getShadowsEnabled();
		/*SETTERS*/
		void setWidth(int _value);
		void setHeight(int _value);
		void setHWND(HWND _hwnd);
	};
}


#endif // !RTX_MANAGER_H