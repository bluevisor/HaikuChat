#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <GraphicsDefs.h>

// Message codes
enum {
	kMsgSendMessage = 'send',
	kMsgNewChat = 'ncha',
	kMsgClearChat = 'clch',
	kMsgShowSettings = 'sset',
	kMsgShowAbout = 'abot',
	kMsgSettingsSave = 'stsv',
	kMsgSettingsCancel = 'stcn',
	kMsgLLMChunk = 'llmc',
	kMsgLLMDone = 'llmd',
	kMsgLLMError = 'llme',
	kMsgInputChanged = 'inch',
	kMsgApiTypeChanged = 'aptp',
	kMsgFetchModels = 'ftmd',
	kMsgModelsReceived = 'mdrc',
	kMsgModelSelected = 'mdsl',
	kMsgToggleSidebar = 'tgsd',
	kMsgSelectChat = 'slch',
	kMsgDeleteChat = 'dlch',
	kMsgShowUser = 'shus',
	kMsgThemeChanged = 'thch'
};

// API Types
enum ApiType {
	kApiTypeOpenAI = 0,
	kApiTypeClaude = 1,
	kApiTypeGemini = 2
};

// ============== DARK THEME ==============
// Main areas
const rgb_color kDarkBackgroundColor = {33, 33, 33, 255};          // #212121
const rgb_color kDarkSidebarColor = {23, 23, 23, 255};             // #171717

// Message bubbles
const rgb_color kDarkUserBubbleColor = {47, 47, 47, 255};          // #2f2f2f
const rgb_color kDarkAssistantBubbleColor = {33, 33, 33, 255};     // Same as background

// Text colors
const rgb_color kDarkUserTextColor = {236, 236, 236, 255};         // #ececec
const rgb_color kDarkAssistantTextColor = {209, 213, 219, 255};    // #d1d5db
const rgb_color kDarkSidebarTextColor = {236, 236, 236, 255};

// Interactive elements
const rgb_color kDarkSidebarHoverColor = {47, 47, 47, 255};        // #2f2f2f
const rgb_color kDarkInputBackgroundColor = {47, 47, 47, 255};     // #2f2f2f
const rgb_color kDarkBorderColor = {64, 64, 64, 255};              // #404040

// ============== LIGHT THEME ==============
// Main areas
const rgb_color kLightBackgroundColor = {255, 255, 255, 255};      // White
const rgb_color kLightSidebarColor = {243, 244, 246, 255};         // #f3f4f6

// Message bubbles
const rgb_color kLightUserBubbleColor = {229, 231, 235, 255};      // #e5e7eb
const rgb_color kLightAssistantBubbleColor = {255, 255, 255, 255}; // White

// Text colors
const rgb_color kLightUserTextColor = {17, 24, 39, 255};           // #111827
const rgb_color kLightAssistantTextColor = {31, 41, 55, 255};      // #1f2937
const rgb_color kLightSidebarTextColor = {17, 24, 39, 255};

// Interactive elements
const rgb_color kLightSidebarHoverColor = {229, 231, 235, 255};    // #e5e7eb
const rgb_color kLightInputBackgroundColor = {249, 250, 251, 255}; // #f9fafb
const rgb_color kLightBorderColor = {209, 213, 219, 255};          // #d1d5db

// Accent color (same for both themes)
const rgb_color kAccentColor = {16, 163, 127, 255};                // #10a37f - Green

// Current theme colors (set by Theme class)
extern rgb_color kBackgroundColor;
extern rgb_color kSidebarColor;
extern rgb_color kUserBubbleColor;
extern rgb_color kAssistantBubbleColor;
extern rgb_color kUserTextColor;
extern rgb_color kAssistantTextColor;
extern rgb_color kSidebarTextColor;
extern rgb_color kSidebarHoverColor;
extern rgb_color kInputBackgroundColor;
extern rgb_color kBorderColor;

// Theme management
void SetDarkTheme(bool dark);
bool IsDarkTheme();

// Layout constants
const float kBubbleMaxWidthRatio = 0.75f;
const float kBubblePadding = 16.0f;
const float kBubbleMargin = 12.0f;
const float kBubbleRadius = 12.0f;
const float kInputMinHeight = 52.0f;
const float kInputMaxHeight = 200.0f;
const float kSendButtonWidth = 44.0f;
const float kSidebarWidth = 260.0f;
const float kSidebarCollapsedWidth = 0.0f;
const float kTopBarHeight = 48.0f;
const float kChatItemHeight = 48.0f;
const float kUserIconSize = 28.0f;

// Settings file paths
#define SETTINGS_DIR "/boot/home/config/settings/HaikuChat"
#define SETTINGS_FILE "settings"
#define CHATS_DIR "/boot/home/config/settings/HaikuChat/chats"

#endif // CONSTANTS_H
