#pragma once

#define BUILD_WOW6432

#include <Windows.h>
#include <kbd.h>
#include <string>

class Keyboard
{
	PVK_TO_WCHARS1 vkToWchars1;
	PVK_TO_WCHARS2 vkToWchars2;
	PVK_TO_WCHARS3 vkToWchars3;
	PVK_TO_WCHARS4 vkToWchars4;
	PVK_TO_WCHARS5 vkToWchars5;
	PVK_TO_WCHARS6 vkToWchars6;
	PVK_TO_WCHARS7 vkToWchars7;
	PVK_TO_WCHARS8 vkToWchars8;
	PVK_TO_WCHARS9 vkToWchars9;
	PVK_TO_WCHARS10 vkToWchars10;

	BYTE maxLigature;
	PLIGATURE1 ligatureInformation;
	BYTE ligatureInformationEntrySize;

	WCHAR lastDeadCharacter;
	PDEADKEY deadKeysInformation;

	BYTE unicodeStringIndex;

	PMODIFIERS modifierKeysInformation;

	WCHAR loadedKeyboardLayoutName[KL_NAMELENGTH];

	void freeAllocatedMemory();

	void lookUpDeadCharacterOutput(WCHAR* unicodeString);

	void intitializeDeadKeysInformation(PDEADKEY deadKey);

	void initializeLigatureInformation(PLIGATURE1 ligature);

	void initializeModifierKeysInformation(PMODIFIERS modifiers);

	bool getKeyboardLayoutFilePath(WCHAR* keyboardLayoutFilePath);

	void initializeConversionTables(PVK_TO_WCHAR_TABLE vkToWcharTable);

	void lookUpVirtualKeyInConversionTables(BYTE virtualKey, DWORD modifierKeysShiftStatesFlags, WCHAR* unicodeString);

	template <typename T>
	bool lookUpVirtualKeyInConversionTable(T vkToWchars, BYTE virtualKey, DWORD modifierKeysShiftStatesFlags, WCHAR* unicodeString);

public:
	Keyboard();

	~Keyboard();

	bool changeKeyboardLayout();

	bool hasKeyboardLayoutChanged();

	void convertVirtualKeyToUnicodeString(BYTE virtualKey, WCHAR* unicodeString);
};

#define IS_KEY_TOGGLED(VIRTUAL_KEY_CODE) (GetKeyState(VIRTUAL_KEY_CODE) & 1)

template <typename T>
bool Keyboard::lookUpVirtualKeyInConversionTable(T vkToWchars, BYTE virtualKey, DWORD modifierKeysShiftStatesFlags, WCHAR* unicodeString)
{
	if(!vkToWchars)
		return false;

	WCHAR& firstUnicodeChar = unicodeString[0];

	for(DWORD i = 0; vkToWchars[i].VirtualKey; i++)
	{
		if(vkToWchars[i].VirtualKey != virtualKey)
			continue;

		BYTE currentCharacterShiftStatesAmount = sizeof vkToWchars->wch / sizeof(WCHAR);

		if(((vkToWchars[i].Attributes & CAPLOK && (!modifierKeysShiftStatesFlags || modifierKeysShiftStatesFlags == KBDSHIFT)) ||
			(vkToWchars[i].Attributes & CAPLOKALTGR && modifierKeysShiftStatesFlags & KBDALT && modifierKeysShiftStatesFlags & KBDCTRL)) &&
			IS_KEY_TOGGLED(VK_CAPITAL))
			modifierKeysShiftStatesFlags ^= KBDSHIFT;

		// if flag is bigger than max then it's invalid
		if(modifierKeysShiftStatesFlags > modifierKeysInformation->wMaxModBits)
		{
			firstUnicodeChar = 0;

			return true;
		}

		BYTE outputCharacterIndex = modifierKeysInformation->ModNumber[modifierKeysShiftStatesFlags];

		if(outputCharacterIndex >= currentCharacterShiftStatesAmount || outputCharacterIndex == SHFT_INVALID)
		{
			firstUnicodeChar = 0;

			return true;
		}

		if(vkToWchars[i].Attributes & SGCAPS && IS_KEY_TOGGLED(VK_CAPITAL))
			outputCharacterIndex += currentCharacterShiftStatesAmount + 1;

		firstUnicodeChar = vkToWchars[i].wch[outputCharacterIndex];

		if(firstUnicodeChar == WCH_NONE)
			firstUnicodeChar = 0;
		else if(firstUnicodeChar == WCH_DEAD) 
		{
			if(lastDeadCharacter)
			{
				firstUnicodeChar = vkToWchars[i + 1].wch[outputCharacterIndex];

				lookUpDeadCharacterOutput(unicodeString);
			}
			else
				lastDeadCharacter = vkToWchars[i + 1].wch[outputCharacterIndex];
		}
		else if(firstUnicodeChar == WCH_LGTR) // ligature
		{
			PLIGATURE1 currentLigatureInformation = ligatureInformation;

			while(currentLigatureInformation->VirtualKey)
			{
				if(currentLigatureInformation->VirtualKey == virtualKey && currentLigatureInformation->ModificationNumber == outputCharacterIndex)
				{
					// assuming that WCH_NONE and NULL char won't be between within ligature chars
					// because we're checking if it's the end of the ligature
					for(BYTE j = 0; j < maxLigature; j++)
					{
						if(!currentLigatureInformation->wch[j] || currentLigatureInformation->wch[j] == WCH_NONE)
							break;

						// if j == 0 then firstUnicodeChar == WCH_LGTR which has to be changed
						if(j > 0)
							unicodeStringIndex++;

						unicodeString[unicodeStringIndex] = currentLigatureInformation->wch[j];

						if(lastDeadCharacter)
							lookUpDeadCharacterOutput(unicodeString);
					}

					break;
				}

				currentLigatureInformation = (PLIGATURE1)((DWORD64)currentLigatureInformation + ligatureInformationEntrySize);
			}

			if(!lastDeadCharacter && !firstUnicodeChar)
				for(BYTE j = 0; j <= unicodeStringIndex; j++)
					if(unicodeString[j])
					{
						// for however many remaining chars
						for(BYTE k = 0; k <= unicodeStringIndex - j; k++)
							unicodeString[k] = unicodeString[k + j];

						unicodeStringIndex -= j;

						unicodeString[unicodeStringIndex + 1] = 0;

						// so if it was e.g. {0, 0, 0, x, y, z}
						// then now it's {x, y, z, 0}

						break;
					}
		}
		else if(lastDeadCharacter) // if a plain key was pressed and a dead key had been pressed then combine them (if it's possible)
			lookUpDeadCharacterOutput(unicodeString);

		// if there's still a dead char
		if(lastDeadCharacter)
			firstUnicodeChar = 0;

		return true;
	}

	return false;
}
