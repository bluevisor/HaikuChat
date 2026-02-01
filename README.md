# HaikuChat

A native AI chat client for Haiku OS with support for OpenAI, Claude, and Gemini APIs.

## Features

### Multi-Provider Support
- **OpenAI-compatible** endpoints (GPT-4, GPT-3.5, etc.)
- **Claude** (Anthropic) API support
- **Gemini** (Google) API support
- Per-provider API keys, endpoints, and model configurations
- Model caching to avoid redundant API calls

### Chat Interface
- **Modern ChatGPT-style UI** with message bubbles
- **Collapsible sidebar** with chat history
- **Real-time message streaming** with live formatting
- **Markdown support** including:
  - Bold (`**text**`), Italic (`*text*`)
  - Code blocks with syntax highlighting (` ``` `)
  - Inline code (`` `code` ``)
  - Headers (`# H1` through `###### H6`)
  - Bullet points (`- item`)
- **Auto-scrolling** to latest messages
- **Dark and Light themes** with real-time toggle

### Chat Management
- **Multiple chat sessions** with persistent storage
- **Chat history** in sidebar showing latest conversations
- **Automatic session saving** to local filesystem
- **New chat** button to start fresh conversations
- **Delete chat** functionality

### Settings
- **Theme selection** (Dark/Light) with real-time updates
- **Per-provider configuration**:
  - API keys (hidden during input)
  - API endpoints (customizable)
  - Model selection with "Fetch Models" button
- **Model caching** to reduce API calls

### Developer Features
- **Console logging** with `-log` flag for debugging
- **Help system** with `-h` or `--help` flags
- **Persistent settings** stored in `~/.config/settings/HaikuChat/`

## Building

### Requirements
- Haiku OS (R1 or later)
- GCC compiler with C++11 support
- Haiku development libraries

### Build Instructions
```bash
cd /boot/home/Dev/chat
make clean
make
```

### Running
```bash
./HaikuChat              # Normal mode
./HaikuChat -log         # With debug logging
./HaikuChat -h           # Show help
```

## Architecture

### Core Components

**MainWindow** - Main application window
- Manages the split view layout
- Handles message sending and receiving
- Manages chat sessions
- Theme refresh coordination

**ChatView** - Message display area
- Scrollable area for chat bubbles
- Auto-layout and scrolling to bottom
- Streaming message updates

**MessageBubble** - Individual message display
- User vs assistant message styling
- Markdown formatting (bold, italic, code, headers, lists)
- Streaming support with incomplete formatting
- Proper text wrapping

**SidebarView** - Chat history sidebar
- List of saved chat sessions
- New Chat button
- User profile area
- Collapsible with toggle button

**InputView** - Message input field
- Resizable text input with word wrap
- Dynamic height adjustment
- Send button with keyboard shortcut (Cmd+Enter)

**LLMClient** - API communication
- Streaming response handling via Server-Sent Events (SSE)
- Per-provider request formatting
- Error handling with helpful messages
- Model listing support

**Settings** - Configuration management
- Per-provider API keys, endpoints, models
- Theme preference
- Window frame and layout state
- Chat session management
- Model caching

### File Organization
```
src/
├── App.cpp/h              # Application entry point
├── MainWindow.cpp/h       # Main window and layout
├── ChatView.cpp/h         # Message display
├── MessageBubble.cpp/h    # Individual message
├── InputView.cpp/h        # Message input
├── SidebarView.cpp/h      # Chat history sidebar
├── LLMClient.cpp/h        # API communication
├── ChatSession.cpp/h      # Chat session data
├── ChatMessage.cpp/h      # Message data
├── Settings.cpp/h         # Settings storage
├── Theme.cpp              # Theme management
├── Constants.h            # Colors, sizes, message codes
├── Log.cpp/h              # Debug logging
└── Makefile

resources/
└── chat.rdef              # Application resources
```

## Data Storage

### Settings Directory
```
~/.config/settings/HaikuChat/
├── settings               # Main settings file (BMessage format)
└── chats/
    ├── chat_1234567890_0.chat
    ├── chat_1234567891_0.chat
    └── ...
```

### Settings Format
- API keys and endpoints per provider
- Theme preference (dark/light)
- Window frame dimensions
- Sidebar collapsed state
- Current active chat session
- Model cache per provider

### Chat Session Format
- Session ID and title
- Message list (role + content)
- Creation and update timestamps

## UI Customization

### Colors
- **Dark Theme**: Dark backgrounds with light text
- **Light Theme**: Light backgrounds with dark text
- **Accent Color**: Green (#10a37f) for highlights and buttons
- **Code Blocks**: Monospace font with dimmed background

### Layout
- **Sidebar Width**: 260px (collapsible)
- **Top Bar Height**: 48px
- **Chat Item Height**: 48px
- **Input Field Min Height**: 52px
- **Message Bubble Max Width**: 75% of window width
- **Bubble Radius**: 12px with rounded corners

## Keyboard Shortcuts
- **Cmd+Enter** or **Ctrl+Enter**: Send message
- **Esc**: Close settings window
- **⌘ (Menu)**: Toggle sidebar

## Known Limitations

1. **Markdown rendering** - Basic implementation without full CommonMark support
2. **Code syntax highlighting** - No language-specific highlighting in code blocks
3. **Rate limiting** - No built-in rate limit handling (relies on API)
4. **Model availability** - Model list depends on API provider availability
5. **File uploads** - Not supported (API-based only)

## Configuration

### API Keys
Store API keys in the Settings dialog:
1. Click the ⚙ (Settings) button in the top right
2. Select your provider (OpenAI, Claude, or Gemini)
3. Enter your API key
4. Optionally customize the endpoint URL
5. Click "Save"

### Model Selection
1. Open Settings
2. Enter your API key for the provider
3. Click "Fetch Models"
4. Select desired model from the list
5. Click "Save"

Models are cached locally, so subsequent starts won't need to fetch again.

## Troubleshooting

### "API key" error
- Verify your API key is correct in Settings
- Check that you've selected the right provider
- Ensure the key has necessary permissions

### Model not found
- Click "Fetch Models" in Settings to get the latest list
- Verify the model name matches the API provider's available models

### Messages not streaming
- Check internet connection
- Verify API endpoint is correct
- Check application logs with `-log` flag

### Theme not updating
- Theme changes take effect after saving Settings
- If stuck, restart the application

## Development

### Adding a New Provider
1. Add provider enum to `Constants.h`
2. Add default endpoint to `Settings::Settings()`
3. Update `LLMClient::FetchModels()` and `SendChatRequest()`
4. Update settings UI in `SettingsWindow.cpp`
5. Add provider to model selection UI

### Adding New Markdown Features
1. Update `MessageBubble::_ApplyMarkdown()`
2. Add style definitions for new format
3. Test with streaming messages

### Debugging
Run with `-log` flag to see debug output:
```bash
./HaikuChat -log 2>&1 | tee chat.log
```

## License

[Add your license here]

## Credits

HaikuChat - Native AI chat client for Haiku OS
