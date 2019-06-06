/****************************************************************************
**
** Copyright (C) 2007-2008 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef _QT3DS_CONFIG_H
#define _QT3DS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <KD/kd.h>

/** \file nv_config.h
    Support for block-based text config files.

    Supports:
    <ul>
        <li> C-style comment blocks
        <li> C++-style comment lines
        <li> Line continuation via backslash
        <li> Linux, Mac and Windows line endings
        <li> Arrays of tokens, broken out by line
        <li> Tokens are split by commas and whitespace
        <li> Quoted string tokens containing any characters
    </ul>
 */

/** The in-memory representation of a loaded config file.

    This opaque data structure is passed by pointer to all of the
    configuration functions and represents the loaded configuration
    file.  Once loaded, there is no mutable persistent state in this
    object.  It is passed as const to all functions
*/
typedef struct NvConfigData NvConfigData;

/** The representation of a sub-range of lines in a config file.

    This structure is application-readable and mutable, and is used to
    represent a subset of the lines in a config file.  Generally, these
    ranges represent delineated blocks in the config file.  Special versions
    of many of the config file functions take data ranges instead of
    entire config files

    @see NvConfigFindDelimitedBlock()
*/
typedef struct NvConfigDataRange
{
    /** A reference to the "parent" config file for this range */
    NvConfigData *data;
    /** The first line of the range (inclusive) */
    KDint startLine;
    /** The last line of the range (inclusive) */
    KDint endLine;
} NvConfigDataRange;

/** Creates a config data object that represents an existing config file

    @return A pointer to the allocated config data, or NULL if the file cannot
        be found or does not conform to the basic config file format
    @param filename The path of the config file to be loaded
    @see NvConfigFree()
*/
NvConfigData *NvConfigCreateFromFile(const KDchar *filename);

/** Creates a config data object from an-in memory string

    Useful when the config file is loaded from a larger, packed data file,
    or generated on-the-fly in the application.  Note that the code does not
    automatically handle NULL-terminated strings

    @return A pointer to the allocated config data, or NULL if the string
    does not conform to the basic config file format
    @param fileBuffer A pointer to the start of the string of config data
    @param fileSize The length of the string in characters
    @see NvConfigFree()
*/
NvConfigData *NvConfigCreateFromBuffer(const KDchar *fileBuffer, KDint fileSize);

/** Deletes an existing config file data object

    @param data A pointer to the data object to delete
*/
void NvConfigFree(NvConfigData *data);

/** Returns the number of token-bearing lines in the config data object.

    This does not count merged lines, comment lines, or whitespace.  This
    value is one greater than the maximum valid line index in the file

    @return The line count
    @param data A pointer to the data object
*/
KDint NvConfigGetLineCount(const NvConfigData *data);

/** Returns the number of tokens in the indexed line

    @return The token count
    @param data A pointer to the data object
    @param line The line whose count is to be returned.  This parameter must
    be at least 0 and must be less than the line count of the object
    @see NvConfigGetLineCount
*/
KDint NvConfigGetTokenCount(const NvConfigData *data, KDint line);

/** Returns a NULL-terminated string token at the given line and token index

    The parameters are unchecked.

    @return A non-mutable token pointer for the referenced token
    @param data A pointer to the data object
    @param line The line whose token is to be returned.  This parameter must
    be at least 0 and must be less than the line count of the object
    @param token The token index of the token to be returned.  This parameter must
    be at least 0 and must be less than the token count of the line
    @see NvConfigGetLineCount
    @see NvConfigGetTokenCount
*/
const KDchar *NvConfigGetToken(const NvConfigData *data, KDint line, KDint token);

/** Returns the string token converted to an integer

    Returns the string token at the given line and token index converted to an integer.
    The parameters are unchecked, and the string token is assumed to be convertible to
    an integer value.

    @return The integer value of the referenced token
    @param data A pointer to the data object
    @param line The line whose token is to be returned.  This parameter must
    be at least 0 and must be less than the line count of the object
    @param token The token index of the token to be returned.  This parameter must
    be at least 0 and must be less than the token count of the line
    @see NvConfigGetLineCount
    @see NvConfigGetTokenCount
*/
KDint NvConfigGetIntToken(const NvConfigData *data, KDint line, KDint token);

/** Returns the string token converted to a floating-point value

    Returns the string token at the given line and token index converted to a
    floating-point number. The parameters are unchecked, and the string token
    is assumed to be convertible to a real value.

    @return The floating-point value of the referenced token
    @param data A pointer to the data object
    @param line The line whose token is to be returned.  This parameter must
    be at least 0 and must be less than the line count of the object
    @param token The token index of the token to be returned.  This parameter must
    be at least 0 and must be less than the token count of the line
    @see NvConfigGetLineCount
    @see NvConfigGetTokenCount
*/
KDfloat32 NvConfigGetFloatToken(const NvConfigData *data, KDint line, KDint token);

/** Returns the count of lines in the config data whose leading token
    matches a given string

    The search starts at the indicated line and continues to the end of the file

    @return The count of matching lines
    @param data A pointer to the data object
    @param name The string to be matched (case-sensitive)
    @param startLine The line at which the search should start.  This parameter must
    be at least 0 and must be less than the line count of the object
    @see NvConfigGetLineCount
*/
KDint NvConfigCountMatchingLines(const NvConfigData *data, const KDchar *name, KDint startLine);

/** Returns the count of lines in the subrange of the config data
    whose leading token matches a given string

    The search is run only in the given subrange of the file

    @return The count of matching lines
    @param range A pointer to the subrange of the data object
    @param name The string to be matched (case-sensitive)
*/
KDint NvConfigCountMatchingLinesInRange(const NvConfigDataRange *range, const KDchar *name);

/** Returns the index of the first line in the config data (at or after
    the given start line) whose leading token matches a given string

    The search starts at the indicated line and continues to the end of the
    file

    @return The index of the first matching line, or -1 if no match
    @param data A pointer to the data object
    @param name The string to be matched (case-sensitive)
    @param startLine The line at which the search should start.  This parameter must
    be at least 0 and must be less than the line count of the object
    @see NvConfigGetLineCount
*/
KDint NvConfigFindMatchingLine(const NvConfigData *data, const KDchar *name, KDint startLine);

/** Returns the index of the first line in the subrange of the config data
    whose leading token matches a given string

    The search is run only in the given subrange of the file

    @return The index of the first matching line, or -1 if no match
    @param range A pointer to the subrange of the data object
    @param name The string to be matched (case-sensitive)
*/
KDint NvConfigFindMatchingLineInRange(const NvConfigDataRange *range, const KDchar *name);

/** Finds a matching token-delimited block in the file and returns it as a subrange.

    Searches for a token-delimited open/close block in the given line range.
    If the nested parameter is true, then additional nested open/close blocks
    are tracked and the returned block is the one that properly closes the
    given open block.  If the nested flag is false, then the open/close are
    treated like C-comments, and the first closing token found is returned

    @return KD_TRUE if a block with both an open and a close token is found,
    KD_FALSE otherwise
    @param data A pointer to the data object
    @param openText The string token to be matched as the start of a block.
    (case-sensitive) Only the leading token of each line is tested
    @param closeText The string token to be matched as the end of a block.
    (case-sensitive) Only the leading token of each line is tested
    @param nested If KD_TRUE, C-bracket-style nesting is used to determine
    which end token is matched.  If KD_FALSE, C-comment-style flat matching
    is used to match the end token.
    @param startLine The index of the first line to search (inclusive)
    @param endLine The index of the last line to search (inclusive)
    @param range Output: A pointer to the subrange of the data object.  This
    should be a pointer to an allocated struct
*/
KDboolean NvConfigFindDelimitedBlock(const NvConfigData *data, const KDchar *openText,
                                     const KDchar *closeText, KDboolean nested, KDint startLine,
                                     KDint endLine, NvConfigDataRange *range);

/** Returns the index of the first matching token in the given line

    Returns the index of the first token on the given line (if any) that
    matches the given string.

    @return The index of the matching string token in the line, or else -1 if
    the token is not found in the line
    @param data A pointer to the data object
    @param token The string to be matched (case-sensitive)
    @param line The line to search.  This parameter must
    be at least 0 and must be less than the line count of the object
    @see NvConfigGetLineCount
*/
KDint NvConfigFindMatchingTokenInLine(const NvConfigData *data, const KDchar *token, KDint line);

#ifdef __cplusplus
};
#endif

#endif
