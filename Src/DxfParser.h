// DxfParser.h

#pragma once

#include "DxfDrawing.h"




#if 0
namespace Dxf
{





// fwd:
class Parser;





class Parser
{
protected:
	TMStream *    mStream;
	TDXFDrawing * mDrawing;  // Owned by this until ReleaseDrawing() is called

	bool   mIsEOF;
	bool   mNext;
	int    mCurrentGroup;
	TCHAR * mCurrentValue;

	explicit TDXFParser(TMStream * iStream);
	~TDXFParser();

	STATUS ReadNext(void);  // Updates mCurrentGroup and mCurrentValue, sets mIsEOF if appropriate
	STATUS Parse(bool iContinueAfterLayerList = true);  // Can both parse the entire file or only the layer list
	STATUS ParseHeaderSection();
	STATUS ParseClassesSection();
	STATUS ParseTablesSection();
	STATUS ParseBlocksSection();
	STATUS ParseEntitiesSection(TDXFBlockDefinition * iParentBlockDef);
	void ParseExtendedEntityData(TDXFExtendedData & extended);
	STATUS ParseObjectsSection();
	STATUS SkipUntilSectionEnd(void);  // Skips mStream until an "0, endsec" pair is found

	TString ParseDXFText(const AString & iDXFText);  // Parses DXF text into displayable text
	AString ParseToDXFText(const TString & iText);  // Parses displayable text into DXF text

	TDXFDrawing * ReleaseDrawing(void);  // Releases ownership of mDrawing to the caller

public:
	static STATUS Parse          (const TString & iFileName,  TDXFDrawing * & oDrawing);  // oDrawing is supposed to get owned by caller
	static STATUS Parse          (TMStream * iStream, TDXFDrawing * & oDrawing);  // oDrawing is supposed to get owned by caller
	static STATUS ParseLayerNames(const TString & iFileName,  TStrings & oLayerNames);
	static STATUS ParseLayerNames(TMStream * iStream, TStrings & oLayerNames);

} ;





#endif  0
