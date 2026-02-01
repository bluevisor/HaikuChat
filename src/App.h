#ifndef APP_H
#define APP_H

#include <Application.h>

#include "MainWindow.h"
#include "Settings.h"

class App : public BApplication {
public:
						App();
	virtual				~App();

	virtual void		ArgvReceived(int32 argc, char** argv);
	virtual void		ReadyToRun();
	virtual void		AboutRequested();

private:
	Settings*			fSettings;
	MainWindow*			fMainWindow;
};

#endif // APP_H
