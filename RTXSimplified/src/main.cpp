#include "RTX_Manager.h"
#include <iostream>
#include <memory>

int main()
{
	std::cout << "hi" << std::endl;
	std::shared_ptr<RTXSimplified::RTX_Manager> myManager = myManager->initialize("Raygen.hlsl", "Miss.hlsl", "Hit.hlsl");

	return 0;
}