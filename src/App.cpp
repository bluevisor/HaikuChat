#include "App.h"

#include <Alert.h>
#include <cstring>

#include "Constants.h"
#include "Log.h"

App::App()
	:
	BApplication("application/x-vnd.HaikuChat"),
	fSettings(NULL),
	fMainWindow(NULL)
{
	fSettings = new Settings();
	status_t status = fSettings->Load();
	if (status != B_OK) {
		LOG("Settings load returned: %s (this is normal on first run)",
			strerror(status));
	}
}


App::~App()
{
	delete fSettings;
}


void
App::ArgvReceived(int32 argc, char** argv)
{
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-log") == 0 || strcmp(argv[i], "--log") == 0) {
			InitLogging(true);
			LOG("Logging enabled via command line");
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("HaikuChat - Native AI chat client for Haiku\n\n");
			printf("Usage: HaikuChat [options]\n\n");
			printf("Options:\n");
			printf("  -log, --log    Enable console logging for debugging\n");
			printf("  -h, --help     Show this help message\n");
			printf("\n");
			printf("Supports OpenAI, Claude, and Gemini APIs.\n");
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
	}
}


void
App::ReadyToRun()
{
	LOG("App::ReadyToRun - Creating main window");

	// Initialize theme from settings
	SetDarkTheme(fSettings->IsDarkTheme());
	LOG("Theme initialized: %s", fSettings->IsDarkTheme() ? "dark" : "light");

	fMainWindow = new MainWindow(fSettings);
	fMainWindow->Show();
	LOG("Main window shown");
}


void
App::AboutRequested()
{
	BAlert* alert = new BAlert("About HaikuChat",
		"HaikuChat\n\n"
		"A native AI chat client for Haiku.\n"
		"Supports OpenAI, Claude, and Gemini APIs.\n\n"
		"Version 1.0\n\n"
		"Run with -log flag for debug output.",
		"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go();
}


int
main(int argc, char** argv)
{
	// Check for logging flag before creating app
	bool enableLogging = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-log") == 0 || strcmp(argv[i], "--log") == 0) {
			enableLogging = true;
			break;
		}
	}
	InitLogging(enableLogging);

	App app;
	app.Run();
	return 0;
}
