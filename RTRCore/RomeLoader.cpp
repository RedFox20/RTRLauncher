#include "RomeLoader.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <log.h>
#include <rpp/file_io.h>
#include <remote_dll_injector.h>
#include "lzma\LzmaLib.h"
#include "../Launcher/resource.h"
#include <memory_map.h>

static const char secOK[] = "[+] RomeLoader [+]";
static const char secFF[] = "[!] RomeLoader [!]";


struct RCDATA
{
	int   size; // size of data buffer in bytes
	char* data; // malloc'd data buffer
	void* hres; // NULL if read-write data, otherwise read-only data

	~RCDATA()                                 
	{
		if (hres)      FreeResource(hres); 
		else if (data) free(data);
	}
	RCDATA(int resourceId, bool readonly = true) : size(0), data(0), hres(0) { load(resourceId, readonly); }
	RCDATA()                                     : size(0), data(0), hres(0) { }
	RCDATA(RCDATA&)                  = delete;
	RCDATA& operator=(const RCDATA&) = delete;
	RCDATA& operator=(RCDATA&&)      = delete;
	
	//// @note Load the resource into a malloc'd buffer so we can modify it
	////       JFYI resources are RO data
	void load(int resourceId, bool readonly = true)
	{
		HRSRC info = FindResourceA(0, MAKEINTRESOURCEA(resourceId), RT_RCDATA);
		if (void* res = LoadResource(0, info))
		{
			size = SizeofResource(0, info);
			if (readonly)
			{
				data = (char*)LockResource(res);
				hres = res;
			}
			else
			{
				data = (char*)malloc(size);
				memcpy(data, LockResource(res), size);
				FreeResource(res);
			}
		}
	}
};
static void digest_data(void* data, int size)
{
	int digest[] = {
		83909668, 70983089, 58973346, 49367121, 72867320, 30189684, 35733769, 31514805, 95692450, 99342547,
		44789105, 56805210, 15817535, 91918311, 40257068, 31166305, 56902993, 12565043, 53731866, 52663902,
		30196097, 41770641, 59506144, 95219823, 88866169, 22223402, 67977034, 16162080, 6445311,  35645479,
		32790095, 30660170, 4450843,  52241824, 22387167, 72085271, 63475958, 15296082, 76259338, 66473275,
		938787,   11124630, 77950686, 6911032,  63475035, 12474260, 59397712, 93217612, 3139110,  98580382,
		27749155, 84460460, 87067640, 25346222, 66513600, 53229431, 59008409, 62926187, 8391080,  34863553,
		50378298, 78618437, 46173941, 59795798, 14550097, 91113066, 60557171, 8958616,  76914992, 37803569,
		18911312, 5341668,  27249343, 6247167,  8926192,  13679800, 217403,   88778262, 44629525, 6970022,
		29214878, 79225771, 84668085, 95568992, 62319037, 96349493, 17288771, 12524826, 34468530, 15677703,
		43899185, 18468151, 34218329, 96693159, 19674360, 53346805, 54121257, 6641460,  31732787, 27049901,
		4017684,  50056788, 47361149, 66598700, 156408,   81661795, 48626718, 39782423, 92922449, 78893624,
		73388077, 82852026, 6013169,  77746950, 22942585, 2084706,  38012483, 87665584, 51349256, 23058102,
		70485027, 82873399, 58155611, 20299085, 94660555, 1569492,  20828446, 30215989,
	};
	const int maxBytes = size - sizeof(int);
	int* dst = (int*)data;
	for (int i = 0; i < maxBytes;)
		for (int j = 0; j < 128 && i < maxBytes; i += 4, ++j)
			*dst++ ^= digest[j];
}
size_t load_patches(char*& outData)
{
	const int numPatches = 4;
	RCDATA patchinf(IDR_PATCH);
	RCDATA patches[4];
	size_t totalSize = 0;
	for (int i = 0; i < 4; ++i) patches[i].load(IDR_PATCH0 + i);
	for (int i = 0; i < 4; ++i) totalSize += patches[i].size;
	UCHAR* inBuff = (UCHAR*)malloc(totalSize);
	UCHAR* dst = inBuff;
	for (int i = 0; i < 4; ++i) {
		memcpy(dst, patches[i].data, patches[i].size);
		dst += patches[i].size;
	}
	_ASSERTE((dst - inBuff) == totalSize);
	digest_data(inBuff, totalSize);
	UCHAR props[8];
	size_t unSize = *(size_t*)patchinf.data;
	memcpy(props, patchinf.data + sizeof size_t, 8);
	size_t inSize = totalSize;
	UCHAR* unBuff = (UCHAR*)malloc(unSize);
	int res = LzmaUncompress(unBuff, &unSize, inBuff, &inSize, props, LZMA_PROPS_SIZE);
	free(inBuff);
	outData = (char*)unBuff;
	return unSize;
}
struct DecompressParams
{
	size_t comprSize;
	size_t origSize;
	UCHAR  props[8];
};
struct DecompressedData
{
	char* data;
	int   size;
	DecompressedData(UCHAR* buff, size_t sz) : data((char*)buff), size(sz)  { }
	DecompressedData(DecompressedData&& mv)  : data(mv.data), size(mv.size) { mv.data = NULL, mv.size = 0; }
	~DecompressedData()                                                     { if (data) free(data); }
	DecompressedData(const DecompressedData&)            = delete;
	DecompressedData& operator=(const DecompressedData&) = delete;
	DecompressedData& operator=(DecompressedData&&)      = delete;
};
static DecompressedData generic_decompress(DecompressParams& params, const void* data)
{
	size_t inSize = params.comprSize;
	size_t unSize = params.origSize;
	UCHAR* inBuff = (UCHAR*)data;
	UCHAR* unBuff = (UCHAR*)malloc(unSize);
	int res = LzmaUncompress(unBuff, &unSize, inBuff, &inSize, params.props, LZMA_PROPS_SIZE);
	return DecompressedData(unBuff, unSize);
}
static DecompressedData unpack_resource(int resourceId)
{
	// need to unpack data first
	RCDATA pz(resourceId, false);
	digest_data(pz.data, pz.size);
	return generic_decompress(*(DecompressParams*)pz.data, pz.data + sizeof(DecompressParams));
}



static bool InjectAttach(char(&errmsg)[512], const CoreSettings& st, PROCESS_INFORMATION& pi);

bool RomeLoader::Start(char (&errmsg)[512], 
					   const string& cmd, 
					   const string& gameDir,
					   const CoreSettings& settings)
{
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si		   = { sizeof(si) };

	if (CreateProcessA(0,(char*)cmd.c_str(),0,0,0,CREATE_SUSPENDED,0,gameDir.c_str(),&si,&pi))
	{
		char*  buffPatch = NULL;
		size_t sizePatch = load_patches(buffPatch);

		memory_map mmap = memory_map::create("RTRGameEngine", sizePatch);
		map_view view = mmap.create_view();
		memcpy(view, buffPatch, sizePatch);
		free(buffPatch);

		log("RTW.MainThreadId: %d\n", pi.dwThreadId);

		// try to inject the DLL and Attach the debugger:
		if (InjectAttach(errmsg, settings, pi))
			return true; // success
		//InjectAttach(errmsg, settings, pi); // -- debug -- fall through and terminate

		TerminateProcess(pi.hProcess, 0);
		logsec(secFF, "Error: Failed to inject GameEngine.dll\n");

		// fall through and get last error:
	}
	return false;
}


static bool ValidateGameEngineDll(int expectedSize)
{
	if (rpp::file_exists("GameEngine.dll"))
	{
		if (rpp::file_size("GameEngine.dll") == expectedSize)
			return true;
		logsec(secFF, "Warning: Invalid GameEngine.dll detected in immediate directory. Ignoring file.\n");
	}
	return false;
}



// tries to inject DLL and possibly Attach the VSJIT debugger; RomeTW is launched on success
static bool InjectAttach(char(&errmsg)[512], const CoreSettings& st, PROCESS_INFORMATION& pi)
{
	DecompressedData gameEngine = unpack_resource(IDR_DLL_GAMEENGINE);
	bool useFileInject = ValidateGameEngineDll(gameEngine.size);
	if (st.DebugAttach)
	{
		logsec(secOK, "VS JIT Attach\n");
		char jitcommand[MAX_PATH];
		sprintf(jitcommand, "vsjitdebugger.exe -p %d", pi.dwProcessId);
		
		logsec(secOK, "Launching %s\n", jitcommand);
		int result = system(jitcommand);
		if (result != S_OK) logsec(secFF, "Debugger Attach Failed: %p\n", result);

		if (!useFileInject) // nonfatal error case
			logsec(secFF, "Failed to detect GameEngine.dll, reverting to RESOURCE inject\n");
	}

	bool injectSuccess = !st.Inject; // if injecting assume false, otherwise assume true
	if (st.Inject)
	{
		// reserve just enough memory for RomeTW-ALX.exe
		logsec(secOK, "Reserving target memory\n");
		remote_dll_injector::reserve_target_memory(pi.hProcess, 0x0269ea9e);
		if (useFileInject)
		{
			// for debugger inject and if it exists, we use GameEngine.dll FILE
			logsec(secOK, "Injecting DEBUG .\\GameEngine.dll...\n");
			injectSuccess = remote_dll_injector::inject_dllfile(pi.hProcess, "GameEngine.dll");
		}
		else
		{
			// for non-debugger inject, we use the internal DLL resource
			remote_dll_injector injector = remote_dll_injector(gameEngine.data);
			if (injectSuccess = injector) {
				logsec(secOK, "Injecting RESOURCE GameEngine.dll...\n");
				injectSuccess = injector.inject_dllimage(pi.hProcess);
			}
			else logsec(secFF, "Error: GameEngine.dll embedded resource not found!");
		}
	}

	if (injectSuccess) {
		logsec(secOK, "Launching the game!\n\n");
		ResumeThread(pi.hThread); // Launch!!!
		return true;
	}
	return false;
}