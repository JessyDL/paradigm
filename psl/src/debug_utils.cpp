#include "psl/debug_utils.hpp"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#pragma comment(lib, "Dbghelp.lib")
#include <Dbghelp.h>
#include <tlhelp32.h>
#endif

std::vector<utility::debug::trace_info>
utility::debug::trace(size_t offset, size_t depth, std::optional<std::thread::id> id)
{
	std::vector<utility::debug::trace_info> res;
#ifdef PLATFORM_WINDOWS
	static HANDLE process = std::invoke([]() {
		HANDLE h = GetCurrentProcess();
		psl_assert(SymInitialize(h, NULL, TRUE) != FALSE, "SymInitialize failed");
		SymSetOptions(SYMOPT_LOAD_LINES);
		return h;
	});

	DWORD machine = IMAGE_FILE_MACHINE_AMD64;

	HANDLE thread = std::invoke(
	  [](std::optional<std::thread::id> id) -> HANDLE {
		  if(!id || id.value() == std::this_thread::get_id()) return GetCurrentThread();

		  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		  THREADENTRY32 te;
		  te.dwSize = sizeof(te);
		  Thread32First(h, &te);
		  if(Thread32First(h, &te))
		  {
			  do
			  {
				  static_assert(sizeof(id.value()) == sizeof(te.th32ThreadID));
				  if(memcmp(&id.value(), &te.th32ThreadID, sizeof(te.th32ThreadID)) == 0)
				  {
					  return OpenThread(THREAD_ALL_ACCESS, false, te.th32ThreadID);
				  }
				  te.dwSize = sizeof(te);
			  } while(Thread32Next(h, &te));
		  }
		  throw std::runtime_error("could not find the requested thread for stracktrace");
	  },
	  id);

	CONTEXT context		 = {};
	context.ContextFlags = CONTEXT_ALL;
	// RtlCaptureContext(&context);
	GetThreadContext(thread, &context);
	STACKFRAME frame	   = {};
	frame.AddrPC.Offset	   = context.Rip;
	frame.AddrPC.Mode	   = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrFrame.Mode   = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;
	frame.AddrStack.Mode   = AddrModeFlat;

	for(unsigned int i = 0; i < depth + offset; i++)
	{
		if(!StackWalk(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
			break;

		if(i < offset) continue;
		auto& info = res.emplace_back();
		info.addr  = frame.AddrPC.Offset;
		info.name  = {};
		// DWORD64 moduleBase = SymGetModuleBase(process, frame.AddrPC.Offset);
		// char moduleBuff[MAX_PATH];
		// if(moduleBase && GetModuleFileNameA((HINSTANCE)moduleBase, moduleBuff, MAX_PATH))
		//{
		//	info.name += moduleBuff;
		//	info.name += " ";
		//}

		char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
		PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		symbol->SizeOfStruct	= (sizeof IMAGEHLP_SYMBOL) + 255;
		symbol->MaxNameLength	= 254;

		if(SymGetSymFromAddr(process, frame.AddrPC.Offset, NULL, symbol))
		{
			info.name += symbol->Name;
			info.name += " ";
		}

		DWORD offset = 0;
		IMAGEHLP_LINE line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

		if(SymGetLineFromAddr(process, frame.AddrPC.Offset, &offset, &line))
		{
			info.name += psl::string8_t(line.FileName) + ":" + std::to_string(line.LineNumber);
		}
	}

	if(id && id.value() != std::this_thread::get_id()) CloseHandle(thread);
#endif
	return res;
}

std::vector<void*> utility::debug::raw_trace(size_t offset, size_t depth)
{
	std::vector<void*> stack;
#ifdef PLATFORM_WINDOWS
	stack.resize(depth);
	unsigned short frames;
	static HANDLE process = std::invoke([]() {
		HANDLE h = GetCurrentProcess();
		SymInitialize(h, NULL, TRUE);
		return h;
	});

	frames = CaptureStackBackTrace((DWORD)(offset + 1u), (DWORD)depth, stack.data(), NULL);
	stack.resize(frames);
#endif
	return stack;
}


utility::debug::trace_info utility::debug::demangle(void* target)
{
	utility::debug::trace_info info;
#ifdef PLATFORM_WINDOWS
	static std::unique_ptr<IMAGEHLP_SYMBOL64> symbol = std::invoke([]() {
		IMAGEHLP_SYMBOL64* sPtr;
		sPtr				= (IMAGEHLP_SYMBOL64*)calloc(sizeof(IMAGEHLP_SYMBOL64) + 256 * sizeof(char), 1);
		sPtr->MaxNameLength = 255;
		sPtr->SizeOfStruct	= sizeof(IMAGEHLP_SYMBOL64);
		std::unique_ptr<IMAGEHLP_SYMBOL64> sUPtr {sPtr};
		return sUPtr;
	});
	static HANDLE process							 = std::invoke([]() {
		   HANDLE h = GetCurrentProcess();
		   SymInitialize(h, NULL, TRUE);
		   return h;
	   });

	SymGetSymFromAddr64(process, (DWORD64)(target), 0, symbol.get());
	info.name = psl::from_string8_t(psl::string8_t(symbol->Name));
	info.addr = symbol->Address;
#endif
	return info;
}