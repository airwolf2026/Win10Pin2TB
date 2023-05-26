// win10Pin2TB.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <ShlDisp.h>
#include <comutil.h>


#pragma comment(lib,"Shlwapi.lib")

BOOL WriteMsg2Console(LPCWSTR msg, ...)
{
	va_list va; // [rsp+258h] [rbp+10h]
	va_start(va, msg);
	WCHAR Str[MAX_PATH] = { 0 };
	StringCchVPrintf(Str, MAX_PATH, msg, va);
	size_t strLen = wcslen(Str);
	HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD NumberOfCharsWritten = 0;
	return WriteConsole(stdHandle, &Str, (DWORD)strLen, &NumberOfCharsWritten, 0i64);
}

#define BUFFER_SIZE  0x478
#define COMMAND_BUFFER_SIZE 0x64

static const void WINAPI replace_wchar(const wchar_t *srcStr, wchar_t replaceChar)
{
	if (srcStr == NULL)
		return ;

	int strLen = 0;
	const wchar_t* sptr = srcStr;
	while (*sptr++) ++strLen;

	if (strLen >= 0)
	{
		int lastIndex = strLen;
		wchar_t* pSrcChar = (wchar_t *)&srcStr[lastIndex];
		do
		{
			if (*pSrcChar == replaceChar)
			{
				if (lastIndex == strLen)
				{
					*pSrcChar = 0;
				}
				else if (lastIndex < strLen)
				{
					int diff = strLen - lastIndex;
					wchar_t* pOldChar = pSrcChar + 1;
					wchar_t* pNewLocation = pSrcChar;
					while (diff)
					{
						*pNewLocation = *pOldChar;
						++pOldChar;
						++pNewLocation;
						--diff;
					}
				}
			}
			--lastIndex;
			--pSrcChar;
		} while (lastIndex >= 0);
	}
}

typedef DWORD(WINAPI *thread_callback)(void* pContextData);
static DWORD WINAPI thread_func(void* pContextData)
{
	CoInitialize(0i64);
	DWORD opCode = *(DWORD *)((char*)pContextData + BUFFER_SIZE-4);
	WCHAR commandString[MAX_PATH] = { 0 };
	if (opCode != 0)
	{
		HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
		LoadStringW(hShell32, opCode, commandString, MAX_PATH);
		CharLowerW(commandString);
	}
	else
	{
		StringCchCopyW(commandString, MAX_PATH, (WCHAR*)((char*)pContextData + MAX_PATH * 4));
	}

	IShellDispatch* pIShellDispatch = NULL;
	HRESULT hResult = S_FALSE;
	hResult = CoCreateInstance(CLSID_Shell, 0i64, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
		IID_IShellDispatch, (LPVOID*)&pIShellDispatch);

	if (!SUCCEEDED(hResult) || pIShellDispatch == NULL)
	{
		return false;
	}
	else
	{
		VARIANTARG variantDir;
		VariantInit(&variantDir);
		variantDir.vt = VT_BSTR;
		char* pDir = (char*)pContextData + (MAX_PATH * 2);//520
		variantDir.llVal = LONGLONG(pDir);

		Folder *pFolder = NULL;
		hResult = pIShellDispatch->NameSpace(variantDir, &pFolder);
		if (pFolder != NULL)
		{
			FolderItem* pFolderItem = NULL;
			BSTR fileName = (WCHAR*)pContextData;
			hResult = pFolder->ParseName(fileName, &pFolderItem);
			FolderItemVerbs* pVerbs = NULL;
			if (pFolderItem != NULL)
			{
				hResult = pFolderItem->Verbs(&pVerbs);
				long verbsCount = 0;
				if (pVerbs != NULL)
				{
					hResult = pVerbs->get_Count(&verbsCount);
					replace_wchar(commandString, L'&');
					FolderItemVerb* pTargetVerb = NULL;
					wchar_t tempName[MAX_PATH] = { 0 };
					for (int i = 0; i < verbsCount; i++)
					{
						VARIANTARG variantIndex;
						variantIndex.lVal = i;
						variantIndex.vt = VT_I4;
						FolderItemVerb* pVerb = NULL;
						hResult = pVerbs->Item(variantIndex, &pVerb);
						if (pVerb != NULL)
						{
							BSTR pVerbName = NULL;
							hResult = pVerb->get_Name(&pVerbName);
							if (pVerbName == NULL || *(wchar_t*)pVerbName == 0)
								continue;
							StringCchCopyW(tempName, MAX_PATH, pVerbName);
							replace_wchar((wchar_t*)tempName, L'&');
							CharLowerW(tempName);
							if (SUCCEEDED(hResult) && !lstrcmpW((wchar_t*)tempName, commandString))
							{
								pTargetVerb = pVerb;
								break;
							}
							pVerb->Release();
						}
					}
					if (pTargetVerb != NULL)
					{
						hResult = pTargetVerb->DoIt();
						pTargetVerb->Release();
					}
					pVerbs->Release();
				}
				pFolderItem->Release();
			}
			pFolder->Release();
		}
		pIShellDispatch->Release();
	}

	CoUninitialize();

	return SUCCEEDED(hResult);
}

BOOL __fastcall InjectFun2Explorer(LPCVOID lpThreadArgs, HANDLE hProcess, thread_callback callBack)
{
	HMODULE hModule = GetModuleHandleW(NULL);
	PIMAGE_NT_HEADERS pNTH = (PIMAGE_NT_HEADERS)((ULONG_PTR)hModule + ((IMAGE_DOS_HEADER*)hModule)->e_lfanew);

	BOOL ret = FALSE;
	if (pNTH->Signature == IMAGE_NT_SIGNATURE)
	{
		DWORD image_size = pNTH->OptionalHeader.SizeOfImage;
		ret = IsBadReadPtr(hModule, image_size);
		if (!ret)
		{
			LPVOID pProgman_mem = VirtualAllocEx(hProcess, NULL, image_size + BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pProgman_mem)
			{
				LPVOID pLocalMem = VirtualAlloc(NULL, image_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (pLocalMem)
				{
					memcpy(pLocalMem, (char*)hModule, image_size);
					PIMAGE_BASE_RELOCATION pReloc = (PIMAGE_BASE_RELOCATION)&pNTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
					if(pReloc->SizeOfBlock)
					{
						if(pReloc->VirtualAddress)
						{
							PIMAGE_BASE_RELOCATION pRelocInLocalMem = (PIMAGE_BASE_RELOCATION)&((char*)pLocalMem)[pReloc->VirtualAddress];
							LONG_PTR delta_progman = (ULONG_PTR)((ULONGLONG)pProgman_mem - pNTH->OptionalHeader.ImageBase);
							LONG_PTR delta_localMem = (ULONG_PTR)((ULONGLONG)hModule - pNTH->OptionalHeader.ImageBase);
							while (pRelocInLocalMem && pRelocInLocalMem->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
							{
								DWORD countOfBlock = (pRelocInLocalMem->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) >> 1;
								if (countOfBlock)
								{
									WORD* typeoffset = (WORD*)(pRelocInLocalMem + 1);
									do
									{
										if (*typeoffset)
										{
											PULONG_PTR address_to_reloc = (PULONG_PTR)&((char*)pLocalMem)[pRelocInLocalMem->VirtualAddress + (*typeoffset & 0xFFF)];
											*address_to_reloc += delta_progman - delta_localMem;
										}
										++typeoffset;
										--countOfBlock;
									} while (countOfBlock);
								}
								pRelocInLocalMem = (PIMAGE_BASE_RELOCATION)((char*)pRelocInLocalMem + pRelocInLocalMem->SizeOfBlock);
							}
						}
					}
					((char*)pLocalMem)[pNTH->OptionalHeader.AddressOfEntryPoint] = 0x55;//todo:why change entryPoint？Edited by xyq@2019-10-24 16:06:08
					if (WriteProcessMemory(hProcess, pProgman_mem, pLocalMem, image_size, 0i64))
					{
						SIZE_T NumberOfBytesWritten = 0;
						WriteProcessMemory(hProcess, (char*)pProgman_mem + image_size, lpThreadArgs, BUFFER_SIZE, &NumberOfBytesWritten);
						HANDLE hTread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((char*)pProgman_mem + ((char*)callBack - (char*)hModule)),
							(char*)pProgman_mem + image_size, 0, 0i64);
						WaitForSingleObject(hTread, 15 * 1000);
						TerminateThread(hTread, 0);
						CloseHandle(hTread);
					}
					VirtualFree(pLocalMem, 0i64, MEM_RELEASE);
				}
				ret = VirtualFreeEx(hProcess, pProgman_mem, 0i64, MEM_RELEASE);
			}
		}
	}
	return ret;
}

//Uncomment this for dbgviewer in release build mode 
//#define _DEBUG 1 

int main()
{
	HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	LPWSTR commandline =  GetCommandLine();
	int pNumArgs = 0;
	LPWSTR *szArglist = CommandLineToArgvW(commandline, &pNumArgs);
	if (pNumArgs<3)
	{
		WriteMsg2Console(commandline);
		OutputDebugString(commandline);
		return 1i64;
	}

	OutputDebugString(commandline);

	WCHAR fileName[MAX_PATH] = { 0 };
	memset(fileName, 0, 2 * MAX_PATH);
	WCHAR folder[MAX_PATH] = { 0 };
	memset(folder, 0, 2 * MAX_PATH);
	wcscpy_s(folder, szArglist[1]);
	wcscpy_s(fileName, szArglist[1]);
	DWORD fileAttributes =  GetFileAttributes(folder); 
	if ((fileAttributes == INVALID_FILE_ATTRIBUTES || fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		OutputDebugString(L"Can't get file attributes");
		SetConsoleTextAttribute(stdHandle, 0xAu); //BACKGROUND_BLUE
		ExitProcess(0); 
	}

	char buffer[BUFFER_SIZE] = { 0 };
	memset(buffer, 0, BUFFER_SIZE);
	PathStripPath(fileName);
	memcpy_s(buffer, BUFFER_SIZE, fileName, MAX_PATH * 2);

	PathRemoveFileSpec(folder);
	memcpy_s(buffer + MAX_PATH * 2, BUFFER_SIZE, folder, MAX_PATH * 2);

	WCHAR command[COMMAND_BUFFER_SIZE] = { 0 };
	wcscpy_s(command, szArglist[2]);
	memcpy_s(buffer + MAX_PATH * 4, BUFFER_SIZE, command, COMMAND_BUFFER_SIZE);

	int opCode = 0;
	if (iswdigit((wint_t)*command))
	{
		StrToIntEx(command, 0, &opCode);
		memcpy_s(buffer + MAX_PATH * 4 + COMMAND_BUFFER_SIZE, BUFFER_SIZE, &opCode, sizeof(int));
	}
	else
	{
		_wcslwr_s(command);
	}

	HWND hProgman = FindWindowW(L"Progman", NULL);
	if(hProgman==NULL)
		OutputDebugString(L"Can't find progman");

	DWORD 	dwProcessId = 0;
	GetWindowThreadProcessId(hProgman, &dwProcessId);
	if (dwProcessId)
	{
		HANDLE hExplorer = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, 0, dwProcessId);
		if (hExplorer != INVALID_HANDLE_VALUE)
		{
			InjectFun2Explorer(buffer, hExplorer, thread_func);
		}
		else
		{
			OutputDebugString(L"OpenProcess error");
		}
	}
}
