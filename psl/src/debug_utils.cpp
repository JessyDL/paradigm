#include "psl/debug_utils.hpp"
#include "psl/assertions.hpp"
#include <stdexcept>

#ifdef PE_PLATFORM_WINDOWS
	#include <Windows.h>
	#pragma comment(lib, "Dbghelp.lib")
	#include <Dbghelp.h>
	#include <tlhelp32.h>
#endif

std::vector<psl::utility::debug::trace_info>
psl::utility::debug::trace(size_t offset, size_t depth, std::optional<std::thread::id> id) {
	std::vector<psl::utility::debug::trace_info> res;
#ifdef PE_PLATFORM_WINDOWS
	static HANDLE process = std::invoke([]() {
		HANDLE h = GetCurrentProcess();
		psl_assert(SymInitialize(h, NULL, TRUE) != FALSE, "SymInitialize failed");
		SymSetOptions(SYMOPT_LOAD_LINES);
		return h;
	});

	DWORD machine = IMAGE_FILE_MACHINE_AMD64;

	HANDLE thread = std::invoke(
	  [](std::optional<std::thread::id> id) -> HANDLE {
		  if(!id || id.value() == std::this_thread::get_id())
			  return GetCurrentThread();

		  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		  THREADENTRY32 te;
		  te.dwSize = sizeof(te);
		  Thread32First(h, &te);
		  if(Thread32First(h, &te)) {
			  do {
				  static_assert(sizeof(id.value()) == sizeof(te.th32ThreadID));
				  if(memcmp(&id.value(), &te.th32ThreadID, sizeof(te.th32ThreadID)) == 0) {
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
	GetThreadContext(thread, &context);
	STACKFRAME frame = {};
	#if defined(PE_ARCHITECTURE_X86)
	frame.AddrPC.Offset	   = context.Eip;
	frame.AddrPC.Mode	   = AddrModeFlat;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrFrame.Mode   = AddrModeFlat;
	frame.AddrStack.Offset = context.Esp;
	frame.AddrStack.Mode   = AddrModeFlat;
	#elif defined(PE_ARCHITECTURE_X86_64)
	frame.AddrPC.Offset		   = context.Rip;
	frame.AddrPC.Mode		   = AddrModeFlat;
	frame.AddrFrame.Offset	   = context.Rbp;
	frame.AddrFrame.Mode	   = AddrModeFlat;
	frame.AddrStack.Offset	   = context.Rsp;
	frame.AddrStack.Mode	   = AddrModeFlat;
	#endif

	for(unsigned int i = 0; i < depth + offset; i++) {
		if(!StackWalk(machine, process, thread, &frame, &context, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL))
			break;

		if(i < offset)
			continue;
		auto& info = res.emplace_back();
		info.addr  = frame.AddrPC.Offset;
		info.name  = {};

		char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
		PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		symbol->SizeOfStruct	= (sizeof IMAGEHLP_SYMBOL) + 255;
		symbol->MaxNameLength	= 254;

		if(SymGetSymFromAddr(process, frame.AddrPC.Offset, NULL, symbol)) {
			info.name += symbol->Name;
			info.name += " ";
		}

		DWORD offset = 0;
		IMAGEHLP_LINE line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

		if(SymGetLineFromAddr(process, frame.AddrPC.Offset, &offset, &line)) {
			info.name += psl::string8_t(line.FileName) + ":" + std::to_string(line.LineNumber);
		}
	}

	if(id && id.value() != std::this_thread::get_id())
		CloseHandle(thread);
#endif
	return res;
}

std::vector<void*> psl::utility::debug::raw_trace(size_t offset, size_t depth) {
	std::vector<void*> stack;
#ifdef PE_PLATFORM_WINDOWS
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


psl::utility::debug::trace_info psl::utility::debug::demangle(void* target) {
	psl::utility::debug::trace_info info;
#ifdef PE_PLATFORM_WINDOWS
	#if defined(PE_ARCHITECTURE_X86)
	using IMAGEHLP_SYMBOL_TYPE = IMAGEHLP_SYMBOL;
	using DWORD_TYPE		   = DWORD;
	#elif defined(PE_ARCHITECTURE_X86_64)
	using IMAGEHLP_SYMBOL_TYPE = IMAGEHLP_SYMBOL64;
	using DWORD_TYPE		   = DWORD64;
	#endif
	static std::unique_ptr<IMAGEHLP_SYMBOL_TYPE> symbol = std::invoke([]() {
		IMAGEHLP_SYMBOL_TYPE* sPtr;
		sPtr				= (IMAGEHLP_SYMBOL_TYPE*)calloc(sizeof(IMAGEHLP_SYMBOL_TYPE) + 256 * sizeof(char), 1);
		sPtr->MaxNameLength = 255;
		sPtr->SizeOfStruct	= sizeof(IMAGEHLP_SYMBOL_TYPE);
		std::unique_ptr<IMAGEHLP_SYMBOL_TYPE> sUPtr {sPtr};
		return sUPtr;
	});
	static HANDLE process								= std::invoke([]() {
		  HANDLE h = GetCurrentProcess();
		  SymInitialize(h, NULL, TRUE);
		  return h;
	  });

	#if defined(PE_ARCHITECTURE_X86)
	SymGetSymFromAddr(process, (DWORD_TYPE)(target), 0, symbol.get());
	#elif defined(PE_ARCHITECTURE_X86_64)
	SymGetSymFromAddr64(process, (DWORD_TYPE)(target), 0, symbol.get());
	#endif
	info.name = psl::from_string8_t(psl::string8_t(symbol->Name));
	info.addr = symbol->Address;
#endif
	return info;
}
