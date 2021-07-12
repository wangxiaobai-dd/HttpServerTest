#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include <map>
#include <list>
#include <string>
#include <memory>
#include <functional>
#include "HttpRequest.h"
#include "event2/event.h"
#include <event2/http.h>
#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>

#define NF_SHARE_PTR std::shared_ptr

typedef std::function<bool(NF_SHARE_PTR<NFHttpRequest> req)> HTTP_RECEIVE_FUNCTOR;
typedef std::shared_ptr<HTTP_RECEIVE_FUNCTOR> HTTP_RECEIVE_FUNCTOR_PTR;

typedef std::function<NFWebStatus(NF_SHARE_PTR<NFHttpRequest> req)> HTTP_FILTER_FUNCTOR;
typedef std::shared_ptr<HTTP_FILTER_FUNCTOR> HTTP_FILTER_FUNCTOR_PTR;

class NFHttpServer
{
	public:
		~NFHttpServer();

		void Execute();
		int InitServer(const unsigned short nPort);

		NF_SHARE_PTR<NFHttpRequest> GetHttpRequest(const int64_t index);

		bool ResponseMsg(NF_SHARE_PTR<NFHttpRequest> req, const std::string& msg, NFWebStatus code, const std::string& strReason = "OK");

		// 添加消息回调
		bool AddMsgCB(const std::string& strCommand, const NFHttpType eRequestType, const HTTP_RECEIVE_FUNCTOR_PTR& cb);
		// 添加过滤器
		bool AddFilterCB(const std::string& strCommand, const HTTP_FILTER_FUNCTOR_PTR& cb);
		bool OnReceiveNetPack(NF_SHARE_PTR<NFHttpRequest> req);
		NFWebStatus OnFilterPack(NF_SHARE_PTR<NFHttpRequest> req);

		// 全局函数
		bool AddRequestHandler(const std::string& strPath, const NFHttpType eRequestType, bool (handleReceiver)(NF_SHARE_PTR<NFHttpRequest> req))
		{
			HTTP_RECEIVE_FUNCTOR functor = std::bind(handleReceiver, std::placeholders::_1);
			HTTP_RECEIVE_FUNCTOR_PTR functorPtr(new HTTP_RECEIVE_FUNCTOR(functor));
			return AddMsgCB(strPath, eRequestType, functorPtr);
		}

		// 成员函数
		template<typename BaseType>
		bool AddRequestHandler(const std::string& strPath, const NFHttpType eRequestType, BaseType* pBase, bool (BaseType::*handleReceiver)(NF_SHARE_PTR<NFHttpRequest> req))
		{
			HTTP_RECEIVE_FUNCTOR functor = std::bind(handleReceiver, pBase, std::placeholders::_1);
			HTTP_RECEIVE_FUNCTOR_PTR functorPtr(new HTTP_RECEIVE_FUNCTOR(functor));
			return AddMsgCB(strPath, eRequestType, functorPtr);
		}

		bool AddNetFilter(const std::string& strPath, NFWebStatus (handleFilter)(NF_SHARE_PTR<NFHttpRequest> req))
		{
			HTTP_FILTER_FUNCTOR functor = std::bind(handleFilter, std::placeholders::_1);
			HTTP_FILTER_FUNCTOR_PTR functorPtr(new HTTP_FILTER_FUNCTOR(functor));

			return AddFilterCB(strPath, functorPtr);
		}

		template<typename BaseType>
		bool AddNetFilter(const std::string& strPath, BaseType* pBase, NFWebStatus(BaseType::*handleFilter)(NF_SHARE_PTR<NFHttpRequest> req))
		{
			HTTP_FILTER_FUNCTOR functor = std::bind(handleFilter, pBase, std::placeholders::_1);
			HTTP_FILTER_FUNCTOR_PTR functorPtr(new HTTP_FILTER_FUNCTOR(functor));

			return AddFilterCB(strPath, functorPtr);
		}

		std::list<NF_SHARE_PTR<NFHttpRequest>> mxHttpRequestPool;
	private:
		int64_t mIndex = 0;

		struct event_base* base = nullptr;

		NF_SHARE_PTR<NFHttpRequest> AllocHttpRequest();

		std::map<NFHttpType, std::map<std::string, HTTP_RECEIVE_FUNCTOR_PTR>> mMsgCBMap;
		std::map<std::string, HTTP_FILTER_FUNCTOR_PTR> mMsgFliterMap;

		static void listener_cb(struct evhttp_request* req, void* arg);
};

#endif // _HTTPSERVER_H
