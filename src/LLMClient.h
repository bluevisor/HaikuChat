#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <DataIO.h>
#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>
#include <UrlProtocolListener.h>
#include <UrlRequest.h>

#include "Constants.h"

using namespace BPrivate::Network;

class LLMClient;

// Custom output that forwards data to LLMClient as it arrives
class StreamingOutput : public BDataIO {
public:
						StreamingOutput(LLMClient* client);
	virtual				~StreamingOutput();

	virtual ssize_t		Write(const void* buffer, size_t size);

private:
	LLMClient*			fClient;
};


// Output that collects all data for non-streaming requests
class CollectingOutput : public BDataIO {
public:
						CollectingOutput();
	virtual				~CollectingOutput();

	virtual ssize_t		Write(const void* buffer, size_t size);
	const BString&		Data() const { return fData; }
	void				Clear() { fData = ""; }

private:
	BString				fData;
};


class LLMProtocolListener : public BUrlProtocolListener {
public:
						LLMProtocolListener(LLMClient* client);
	virtual				~LLMProtocolListener();

	virtual void		RequestCompleted(BUrlRequest* caller, bool success);

private:
	LLMClient*			fClient;
};


class ModelsProtocolListener : public BUrlProtocolListener {
public:
						ModelsProtocolListener(LLMClient* client);
	virtual				~ModelsProtocolListener();

	virtual void		RequestCompleted(BUrlRequest* caller, bool success);

private:
	LLMClient*			fClient;
};


class LLMClient : public BLooper {
public:
						LLMClient(BMessenger target);
	virtual				~LLMClient();

	virtual void		MessageReceived(BMessage* message);

	void				SendChatRequest(const char* messagesJson,
							ApiType apiType, const char* endpoint,
							const char* apiKey, const char* model);
	void				FetchModels(ApiType apiType, const char* endpoint,
							const char* apiKey);
	void				Cancel();

	void				HandleDataReceived(const char* data, ssize_t size);
	void				HandleRequestCompleted(bool success);
	void				HandleModelsRequestCompleted(bool success);

	CollectingOutput*	GetModelsOutput() { return fModelsOutput; }

private:
	void				_ProcessOpenAIChunk(const BString& line);
	void				_ProcessClaudeChunk(const BString& line);
	void				_ProcessGeminiChunk(const BString& line);
	void				_ParseOpenAIModels(const BString& json);
	void				_ParseClaudeModels(const BString& json);
	void				_ParseGeminiModels(const BString& json);
	void				_SendChunk(const char* text);
	void				_SendError(const char* error);
	void				_SendDone();
	void				_SendModels(const BObjectList<BString>& models);

	BMessenger			fTarget;
	BUrlRequest*		fRequest;
	BUrlRequest*		fModelsRequest;
	LLMProtocolListener* fListener;
	ModelsProtocolListener* fModelsListener;
	StreamingOutput*	fOutput;
	CollectingOutput*	fModelsOutput;
	ApiType				fCurrentApiType;
	bool				fCancelled;
	BString				fBuffer;
	BString				fEventType;
	BString				fGeminiBuffer;
};

#endif // LLM_CLIENT_H
