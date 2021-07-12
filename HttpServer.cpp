#include <signal.h>
#include "HttpServer.h"

NFHttpServer::~NFHttpServer()
{
	mMsgCBMap.clear();
	mMsgFliterMap.clear();
}

void NFHttpServer::Execute()
{
	if (base)
	{
		event_base_loop(base, EVLOOP_ONCE | EVLOOP_NONBLOCK);
	}
}


int NFHttpServer::InitServer(const unsigned short port)
{
	struct evhttp* http;
	struct evhttp_bound_socket* handle;

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return -1;

	base = event_base_new();
	if (!base)
	{
		std::cout << "create event_base fail" << std::endl;;
		return -1;
	}

	http = evhttp_new(base);
	if (!http)
	{
		std::cout << "create evhttp fail" << std::endl;;
		return -1;
	}
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle)
	{
		std::cout << "bind port :" << port << " fail" << std::endl;;
		return -1;
	}
	evhttp_set_gencb(http, listener_cb, (void*) this);

	return 0;
}

/*
 * callback
 */
void NFHttpServer::listener_cb(struct evhttp_request* req, void* arg)
{
	if (req == NULL)
	{
		std::cout << "req == NULL" << " " << __FUNCTION__ << " " << __LINE__;
		return;
	}

	NFHttpServer* pNet = (NFHttpServer*)arg;
	if (pNet == NULL)
	{
		std::cout << "pNet == NULL" << " " << __FUNCTION__ << " " << __LINE__;
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	NF_SHARE_PTR<NFHttpRequest> pRequest = pNet->AllocHttpRequest();
	if (pRequest == nullptr)
	{
		std::cout << "pRequest == NULL" << " " << __FUNCTION__ << " " << __LINE__;
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	pRequest->req = req;

	//headers
	struct evkeyvalq * header = evhttp_request_get_input_headers(req);
	if (header == NULL)
	{
		pNet->mxHttpRequestPool.push_back(pRequest);

		std::cout << "header == NULL" << " " << __FUNCTION__ << " " << __LINE__;
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	struct evkeyval* kv = header->tqh_first;
	while (kv)
	{
		pRequest->headers.insert(std::map<std::string, std::string>::value_type(kv->key, kv->value));

		kv = kv->next.tqe_next;
	}

	//uri
	const char* uri = evhttp_request_get_uri(req);
	if (uri == NULL)
	{
		pNet->mxHttpRequestPool.push_back(pRequest);

		std::cout << "uri == NULL" << " " << __FUNCTION__ << " " << __LINE__;
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	pRequest->url = uri;
	pRequest->remoteHost = req->remote_host;
	pRequest->type = (NFHttpType)evhttp_request_get_command(req);

	//get decodeUri
	struct evhttp_uri* decoded = evhttp_uri_parse(uri);
	if (!decoded)
	{
		pNet->mxHttpRequestPool.push_back(pRequest);

		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		std::cout << "bad request " << " " << __FUNCTION__ << " " << __LINE__;
		return;
	}

	//path
	const char* urlPath = evhttp_uri_get_path(decoded);
	if (urlPath != NULL)
	{
		pRequest->path = urlPath;
	}
	else
	{
		std::cout << "urlPath == NULL " << " " << __FUNCTION__ << " " << __LINE__;
	}

	evhttp_uri_free(decoded);

	//std::cout << "Got a GET request:" << uri << std::endl;
	if (evhttp_request_get_command(req) == evhttp_cmd_type::EVHTTP_REQ_GET)
	{
		std::cout << "EVHTTP_REQ_GET" << std::endl;

		struct evkeyvalq params;
		evhttp_parse_query(uri, &params);
		struct evkeyval* kv = params.tqh_first;
		while (kv)
		{
			pRequest->params.insert(std::map<std::string, std::string>::value_type(kv->key, kv->value));

			kv = kv->next.tqe_next;
		}
	}

	struct evbuffer *in_evb = evhttp_request_get_input_buffer(req);
	if (in_evb == NULL)
	{
		pNet->mxHttpRequestPool.push_back(pRequest);

		std::cout << "urlPath == NULL " << " " << __FUNCTION__ << " " << __LINE__;
		return;
	}

	size_t len = evbuffer_get_length(in_evb);
	if (len > 0)
	{
		unsigned char *pData = evbuffer_pullup(in_evb, len);
		pRequest->body.clear();

		if (pData != NULL)
		{
			pRequest->body.append((const char *)pData, len);
		}
	}

	// filter 
	NFWebStatus xWebStatus = pNet->OnFilterPack(pRequest);
	if (xWebStatus != NFWebStatus::WEB_OK)
	{
		pNet->mxHttpRequestPool.push_back(pRequest);
		pNet->ResponseMsg(pRequest, "Filter error", xWebStatus);
		return;
	}

	// call cb
	try
	{
		pNet->OnReceiveNetPack(pRequest);
	}
	catch(std::exception& e)
	{
		pNet->ResponseMsg(pRequest, e.what(), NFWebStatus::WEB_ERROR);
	}
	catch(...)
	{
		pNet->ResponseMsg(pRequest, "UNKNOW ERROR", NFWebStatus::WEB_ERROR);
	}	
}

bool NFHttpServer::ResponseMsg(NF_SHARE_PTR<NFHttpRequest> req, const std::string& msg, NFWebStatus code, const std::string& strReason)
{
	if (req == nullptr)
	{
		return false;
	}

	evhttp_request* pHttpReq = (evhttp_request*)req->req;
    //create buffer
    struct evbuffer* eventBuffer = evbuffer_new();

    //send data
    evbuffer_add_printf(eventBuffer, "%s", msg.c_str());

    evhttp_add_header(evhttp_request_get_output_headers(pHttpReq), "Content-Type", "application/json");
	
	evhttp_send_reply(pHttpReq, code, strReason.c_str(), eventBuffer);

    //free
    evbuffer_free(eventBuffer);

	mxHttpRequestPool.push_back(req);
	
    return true;
}

/*
 * reserve 100 Request
 */
NF_SHARE_PTR<NFHttpRequest> NFHttpServer::AllocHttpRequest()
{
	if (mxHttpRequestPool.size() <= 0)
	{
		for (int i = 0; i < 100; ++i)
		{
			NF_SHARE_PTR<NFHttpRequest> request = NF_SHARE_PTR<NFHttpRequest>(new NFHttpRequest(++mIndex));
			mxHttpRequestPool.push_back(request);
		}
	}

	NF_SHARE_PTR<NFHttpRequest> pRequest = mxHttpRequestPool.front();
	mxHttpRequestPool.pop_front();
	pRequest->Reset();

	return pRequest;
}

bool NFHttpServer::OnReceiveNetPack(NF_SHARE_PTR<NFHttpRequest> req)
{
	if (req == nullptr)
	{
		return false;
	}

	auto it = mMsgCBMap.find(req->type);
	if (it != mMsgCBMap.end())
	{
		auto itPath = it->second.find(req->path);
		if (itPath != it->second.end())
		{
			HTTP_RECEIVE_FUNCTOR_PTR& pFunPtr = itPath->second;
			HTTP_RECEIVE_FUNCTOR* pFunc = pFunPtr.get();
			try
			{
				pFunc->operator()(req);
			}
			catch (const std::exception&)
			{
				ResponseMsg(req, "unknow error", NFWebStatus::WEB_INTER_ERROR);
			}
			return true;
		}
	}

	return ResponseMsg(req, "", NFWebStatus::WEB_ERROR);
}

NFWebStatus NFHttpServer::OnFilterPack(NF_SHARE_PTR<NFHttpRequest> req)
{
	if (req == nullptr)
	{
		return NFWebStatus::WEB_INTER_ERROR;
	}

	auto itPath = mMsgFliterMap.find(req->path);
	if (mMsgFliterMap.end() != itPath)
	{
		HTTP_FILTER_FUNCTOR_PTR& pFunPtr = itPath->second;
		HTTP_FILTER_FUNCTOR* pFunc = pFunPtr.get();
		return pFunc->operator()(req);
	}

	return NFWebStatus::WEB_OK;
}

bool NFHttpServer::AddMsgCB(const std::string& strCommand, const NFHttpType eRequestType, const HTTP_RECEIVE_FUNCTOR_PTR& cb)
{
	auto ret = mMsgCBMap[eRequestType].insert({strCommand, cb});
	if(ret.second)
		return true;
	else
		std::cout << "AddMsgCB:" << eRequestType << " " << strCommand << " has added!" << std::endl;
	return false;
}

bool NFHttpServer::AddFilterCB(const std::string& strCommand, const HTTP_FILTER_FUNCTOR_PTR& cb)
{
	auto ret = mMsgFliterMap.insert({strCommand, cb});
	if(ret.second)
		return true;
	else
		std::cout << "AddFilterCB:" << strCommand << "has added!" << std::endl;
}

