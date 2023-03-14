#include "KeystrokesRecorder.h"

#include <iostream>

int main()
{
	KeystrokesRecorder* instance = KeystrokesRecorder::getInstance();
	
	instance->startRecording();
	
	std::cout << "Recording key strokes to " << instance->getLogFileName() << std::endl << std::endl;

	system("pause");

	instance->stopRecording();

	return 0;
}