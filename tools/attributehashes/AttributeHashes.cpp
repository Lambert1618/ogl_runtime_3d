/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union

#ifndef _WIN32_WINNT // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT                                                                               \
    0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif

//==============================================================================
//    Includes
//==============================================================================
#include <stdio.h>

#if defined(_PCPLATFORM) || defined(_TEGRAPLATFORM)
#include <tchar.h>
#endif

#include <string.h>
#include <ctype.h>
#include <vector>
#include <fstream>
#include <string>

#include "Qt3DSConfig.h"
#include "Qt3DSTypes.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSHash.h"

//==============================================================================
//    Types
//==============================================================================
typedef std::vector<std::string> TStringList;

//==============================================================================
//    Constants
//==============================================================================
const char g_Text[] = "Qt3DSAttributeHashes.txt";
const char g_Include[] = "Qt3DSAttributeHashes.h";
const char g_Source[] = "Qt3DSAttributeHashes.cpp";
const char g_TestString[] = "qt.io";

//==============================================================================
//    Globals
//==============================================================================
TStringList g_StringList;
TStringList g_EnumList;

//==============================================================================
/**
 *    Convert a string to an enum entry and write it out
 */
void WriteEnumEntry(FILE *inDestination, const char *inEnum, const char *inString)
{
    // Save out the enum
    if (inEnum) {
        // Right justify the value by cutting a string of spaces
        char thePadding[] = "                               ";
        int theLength = (int)strlen(inEnum);
        theLength = theLength > 30 ? 30 : theLength;
        thePadding[30 - theLength] = '\0';

        // "ATTRIBUTE_NAME =          0x04CECA21, // name"
        fprintf(inDestination, "    %s = %s0x%08X, // %s\n", inEnum, thePadding,
                Q3DStudio::CHash::HashAttribute(inString), inString);
    } else
        fprintf(inDestination, "\n");
}

//==============================================================================
/**
 *    Write out the enum section
 */
void WriteEnum(FILE *inDestination)
{
    // Enum entry
    fprintf(inDestination, "/// Key for the CElement attribute-value pair\n"
                           "enum EAttribute {\n");

    // Write all enums
    for (TStringList::size_type theIndex = 0; theIndex < g_EnumList.size(); ++theIndex)
        if (!g_StringList[theIndex].empty())
            WriteEnumEntry(inDestination, g_EnumList[theIndex].c_str(),
                           g_StringList[theIndex].c_str());

    // Enum Footer
    fprintf(inDestination, "}; // enum EAttribute\n"
                           "\n"
                           "#define AK_STRING_QT_IO \"%s\"\n",
            g_TestString);
}

//==============================================================================
/**
 *    Convert strings to enums
 */
void ProcessEnums()
{
    // Write all enums
    for (TStringList::iterator theAttribute = g_StringList.begin();
         theAttribute != g_StringList.end(); ++theAttribute) {
        std::string theEnum = std::string("ATTRIBUTE_");
        theEnum += *theAttribute;

        // Replace all '.' with '_'
        // Make uppercase
        for (std::string::size_type thePosition = 0; thePosition < theEnum.length();
             ++thePosition) {
            if (theEnum[thePosition] == '.')
                theEnum[thePosition] = '_';

            if (theEnum[thePosition] == '\n' || theEnum[thePosition] == '\r')
                theEnum[thePosition] = '\0';

            theEnum[thePosition] = static_cast<char>(toupper(theEnum[thePosition]));
        }

        g_EnumList.push_back(theEnum);
    }
}

//==============================================================================
/**
 *    Write out lookup function section
 */
void WriteLookup(FILE *inDestination)
{
    // Lookup function entry
    fprintf(inDestination, "\n"
                           "\n"
                           "/// Function providing reverse hash lookup\n"
                           "const char *GetAttributeString(const EAttribute inAttribute)\n"
                           "{\n"
                           "    switch (inAttribute) {\n");

    // Write all enums
    for (TStringList::size_type theIndex = 0; theIndex < g_EnumList.size(); ++theIndex)
        if (!g_StringList[theIndex].empty())
            fprintf(inDestination, "        case %s: return \"%s\";\n", g_EnumList[theIndex].c_str(),
                    g_StringList[theIndex].c_str());

    // End function
    fprintf(inDestination,
            "        default: {\n"
            "            static char s_UnknownHash[16];\n"
            "            sprintf(s_UnknownHash, \"(0x%%08X)\", inAttribute);\n"
            "            return s_UnknownHash;\n"
            "        }\n"
            "    }\n"
            "}\n");
}

//==============================================================================
/**
 *    Single lookup to check new strings
 */
int SingleLookup(const char *inString)
{
    fprintf(stdout, "String: %s\n", inString);
    fprintf(stdout, "Hash:   0x%08X\n", Q3DStudio::CHash::HashAttribute(inString));
    return 0;
}

//==============================================================================
/**
 *    Main function.
 *    Open two files, read lines from source and write enum entry to destination.
 */
int main(int argc, char *argv[])
{
    // Application notice
    fprintf(
        stdout,
        "AttributeHashes\nConverting a string or a file of strings into enums of hashes...\n\n");

    // Checking a single string?
    if (argc > 1)
        return SingleLookup(argv[1]);

    // Standard file conversion
    fprintf(stdout, "Input:  %s\n", g_Text);
    fprintf(stdout, "Output: %s, %s\n\n", g_Include, g_Source);

    // Open source
    std::ifstream theText(g_Text);
    if (!theText) {
        fprintf(stderr, "Failed: Could not input text file, set working directory to "
                        "src\\Runtime\\ogl-runtime\\src\\runtime and try again.\n");
        return -1;
    }

    // Add all the strings in text file
    while (theText) {
        std::string theLine;
        theText >> theLine;
        g_StringList.push_back(theLine);
    }
    theText.close();

    // Add unit test string
    g_StringList.push_back(g_TestString);

    // Process all string to enums
    ProcessEnums();

    // Open include file
    FILE *theInclude = NULL;
    fopen_s(&theInclude, g_Include, "w");
    if (!theInclude) {
        fprintf(stderr, "Failed: Could not open output include file\n");
        return -1;
    }

    const char* theFileHeader =
        "/****************************************************************************\n"
        "**\n"
        "** Copyright (C) 1993-2009 NVIDIA Corporation.\n"
        "** Copyright (C) 2019 The Qt Company Ltd.\n"
        "** Contact: https://www.qt.io/licensing/\n"
        "**\n"
        "** This file is part of Qt 3D Studio.\n"
        "**\n"
        "** $QT_BEGIN_LICENSE:GPL$\n"
        "** Commercial License Usage\n"
        "** Licensees holding valid commercial Qt licenses may use this file in\n"
        "** accordance with the commercial license agreement provided with the\n"
        "** Software or, alternatively, in accordance with the terms contained in\n"
        "** a written agreement between you and The Qt Company. For licensing terms\n"
        "** and conditions see https://www.qt.io/terms-conditions. For further\n"
        "** information use the contact form at https://www.qt.io/contact-us.\n"
        "**\n"
        "** GNU General Public License Usage\n"
        "** Alternatively, this file may be used under the terms of the GNU\n"
        "** General Public License version 3 or (at your option) any later version\n"
        "** approved by the KDE Free Qt Foundation. The licenses are as published by\n"
        "** the Free Software Foundation and appearing in the file LICENSE.GPL3\n"
        "** included in the packaging of this file. Please review the following\n"
        "** information to ensure the GNU General Public License requirements will\n"
        "** be met: https://www.gnu.org/licenses/gpl-3.0.html.\n"
        "**\n"
        "** $QT_END_LICENSE$\n"
        "**\n"
        "****************************************************************************/\n\n";

    // Header
    fprintf(theInclude, theFileHeader);
    fprintf(
        theInclude,
        "#pragma once\n"
        "\n"
        "//==============================================================================\n"
        "//    Namespace\n"
        "//==============================================================================\n"
        "namespace Q3DStudio {\n"
        "\n"
        "// !!!!! AUTOGENERATED CODE - DO NOT MODIFY MANUALLY !!!!!\n"
        "\n"
        "// Run the AttributeHashes project to regenerate this file from Attributehashes.txt list\n"
        "\n");

    WriteEnum(theInclude);

    // Footer
    fprintf(theInclude, "\n"
                        "/// Function providing reverse hash lookup\n"
                        "const char *GetAttributeString(const EAttribute inAttribute);\n"
                        "\n"
                        "} // namespace Q3DStudio\n"
                        "\n");

    // Close include
    fclose(theInclude);

    // Open source file
    FILE *theSource = NULL;
    fopen_s(&theSource, g_Source, "w");
    if (!theSource) {
        fprintf(stderr, "Failed: Could not open output source file\n");
        return -1;
    }

    fprintf(theSource, theFileHeader);

    // Source
    fprintf(
        theSource,
        "#include \"RuntimePrefix.h\"\n"
        "\n"
        "//==============================================================================\n"
        "//    Includes\n"
        "//==============================================================================\n"
        "#include \"Qt3DSAttributeHashes.h\"\n"
        "\n"
        "//==============================================================================\n"
        "//    Namespace\n"
        "//==============================================================================\n"
        "namespace Q3DStudio {\n"
        "\n"
        "// !!!!! AUTOGENERATED CODE - DO NOT MODIFY MANUALLY !!!!!\n"
        "\n"
        "// Run the AttributeHashes project to regenerate this file from Attributehashes.txt list\n"
        "\n");

    WriteLookup(theSource);

    // Footer
    fprintf(theSource, "\n"
                       "} // namespace Q3DStudio\n"
                       "\n");

    // Close include
    fclose(theSource);

    // Completed
    fprintf(stdout, "Completed converting %d string to hashes.\n", int(g_StringList.size()));
    return 0;
}
