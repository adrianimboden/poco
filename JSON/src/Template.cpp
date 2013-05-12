//
// Template.cpp
//
// $Id$
//
// Library: JSON
// Package: JSON
// Module:  Template
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


#include "Poco/JSON/Template.h"
#include "Poco/JSON/TemplateCache.h"
#include "Poco/JSON/Query.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"


using Poco::Dynamic::Var;


namespace Poco {
namespace JSON {


POCO_IMPLEMENT_EXCEPTION(JSONTemplateException, Exception, "Template Exception")


class Part
{
public:
	Part()
	{
	}

	virtual ~Part()
	{
	}

	virtual void render(const Var& data, std::ostream& out) const = 0;

	typedef std::vector<SharedPtr<Part> > VectorParts;
};


class StringPart: public Part
{
public:
	StringPart() : Part()
	{
	}

	StringPart(const std::string& content) : Part(), _content(content)
	{
	}

	virtual ~StringPart()
	{
	}

	void render(const Var& data, std::ostream& out) const
	{
		out << _content;
	}

	void setContent(const std::string& content)
	{
		_content = content;
	}

	inline std::string getContent() const
	{
		return _content;
	}

private:
	std::string _content;
};


class MultiPart: public Part
{
public:
	MultiPart()
	{
	}

	virtual ~MultiPart()
	{
	}

	virtual void addPart(const SharedPtr<Part>& part)
	{
		_parts.push_back(part);
	}

	void render(const Var& data, std::ostream& out) const
	{
		for(VectorParts::const_iterator it = _parts.begin(); it != _parts.end(); ++it)
		{
			(*it)->render(data, out);
		}
	}

protected:
	VectorParts _parts;
};


class EchoPart: public Part
{
public:
	EchoPart(const std::string& query) : Part(), _query(query)
	{
	}

	virtual ~EchoPart()
	{
	}

	void render(const Var& data, std::ostream& out) const
	{
		Query query(data);
		Var value = query.find(_query);

		if ( ! value.isEmpty() )
		{
			out << value.convert<std::string>();
		}
	}

private:
	std::string _query;
};


class LogicQuery
{
public:
	LogicQuery(const std::string& query) : _queryString(query)
	{
	}

	virtual ~LogicQuery()
	{
	}

	virtual bool apply(const Var& data) const
	{
		bool logic = false;

		Query query(data);
		Var value = query.find(_queryString);

		if ( ! value.isEmpty() ) // When empty, logic will be false
		{
			if ( value.isString() )
				// An empty string must result in false, otherwise true
				// Which is not the case when we convert to bool with Var
			{
				std::string s = value.convert<std::string>();
				logic = ! s.empty();
			}
			else
			{
				// All other values, try to convert to bool
				// An empty object or array will turn into false
				// all other values depend on the convert<> in Var
				logic = value.convert<bool>();
			}
		}

		return logic;
	}

protected:
	std::string _queryString;
};


class LogicExistQuery : public LogicQuery
{
public:
	LogicExistQuery(const std::string& query) : LogicQuery(query)
	{
	}

	virtual ~LogicExistQuery()
	{
	}

	virtual bool apply(const Var& data) const
	{
		Query query(data);
		Var value = query.find(_queryString);

		return !value.isEmpty();
	}
};


class LogicElseQuery : public LogicQuery
{
public:
	LogicElseQuery() : LogicQuery("")
	{
	}

	virtual ~LogicElseQuery()
	{
	}

	virtual bool apply(const Var& data) const
	{
		return true;
	}
};


class LogicPart : public MultiPart
{
public:
	LogicPart() : MultiPart()
	{
	}

	virtual ~LogicPart()
	{
	}

	void addPart(const SharedPtr<LogicQuery>& query, const SharedPtr<Part>& part)
	{
		MultiPart::addPart(part);
		_queries.push_back(query);
	}

	void addPart(const SharedPtr<Part>& part)
	{
		MultiPart::addPart(part);
		_queries.push_back(SharedPtr<LogicElseQuery>(new LogicElseQuery()));
	}

	void render(const Var& data, std::ostream& out) const
	{
		int count = 0;
		for(std::vector<SharedPtr<LogicQuery> >::const_iterator it = _queries.begin(); it != _queries.end(); ++it, ++count)
		{
			if( (*it)->apply(data) && _parts.size() > count )
			{
				_parts[count]->render(data, out);
				break;
			}
		}
	}

private:
	std::vector<SharedPtr<LogicQuery> > _queries;
};


class LoopPart: public MultiPart
{
public:
	LoopPart(const std::string& name, const std::string& query) : MultiPart(), _name(name), _query(query)
	{
	}

	virtual ~LoopPart()
	{
	}

	void render(const Var& data, std::ostream& out) const
	{
		Query query(data);

		if ( data.type() == typeid(Object::Ptr) )
		{
			Object::Ptr dataObject = data.extract<Object::Ptr>();
			Array::Ptr array = query.findArray(_query);
			if ( ! array.isNull() )
			{
				for(int i = 0; i < array->size(); i++)
				{
					Var value = array->get(i);
					dataObject->set(_name, value);
					MultiPart::render(data, out);
				}
				dataObject->remove(_name);
			}
		}
	}

private:
	std::string _name;
	std::string _query;
};


class IncludePart: public Part
{
public:

	IncludePart(const Path& parentPath, const Path& path)
		: Part()
		, _path(path)
	{
		// When the path is relative, try to make it absolute based
		// on the path of the parent template. When the file doesn't
		// exist, we keep it relative and hope that the cache can
		// resolve it.
		if ( _path.isRelative() )
		{
			Path templatePath(parentPath, _path);
			File templateFile(templatePath);
			if ( templateFile.exists() )
			{
				_path = templatePath;
			}
		}
	}

	virtual ~IncludePart()
	{
	}

	void render(const Var& data, std::ostream& out) const
	{
		TemplateCache* cache = TemplateCache::instance();
		if ( cache == NULL )
		{
			Template tpl(_path);
			tpl.parse();
			tpl.render(data, out);
		}
		else
		{
			Template::Ptr tpl = cache->getTemplate(_path);
			tpl->render(data, out);
		}
	}

private:
	Path _path;
};


Template::Template(const Path& templatePath)
	: _templatePath(templatePath)
{
}


Template::Template()
{
}


Template::~Template()
{
}


void Template::parse()
{
	File file(_templatePath);
	if ( file.exists() )
	{
		FileInputStream fis(_templatePath.toString());
		parse(fis);
	}
}


void Template::parse(std::istream& in)
{
	_parseTime.update();

	_parts = SharedPtr<MultiPart>(new MultiPart());
	_currentPart = _parts;

	while(in.good())
	{
		std::string text = readText(in); // Try to read text first
		if ( text.length() > 0 )
		{
			_currentPart->addPart(SharedPtr<StringPart>(new StringPart(text)));
		}

		if ( in.bad() )
			break; // Nothing to do anymore

		std::string command = readTemplateCommand(in);  // Try to read a template command
		if ( command.empty() )
		{
			break;
		}

		readWhiteSpace(in);

		if ( command.compare("echo") == 0 )
		{
			std::string query = readQuery(in);
			if ( query.empty() )
			{
				throw JSONTemplateException("Missing query in <? echo ?>");
			}
			_currentPart->addPart(SharedPtr<EchoPart>(new EchoPart(query)));
		}
		else if ( command.compare("for") == 0 )
		{
			std::string loopVariable = readWord(in);
			if ( loopVariable.empty() )
			{
				throw JSONTemplateException("Missing variable in <? for ?> command");
			}
			readWhiteSpace(in);

			std::string query = readQuery(in);
			if ( query.empty() )
			{
				throw JSONTemplateException("Missing query in <? for ?> command");
			}

			_partStack.push(_currentPart);
			SharedPtr<LoopPart> part(new LoopPart(loopVariable, query));
			_partStack.push(part);
			_currentPart->addPart(part);
			_currentPart = part;
		}
		else if ( command.compare("else") == 0 )
		{
			if ( _partStack.size() == 0 )
			{
				throw JSONTemplateException("Unexpected <? else ?> found");
			}
			_currentPart = _partStack.top();
			SharedPtr<LogicPart> lp = _currentPart.cast<LogicPart>();
			if ( !lp )
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? else ?>");
			}
			SharedPtr<MultiPart> part(new MultiPart());
			lp->addPart(part);
			_currentPart = part;
		}
		else if (    command.compare("elsif") == 0
		             || command.compare("elif") == 0 )
		{
			std::string query = readQuery(in);
			if ( query.empty() )
			{
				throw JSONTemplateException("Missing query in <? " + command + " ?>");
			}

			if ( _partStack.size() == 0 )
			{
				throw JSONTemplateException("Unexpected <? elsif / elif ?> found");
			}

			_currentPart = _partStack.top();
			SharedPtr<LogicPart> lp = _currentPart.cast<LogicPart>();
			if ( lp )
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? elsif / elif ?>");
			}
			SharedPtr<MultiPart> part(new MultiPart());
			lp->addPart(SharedPtr<LogicQuery>(new LogicQuery(query)), part);
			_currentPart = part;
		}
		else if ( command.compare("endfor") == 0 )
		{
			if ( _partStack.size() < 2 )
			{
				throw JSONTemplateException("Unexpected <? endfor ?> found");
			}
			SharedPtr<MultiPart> loopPart = _partStack.top();
			SharedPtr<LoopPart> lp = loopPart.cast<LoopPart>();
			if ( !lp )
			{
				throw JSONTemplateException("Missing <? for ?> command");
			}
			_partStack.pop();
			_currentPart = _partStack.top();
			_partStack.pop();
		}
		else if ( command.compare("endif") == 0 )
		{
			if ( _partStack.size() < 2 )
			{
				throw JSONTemplateException("Unexpected <? endif ?> found");
			}

			_currentPart = _partStack.top();
			SharedPtr<LogicPart> lp = _currentPart.cast<LogicPart>();
			if ( !lp )
			{
				throw JSONTemplateException("Missing <? if ?> or <? ifexist ?> for <? endif ?>");
			}

			_partStack.pop();
			_currentPart = _partStack.top();
			_partStack.pop();
		}
		else if (    command.compare("if") == 0
		             || command.compare("ifexist") == 0 )
		{
			std::string query = readQuery(in);
			if ( query.empty() )
			{
				throw JSONTemplateException("Missing query in <? " + command + " ?>");
			}
			_partStack.push(_currentPart);
			SharedPtr<LogicPart> lp(new LogicPart());
			_partStack.push(lp);
			_currentPart->addPart(lp);
			_currentPart = SharedPtr<MultiPart>(new MultiPart());
			if ( command.compare("ifexist") == 0 )
			{
				lp->addPart(SharedPtr<LogicExistQuery>(new LogicExistQuery(query)), _currentPart);
			}
			else
			{
				lp->addPart(SharedPtr<LogicQuery>(new LogicQuery(query)), _currentPart);
			}
		}
		else if ( command.compare("include") == 0 )
		{
			readWhiteSpace(in);
			std::string filename = readString(in);
			if ( filename.empty() )
			{
				throw JSONTemplateException("Missing filename in <? include ?>");
			}
			else
			{
				Path resolvePath(_templatePath);
				resolvePath.makeParent();
				_currentPart->addPart(SharedPtr<IncludePart>(new IncludePart(resolvePath, filename)));
			}
		}
		else
		{
			throw JSONTemplateException("Unknown command " + command);
		}

		readWhiteSpace(in);

		int c = in.get();
		if ( c == '?' && in.peek() == '>' )
		{
			in.get(); // forget '>'

			if ( command.compare("echo") != 0 )
			{
				if ( in.peek() == '\r' )
				{
					in.get();
				}
				if ( in.peek() == '\n' )
				{
					in.get();
				}
			}
		}
		else
		{
			throw JSONTemplateException("Missing ?>");
		}
	}
}


std::string Template::readText(std::istream& in)
{
	std::string text;
	int c = in.get();
	while(c != -1)
	{
		if ( c == '<' )
		{
			if ( in.peek() == '?' )
			{
				in.get(); // forget '?'
				break;
			}
		}
		text += c;

		c = in.get();
	}
	return text;
}


std::string Template::readTemplateCommand(std::istream& in)
{
	std::string command;

	readWhiteSpace(in);

	int c = in.get();
	while(c != -1)
	{
		if ( Ascii::isSpace(c) )
			break;

		if ( c == '?' && in.peek() == '>' )
		{
			in.putback(c);
			break;
		}

		if ( c == '=' && command.length() == 0 )
		{
			command = "echo";
			break;
		}

		command += c;

		c = in.get();
	}

	return command;
}


std::string Template::readWord(std::istream& in)
{
	std::string word;
	int c;
	while((c = in.peek()) != -1 && ! Ascii::isSpace(c))
	{
		in.get();
		word += c;
	}
	return word;
}


std::string Template::readQuery(std::istream& in)
{
	std::string word;
	int c;
	while((c = in.get()) != -1)
	{
		if ( c == '?' && in.peek() == '>' )
		{
			in.putback(c);
			break;
		}

		if ( Ascii::isSpace(c) )
		{
			break;
		}
		word += c;
	}
	return word;
}


void Template::readWhiteSpace(std::istream& in)
{
	int c;
	while((c = in.peek()) != -1 && Ascii::isSpace(c))
	{
		in.get();
	}
}


std::string Template::readString(std::istream& in)
{
	std::string str;

	int c = in.get();
	if ( c == '"' )
	{
		while((c = in.get()) != -1 && c != '"')
		{
			str += c;
		}
	}
	return str;
}


void Template::render(const Var& data, std::ostream& out) const
{
	_parts->render(data, out);
}


} } // Namespace Poco::JSON
