#include <Windows.h>
#include <powrprof.h>
#include <stdio.h>

#define PROGRAM_NAME "toast"

static HANDLE system_wakeup_timer (long long *sec) {
	
	SYSTEMTIME stLoc, stUtc;
	FILETIME ftUtc;

	GetLocalTime (&stLoc);
	if (!TzSpecificLocalTimeToSystemTime (NULL, &stLoc, &stUtc)) {
		puts ("Fail to convert local time to Utc time.");
		return NULL;
	}
	if (!SystemTimeToFileTime (&stUtc, &ftUtc)) {
		puts ("Fail to convert Utc time to file time.");
		return NULL;
	}
	
	HANDLE hTimer = NULL;
    LARGE_INTEGER dueTime = {
		.u.LowPart = ftUtc.dwLowDateTime,
		.u.HighPart = ftUtc.dwHighDateTime
	};
    dueTime.QuadPart += (*sec * 10000000LL);
	
	if ((hTimer = CreateWaitableTimerW (NULL, TRUE, NULL)) == NULL) {
		puts ("Fail to create waitable timer.");
		return NULL;
	}
	else if (GetLastError () == ERROR_NOT_SUPPORTED) {
		puts ("Wake timers are not supported by hardware.");
		return NULL;
	}
	
	if (!SetWaitableTimer (hTimer, &dueTime, 0, NULL, NULL, TRUE)) {
		puts ("Fail to set waitable timer.");
		return NULL;
	}
	return hTimer;
}

static int system_suspend (int state) {
	
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	
	/* Get a token for this process. */
	if (!OpenProcessToken (GetCurrentProcess (), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return -1; 
	  
	/* Get a LUID for shutdown privilege */
	LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME,
		&tkp.Privileges[0].Luid);
		
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
	
	/* Get the shutdown privilege for this process. */
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 
	
	CloseHandle (hToken);
	
	if (GetLastError() != ERROR_SUCCESS) {
		puts ("Fail to get SE_SHUTDOWN_NAME privilege.");
		return -1;
	}
	
	/* hibernate system */
	if (!SetSuspendState ((state > 3)? TRUE: FALSE, 0, FALSE)) {
		puts ("Fail to hibernate system.");
		return -1;
	}
	
	return 0;
}

#define USAGE() \
	puts ("Usage: "PROGRAM_NAME" [-f] <min> <min>"), 0

int main (int argc, char **argv) {
	long long sec[2] = {0LL,0LL};
	HANDLE hTimer = NULL;
	
	if (argc <= 2)
		return USAGE ();
	
	fputs (PROGRAM_NAME": ", stdout);
	
	if (argv[1][0] == '-' || argv[0][0] == '-') {
		puts ("time error.");
		return -1;
	}
	
	sscanf (argv[1], "%lld", sec+0);
	sscanf (argv[2], "%lld", sec+1);
	printf ("wait %lld min, block %lld min", sec[0], sec[1]);
	sec[0] *= 60LL;
	sec[1] *= 60LL;
	
	if (sec[0] == 0LL || sec[1] == 0LL) {
		puts (", time error.");
		return -1;
	}
	
	// for (int i = 0; i < 2; i++) {
		// sscanf (argv[1 + i], "%ld%c", sec + i, suffix + i);
		// switch (suffix[i]) {
			// case 's': break;
			// case 'm': sec[i] *= 60LL; break;
			// case 'h': sec[i] *= 3600LL; break;
			// case 'd': sec[i] *= (3600LL * 24LL); break;
			// default:
				// printf ("No sush suffix \'%c\'\n", suffix[i]);
				// return -1;
		// }
		// if (sec[i] <= 0LL) {
			// printf ("Fail to %s %ld min.",i ? "block" : "wait", sec[i] / 60LL);
			// return -1;
		// }
		// printf ("%s for %ld min...\n",i ? "Block" : "Wait", sec[i] / 60LL);
	// }
	
	
	// Process wait ======================================
	if ((hTimer = system_wakeup_timer (sec+0)) == NULL)
		return -1;
	if (WaitForSingleObject (hTimer, INFINITE) != WAIT_OBJECT_0) {
		puts (", fail to wait.");
		return -1;
	}
	fputs (", ", stdout);
	
	// System sleep ======================================
	if ((hTimer = system_wakeup_timer (sec + 1)) == NULL)
		return -1;
	if (system_suspend (4) != 0)
		return -1;
	if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
		puts ("fail to wait timer.");
		return -1;
	}
	puts ("finish block.");
	
	// wake 1 min
	for (int i=0; i<1200; i++) {
		if (SetThreadExecutionState (ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED) == 0) {
			puts ("fail to keep system wake up.");
			return -1;
		}
		SendMessage (HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, -1);
		Sleep (50);
	}
	CloseHandle (hTimer);
	return 0;
}