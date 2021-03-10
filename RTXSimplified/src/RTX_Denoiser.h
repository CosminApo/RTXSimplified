#ifndef RTX_DENOISER_H
#define RTX_DENOISER_H

namespace RTXSimplified
{
	/**
	*	\brief The class responsible for denoising the final output.
	*	
	*	Uses SVGF (Spatiotemporal Variance-Guided Filtering) to denoise the raytraced ouput.
	*/
	class RTX_Denoiser
	{
	public:
		int denoise(); 
	};
}
#endif // !RTX_DENOISER_H