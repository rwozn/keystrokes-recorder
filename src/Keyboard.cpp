#include "Keyboard.h"

#define FREE_CONVERSION_TABLE(N) \
if(vkToWchars##N)                \
{                                \
   free(vkToWchars##N);          \
                                 \
	vkToWchars##N = NULL;        \
}

void Keyboard::freeAllocatedMemory()
{
	if(deadKeysInformation)
	{
		free(deadKeysInformation);

		deadKeysInformation = NULL;
	}

	if(ligatureInformation)
	{
		free(ligatureInformation);

		ligatureInformation = NULL;
	}

	if(modifierKeysInformation)
	{
		free(modifierKeysInformation->pVkToBit);

		free(modifierKeysInformation);

		modifierKeysInformation = NULL;
	}

	FREE_CONVERSION_TABLE(1)
	FREE_CONVERSION_TABLE(2)
	FREE_CONVERSION_TABLE(3)
	FREE_CONVERSION_TABLE(4)
	FREE_CONVERSION_TABLE(5)
	FREE_CONVERSION_TABLE(6)
	FREE_CONVERSION_TABLE(7)
	FREE_CONVERSION_TABLE(8)
	FREE_CONVERSION_TABLE(9)
	FREE_CONVERSION_TABLE(10)
}

bool getSystemDirectory(WCHAR* systemDirectory)
{
	static bool once = false;

	static BOOL runningOnWOW64 = FALSE;

	if(!once)
	{
		IsWow64Process(GetCurrentProcess(), &runningOnWOW64);

		once = true;
	}

	if(runningOnWOW64)
		return GetSystemWow64Directory(systemDirectory, MAX_PATH);

	return GetSystemDirectory(systemDirectory, MAX_PATH);
}

#define KEYBOARD_LAYOUTS_REGISTRY_PATH  L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\"

// Keyboard layouts registry path length; = 51
#define KLRP_LENGTH (sizeof KEYBOARD_LAYOUTS_REGISTRY_PATH / sizeof *KEYBOARD_LAYOUTS_REGISTRY_PATH)

#define KEYBOARD_LAYOUT_REGISTRY_PATH_LENGTH (KLRP_LENGTH + KL_NAMELENGTH)

bool Keyboard::getKeyboardLayoutFilePath(WCHAR* keyboardLayoutFilePath)
{
	WCHAR keyboardLayoutRegistryPath[KEYBOARD_LAYOUT_REGISTRY_PATH_LENGTH] = {};

	swprintf_s(keyboardLayoutRegistryPath, KEYBOARD_LAYOUT_REGISTRY_PATH_LENGTH, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s", loadedKeyboardLayoutName);

	HKEY registryKeyHandle = NULL;

	// phkResult - A pointer to a variable that receives a handle to the opened key
	// If the key is not one of the predefined registry keys, call the RegCloseKey function after you have finished using the handle.
	// We're using a predefined registry key (HKEY_LOCAL_MACHINE) so we don't call RegCloseKey on it
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyboardLayoutRegistryPath, 0, KEY_QUERY_VALUE, &registryKeyHandle) != ERROR_SUCCESS)
		return false;

	DWORD bufferSize = MAX_PATH;

	DWORD registryValueType = REG_SZ;

	WCHAR keyboardLayoutFileName[MAX_PATH] = {};

	if(RegQueryValueEx(registryKeyHandle, L"Layout File", NULL, &registryValueType, (BYTE*)keyboardLayoutFileName, &bufferSize) != ERROR_SUCCESS)
		return false;

	WCHAR systemDirectory[MAX_PATH] = {};

	if(!getSystemDirectory(systemDirectory))
		return false;

	swprintf_s(keyboardLayoutFilePath, MAX_PATH, L"%s\\%s", systemDirectory, keyboardLayoutFileName);

	return true;
}

#define INITIALIZE_CONVERSION_TABLE(N)                                                  \
{                                                                                       \
   DWORD j = 1;                                                                         \
                                                                                        \
   while(((PVK_TO_WCHARS##N)vkToWcharTable[i].pVkToWchars)[j - 1].VirtualKey)           \
      j++;                                                                              \
                                                                                        \
   vkToWchars##N = (PVK_TO_WCHARS##N)malloc(j * vkToWcharTable[i].cbSize);              \
                                                                                        \
	memcpy(vkToWchars##N, vkToWcharTable[i].pVkToWchars, j * vkToWcharTable[i].cbSize); \
}

void Keyboard::initializeConversionTables(PVK_TO_WCHAR_TABLE vkToWcharTable)
{
	for(DWORD i = 0; vkToWcharTable[i].cbSize; i++)
	{
		if(vkToWcharTable[i].nModifications == 1)
			INITIALIZE_CONVERSION_TABLE(1)
		else if(vkToWcharTable[i].nModifications == 2)
			INITIALIZE_CONVERSION_TABLE(2)
		else if(vkToWcharTable[i].nModifications == 3)
			INITIALIZE_CONVERSION_TABLE(3)
		else if(vkToWcharTable[i].nModifications == 4)
			INITIALIZE_CONVERSION_TABLE(4)
		else if(vkToWcharTable[i].nModifications == 5)
			INITIALIZE_CONVERSION_TABLE(5)
		else if(vkToWcharTable[i].nModifications == 6)
			INITIALIZE_CONVERSION_TABLE(6)
		else if(vkToWcharTable[i].nModifications == 7)
			INITIALIZE_CONVERSION_TABLE(7)
		else if(vkToWcharTable[i].nModifications == 8)
			INITIALIZE_CONVERSION_TABLE(8)
		else if(vkToWcharTable[i].nModifications == 9)
			INITIALIZE_CONVERSION_TABLE(9)
		else if(vkToWcharTable[i].nModifications == 10)
			INITIALIZE_CONVERSION_TABLE(10)
	}
}

void Keyboard::lookUpVirtualKeyInConversionTables(BYTE virtualKey, DWORD modifierKeysShiftStatesFlags, WCHAR* unicodeString)
{
	if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS1>(vkToWchars1, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
		if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS2>(vkToWchars2, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
			if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS3>(vkToWchars3, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
				if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS4>(vkToWchars4, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
					if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS5>(vkToWchars5, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
						if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS6>(vkToWchars6, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
							if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS7>(vkToWchars7, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
								if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS8>(vkToWchars8, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
									if(!lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS9>(vkToWchars9, virtualKey, modifierKeysShiftStatesFlags, unicodeString))
										lookUpVirtualKeyInConversionTable<PVK_TO_WCHARS10>(vkToWchars10, virtualKey, modifierKeysShiftStatesFlags, unicodeString);
}

Keyboard::Keyboard():
	vkToWchars1(NULL),
	vkToWchars2(NULL),
	vkToWchars3(NULL),
	vkToWchars4(NULL),
	vkToWchars5(NULL),
	vkToWchars6(NULL),
	vkToWchars7(NULL),
	vkToWchars8(NULL),
	vkToWchars9(NULL),
	vkToWchars10(NULL),
	maxLigature(0),
	ligatureInformation(NULL),
	ligatureInformationEntrySize(0),
	lastDeadCharacter(0),
	deadKeysInformation(NULL),
	unicodeStringIndex(0),
	modifierKeysInformation(NULL)
{
	ZeroMemory(loadedKeyboardLayoutName, sizeof loadedKeyboardLayoutName);
}

Keyboard::~Keyboard()
{
	freeAllocatedMemory();
}

void Keyboard::intitializeDeadKeysInformation(PDEADKEY deadKey)
{
	if(!deadKey)
		return;

	// because the last one (that one that has dwBoth == 0) has also be included
	size_t i = 1;

	// i - 1 because we're starting from i = 1
	while(deadKey[i - 1].dwBoth)
		i++;
	
	const size_t size = i * sizeof *deadKey;

	memcpy(deadKeysInformation = (PDEADKEY)malloc(size), deadKey, size);
}

void Keyboard::initializeLigatureInformation(PLIGATURE1 ligature)
{
	if(!ligature)
		return;

	// because the last currentLigatureInformation was checked and it'll also be when we're handling a ligature, so the last one also has to be copied
	// i.e. the zeroed one also has to be included
	size_t i = 1;

	PLIGATURE1 currentLigature = ligature;

	while(currentLigature->VirtualKey)
	{
		currentLigature = (PLIGATURE1)((DWORD64)currentLigature + ligatureInformationEntrySize);

		i++;
	}

	const size_t size = i * ligatureInformationEntrySize;

	memcpy(ligatureInformation = (PLIGATURE1)malloc(size), ligature, size);
}

void Keyboard::initializeModifierKeysInformation(PMODIFIERS modifiers)
{
	if(!modifiers)
		return;

	// modifiers->wMaxModBits + 1 because that's the maximum number of elements in modifiers->ModNumber (wMaxModBits is the max index in ModNumber
	// so it's that + 1)
	// + sizeof modifiers->pVkToBit because in order to access the pointer we have to allocate memory for it first
	modifierKeysInformation = (PMODIFIERS)malloc(sizeof modifiers->pVkToBit + sizeof modifiers->wMaxModBits + modifiers->wMaxModBits + 1);
	
	modifierKeysInformation->wMaxModBits = modifiers->wMaxModBits;

	memcpy(modifierKeysInformation->ModNumber, modifiers->ModNumber, modifiers->wMaxModBits + 1);

	BYTE i = 1;

	//i - 1 because we're starting from i = 1
	while(modifiers->pVkToBit[i - 1].Vk)
		i++;

	const size_t size = i * sizeof*modifiers->pVkToBit;

	memcpy(modifierKeysInformation->pVkToBit = (PVK_TO_BIT)malloc(size), modifiers->pVkToBit, size);
}

HKL getForegroundThreadKeyboardLayout()
{
	return GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL));
}

#define CLEAR_STRING(STR) ZeroMemory(STR, sizeof STR)

bool Keyboard::changeKeyboardLayout()
{
	CLEAR_STRING(loadedKeyboardLayoutName);

	if(!GetKeyboardLayoutNameW(loadedKeyboardLayoutName))
		return false;

	WCHAR keyboardLayoutFilePath[MAX_PATH];

	if(!getKeyboardLayoutFilePath(keyboardLayoutFilePath))
	{
		CLEAR_STRING(loadedKeyboardLayoutName);

		return false;
	}

	HMODULE loadedKeyboardLayoutFileHandle = LoadLibrary(keyboardLayoutFilePath);

	typedef PKBDTABLES (WINAPI* KbdLayerDescriptor)();

	KbdLayerDescriptor keyboardLayerDescriptor = (KbdLayerDescriptor)GetProcAddress(loadedKeyboardLayoutFileHandle, "KbdLayerDescriptor");

	if(!keyboardLayerDescriptor)
	{
		CLEAR_STRING(loadedKeyboardLayoutName);

		FreeLibrary(loadedKeyboardLayoutFileHandle);

		return false;
	}

	PKBDTABLES keyboardTables = keyboardLayerDescriptor();

	if(!keyboardTables)
	{
		CLEAR_STRING(loadedKeyboardLayoutName);

		FreeLibrary(loadedKeyboardLayoutFileHandle);

		return false;
	}

	freeAllocatedMemory();

	lastDeadCharacter = 0;

	maxLigature = keyboardTables->nLgMax;
	ligatureInformationEntrySize = keyboardTables->cbLgEntry;
	initializeLigatureInformation(keyboardTables->pLigature);

	intitializeDeadKeysInformation(keyboardTables->pDeadKey);

	initializeConversionTables(keyboardTables->pVkToWcharTable);

	initializeModifierKeysInformation(keyboardTables->pCharModifiers);

	FreeLibrary(loadedKeyboardLayoutFileHandle);

	return true;
}

bool Keyboard::hasKeyboardLayoutChanged()
{
	if(!ActivateKeyboardLayout(getForegroundThreadKeyboardLayout(), 0))
		return true;

	WCHAR currentKeyboardLayoutName[KL_NAMELENGTH] = {};

	if(!GetKeyboardLayoutNameW(currentKeyboardLayoutName))
		return true;

	return wcscmp(currentKeyboardLayoutName, loadedKeyboardLayoutName);
}

void Keyboard::lookUpDeadCharacterOutput(WCHAR* unicodeString)
{
	for(DWORD i = 0; deadKeysInformation[i].dwBoth; i++)
	{
		WCHAR baseCharacter = (WCHAR)deadKeysInformation[i].dwBoth;

		WCHAR diacriticCharacter = (WCHAR)(deadKeysInformation[i].dwBoth >> 16);

		// if a base char has just been pressed (e.g. E) and a dead key had been pressed (e.g. ~) then we'll have Ê
		if(unicodeString[unicodeStringIndex] == baseCharacter && lastDeadCharacter == diacriticCharacter)
		{
			// if the result is a dead char then last dead char is the result
			// andd current char is null
			if(deadKeysInformation[i].uFlags & DKF_DEAD)
			{
				unicodeString[unicodeStringIndex] = 0;

				lastDeadCharacter = deadKeysInformation[i].wchComposed;
			}
			else
			{
				// buf if the resulting character isn't another dead char
				// then last dead = null and return the result
				// (e.g. ~ + e = ê)
				lastDeadCharacter = 0;

				unicodeString[unicodeStringIndex] = deadKeysInformation[i].wchComposed;
			}

			return;
		}
	}

	// if the dead char doesn't combine with the character
	// (so e.g. ~ + p = ~p, but also e.g.: ~ + ~ = ~~)

	WCHAR baseCharacter = unicodeString[unicodeStringIndex];

	unicodeString[unicodeStringIndex] = lastDeadCharacter;
	unicodeString[++unicodeStringIndex] = baseCharacter;

	lastDeadCharacter = 0;
}

#define SET_BIT(NUMBER, BIT) ((NUMBER) |= (1 << (BIT)))

#define IS_KEY_PRESSED(VIRTUAL_KEY_CODE) (GetKeyState(VIRTUAL_KEY_CODE) & 0x8000)

void Keyboard::convertVirtualKeyToUnicodeString(BYTE virtualKey, WCHAR* unicodeString)
{
	unicodeStringIndex = 0;

	DWORD modifierKeysShiftStatesFlags = 0;

	for(BYTE i = 0; modifierKeysInformation->pVkToBit[i].Vk; i++)
		if(IS_KEY_PRESSED(modifierKeysInformation->pVkToBit[i].Vk))
			SET_BIT(modifierKeysShiftStatesFlags, i);
	
	if((modifierKeysShiftStatesFlags & KBDALT && !(modifierKeysShiftStatesFlags & KBDCTRL)) ||
		(modifierKeysShiftStatesFlags & KBDCTRL && !(modifierKeysShiftStatesFlags & KBDALT)))
	{
		unicodeString[0] = 0;

		return;
	}

	lookUpVirtualKeyInConversionTables(virtualKey, modifierKeysShiftStatesFlags, unicodeString);
}
