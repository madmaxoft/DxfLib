// DxfParser.cpp

// Implements the Dxf::Parser class representing the DXF file format parser

#include "DxfParser.h"




#if 0
TDXFParser::TDXFParser(TMStream * iStream):
	mStream(iStream),
	mDrawing(NULL),
	mIsEOF(false),
	mNext(true),
	mCurrentGroup(-999),
	mCurrentValue(NULL)
{
}





TDXFParser::~TDXFParser(void)
{
	delete mDrawing;
}





STATUS TDXFParser::Parse(bool iContinueAfterLayerList)
{
	delete mDrawing;
	mDrawing = new TDXFDrawing();
	while (!mIsEOF)
	{
		if (mNext)
		{
			RETURN_IF_FAILED(ReadNext());
		}

		switch (mCurrentGroup)
		{
			case 0:
			{
				if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, mCurrentValue, -1, TEXT("eof"), -1) == CSTR_EQUAL)
				{
					return ERROR_SUCCESS;
				}
				break;
			}  // case 0

			case 2:
			{
				InPlaceLowerCase(mCurrentValue);
				if      (_tcscmp(mCurrentValue, TEXT("header"))   == 0) RETURN_IF_FAILED(ParseHeaderSection());
				else if (_tcscmp(mCurrentValue, TEXT("classes"))  == 0) RETURN_IF_FAILED(ParseClassesSection());
				else if (_tcscmp(mCurrentValue, TEXT("tables"))   == 0)
				{
					RETURN_IF_FAILED(ParseTablesSection());
					if (!iContinueAfterLayerList)
					{
						return ERROR_SUCCESS;
					}
				}
				else if (_tcscmp(mCurrentValue, TEXT("blocks"))   == 0) RETURN_IF_FAILED(ParseBlocksSection());
				else if (_tcscmp(mCurrentValue, TEXT("entities")) == 0) RETURN_IF_FAILED(ParseEntitiesSection(NULL));
				else if (_tcscmp(mCurrentValue, TEXT("objects"))  == 0) RETURN_IF_FAILED(ParseObjectsSection());
				break;
			}  // case 2
			default:
				break;
		}  // switch CurrentGroup
	}

	return ERROR_SUCCESS;
}





STATUS TDXFParser::ParseHeaderSection()
{
	return SkipUntilSectionEnd();
}





STATUS TDXFParser::ParseClassesSection()
{
	return SkipUntilSectionEnd();
}





STATUS TDXFParser::SkipUntilSectionEnd()
{
	for (;;)
	{
		if (mNext)
		{
			RETURN_IF_FAILED(ReadNext());
		}
		if (mIsEOF)
		{
			return ERROR_INVALID_DATA;
		}
		mNext = true;
		switch (mCurrentGroup)
		{
			case 0:
			{
				if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, mCurrentValue, -1, TEXT("endsec"), -1) == CSTR_EQUAL)
					return ERROR_SUCCESS;
			}
			default:
				break;
			// TODO: parse classes here
		}  // switch (mCurrentGroup)
	}  // for (;;)
}





STATUS TDXFParser::ParseTablesSection()
{
	TDXFLayer * lay = NULL;
	bool IsNewLayer = false;
	enum {ttNone, ttLayer, ttStyle, ttLineType} TableType = ttNone;

	for (;;)
	{
		if (mNext)
		{
			RETURN_IF_FAILED(ReadNext());
		}
		if (mIsEOF)
		{
			return ERROR_INVALID_DATA;
		}
		mNext = true;
		switch (mCurrentGroup)
		{
			case 0:
			{
				InPlaceLowerCase(mCurrentValue);
				if (_tcscmp(mCurrentValue, TEXT("endsec")) == 0) return ERROR_SUCCESS;
				else if (_tcscmp(mCurrentValue, TEXT("table")) == 0) TableType = ttNone;
				else if (_tcscmp(mCurrentValue, TEXT("layer")) == 0)
				{
					IsNewLayer = true;
				}
				else if (_tcscmp(mCurrentValue, TEXT("endtab")) == 0)
				{
					TableType = ttNone;
					IsNewLayer = false;
				}
				// else if (_tcscmp(mCurrentValue, TEXT("style")) == 0)
				// else if (_tcscmp(mCurrentValue, TEXT("ltype")) == 0)
				break;
			}  // case 0

			case 2:
			{
				TString Value(mCurrentValue);
				InPlaceLowerCase(mCurrentValue);
				if      (_tcscmp(mCurrentValue, TEXT("layer")) == 0) TableType = ttLayer;
				else if (_tcscmp(mCurrentValue, TEXT("style")) == 0) TableType = ttStyle;
				else if (_tcscmp(mCurrentValue, TEXT("ltype")) == 0) TableType = ttLineType;
				else /* if (IsNewLayer) */  // _X: some DXFs have the sequence "0 / TABLE / 2 / LAYER / 2 / layername" (without the "0 / layer")
				{
					switch (TableType)
					{
						case ttNone: break;
						case ttLayer:
						{
							lay = mDrawing->AddLayer(ParseDXFText(Value));
							break;
						}
						case ttStyle:
						{
							// style->name = ParseDXFText(Value);
							break;
						}
						case ttLineType:
						{
							// ltype->name = ParseDXFText(Value);
							break;
						}
						default:
							SF_ASSERT(FALSE);
							break;
					}  // switch (TableType)
				}  // if (IsNew)
				break;
			}  // case 2

			case 62:
			{
				switch (TableType)
				{
					case ttLayer:
					{
						if (lay != NULL)
						{
							long colorValue;
							if (StringToInt(mCurrentValue, &colorValue) == FALSE
								|| colorValue < INT_MIN || colorValue > INT_MAX) colorValue = 0;
							lay->DefaultColor = static_cast<TDXFColor>(colorValue);
						}
						break;
					}

					case ttNone:
					case ttStyle:
					case ttLineType:
					default:
						// do nothing
						break;
				}
				break;
			}

			default:
				break;
			// TODO: parse other tables and attributes
		}  // switch (mCurrentGroup)
	}  // for (;;)
}





STATUS TDXFParser::ParseBlocksSection()
{
	return SkipUntilSectionEnd();
}





STATUS TDXFParser::ParseEntitiesSection(TDXFBlockDefinition * iBlock)
{
	TDXFPrimitive * last = NULL;
	TDXFPrimitive * cur = NULL;
	TDXFVertex * plp = NULL;
	TString CurrentCaption;  // Accumulator for text / mtext
	bool IsPolylineSequence = false;
	for (;;)
	{
		if (mNext)
		{
			RETURN_IF_FAILED(ReadNext());
		}
		if (mIsEOF)
		{
			return ERROR_SUCCESS;
		}
		mNext = true;
		switch (mCurrentGroup)
		{
			case 0:
			{
				if (cur != NULL)
				{
					if (cur->ParentLayer != NULL)
					{
						if ((last && (last->ObjType == otPolyline) && (cur->ObjType == otVertex) && IsPolylineSequence))
						{
							static_cast<TDXFPolyline *>(last)->AddVertex(cur->x, cur->y, cur->z);
						}
						else
						{
							last = cur;
							if (iBlock)
							{
								iBlock->Reg(cur);
							}
						}  // not poly vertex
						cur = NULL;
					}
					else  // (cur->ParentLayer != NULL)
					{
						// layer doesnt exist
						delete cur;
						cur = NULL;
					}
				}
				TCHAR * v = mCurrentValue;
				InPlaceLowerCase(v);
				if (_tcscmp(v, TEXT("endsec")) == 0) return ERROR_SUCCESS;
				if (_tcscmp(v, TEXT("endblk")) == 0) return ERROR_SUCCESS;
				if (_tcscmp(v, TEXT("seqend")) == 0)
				{
					IsPolylineSequence = false;
				}
				else if (_tcscmp(v, TEXT("line")) == 0)
				{
					cur = new TDXFLine;
				}
				else if (_tcscmp(v, TEXT("polyline")) == 0)
				{
					cur = new TDXFPolyline;
					IsPolylineSequence = true;
				}
				else if (_tcscmp(v, TEXT("vertex")) == 0)
				{
					cur = new TDXFVertex;
				}
				else if (_tcscmp(v, TEXT("lwpolyline")) == 0)
				{
					cur = new TDXFLWPolyline;
				}
				else if (_tcscmp(v, TEXT("text")) == 0)
				{
					cur = new TDXFText;
				}
				else if (_tcscmp(v, TEXT("mtext")) == 0)
				{
					cur = new TDXFMText;
				}
				else if (_tcscmp(v, TEXT("point")) == 0)
				{
					cur = new TDXFPoint;
				}
				else
				{
					#ifdef DEBUG_UNKNOWN_ENTITIES
					DEBUGTRACE(TEXT("Unknown entity: %s"), v);
					#endif  // DEBUG_UNKNOWN_ENTITIES
				}
				break;
			}  // case 0

			case 1:
			{
				// text parsing:
				if (cur != NULL)
				{
					switch (cur->ObjType)
					{
						case otText:
						{
							CurrentCaption.append(mCurrentValue);
							TDXFText * const text = static_cast<TDXFText *>(cur);
							text->Caption.assign(ParseDXFText(CurrentCaption));
							CurrentCaption.clear();
							break;
						}

						case otMText:
						{
							CurrentCaption.append(mCurrentValue);
							TDXFMText * const mtext = static_cast<TDXFMText *>(cur);
							mtext->Caption.assign(ParseDXFText(CurrentCaption));
							CurrentCaption.clear();
							mtext->RemoveFormatting();
							break;
						}

						case otError:
						case otLine:
						case otPolyline:
						case otLWPolyline:
						case otPolygon:
						case otSolid:
						case otCircle:
						case otSimpleEllipse:
						case otHatch:
						case otArc:
						case otBlock:
						case otVertex:
						case otPoint:
						default:
							// do nothing
							break;
					}
				}
				break;
			}  // case 1

			case 3:
			{
				// additional text parsing:
				if (cur != NULL)
				{
					switch (cur->ObjType)
					{
						// Note that TEXT element never defines group no. 3, used only for sure
						case otText:
						case otMText: (void)CurrentCaption.append(mCurrentValue); break;

						case otError:
						case otLine:
						case otPolyline:
						case otLWPolyline:
						case otPolygon:
						case otSolid:
						case otCircle:
						case otSimpleEllipse:
						case otHatch:
						case otArc:
						case otBlock:
						case otVertex:
						case otPoint:
						default:
							// do nothing
							break;
					}
				}
				break;
			}  // case 3

			case 8:  // layer
			{
				if (cur != NULL)
				{
					// InPlaceLowerCase(mCurrentValue);
					TDXFLayer * lay = mDrawing->FindLayerByName(mCurrentValue);
					if (lay != NULL)
					{
						lay->RegObject(cur);
					}
				}
				break;
			}

			case 10:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otLWPolyline:
					{
						plp = static_cast<TDXFLWPolyline *>(cur)->AddVertex(_tstof(mCurrentValue), 0);
						break;
					}
					case otLine:
					case otPoint:
					case otVertex:
					case otText:
					case otMText:
					{
						cur->x = _tstof(mCurrentValue);
						break;
					}
					case otPolyline: break;  // Ignore

					case otError:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 10");
						break;
				}
				break;
			}  // case 10

			case 11:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otText:
					case otMText:
						break;  // Ignore
					case otLine: static_cast<TDXFLine *>(cur)->x2 = _tstof(mCurrentValue); break;

					case otError:
					case otPolyline:
					case otLWPolyline:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					case otVertex:
					case otPoint:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 11");
						break;
				}
				break;
			}  // case 11

			case 20:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otLWPolyline:
					{
						if (plp) plp->y = _tstof(mCurrentValue);
						break;
					}
					case otLine:
					case otPoint:
					case otVertex:
					case otText:
					case otMText:
					{
						cur->y = _tstof(mCurrentValue);
						break;
					}
					case otPolyline: break;  // Ignore

					case otError:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 20");
						break;
				}
				break;
			}  // case 20

			case 21:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otText:
					case otMText:
						break;  // Ignore
					case otLine: static_cast<TDXFLine *>(cur)->y2 = _tstof(mCurrentValue); break;

					case otError:
					case otPolyline:
					case otLWPolyline:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					case otVertex:
					case otPoint:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 21");
						break;
				}
				break;
			}  // case 21

			case 30:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otLine:
					case otPoint:
					case otVertex:
					case otText:
					case otMText:
					{
						cur->z = _tstof(mCurrentValue);
						break;
					}
					case otPolyline: break;  // Ignore

					case otError:
					case otLWPolyline:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 30");
						break;
				}
				break;
			}  // case 30

			case 31:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otText:
					case otMText:
						break; // Ignore
					case otLine: static_cast<TDXFLine *>(cur)->z2 = _tstof(mCurrentValue); break;

					case otError:
					case otPolyline:
					case otLWPolyline:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otBlock:
					case otVertex:
					case otPoint:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 31");
						break;
				}
				break;
			}  // case 31

			case 38:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otLWPolyline: static_cast<TDXFLWPolyline *>(cur)->z = _tstof(mCurrentValue); break;
					case otPolyline:
					{
						// Silently ignore this "error"
						break;
					}

					case otError:
					case otLine:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otText:
					case otMText:
					case otBlock:
					case otVertex:
					case otPoint:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 38");
						break;
				}
				break;
			}  // case 38

			case 70:
			{
				if (cur == NULL) break;
				switch (cur->ObjType)
				{
					case otPolyline:
					case otLWPolyline:
					{
						long flagsValue;
						if (StringToInt(mCurrentValue, &flagsValue) == FALSE) flagsValue = 0;
						static_cast<TDXFPolyline *>(cur)->Flags = static_cast<TDXFPolylineFlags>(flagsValue);
						break;
					}
					case otVertex: break;  // Ignore

					case otError:
					case otLine:
					case otPolygon:
					case otSolid:
					case otCircle:
					case otSimpleEllipse:
					case otHatch:
					case otArc:
					case otText:
					case otMText:
					case otBlock:
					case otPoint:
					default:
						SF_ASSERT(!"Unhandled object type with groupcode 70");
						break;
				}  // switch (cur->ObjType)
				break;
			}  // case 70

			default:
			{
				if (cur == NULL) break;
				if (cur->Extended == NULL)
				{
					cur->Extended = new TDXFExtendedData();
				}

				ParseExtendedEntityData(*cur->Extended);

				// _X: too many asserts on DXF2000, removing: SF_ASSERT(!"Unhandled groupcode");
				break;
			}
		}  // switch (mCurrentGroup)
	}  // for (;;)
}





void TDXFParser::ParseExtendedEntityData(TDXFExtendedData & extended)
{
	switch (mCurrentGroup)
	{
		case 1000:
		{
			extended.String.assign(mCurrentValue);
			break;
		}
		case 1001:
		{
			// TODO: Add check that the group is the first one within the extended data and the name is in a reference table
			extended.ApplicationName.assign(mCurrentValue);
			break;
		}
		case 1002:
		{
			// TODO: Add support for blocks
			break;
		}
		case 1003:
		{
			extended.LayerName.assign(mCurrentValue);
			break;
		}
		case 1004:
		{
			// TODO: Add parser of binary data
			break;
		}
		case 1005:
		{
			long value;
			if (StringToInt(mCurrentValue, &value) == FALSE || value < INT_MIN || value > INT_MAX) value = 0u;
			extended.DatabaseHandle = static_cast<int>(value);
			break;
		}
		case 1010:
		{
			extended.X = _tstof(mCurrentValue);
			break;
		}
		case 1020:
		{
			extended.Y = _tstof(mCurrentValue);
			break;
		}
		case 1030:
		{
			extended.Z = _tstof(mCurrentValue);
			break;
		}
		case 1011:
		{
			extended.WorldSpacePositionX = _tstof(mCurrentValue);
			break;
		}
		case 1021:
		{
			extended.WorldSpacePositionY = _tstof(mCurrentValue);
			break;
		}
		case 1031:
		{
			extended.WorldSpacePositionZ = _tstof(mCurrentValue);
			break;
		}
		case 1012:
		{
			extended.WorldSpaceDisplacementX = _tstof(mCurrentValue);
			break;
		}
		case 1022:
		{
			extended.WorldSpaceDisplacementY = _tstof(mCurrentValue);
			break;
		}
		case 1032:
		{
			extended.WorldSpaceDisplacementZ = _tstof(mCurrentValue);
			break;
		}
		case 1013:
		{
			extended.WorldDirectionX = _tstof(mCurrentValue);
			break;
		}
		case 1023:
		{
			extended.WorldDirectionY = _tstof(mCurrentValue);
			break;
		}
		case 1033:
		{
			extended.WorldDirectionZ = _tstof(mCurrentValue);
			break;
		}
		case 1040:
		{
			extended.Real = _tstof(mCurrentValue);
			break;
		}
		case 1041:
		{
			extended.Distance = _tstof(mCurrentValue);
			break;
		}
		case 1042:
		{
			extended.ScaleFactor = _tstof(mCurrentValue);
			break;
		}
		case 1070:
		{
			long value;
			if (StringToInt(mCurrentValue, &value) == FALSE || value < 0 || value > USHRT_MAX) value = 0;
			extended.Integer = static_cast<unsigned short int>(value);
			break;
		}
		case 1071:
		{
			long value;
			if (StringToInt(mCurrentValue, &value) == FALSE || value < INT_MIN || value > INT_MAX) value = 0;
			extended.Long = static_cast<int>(value);
			break;
		}
		default:
		{
			break;
		}
	}
}





STATUS TDXFParser::ParseObjectsSection()
{
	return SkipUntilSectionEnd();
}





TString TDXFParser::ParseDXFText(const TString & iDXFText)
{
	#ifdef UNICODE
		#error UNICODE text translation not implemented yet!
	#endif  // else UNICODE

	return iDXFText;
}





AString TDXFParser::ParseToDXFText(const TString & iText)
{
	UNREFERENCED_PARAMETER(iText);

	return AString();
}





STATUS TDXFParser::ReadNext()
{
	char * txt = mStream->ReadLn();
	if (!txt)
	{
		mIsEOF = true;
		mCurrentGroup = -999;
		mCurrentValue = NULL;
		return ERROR_SUCCESS;
	}

	long groupValue;
	if (StringToInt(InPlaceTrim(txt), &groupValue) == FALSE
		|| groupValue < INT_MIN || groupValue > INT_MAX) groupValue = 0;
	mCurrentGroup = static_cast<int>(groupValue);
	txt = mStream->ReadLn();
	if (!txt)
	{
		mIsEOF = true;
		mCurrentGroup = -999;
		mCurrentValue = NULL;
		return ERROR_INVALID_DATA;
	}
	mCurrentValue = InPlaceTrim(txt);
	return ERROR_SUCCESS;
}





TDXFDrawing * TDXFParser::ReleaseDrawing(void)
{
	TDXFDrawing * Drawing = mDrawing;
	mDrawing = NULL;
	return Drawing;
}





STATUS TDXFParser::Parse(TMStream * iStream, TDXFDrawing  * & oDrawing)
{
	TDXFParser Parser(iStream);
	RETURN_IF_FAILED(Parser.Parse());
	oDrawing = Parser.ReleaseDrawing();
	return ERROR_SUCCESS;
}





STATUS TDXFParser::Parse(const TString & iFileName, TDXFDrawing * & oDrawing)
{
	TMFileStream Stream(iFileName.c_str(), GENERIC_READ, OPEN_EXISTING);
	if (!Stream.IsFileOpen())
	{
		return GetLastError();
	}
	return Parse(&Stream, oDrawing);
}





STATUS TDXFParser::ParseLayerNames(TMStream * iStream, TStrings & oLayerNames)
{
	TDXFParser Parser(iStream);
	RETURN_IF_FAILED(Parser.Parse(false));
	oLayerNames.clear();
	for (TDXFLayers::const_iterator itr = Parser.mDrawing->Layers.begin(); itr != Parser.mDrawing->Layers.end(); ++itr)
	{
		oLayerNames.push_back((*itr)->Name);
	}  // for itr - mDrawing->Layers[]
	return ERROR_SUCCESS;
}





STATUS TDXFParser::ParseLayerNames(const TString & iFileName, TStrings & oLayerNames)
{
	TMFileStream Stream(iFileName.c_str(), GENERIC_READ, OPEN_EXISTING);
	if (!Stream.IsFileOpen())
	{
		return GetLastError();
	}
	return ParseLayerNames(&Stream, oLayerNames);
}





void MText::RemoveFormatting()
{
	// AutoCAD DXF - Format Multiline Text in an Alternate Text Editor

	std::string unformatted;
	unsigned int bracesNestedLevel = 0u;
	bool isCodeParameter = false;

	for (std::string::const_iterator i = Caption.begin(); i != Caption.end(); ++i)
	{
		if (isCodeParameter)
		{
			if (*i == ';')
			{
				isCodeParameter = false;
			}

			continue;
		}
		else if (*i == '{')
		{
			++bracesNestedLevel;
			continue;
		}
		else if (*i == '}')
		{
			// If the level is 0, skip it and tolerate this fault
			if (bracesNestedLevel > 0)
			{
				--bracesNestedLevel;
			}

			continue;
		}
		else if (*i != '\\')
		{
			unformatted.push_back(*i);
			continue;
		}

		++i;
		if (i == Caption.end())
		{
			unformatted.push_back('\\');
			break;
		}

		switch (*i)
		{
			case 'L':  // Turns overline on
			case 'l':  // Turns overline off
			case 'O':  // Turns underline on
			case 'o':  // Turns underline off
			case 'K':  // Turns strike-through on
			case 'k':  // Turns strike-through off
				// Skip
				break;
			case '~':  // Inserts a nonbreaking space
				unformatted.push_back(' ');
				break;
			case '\\':  // Inserts a backslash
				unformatted.push_back('\\');
				break;
			case '{':  // Inserts an opening brace
				unformatted.push_back('{');
				break;
			case '}':  // Inserts a closing brace
				unformatted.push_back('}');
				break;
			case 'P':  // Ends paragraph
				unformatted.push_back(' ');
				break;
			case 'X':  // Paragraph wrap on the dimension line (only in dimensions)
			case 'C':  // Changes to the specified color
			case 'F':  // Changes to the specified font file
			case 'f':  // -||-
			case 'H':  // Changes to the text height specified in drawing units or
							// Changes the text height to a multiple of the current text height
			case 'S':  // Stacks the subsequent text at the /, #, or ^ symbol
			case 'T':  // Adjusts the space between characters
			case 'Q':  // Changes obliquing angle
			case 'W':  // Changes width factor to produce wide text
			case 'A':  // Sets the alignment value; valid values: 0, 1, 2 (bottom, center, top)
				isCodeParameter = true;
				break;
			case 'p':
				if ((*(i + 1u) == 'x') && (*(i + 2u) == 'i'))
				{
					// \pxi format code
					isCodeParameter = true;
				}
				else
				{
					unformatted.push_back('\\');
					unformatted.push_back(*i);
				}
				break;
			case 'U':
				if (*(i + 1u) == '+')
				{
					++i;

					unsigned int wideCharacter = 0u;
					for (size_t byteCounter = 0u; byteCounter < 4u; ++byteCounter)
					{
						++i;
						if (i != Caption.end())
						{
							wideCharacter <<= 4u;

							if ((*i >= '0') && (*i <= '9'))
							{
								wideCharacter += (*i - '0');
							}
							else if ((*i >= 'a') && (*i <= 'f'))
							{
								wideCharacter += (*i - 'a') + 10u;
							}
							else if ((*i >= 'A') && (*i <= 'F'))
							{
								wideCharacter += (*i - 'A') + 10u;
							}
						}
						else
						{
							break;
						}
					}

					if (wideCharacter < 0x7fu)
					{
						// The first 128 characters are the same for ANSI and UNICODE
						unformatted.push_back(static_cast<TCHAR>(wideCharacter));
					}
					else
					{
						WString wideString;
						wideString.push_back(static_cast<wchar_t>(wideCharacter));
						// It converts character from UNICODE to ANSI of current codepage
						const std::string & utf8String(UnicodeToText(wideString));
						unformatted.append(utf8String);
					}
					break;
				}
				// Fall through
			default:  // Unrecognized code
				unformatted.push_back('\\');
				unformatted.push_back(*i);
				break;
		}
	}

	Caption.assign(unformatted);
}





#endif 0
