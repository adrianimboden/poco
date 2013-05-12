//
// RegularExpression.cpp
//
// $Id$
//
// Library: MongoDB
// Package: MongoDB
// Module:  RegularExpression
//
// Implementation of the RegularExpression class.
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


#include "Poco/MongoDB/RegularExpression.h"
#include <sstream>


namespace Poco {
namespace MongoDB {


RegularExpression::RegularExpression()
{
}


RegularExpression::RegularExpression(const std::string& pattern, const std::string& options) : _pattern(pattern), _options(options)
{
}


RegularExpression::~RegularExpression()
{
}


SharedPtr<Poco::RegularExpression> RegularExpression::createRE() const
{
	int options = 0;
	for(std::string::const_iterator optIt = _options.begin(); optIt != _options.end(); ++optIt)
	{
		switch(*optIt)
		{
		case 'i': // Case Insensitive
			options |= Poco::RegularExpression::RE_CASELESS;
			break;
		case 'm': // Multiline matching
			options |= Poco::RegularExpression::RE_MULTILINE;
			break;
		case 'x': // Verbose mode
			//No equivalent in Poco
			break;
		case 'l': // \w \W Locale dependent
			//No equivalent in Poco
			break;
		case 's': // Dotall mode
			options |= Poco::RegularExpression::RE_DOTALL;
			break;
		case 'u': // \w \W Unicode
			//No equivalent in Poco
			break;
		}
	}
	return SharedPtr<Poco::RegularExpression>(new Poco::RegularExpression(_pattern, options));
}


} } // namespace Poco::MongoDB
