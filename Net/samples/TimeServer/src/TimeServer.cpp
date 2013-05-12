//
// TimeServer.cpp
//
// $Id: //poco/1.4/Net/samples/TimeServer/src/TimeServer.cpp#1 $
//
// This sample demonstrates the TCPServer and ServerSocket classes.
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
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


#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/TCPServerParams.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>


using Poco::Net::ServerSocket;
using Poco::Net::StreamSocket;
using Poco::Net::TCPServerConnection;
using Poco::Net::TCPServerConnectionFactory;
using Poco::Net::TCPServer;
using Poco::Timestamp;
using Poco::DateTimeFormatter;
using Poco::DateTimeFormat;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;


class TimeServerConnection: public TCPServerConnection
	/// This class handles all client connections.
	///
	/// A string with the current date and time is sent back to the client.
{
public:
	TimeServerConnection(const StreamSocket& s, const std::string& format): 
		TCPServerConnection(s),
		_format(format)
	{
	}
	
	void run()
	{
		Application& app = Application::instance();
		app.logger().information("Request from " + this->socket().peerAddress().toString());
		try
		{
			Timestamp now;
			std::string dt(DateTimeFormatter::format(now, _format));
			dt.append("\r\n");
			socket().sendBytes(dt.data(), (int) dt.length());
		}
		catch (Poco::Exception& exc)
		{
			app.logger().log(exc);
		}
	}
	
private:
	std::string _format;
};


class TimeServerConnectionFactory: public TCPServerConnectionFactory
	/// A factory for TimeServerConnection.
{
public:
	TimeServerConnectionFactory(const std::string& format):
		_format(format)
	{
	}
	
	TCPServerConnection* createConnection(const StreamSocket& socket)
	{
		return new TimeServerConnection(socket, _format);
	}

private:
	std::string _format;
};


class TimeServer: public Poco::Util::ServerApplication
	/// The main application class.
	///
	/// This class handles command-line arguments and
	/// configuration files.
	/// Start the TimeServer executable with the help
	/// option (/help on Windows, --help on Unix) for
	/// the available command line options.
	///
	/// To use the sample configuration file (TimeServer.properties),
	/// copy the file to the directory where the TimeServer executable
	/// resides. If you start the debug version of the TimeServer
	/// (TimeServerd[.exe]), you must also create a copy of the configuration
	/// file named TimeServerd.properties. In the configuration file, you
	/// can specify the port on which the server is listening (default
	/// 9911) and the format of the date/time string sent back to the client.
	///
	/// To test the TimeServer you can use any telnet client (telnet localhost 9911).
{
public:
	TimeServer(): _helpRequested(false)
	{
	}
	
	~TimeServer()
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
	}
		
	void uninitialize()
	{
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options)
	{
		ServerApplication::defineOptions(options);
		
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value)
	{
		ServerApplication::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A server application that serves the current date and time.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args)
	{
		if (_helpRequested)
		{
			displayHelp();
		}
		else
		{
			// get parameters from configuration file
			unsigned short port = (unsigned short) config().getInt("TimeServer.port", 9911);
			std::string format(config().getString("TimeServer.format", DateTimeFormat::ISO8601_FORMAT));
			
			// set-up a server socket
			ServerSocket svs(port);
			// set-up a TCPServer instance
			TCPServer srv(TimeServerConnectionFactory::Ptr(new TimeServerConnectionFactory(format)), svs);
			// start the TCPServer
			srv.start();
			// wait for CTRL-C or kill
			waitForTerminationRequest();
			// Stop the TCPServer
			srv.stop();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool _helpRequested;
};


int main(int argc, char** argv)
{
	TimeServer app;
	return app.run(argc, argv);
}
