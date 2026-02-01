#include "Log.h"

#include <ctime>

bool gLoggingEnabled = false;

void
InitLogging(bool enabled)
{
	gLoggingEnabled = enabled;
	if (enabled) {
		printf("=== HaikuChat Logging Enabled ===\n");
		time_t now = time(NULL);
		printf("Started: %s", ctime(&now));
		printf("=================================\n\n");
	}
}


void
Log(const char* format, ...)
{
	if (!gLoggingEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("[INFO] ");
	vprintf(format, args);
	printf("\n");
	fflush(stdout);
	va_end(args);
}


void
LogError(const char* format, ...)
{
	if (!gLoggingEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("[ERROR] ");
	vprintf(format, args);
	printf("\n");
	fflush(stdout);
	va_end(args);
}


void
LogDebug(const char* format, ...)
{
	if (!gLoggingEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("[DEBUG] ");
	vprintf(format, args);
	printf("\n");
	fflush(stdout);
	va_end(args);
}
