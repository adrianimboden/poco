//
// WebSocketTest.cpp
//
// $Id: //poco/1.4/Net/testsuite/src/WebSocketTest.cpp#3 $
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "WebSocketTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Thread.h"


using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;


namespace
{
	class WebSocketRequestHandler: public Poco::Net::HTTPRequestHandler
	{
	public:
		void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
		{
			try
			{
				WebSocket ws(request, response);
				char buffer[1024];
				int flags;
				int n;
				do
				{
					n = ws.receiveFrame(buffer, sizeof(buffer), flags);
					ws.sendFrame(buffer, n, flags);
				}
				while (n > 0 || (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);
			}
			catch (WebSocketException& exc)
			{
				switch (exc.code())
				{
				case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
					response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
					// fallthrough
				case WebSocket::WS_ERR_NO_HANDSHAKE:
				case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
				case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
					response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
					response.setContentLength(0);
					response.send();
					break;
				}
			}
		}
	};
	
	class WebSocketRequestHandlerFactory: public Poco::Net::HTTPRequestHandlerFactory
	{
	public:
		Poco::Net::HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request)
		{
			return new WebSocketRequestHandler;
		}
	};
}


WebSocketTest::WebSocketTest(const std::string& name): CppUnit::TestCase(name)
{
}


WebSocketTest::~WebSocketTest()
{
}


void WebSocketTest::testWebSocket()
{
	Poco::Net::ServerSocket ss(0);
	Poco::Net::HTTPServer server(WebSocketRequestHandlerFactory::Ptr(new WebSocketRequestHandlerFactory), ss, new Poco::Net::HTTPServerParams);
	server.start();
	
	Poco::Thread::sleep(200);
	
	HTTPClientSession cs("localhost", ss.address().port());
	HTTPRequest request(HTTPRequest::HTTP_GET, "/ws");
	HTTPResponse response;
	WebSocket ws(cs, request, response);

	std::string payload("x");
	ws.sendFrame(payload.data(), (int) payload.size());
	char buffer[1024];
	int flags;
	int n = ws.receiveFrame(buffer, sizeof(buffer), flags);
	assert (n == payload.size());
	assert (payload.compare(0, payload.size(), buffer, 0, n) == 0);
	assert (flags == WebSocket::FRAME_TEXT);

	for (int i = 2; i < 20; i++)
	{
		payload.assign(i, 'x');
		ws.sendFrame(payload.data(), (int) payload.size());
		n = ws.receiveFrame(buffer, sizeof(buffer), flags);
		assert (n == payload.size());
		assert (payload.compare(0, payload.size(), buffer, 0, n) == 0);
		assert (flags == WebSocket::FRAME_TEXT);
	}

	for (int i = 125; i < 129; i++)
	{
		payload.assign(i, 'x');
		ws.sendFrame(payload.data(), (int) payload.size());
		n = ws.receiveFrame(buffer, sizeof(buffer), flags);
		assert (n == payload.size());
		assert (payload.compare(0, payload.size(), buffer, 0, n) == 0);
		assert (flags == WebSocket::FRAME_TEXT);
	}

	payload = "Hello, world!";
	ws.sendFrame(payload.data(), (int) payload.size());
	n = ws.receiveFrame(buffer, sizeof(buffer), flags);
	assert (n == payload.size());
	assert (payload.compare(0, payload.size(), buffer, 0, n) == 0);
	assert (flags == WebSocket::FRAME_TEXT);
	
	payload = "Hello, universe!";
	ws.sendFrame(payload.data(), (int) payload.size(), WebSocket::FRAME_BINARY);
	n = ws.receiveFrame(buffer, sizeof(buffer), flags);
	assert (n == payload.size());
	assert (payload.compare(0, payload.size(), buffer, 0, n) == 0);
	assert (flags == WebSocket::FRAME_BINARY);	
	
	ws.shutdown();
	n = ws.receiveFrame(buffer, sizeof(buffer), flags);
	assert (n == 2);
	assert ((flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_CLOSE);
	
	server.stop();
}


void WebSocketTest::setUp()
{
}


void WebSocketTest::tearDown()
{
}


CppUnit::Test* WebSocketTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("WebSocketTest");

	CppUnit_addTest(pSuite, WebSocketTest, testWebSocket);

	return pSuite;
}
