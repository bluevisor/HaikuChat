#include "Constants.h"

// Current theme colors - initialized to dark theme by default
rgb_color kBackgroundColor = kDarkBackgroundColor;
rgb_color kSidebarColor = kDarkSidebarColor;
rgb_color kUserBubbleColor = kDarkUserBubbleColor;
rgb_color kAssistantBubbleColor = kDarkAssistantBubbleColor;
rgb_color kUserTextColor = kDarkUserTextColor;
rgb_color kAssistantTextColor = kDarkAssistantTextColor;
rgb_color kSidebarTextColor = kDarkSidebarTextColor;
rgb_color kSidebarHoverColor = kDarkSidebarHoverColor;
rgb_color kInputBackgroundColor = kDarkInputBackgroundColor;
rgb_color kBorderColor = kDarkBorderColor;

static bool sDarkTheme = true;


void
SetDarkTheme(bool dark)
{
	sDarkTheme = dark;

	if (dark) {
		kBackgroundColor = kDarkBackgroundColor;
		kSidebarColor = kDarkSidebarColor;
		kUserBubbleColor = kDarkUserBubbleColor;
		kAssistantBubbleColor = kDarkAssistantBubbleColor;
		kUserTextColor = kDarkUserTextColor;
		kAssistantTextColor = kDarkAssistantTextColor;
		kSidebarTextColor = kDarkSidebarTextColor;
		kSidebarHoverColor = kDarkSidebarHoverColor;
		kInputBackgroundColor = kDarkInputBackgroundColor;
		kBorderColor = kDarkBorderColor;
	} else {
		kBackgroundColor = kLightBackgroundColor;
		kSidebarColor = kLightSidebarColor;
		kUserBubbleColor = kLightUserBubbleColor;
		kAssistantBubbleColor = kLightAssistantBubbleColor;
		kUserTextColor = kLightUserTextColor;
		kAssistantTextColor = kLightAssistantTextColor;
		kSidebarTextColor = kLightSidebarTextColor;
		kSidebarHoverColor = kLightSidebarHoverColor;
		kInputBackgroundColor = kLightInputBackgroundColor;
		kBorderColor = kLightBorderColor;
	}
}


bool
IsDarkTheme()
{
	return sDarkTheme;
}
