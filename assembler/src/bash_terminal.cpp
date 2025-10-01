#include "bash_terminal.h"
#include "stdafx.h"
#include <iostream>
#include <thread>
using namespace tools;


const psl::string_view MAGIC_END_CMD = "\"echo endl of output\"";

const psl::string_view MAGIC_END = "endl of output";

void bash_terminal::pipe::close() {
	CloseHandle(write);
	CloseHandle(read);
}


bash_terminal::bash_terminal(uint64_t bufferSize, std::optional<psl::string> params) {
	m_Buffer.resize(bufferSize);


	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	// SECURITY_DESCRIPTOR sd;               //security information for pipes

	sa.lpSecurityDescriptor = NULL;
	sa.nLength				= sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle		= true;	   // allow inheritable handles

	if(!CreatePipe(&m_Out.read, &m_Out.write, &sa, 0))	  // create stdin pipe
	{
		throw std::runtime_error("CreatePipe");
		return;
	}
	if(!CreatePipe(&m_In.read, &m_In.write, &sa, 0))	// create stdout pipe
	{
		m_Out.close();
		throw std::runtime_error("CreatePipe");
		return;
	}
	if(!CreatePipe(&m_Error.read, &m_Error.write, &sa, 0))	  // create stderror pipe
	{
		m_In.close();
		m_Out.close();
		throw std::runtime_error("CreatePipe");
		return;
	}

	GetStartupInfo(&si);	// set startupinfo for the spawned process
	/*
	The dwFlags member tells CreateProcess how to make the process.
	STARTF_USESTDHANDLES validates the hStd* members. STARTF_USESHOWWINDOW
	validates the wShowWindow member.
	*/
	si.dwFlags	   = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput  = m_Out.write;
	si.hStdError   = m_Out.write;	 // set the new handles for the child process
	si.hStdInput   = m_In.read;

	std::wstring app = _T("c:\\windows\\system32\\bash.exe");
	// spawn the child process
	if(!CreateProcess(app.c_str(),
					  ((params) ? psl::to_platform_string(params.value()).data() : NULL),
					  NULL,
					  NULL,
					  TRUE,
					  CREATE_NEW_CONSOLE,
					  NULL,
					  NULL,
					  &si,
					  &pi)) {
		m_In.close();
		m_Out.close();
		m_Error.close();
		throw std::runtime_error("Could not create the command terminal process.");
		return;
	}
}

bash_terminal::~bash_terminal() {
	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example.
	exec("exit");
	Sleep(100);

	m_In.close();
	m_Out.close();
	m_Error.close();
	TerminateProcess(pi.hProcess, 0);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

const psl::string_view FORCE_SYNC_CMD = "\necho IMMAGICALLIKEAUNICORN";
const psl::string_view FORCE_SYNC_STR = "IMMAGICALLIKEAUNICORN\n";
psl::string bash_terminal::read(bool bOutputImmediately,
								std::chrono::duration<int64_t, std::micro> timeout,
								std::chrono::duration<int64_t, std::micro> interval) {
	unsigned long exit = 0;		// process exit code
	unsigned long bread;		// bytes read
	unsigned long avail = 0;	// bytes available

	exec(FORCE_SYNC_CMD);
	psl::string result = "";
	memset(m_Buffer.data(), '\0', m_Buffer.size());

	while(avail == 0 && timeout.count() > 0) {
		PeekNamedPipe(m_Out.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, &avail, NULL);
		;

		while(avail > 0 || bread == m_Buffer.size() - 1) {
			ReadFile(m_Out.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, NULL);	 // read the stdout pipe

			psl::string8_t str;
			str.resize(bread);
			memcpy(str.data(), m_Buffer.data(), bread);
			result += str;

			memset(m_Buffer.data(), '\0', m_Buffer.size());
			if(bread > avail)
				avail = 0;
			else
				avail -= bread;
		}
		if(result.size() >= FORCE_SYNC_STR.size()) {
			psl::string_view view {&result[result.size() - FORCE_SYNC_STR.size()], FORCE_SYNC_STR.size()};
			if(view == FORCE_SYNC_STR) {
				result.erase(std::end(result) - FORCE_SYNC_STR.size(), std::end(result));
				if(bOutputImmediately)
					psl::cout << psl::to_platform_string(result);
				return result;
			}
		}
		timeout -= interval;
		std::this_thread::sleep_for(interval);

		GetExitCodeProcess(pi.hProcess, &exit);	   // while the process is running
		if(exit != STILL_ACTIVE)
			return "";
	}


	if(timeout.count() <= 0)
		std::cerr << "timed out reading from the bash terminal" << std::endl;
	return result;
}

psl::string bash_terminal::read_error(bool bOutputImmediately,
									  std::chrono::duration<int64_t, std::micro> timeout,
									  std::chrono::duration<int64_t, std::micro> interval) {
	unsigned long exit = 0;					   // process exit code
	unsigned long bread;					   // bytes read
	unsigned long avail = 0;				   // bytes available
	GetExitCodeProcess(pi.hProcess, &exit);	   // while the process is running
	if(exit != STILL_ACTIVE)
		return "";

	psl::string result = "";

	exec(psl::string(FORCE_SYNC_CMD) + " >&2");

	memset(m_Buffer.data(), '\0', m_Buffer.size());

	while(avail == 0 && timeout.count() > 0) {
		PeekNamedPipe(m_Error.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, &avail, NULL);
		;

		while(avail > 0 || bread == m_Buffer.size() - 1) {
			ReadFile(m_Error.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, NULL);	   // read the stdout pipe

			psl::string8_t str;
			str.resize(bread);
			memcpy(str.data(), m_Buffer.data(), bread);
			result += str;

			result += psl::string8_t {m_Buffer.data(), bread};
			memset(m_Buffer.data(), '\0', m_Buffer.size());
			if(bread > avail)
				avail = 0;
			else
				avail -= bread;
		}
		if(result.size() >= FORCE_SYNC_STR.size() &&
		   psl::string_view {&result[result.size() - FORCE_SYNC_STR.size()], FORCE_SYNC_STR.size()} == FORCE_SYNC_STR) {
			result.erase(std::end(result) - FORCE_SYNC_STR.size(), std::end(result));
			if(bOutputImmediately)
				psl::cout << psl::to_platform_string(result);
			return result;
		}
		timeout -= interval;
		std::this_thread::sleep_for(interval);
		GetExitCodeProcess(pi.hProcess, &exit);	   // while the process is running
		if(exit != STILL_ACTIVE)
			return "";
	}

	if(timeout.count() <= 0)
		std::cerr << "timed out reading from the bash terminal" << std::endl;
	return result;
}

psl::string bash_terminal::async_read() {
	unsigned long exit = 0;					   // process exit code
	unsigned long bread;					   // bytes read
	unsigned long avail = 0;				   // bytes available
	GetExitCodeProcess(pi.hProcess, &exit);	   // while the process is running
	if(exit != STILL_ACTIVE)
		return "";

	psl::string result = "";
	memset(m_Buffer.data(), '\0', m_Buffer.size());
	PeekNamedPipe(m_Out.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, &avail, NULL);
	while(avail > 0 || bread == m_Buffer.size() - 1) {
		ReadFile(m_Out.read, m_Buffer.data(), m_Buffer.size() - 1, &bread, NULL);	 // read the stdout pipe
		psl::cout << psl::to_platform_string(psl::string8_t {m_Buffer.data(), bread});
		result += psl::string8_t {m_Buffer.data(), bread};

		memset(m_Buffer.data(), '\0', m_Buffer.size());

		if(bread > avail)
			avail = 0;
		else
			avail -= bread;
	}
	return result;
}
void bash_terminal::exec(psl::string_view command) {
	unsigned long exit = 0;					   // process exit code
	unsigned long bwrite;					   // bytes written
	GetExitCodeProcess(pi.hProcess, &exit);	   // while the process is running
	if(exit != STILL_ACTIVE)
		return;

	// psl::string cmd = psl::string(command) + _T('\n');
	// command = cmd;
	auto cmd = psl::to_platform_string(psl::string {command} + '\n');

	// memcpy_s((void*)&m_Buffer[0], m_Buffer.size(), (void*)&command[0], command.size());
	WriteFile(m_In.write, &cmd[0], sizeof(psl::platform_char_t) * cmd.size(), &bwrite, NULL);	 // send it to stdin
	return;
}
