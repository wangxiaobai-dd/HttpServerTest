#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "HttpServer.h"

using namespace std;

NF_SHARE_PTR<NFHttpServer> pHttpServer = nullptr;

bool TestCallBack(NF_SHARE_PTR<NFHttpRequest> req)
{
	req->Print();
	return pHttpServer->ResponseMsg(req, "Test", NFWebStatus::WEB_OK);
}

NFWebStatus TestFilter(NF_SHARE_PTR<NFHttpRequest> req)
{
	std::cout << "OnFilter ... " << std::endl;

	return NFWebStatus::WEB_OK;
}

int main()
{
	pHttpServer = std::make_shared<NFHttpServer>();
	pHttpServer->InitServer(9009);
	pHttpServer->AddRequestHandler("/json", NFHttpType::NF_HTTP_REQ_GET, TestCallBack);
	pHttpServer->AddNetFilter("/json", TestFilter);
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		pHttpServer->Execute();
	}
	return 0;
}

