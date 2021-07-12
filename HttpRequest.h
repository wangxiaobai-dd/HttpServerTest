#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <algorithm>
#include <iostream>
#include <map>

enum NFWebStatus
{
	WEB_OK = 200,
	WEB_AUTH = 401,
	WEB_ERROR = 404,
	WEB_INTER_ERROR = 500,
	WEB_TIMEOUT = 503,
};

enum NFHttpType
{
	NF_HTTP_REQ_GET = 1 << 0,
	NF_HTTP_REQ_POST = 1 << 1,
	NF_HTTP_REQ_HEAD = 1 << 2,
	NF_HTTP_REQ_PUT = 1 << 3,
	NF_HTTP_REQ_DELETE = 1 << 4,
	NF_HTTP_REQ_OPTIONS = 1 << 5,
	NF_HTTP_REQ_TRACE = 1 << 6,
	NF_HTTP_REQ_CONNECT = 1 << 7,
	NF_HTTP_REQ_PATCH = 1 << 8
};

class NFHttpRequest
{
	public:
		NFHttpRequest(const int64_t index)
		{
			id = index;
			Reset();
		}

		void Print()
		{
			std::cout << "print request:" << id << std::endl;
			std::cout << "url:" << url << std::endl;
			std::cout << "path:" << path << std::endl;
			std::cout << "remoteHost:" << remoteHost << std::endl;
			std::cout << "type:" << type << std::endl;
			std::cout << "headers:" << std::endl;
			std::for_each(headers.begin(), headers.end(), [](auto& pair){   std::cout << pair.first << "-" << pair.second << " ";  });
			std::cout << "\nparams:" << std::endl;
			std::for_each(params.begin(), params.end(), [](auto& pair){   std::cout << pair.first << "-" << pair.second << " ";  });
			std::cout << std::endl;
		}

		void Reset()
		{
			url.clear();
			path.clear();
			remoteHost.clear();
			//type
			body.clear();
			params.clear();
			headers.clear();
		}
		int64_t id;
		void* req;
		std::string url;
		std::string path;
		std::string remoteHost;
		NFHttpType type;
		std::string body;//when using post
		std::map<std::string, std::string> params;//when using get
		std::map<std::string, std::string> headers;
};

#endif // _HTTPREQUEST_H
