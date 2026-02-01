#include "LLMClient.h"

#include <HttpHeaders.h>
#include <HttpRequest.h>
#include <UrlProtocolRoster.h>

#include <cstdio>
#include <cstdlib>

#include "Log.h"

using namespace BPrivate::Network;

// StreamingOutput implementation

StreamingOutput::StreamingOutput(LLMClient* client)
	:
	fClient(client)
{
}


StreamingOutput::~StreamingOutput()
{
}


ssize_t
StreamingOutput::Write(const void* buffer, size_t size)
{
	if (fClient != NULL)
		fClient->HandleDataReceived(static_cast<const char*>(buffer), size);
	return size;
}


// CollectingOutput implementation

CollectingOutput::CollectingOutput()
{
}


CollectingOutput::~CollectingOutput()
{
}


ssize_t
CollectingOutput::Write(const void* buffer, size_t size)
{
	fData.Append(static_cast<const char*>(buffer), size);
	return size;
}


// LLMProtocolListener implementation

LLMProtocolListener::LLMProtocolListener(LLMClient* client)
	:
	fClient(client)
{
}


LLMProtocolListener::~LLMProtocolListener()
{
}


void
LLMProtocolListener::RequestCompleted(BUrlRequest* caller, bool success)
{
	if (fClient != NULL)
		fClient->HandleRequestCompleted(success);
}


// ModelsProtocolListener implementation

ModelsProtocolListener::ModelsProtocolListener(LLMClient* client)
	:
	fClient(client)
{
}


ModelsProtocolListener::~ModelsProtocolListener()
{
}


void
ModelsProtocolListener::RequestCompleted(BUrlRequest* caller, bool success)
{
	if (fClient != NULL)
		fClient->HandleModelsRequestCompleted(success);
}


// LLMClient implementation

LLMClient::LLMClient(BMessenger target)
	:
	BLooper("LLMClient"),
	fTarget(target),
	fRequest(NULL),
	fModelsRequest(NULL),
	fListener(NULL),
	fModelsListener(NULL),
	fOutput(NULL),
	fModelsOutput(NULL),
	fCurrentApiType(kApiTypeOpenAI),
	fCancelled(false)
{
	fListener = new LLMProtocolListener(this);
	fModelsListener = new ModelsProtocolListener(this);
	fOutput = new StreamingOutput(this);
	fModelsOutput = new CollectingOutput();
	Run();
}


LLMClient::~LLMClient()
{
	Cancel();
	delete fListener;
	delete fModelsListener;
	delete fOutput;
	delete fModelsOutput;
}


void
LLMClient::MessageReceived(BMessage* message)
{
	BLooper::MessageReceived(message);
}


void
LLMClient::SendChatRequest(const char* messagesJson, ApiType apiType,
	const char* endpoint, const char* apiKey, const char* model)
{
	const char* apiNames[] = {"OpenAI", "Claude", "Gemini"};
	LOG("LLMClient::SendChatRequest - API: %s, Model: %s, Endpoint: %s",
		apiNames[apiType], model, endpoint);

	// Cancel any existing request
	Cancel();

	fCurrentApiType = apiType;
	fCancelled = false;
	fBuffer = "";
	fEventType = "";
	fGeminiBuffer = "";

	BString url(endpoint);
	BString body;

	if (apiType == kApiTypeOpenAI) {
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("chat/completions");

		body.SetToFormat(
			"{"
			"\"model\": \"%s\","
			"\"messages\": %s,"
			"\"stream\": true"
			"}",
			model, messagesJson);
	} else if (apiType == kApiTypeClaude) {
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("messages");

		body.SetToFormat(
			"{"
			"\"model\": \"%s\","
			"\"max_tokens\": 4096,"
			"\"messages\": %s,"
			"\"stream\": true"
			"}",
			model, messagesJson);
	} else if (apiType == kApiTypeGemini) {
		// Gemini uses a different URL structure
		// https://generativelanguage.googleapis.com/v1beta/models/{model}:streamGenerateContent?key=API_KEY
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("models/");
		url.Append(model);
		url.Append(":streamGenerateContent?alt=sse&key=");
		url.Append(apiKey);

		// Convert messages to Gemini format
		// Gemini uses "contents" with "parts" instead of "messages"
		body = "{\"contents\": [";

		// Parse messagesJson to convert format
		// Simple approach: just wrap the content
		BString msgJson(messagesJson);
		int32 pos = 0;
		bool first = true;

		while (pos < msgJson.Length()) {
			int32 rolePos = msgJson.FindFirst("\"role\"", pos);
			if (rolePos < 0) break;

			int32 contentPos = msgJson.FindFirst("\"content\"", rolePos);
			if (contentPos < 0) break;

			// Extract role
			int32 roleStart = msgJson.FindFirst("\"", rolePos + 6);
			int32 roleEnd = msgJson.FindFirst("\"", roleStart + 1);
			BString role;
			msgJson.CopyInto(role, roleStart + 1, roleEnd - roleStart - 1);

			// Extract content
			int32 contentStart = msgJson.FindFirst("\"", contentPos + 9);
			int32 contentEnd = contentStart + 1;
			while (contentEnd < msgJson.Length()) {
				if (msgJson[contentEnd] == '\\') {
					contentEnd += 2;
					continue;
				}
				if (msgJson[contentEnd] == '"')
					break;
				contentEnd++;
			}
			BString content;
			msgJson.CopyInto(content, contentStart + 1, contentEnd - contentStart - 1);

			// Map roles: user->user, assistant->model
			BString geminiRole = (role == "assistant") ? "model" : "user";

			if (!first) body.Append(",");
			first = false;

			body.Append("{\"role\":\"");
			body.Append(geminiRole);
			body.Append("\",\"parts\":[{\"text\":\"");
			body.Append(content);
			body.Append("\"}]}");

			pos = contentEnd + 1;
		}

		body.Append("]}");
	}

	// Create request using BUrlProtocolRoster with streaming output
	fRequest = BUrlProtocolRoster::MakeRequest(BUrl(url.String()),
		fOutput, fListener, NULL);

	if (fRequest == NULL) {
		_SendError("Failed to create HTTP request");
		return;
	}

	// Cast to BHttpRequest to set HTTP-specific options
	BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(fRequest);
	if (httpRequest == NULL) {
		delete fRequest;
		fRequest = NULL;
		_SendError("Invalid HTTP request");
		return;
	}

	httpRequest->SetMethod(B_HTTP_POST);

	// Set headers
	BHttpHeaders* headers = new BHttpHeaders();
	headers->AddHeader("Content-Type", "application/json");

	if (apiType == kApiTypeOpenAI) {
		BString authHeader;
		authHeader.SetToFormat("Bearer %s", apiKey);
		headers->AddHeader("Authorization", authHeader.String());
	} else if (apiType == kApiTypeClaude) {
		headers->AddHeader("x-api-key", apiKey);
		headers->AddHeader("anthropic-version", "2023-06-01");
	}
	// Gemini uses API key in URL, no auth header needed

	httpRequest->AdoptHeaders(headers);

	// Set body
	BMallocIO* bodyData = new BMallocIO();
	bodyData->Write(body.String(), body.Length());
	bodyData->Seek(0, SEEK_SET);
	httpRequest->AdoptInputData(bodyData, body.Length());

	// Run request in background thread
	fRequest->Run();
}


void
LLMClient::FetchModels(ApiType apiType, const char* endpoint, const char* apiKey)
{
	const char* apiNames[] = {"OpenAI", "Claude", "Gemini"};
	LOG("LLMClient::FetchModels - API: %s, Endpoint: %s", apiNames[apiType], endpoint);

	// Cancel any existing models request
	if (fModelsRequest != NULL) {
		fModelsRequest->Stop();
		delete fModelsRequest;
		fModelsRequest = NULL;
	}

	fModelsOutput->Clear();
	fCurrentApiType = apiType;

	BString url(endpoint);

	if (apiType == kApiTypeOpenAI) {
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("models");
	} else if (apiType == kApiTypeClaude) {
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("models");
	} else if (apiType == kApiTypeGemini) {
		if (!url.EndsWith("/"))
			url.Append("/");
		url.Append("models?key=");
		url.Append(apiKey);
	}

	fModelsRequest = BUrlProtocolRoster::MakeRequest(BUrl(url.String()),
		fModelsOutput, fModelsListener, NULL);

	if (fModelsRequest == NULL) {
		_SendError("Failed to create models request");
		return;
	}

	BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(fModelsRequest);
	if (httpRequest == NULL) {
		delete fModelsRequest;
		fModelsRequest = NULL;
		_SendError("Invalid HTTP request for models");
		return;
	}

	httpRequest->SetMethod(B_HTTP_GET);

	BHttpHeaders* headers = new BHttpHeaders();

	if (apiType == kApiTypeOpenAI) {
		BString authHeader;
		authHeader.SetToFormat("Bearer %s", apiKey);
		headers->AddHeader("Authorization", authHeader.String());
	} else if (apiType == kApiTypeClaude) {
		headers->AddHeader("x-api-key", apiKey);
		headers->AddHeader("anthropic-version", "2023-06-01");
	}

	httpRequest->AdoptHeaders(headers);
	fModelsRequest->Run();
}


void
LLMClient::Cancel()
{
	fCancelled = true;
	if (fRequest != NULL) {
		fRequest->Stop();
		delete fRequest;
		fRequest = NULL;
	}
}


void
LLMClient::HandleDataReceived(const char* data, ssize_t size)
{
	if (fCancelled)
		return;

	BString chunk(data, size);
	fBuffer.Append(chunk);

	// Check for error response early (before processing as stream)
	if (fBuffer.FindFirst("\"error\"") >= 0 && fBuffer.FindFirst("\"message\"") >= 0) {
		// Try to extract error message
		int32 msgPos = fBuffer.FindFirst("\"message\"");
		if (msgPos >= 0) {
			int32 colonPos = fBuffer.FindFirst(":", msgPos);
			int32 quoteStart = fBuffer.FindFirst("\"", colonPos);
			int32 quoteEnd = quoteStart + 1;
			while (quoteEnd < fBuffer.Length() && fBuffer[quoteEnd] != '"')
				quoteEnd++;
			if (quoteStart >= 0 && quoteEnd > quoteStart) {
				BString errorMsg;
				fBuffer.CopyInto(errorMsg, quoteStart + 1, quoteEnd - quoteStart - 1);
				LOG_ERROR("API error response: %s", errorMsg.String());
				_SendError(errorMsg.String());
				fBuffer = "";
				fCancelled = true;
				return;
			}
		}
	}

	// Process complete lines
	int32 pos;
	while ((pos = fBuffer.FindFirst('\n')) >= 0) {
		BString line;
		fBuffer.MoveInto(line, 0, pos + 1);
		line.RemoveAll("\r");
		line.RemoveAll("\n");

		if (line.Length() == 0)
			continue;

		if (fCurrentApiType == kApiTypeOpenAI) {
			_ProcessOpenAIChunk(line);
		} else if (fCurrentApiType == kApiTypeClaude) {
			_ProcessClaudeChunk(line);
		} else if (fCurrentApiType == kApiTypeGemini) {
			_ProcessGeminiChunk(line);
		}
	}
}


void
LLMClient::HandleRequestCompleted(bool success)
{
	LOG("LLMClient::HandleRequestCompleted - success=%s, cancelled=%s",
		success ? "true" : "false", fCancelled ? "true" : "false");

	if (!success && !fCancelled) {
		LOG_ERROR("Request failed");
		_SendError("Request failed - check your API key and network connection");
	}
	_SendDone();

	delete fRequest;
	fRequest = NULL;
}


void
LLMClient::HandleModelsRequestCompleted(bool success)
{
	LOG("LLMClient::HandleModelsRequestCompleted - success=%s", success ? "true" : "false");

	if (!success) {
		LOG_ERROR("Failed to fetch models");
		_SendError("Failed to fetch models - check your API key and endpoint");
		delete fModelsRequest;
		fModelsRequest = NULL;
		return;
	}

	const BString& json = fModelsOutput->Data();
	LOG_DEBUG("Models response length: %d bytes", json.Length());

	// Check for error response
	if (json.FindFirst("\"error\"") >= 0) {
		LOG_ERROR("API returned error response: %s", json.String());
		// Try to extract error message
		int32 msgPos = json.FindFirst("\"message\"");
		if (msgPos >= 0) {
			int32 colonPos = json.FindFirst(":", msgPos);
			int32 quoteStart = json.FindFirst("\"", colonPos);
			int32 quoteEnd = json.FindFirst("\"", quoteStart + 1);
			if (quoteStart >= 0 && quoteEnd > quoteStart) {
				BString errorMsg;
				json.CopyInto(errorMsg, quoteStart + 1, quoteEnd - quoteStart - 1);
				_SendError(errorMsg.String());
				delete fModelsRequest;
				fModelsRequest = NULL;
				return;
			}
		}
		_SendError("API returned an error - check your API key");
		delete fModelsRequest;
		fModelsRequest = NULL;
		return;
	}

	if (fCurrentApiType == kApiTypeOpenAI) {
		_ParseOpenAIModels(json);
	} else if (fCurrentApiType == kApiTypeClaude) {
		_ParseClaudeModels(json);
	} else if (fCurrentApiType == kApiTypeGemini) {
		_ParseGeminiModels(json);
	}

	delete fModelsRequest;
	fModelsRequest = NULL;
}


void
LLMClient::_ProcessOpenAIChunk(const BString& line)
{
	if (!line.StartsWith("data: "))
		return;

	BString data = line;
	data.Remove(0, 6);

	if (data == "[DONE]") {
		return;
	}

	int32 deltaPos = data.FindFirst("\"delta\"");
	if (deltaPos < 0)
		return;

	int32 contentPos = data.FindFirst("\"content\"", deltaPos);
	if (contentPos < 0)
		return;

	int32 colonPos = data.FindFirst(":", contentPos + 9);
	if (colonPos < 0)
		return;

	int32 quoteStart = data.FindFirst("\"", colonPos);
	if (quoteStart < 0)
		return;

	int32 quoteEnd = quoteStart + 1;
	while (quoteEnd < data.Length()) {
		if (data[quoteEnd] == '\\') {
			quoteEnd += 2;
			continue;
		}
		if (data[quoteEnd] == '"')
			break;
		quoteEnd++;
	}

	if (quoteEnd >= data.Length())
		return;

	BString content;
	data.CopyInto(content, quoteStart + 1, quoteEnd - quoteStart - 1);

	content.ReplaceAll("\\n", "\n");
	content.ReplaceAll("\\t", "\t");
	content.ReplaceAll("\\\"", "\"");
	content.ReplaceAll("\\\\", "\\");

	if (content.Length() > 0)
		_SendChunk(content.String());
}


void
LLMClient::_ProcessClaudeChunk(const BString& line)
{
	if (line.StartsWith("event: ")) {
		BString event = line;
		event.Remove(0, 7);
		fEventType = event;
		return;
	}

	if (!line.StartsWith("data: "))
		return;

	BString data = line;
	data.Remove(0, 6);

	if (fEventType == "content_block_delta") {
		int32 textPos = data.FindFirst("\"text\"");
		if (textPos < 0)
			return;

		int32 colonPos = data.FindFirst(":", textPos + 6);
		if (colonPos < 0)
			return;

		int32 quoteStart = data.FindFirst("\"", colonPos);
		if (quoteStart < 0)
			return;

		int32 quoteEnd = quoteStart + 1;
		while (quoteEnd < data.Length()) {
			if (data[quoteEnd] == '\\') {
				quoteEnd += 2;
				continue;
			}
			if (data[quoteEnd] == '"')
				break;
			quoteEnd++;
		}

		if (quoteEnd >= data.Length())
			return;

		BString content;
		data.CopyInto(content, quoteStart + 1, quoteEnd - quoteStart - 1);

		content.ReplaceAll("\\n", "\n");
		content.ReplaceAll("\\t", "\t");
		content.ReplaceAll("\\\"", "\"");
		content.ReplaceAll("\\\\", "\\");

		if (content.Length() > 0)
			_SendChunk(content.String());
	}
}


void
LLMClient::_ProcessGeminiChunk(const BString& line)
{
	// Gemini SSE format: data: {"candidates":[{"content":{"parts":[{"text":"..."}]}}]}
	if (!line.StartsWith("data: "))
		return;

	BString data = line;
	data.Remove(0, 6);

	// Find "text" field in the response
	int32 textPos = data.FindFirst("\"text\"");
	if (textPos < 0)
		return;

	int32 colonPos = data.FindFirst(":", textPos + 6);
	if (colonPos < 0)
		return;

	int32 quoteStart = data.FindFirst("\"", colonPos);
	if (quoteStart < 0)
		return;

	int32 quoteEnd = quoteStart + 1;
	while (quoteEnd < data.Length()) {
		if (data[quoteEnd] == '\\') {
			quoteEnd += 2;
			continue;
		}
		if (data[quoteEnd] == '"')
			break;
		quoteEnd++;
	}

	if (quoteEnd >= data.Length())
		return;

	BString content;
	data.CopyInto(content, quoteStart + 1, quoteEnd - quoteStart - 1);

	content.ReplaceAll("\\n", "\n");
	content.ReplaceAll("\\t", "\t");
	content.ReplaceAll("\\\"", "\"");
	content.ReplaceAll("\\\\", "\\");

	if (content.Length() > 0)
		_SendChunk(content.String());
}


void
LLMClient::_ParseOpenAIModels(const BString& json)
{
	BObjectList<BString> models(20, true);

	// Parse {"data":[{"id":"model-name",...},...]
	int32 pos = 0;
	while ((pos = json.FindFirst("\"id\"", pos)) >= 0) {
		int32 colonPos = json.FindFirst(":", pos + 4);
		if (colonPos < 0) break;

		int32 quoteStart = json.FindFirst("\"", colonPos);
		if (quoteStart < 0) break;

		int32 quoteEnd = quoteStart + 1;
		while (quoteEnd < json.Length() && json[quoteEnd] != '"')
			quoteEnd++;

		if (quoteEnd < json.Length()) {
			BString* modelId = new BString();
			json.CopyInto(*modelId, quoteStart + 1, quoteEnd - quoteStart - 1);
			// Filter to only include chat models
			if (modelId->FindFirst("gpt") >= 0 ||
				modelId->FindFirst("o1") >= 0 ||
				modelId->FindFirst("o3") >= 0 ||
				modelId->FindFirst("chatgpt") >= 0) {
				models.AddItem(modelId);
			} else {
				delete modelId;
			}
		}

		pos = quoteEnd + 1;
	}

	_SendModels(models);
}


void
LLMClient::_ParseClaudeModels(const BString& json)
{
	BObjectList<BString> models(20, true);

	// Claude API returns {"data":[{"id":"claude-...","type":"model",...},...]}
	int32 pos = 0;
	while ((pos = json.FindFirst("\"id\"", pos)) >= 0) {
		int32 colonPos = json.FindFirst(":", pos + 4);
		if (colonPos < 0) break;

		int32 quoteStart = json.FindFirst("\"", colonPos);
		if (quoteStart < 0) break;

		int32 quoteEnd = quoteStart + 1;
		while (quoteEnd < json.Length() && json[quoteEnd] != '"')
			quoteEnd++;

		if (quoteEnd < json.Length()) {
			BString* modelId = new BString();
			json.CopyInto(*modelId, quoteStart + 1, quoteEnd - quoteStart - 1);
			if (modelId->FindFirst("claude") >= 0) {
				models.AddItem(modelId);
			} else {
				delete modelId;
			}
		}

		pos = quoteEnd + 1;
	}

	_SendModels(models);
}


void
LLMClient::_ParseGeminiModels(const BString& json)
{
	BObjectList<BString> models(20, true);

	// Gemini API returns {"models":[{"name":"models/gemini-...","displayName":"..."},...]}
	int32 pos = 0;
	while ((pos = json.FindFirst("\"name\"", pos)) >= 0) {
		int32 colonPos = json.FindFirst(":", pos + 6);
		if (colonPos < 0) break;

		int32 quoteStart = json.FindFirst("\"", colonPos);
		if (quoteStart < 0) break;

		int32 quoteEnd = quoteStart + 1;
		while (quoteEnd < json.Length() && json[quoteEnd] != '"')
			quoteEnd++;

		if (quoteEnd < json.Length()) {
			BString modelName;
			json.CopyInto(modelName, quoteStart + 1, quoteEnd - quoteStart - 1);

			// Remove "models/" prefix if present
			if (modelName.StartsWith("models/")) {
				modelName.Remove(0, 7);
			}

			// Only include generative models
			if (modelName.FindFirst("gemini") >= 0) {
				BString* modelId = new BString(modelName);
				models.AddItem(modelId);
			}
		}

		pos = quoteEnd + 1;
	}

	_SendModels(models);
}


void
LLMClient::_SendChunk(const char* text)
{
	BMessage msg(kMsgLLMChunk);
	msg.AddString("text", text);
	fTarget.SendMessage(&msg);
}


void
LLMClient::_SendError(const char* error)
{
	BMessage msg(kMsgLLMError);
	msg.AddString("error", error);
	fTarget.SendMessage(&msg);
}


void
LLMClient::_SendDone()
{
	BMessage msg(kMsgLLMDone);
	fTarget.SendMessage(&msg);
}


void
LLMClient::_SendModels(const BObjectList<BString>& models)
{
	BMessage msg(kMsgModelsReceived);
	for (int32 i = 0; i < models.CountItems(); i++) {
		msg.AddString("model", models.ItemAt(i)->String());
	}
	fTarget.SendMessage(&msg);
}
