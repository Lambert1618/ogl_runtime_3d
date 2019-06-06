/****************************************************************************\
Datei  : Color.cpp
Projekt: Farbverwaltung
Inhalt : CColor Implementierung
Datum  : 10.01.1999
Autor  : Christian Rodemeyer
Hinweis: (c) 1999 by Christian Rodemeyer
         Info �ber HLS Konvertierungsfunktion
         - Foley and Van Dam: "Fundamentals of Interactive Computer Graphics"
         - MSDN: 'HLS Color Spaces'
         - MSDN: 'Converting Colors Between RGB and HLS'
\****************************************************************************/
#include "Qt3DSCommonPrecompile.h"
#include "CColor.h"

float GetMin( float in1, float in2 );
float GetMin( float in1, float in2 )
{
	return ( in1 < in2 ) ? in1 :  in2;
}

float GetMax( float in1, float in2 );
float GetMax( float in1, float in2 )
{
	return ( in1 > in2 ) ? in1 : in2;
}

/****************************************************************************\
 CColor: Implementierung
\****************************************************************************/
const CColor::DNamedColor CColor::m_namedColor[CColor::numNamedColors] =
{
  {aliceblue            , "aliceblue"},
  {antiquewhite         , "antiquewhite"},
  {aqua                 , "aqua"},
  {aquamarine           , "aquamarine"},
  {azure                , "azure"},
  {beige                , "beige"},
  {bisque               , "bisque"},
  {black                , "black"},
  {blanchedalmond       , "blanchedalmond"},
  {blue                 , "blue"},
  {blueviolet           , "blueviolet"},
  {brown                , "brown"},
  {burlywood            , "burlywood"},
  {cadetblue            , "cadetblue"},
  {chartreuse           , "chartreuse"},
  {chocolate            , "chocolate"},
  {coral                , "coral"},
  {cornflower           , "cornflower"},
  {cornsilk             , "cornsilk"},
  {crimson              , "crimson"},
  {cyan                 , "cyan"},
  {darkblue             , "darkblue"},
  {darkcyan             , "darkcyan"},
  {darkgoldenrod        , "darkgoldenrod"},
  {darkgray             , "darkgray"},
  {darkgreen            , "darkgreen"},
  {darkkhaki            , "darkkhaki"},
  {darkmagenta          , "darkmagenta"},
  {darkolivegreen       , "darkolivegreen"},
  {darkorange           , "darkorange"},
  {darkorchid           , "darkorchid"},
  {darkred              , "darkred"},
  {darksalmon           , "darksalmon"},
  {darkseagreen         , "darkseagreen"},
  {darkslateblue        , "darkslateblue"},
  {darkslategray        , "darkslategray"},
  {darkturquoise        , "darkturquoise"},
  {darkviolet           , "darkviolet"},
  {deeppink             , "deeppink"},
  {deepskyblue          , "deepskyblue"},
  {dimgray              , "dimgray"},
  {dodgerblue           , "dodgerblue"},
  {firebrick            , "firebrick"},
  {floralwhite          , "floralwhite"},
  {forestgreen          , "forestgreen"},
  {fuchsia              , "fuchsia"},
  {gainsboro            , "gainsboro"},
  {ghostwhite           , "ghostwhite"},
  {gold                 , "gold"},
  {goldenrod            , "goldenrod"},
  {gray                 , "gray"},
  {green                , "green"},
  {greenyellow          , "greenyellow"},
  {honeydew             , "honeydew"},
  {hotpink              , "hotpink"},
  {indianred            , "indianred"},
  {indigo               , "indigo"},
  {ivory                , "ivory"},
  {khaki                , "khaki"},
  {lavender             , "lavender"},
  {lavenderblush        , "lavenderblush"},
  {lawngreen            , "lawngreen"},
  {lemonchiffon         , "lemonchiffon"},
  {lightblue            , "lightblue"},
  {lightcoral           , "lightcoral"},
  {lightcyan            , "lightcyan"},
  {lightgoldenrodyellow , "lightgoldenrodyellow"},
  {lightgreen           , "lightgreen"},
  {lightgrey            , "lightgrey"},
  {lightpink            , "lightpink"},
  {lightsalmon          , "lightsalmon"},
  {lightseagreen        , "lightseagreen"},
  {lightskyblue         , "lightskyblue"},
  {lightslategray       , "lightslategray"},
  {lightsteelblue       , "lightsteelblue"},
  {lightyellow          , "lightyellow"},
  {lime                 , "lime"},
  {limegreen            , "limegreen"},
  {linen                , "linen"},
  {magenta              , "magenta"},
  {maroon               , "maroon"},
  {mediumaquamarine     , "mediumaquamarine"},
  {mediumblue           , "mediumblue"},
  {mediumorchid         , "mediumorchid"},
  {mediumpurple         , "mediumpurple"},
  {mediumseagreen       , "mediumseagreen"},
  {mediumslateblue      , "mediumslateblue"},
  {mediumspringgreen    , "mediumspringgreen"},
  {mediumturquoise      , "mediumturquoise"},
  {mediumvioletred      , "mediumvioletred"},
  {midnightblue         , "midnightblue"},
  {mintcream            , "mintcream"},
  {mistyrose            , "mistyrose"},
  {moccasin             , "moccasin"},
  {navajowhite          , "navajowhite"},
  {navy                 , "navy"},
  {oldlace              , "oldlace"},
  {olive                , "olive"},
  {olivedrab            , "olivedrab"},
  {orange               , "orange"},
  {orangered            , "orangered"},
  {orchid               , "orchid"},
  {palegoldenrod        , "palegoldenrod"},
  {palegreen            , "palegreen"},
  {paleturquoise        , "paleturquoise"},
  {palevioletred        , "palevioletred"},
  {papayawhip           , "papayawhip"},
  {peachpuff            , "peachpuff"},
  {peru                 , "peru"},
  {pink                 , "pink"},
  {plum                 , "plum"},
  {powderblue           , "powderblue"},
  {purple               , "purple"},
  {red                  , "red"},
  {rosybrown            , "rosybrown"},
  {royalblue            , "royalblue"},
  {saddlebrown          , "saddlebrown"},
  {salmon               , "salmon"},
  {sandybrown           , "sandybrown"},
  {seagreen             , "seagreen"},
  {seashell             , "seashell"},
  {sienna               , "sienna"},
  {silver               , "silver"},
  {skyblue              , "skyblue"},
  {slateblue            , "slateblue"},
  {slategray            , "slategray"},
  {snow                 , "snow"},
  {springgreen          , "springgreen"},
  {steelblue            , "steelblue"},
  {tan                  , "tan"},
  {teal                 , "teal"},
  {thistle              , "thistle"},
  {tomato               , "tomato"},
  {turquoise            , "turquoise"},
  {violet               , "violet"},
  {wheat                , "wheat"},
  {white                , "white"},
  {whitesmoke           , "whitesmoke"},
  {yellow               , "yellow"},
  {yellowgreen          , "yellowgreen"}
};

const char* CColor::GetNameFromIndex(int i)
{
	assert(0 <= i && i < numNamedColors);

	return m_namedColor[i].name;
}

CColor CColor::GetColorFromIndex(int i)
{
	if ( i < 0 ) i = 0;
	else if ( i >= numNamedColors ) i = numNamedColors - 1;

	return m_namedColor[i].color;
}

CColor CColor::FromString(const char* pcColor)
{
	CColor t( 0, 0, 0) ;
	t.SetString(pcColor);

	return t;
}

CColor::CColor(COLORTYPE cr) :
    m_colorref(cr),
    m_hue( 0.0 ),
    m_saturation( 0.0 ),
    m_luminance( 0.0 ),
    m_bIsRGB(true),
    m_bIsHLS(false)
{
}

CColor::CColor(const QColor &c) :
    CColor(c.red(), c.green(), c.blue())
{
}

CColor::operator COLORTYPE() const
{
    return RGB( GetRed( ), GetGreen( ), GetBlue( ) );
}

CColor::operator QColor() const
{
    return QColor( GetRed( ), GetGreen( ), GetBlue( ) );
}

// RGB

void CColor::SetColor(COLORTYPE color)
{
	this->SetRGB(GetRValue(color), GetGValue(color), GetBValue(color));
}

CColor::CColor( int red, int green, int blue ):
    m_hue( 0.0 ),
    m_saturation( 0.0 ),
     m_luminance( 0.0 ),
    m_bIsRGB( true ),
    m_bIsHLS( false )
{
	this->SetRGB( red, green, blue );
}

void CColor::SetRed(int red)
{
	red &= 0xFF;

	ToRGB();
	m_color[c_red] = static_cast<unsigned char>(red);
	m_bIsHLS = false;
}

void CColor::SetGreen(int green)
{
	green &= 0xFF;

	ToRGB();
	m_color[c_green] = static_cast<unsigned char>(green);
	m_bIsHLS = false;
}

void CColor::SetBlue(int blue)
{
	blue &= 0xFF;

	ToRGB();
	m_color[c_blue] = static_cast<unsigned char>(blue);
	m_bIsHLS = false;
}

void CColor::SetRGB(int red, int green, int blue)
{
	red &= 0xFF;
	blue &= 0xFF;
	green &= 0xFF;

	m_color[c_red]   = static_cast<unsigned char>(red);
	m_color[c_green] = static_cast<unsigned char>(green);
	m_color[c_blue]  = static_cast<unsigned char>(blue);
	m_bIsHLS = false;
	m_bIsRGB = true;
}

unsigned int CColor::GetRed() const
{
	const_cast<CColor*>(this)->ToRGB();

	return m_color[c_red];
}

unsigned int CColor::GetGreen() const
{
	const_cast<CColor*>(this)->ToRGB();

	return m_color[c_green];
}

unsigned int CColor::GetBlue() const
{
	const_cast<CColor*>(this)->ToRGB();

	return m_color[c_blue];
}

COLORTYPE CColor::GetRGBColor()
{
	const_cast<CColor*>(this)->ToRGB();

	return (RGB(m_color[c_red], m_color[c_green], m_color[c_blue])) ;
}

// HSL

void CColor::SetHue(float hue)
{
	if ( hue < 0 ) hue = 0;
	else if ( hue > 360 ) hue = 360;

	ToHLS();
	m_hue = hue;
	m_bIsRGB = false;
}

void CColor::SetSaturation(float saturation)
{
	if ( saturation < 0 ) saturation = 0;
	else if ( saturation > 1 ) saturation = 1;

	ToHLS();
	m_saturation = saturation;
	m_bIsRGB = false;
}

void CColor::SetLuminance(float luminance)
{
	if ( luminance < 0 ) luminance = 0;
	else if ( luminance > 1 ) luminance = 1;

	ToHLS();
	m_luminance = luminance;
	m_bIsRGB = false;
}

void CColor::SetHLS(float hue, float luminance, float saturation)
{
	if ( hue < 0 ) hue = 0;
	else if ( hue > 360 ) hue = 360;
	if ( saturation < 0 ) saturation = 0;
	else if ( saturation > 1 ) saturation = 1;
	if ( luminance < 0 ) luminance = 0;
	else if ( luminance > 1 ) luminance = 1;

	m_hue = hue;
	m_luminance = luminance;
	m_saturation = saturation;
	m_bIsRGB = false;
	m_bIsHLS = true;
}

float CColor::GetHue() const
{
	const_cast<CColor*>(this)->ToHLS();

	return m_hue;
}

float CColor::GetSaturation() const
{
	const_cast<CColor*>(this)->ToHLS();

	return m_saturation;
}

float CColor::GetLuminance() const
{
	const_cast<CColor*>(this)->ToHLS();

	return m_luminance;
}

// Konvertierung

void CColor::ToHLS()
{
	if (!m_bIsHLS)
	{
		// Konvertierung
		unsigned char minval = static_cast<unsigned char>( GetMin(m_color[c_red], GetMin(m_color[c_green], m_color[c_blue])) );
		unsigned char maxval = static_cast<unsigned char>( GetMax(m_color[c_red], GetMax(m_color[c_green], m_color[c_blue])) );
		float mdiff  = float(maxval) - float(minval);
		float msum   = float(maxval) + float(minval);

		m_luminance = msum / 510.0f;

		if (maxval == minval)
		{
			m_saturation = 0.0f;
			m_hue = 0.0f;
		}
		else
		{
			float rnorm = (maxval - m_color[c_red]  ) / mdiff;
			float gnorm = (maxval - m_color[c_green]) / mdiff;
			float bnorm = (maxval - m_color[c_blue] ) / mdiff;

			m_saturation = (m_luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

			if (m_color[c_red] == maxval)
				m_hue = 60.0f * (6.0f + bnorm - gnorm);

			if (m_color[c_green] == maxval)
				m_hue = 60.0f * (2.0f + rnorm - bnorm);

			if (m_color[c_blue]  == maxval)
				m_hue = 60.0f * (4.0f + gnorm - rnorm);

			if (m_hue > 360.0f)
				m_hue = m_hue - 360.0f;
		}

		m_bIsHLS = true;
	}
}

void CColor::ToRGB()
{
	if (!m_bIsRGB)
	{
		if (m_saturation == 0.0) // Grauton, einfacher Fall
		{
			m_color[c_red] = m_color[c_green] = m_color[c_blue] = (unsigned char)(m_luminance * 255.0);
		}
		else
		{
			float rm1, rm2;

			if (m_luminance <= 0.5f)
				rm2 = m_luminance + m_luminance * m_saturation;
			else
				rm2 = m_luminance + m_saturation - m_luminance * m_saturation;

			rm1 = 2.0f * m_luminance - rm2;
			m_color[c_red]   = ToRGB1(rm1, rm2, m_hue + 120.0f);
			m_color[c_green] = ToRGB1(rm1, rm2, m_hue);
			m_color[c_blue]  = ToRGB1(rm1, rm2, m_hue - 120.0f);
		}

		m_bIsRGB = true;
	}
}

unsigned char CColor::ToRGB1(float rm1, float rm2, float rh)
{
	if (rh > 360.0f)
		rh -= 360.0f;
	else
	if (rh < 0.0f)
		rh += 360.0f;

  	if (rh <  60.0f)
		rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
	else
	if (rh < 180.0f)
		rm1 = rm2;
	else
	if (rh < 240.0f)
		rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;

	return static_cast<unsigned char>(rm1 * 255);
}

// Stringkonvertierung im Format RRGGBB

Q3DStudio::CString CColor::GetString() const
{
    Q3DStudio::CString color;
    color.Format( _LSTR( "%02X%02X%02X" ), GetRed(), GetGreen(), GetBlue());

	return color;
}

bool CColor::SetString( const char* pcColor)
{
	int		r, g, b;
	bool	bRet = false;

	if (sscanf(pcColor, "%2x%2x%2x", &r, &g, &b) != 3)
	{
		m_color[c_red] = m_color[c_green] = m_color[c_blue] = 0;
	}
	else
	{
		m_color[c_red]   = static_cast<unsigned char>(r);
		m_color[c_green] = static_cast<unsigned char>(g);
		m_color[c_blue]  = static_cast<unsigned char>(b);
		m_bIsRGB = true;
		m_bIsHLS = false;
		bRet = true;
	}

	return (bRet) ;
}

Q3DStudio::CString CColor::GetName() const
{
    Q3DStudio::CString	theRetStr;

	const_cast<CColor*>(this)->ToRGB();
	int i = numNamedColors;
	while (i-- && m_colorref != m_namedColor[i].color) { }
	if (i < 0)
	{
		theRetStr = L"#" + GetString();
	}
	else
		theRetStr = m_namedColor[i].name;

	return (theRetStr);
}

/////////////////////////////////////////////////////////////////////////////
// global functions
/////////////////////////////////////////////////////////////////////////////

COLORTYPE GetHueColor(int iHue)
{
	COLORTYPE	theHueColor ;
	unsigned char r, g, b;

	HSVToRGB( r, g, b, (float) iHue, 1.0f, 1.0f );

	theHueColor = RGB( r, g, b );

	return (theHueColor) ;
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
	float min, max, delta;

	min = GetMin( GetMin( r, g ), b );
	max = GetMax( GetMax( r, g ), b );
	*v = max;				// v
	delta = max - min;
	if ( max != 0 )
		*s = delta / max;		// s
	else
	{
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if ( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else
	if( g == max )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60;				// degrees
	if ( *h < 0 )
		*h += 360;
}

void RGBToHSV( unsigned char r, unsigned char g, unsigned char b, float* h, float* s, float* v )
{
	// Convert an RGB to HSV

	float	fRed, fGreen, fBlue;

	// convert 0-255 values to 0-1 range
	fRed = ( ( float ) r ) / 255.0f;
	fGreen = ( ( float ) g ) / 255.0f;
	fBlue = ( ( float ) b ) / 255.0f;

	RGBtoHSV( fRed, fGreen, fBlue, h, s, v );
}

void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int		i;
	float	f, p, q, t;

	if ( s == 0 )
	{
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = ( int ) floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i )
	{
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;

		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;

		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;

		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;

		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;

		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}

void HSVToRGB( unsigned char& r, unsigned char& g, unsigned char& b, float h, float s, float v )
{
	// Convert an HSV to RGB

	float	fRed, fGreen, fBlue;

	HSVtoRGB( &fRed, &fGreen, &fBlue, h, s, v );

	r = ( unsigned char ) ( fRed * 255.0f );
	g = ( unsigned char ) ( fGreen * 255.0f );
	b = ( unsigned char ) ( fBlue * 255.0f );
}

short GetHueValue( COLORTYPE inColor )
{
	float	theHue, theSaturation, theBright;
	short	theHueValue;

	RGBToHSV( GetRValue( inColor ), GetGValue( inColor ), GetBValue( inColor ), &theHue, &theSaturation, &theBright );

	theHueValue = ( short ) theHue;

	return theHueValue;
}

short GetSaturationValue( COLORTYPE inColor )
{
	float	theHue, theSaturation, theBright;
	short	theSaturationValue;

	RGBToHSV( GetRValue( inColor ), GetGValue( inColor ), GetBValue( inColor ), &theHue, &theSaturation, &theBright );

	theSaturationValue = ( short ) ( theSaturation * 255.0f );

	return theSaturationValue;
}

short GetBrightnessValue( COLORTYPE inColor )
{
	float	theHue, theSaturation, theBright;
	short	theBrightnessValue;

	RGBToHSV( GetRValue( inColor ), GetGValue( inColor ), GetBValue( inColor ), &theHue, &theSaturation, &theBright );

	theBrightnessValue = ( short ) ( theBright * 255.0f );

	return theBrightnessValue;
}

CColor CColor::GetFractionalColor( long inIndex, long inCount )
{
	double theE = .2;
	double theRCenter = 1.0 / 6;
	double theGCenter = 3.0 / 6;
	double theBCenter = 5.0 / 6;
	double theRCenter2 = 7.0 / 6;
	double theBCenter2 = -1.0 / 6;

	double theInv2eSquare = -1.0 / ( 2 * theE * theE );
	double theMag = 60 / theE;

	double thePos = (double)inIndex / inCount;
	double theR = theMag * exp( ( thePos - theRCenter ) * ( thePos - theRCenter ) * theInv2eSquare );
	double theG = theMag * exp( ( thePos - theGCenter ) * ( thePos - theGCenter ) * theInv2eSquare );
	double theB = theMag * exp( ( thePos - theBCenter ) * ( thePos - theBCenter ) * theInv2eSquare );

	double theR2 = theMag * exp( ( thePos - theRCenter2 ) * ( thePos - theRCenter2 ) * theInv2eSquare );
	double theB2 = theMag * exp( ( thePos - theBCenter2 ) * ( thePos - theBCenter2 ) * theInv2eSquare );

	theR += theR2;
	theB += theB2;

	return CColor( (char)( theR + .5 ), (char)( theG + .5 ), (char)( theB + .5 ) );
}

QColor CColor::getQColor() const
{
    const_cast<CColor *>(this)->ToRGB();

    return QColor(m_color[c_red], m_color[c_green], m_color[c_blue]);
}
