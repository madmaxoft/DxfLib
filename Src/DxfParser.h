// DxfParser.h

#pragma once

#include <string>
#include <stdexcept>
#include <istream>
#include "DxfDrawing.h"




namespace Dxf
{
namespace Parser
{




/** Exception that is thrown when a stream cannot be parsed.
Contains details about the error encountered. */
class Error:
	public std::exception
{
	using Super = std::exception;

	/** The line number where the error was detected. */
	unsigned mLineNumber;

	/** The detailed error message. */
	std::string mMessage;


public:

	Error(unsigned aLineNumber, std::string && aMessage):
		Super("Dxf::Parser::Error"),
		mLineNumber(aLineNumber),
		mMessage(std::move(aMessage))
	{
	}

	/** Returns the line number where the error was detected. */
	unsigned lineNumber() const { return mLineNumber; }

	/** */
	const std::string & message() const { return mMessage; }
};





/** Parses the DXF data from the specified stream, returns the DXF drawing contained within.
Throws a ParseError exception upon an error.
May throw other exceptions coming from the underlying systems, such as when reading the stream. */
std::shared_ptr<Drawing> parse(std::istream & aStream);

/** Parses the DXF data from the specified stream, until it reads the complete layer list, then returns the names of the layers.
Is faster than the full parse, because the layer list is at the top of the file.
Throws a ParseError exception upon an error.
May throw other exceptions coming from the underlying systems, such as when reading the stream. */
std::vector<std::string> parseLayerList(std::istream & aStream);

}  // namespace Parser
}  // namespace Dxf
