/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#pragma once
#ifndef QT3DS_RENDER_STRING_H
#define QT3DS_RENDER_STRING_H
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <string>

#include "Qt3DSRender.h"
#include "EASTL/string.h"

namespace qt3ds {
namespace foundation {

class Qt3DSStringUtils {
public:
    static inline QString ConvertUTFtoQString(const char16_t *string)
    {
        return QString::fromUtf16(string);
    }

    static inline QString ConvertUTFtoQString(const char32_t *string)
    {
        return QString::fromUcs4(string);
    }

    static inline QString ConvertUTFtoQString(const wchar_t *string)
    {
        return QString::fromWCharArray(string);
    }

    static inline QString &append(QString &target, QString::iterator begin,
                                  QString::iterator end)
    {
        for (QString::iterator iter = begin;iter != end;++iter)
            target.append(*iter);
        return target;
    }

    static inline QString &append(QString &target, QString::const_iterator begin,
                                  QString::const_iterator end)
    {
        for (QString::const_iterator iter = begin;iter != end;++iter)
            target.append(*iter);
        return target;
    }

    /**
     * @brief replace Replaces specified portion of a string.
     * @param source Source text that will be used. Note: This is const and will
     * not be directly modified.
     * @param first First character in the range that will be replaced.
     * @param last Last character in the range that will be replaced.
     * @param replaceText Pointer to the character string to use for replacement.
     * @param len Length of the replacement character string.
     * @return Returns a new QString containing the modified result.
     */
    static inline QString replace(const QString &source,
                                   QString::const_iterator first,
                                   QString::const_iterator last,
                                   const char *replaceText, int len)
    {
        return replace(source, first, last, QString::fromUtf8(replaceText, len));
    }

    /**
     * @brief replace Replaces specified portion of a string.
     * @param source Source text that will be used. Note: This is const and will
     * not be directly modified.
     * @param first First character in the range that will be replaced.
     * @param last Last character in the range that will be replaced.
     * @param replaceText String to use for replacement.
     * @return Returns a new QString containing the modified result.
     */
    static inline QString replace(const QString &source,
                                   QString::const_iterator first,
                                   QString::const_iterator last,
                                   const QString &replaceText)
    {
        QString newStr;
        for (QString::const_iterator iter = source.constBegin(); iter != first;
             iter++)
            newStr.append(*iter);

        newStr.append(replaceText);

        for (QString::const_iterator iter = last; iter != source.constEnd();
             iter++)
            newStr.append(*iter);
        return newStr;
    }

    /**
     * @brief replace Replaces specified portion of a string.
     * @param source Source text that will be used. Note: This is const and
     * will not be directly modified.
     * @param first First character in the range that will be replaced.
     * @param last Last character in the range that will be replaced.
     * @param replaceText Pointer to the character string to use for replacement.
     * @param len Length of the replacement character string.
     * @return Returns a temporary object allocated on the stack containing
     * the modified result.
     */
    static inline QString &replace(QString &target,
                                   QString::iterator first,
                                   QString::iterator last,
                                   const char *replaceText, int len)
    {
        return replace(target, first, last, QString::fromUtf8(replaceText, len));
    }

    /**
     * @brief replace Replaces specified portion of a string.
     * @param target Target QString that will be modified.
     * @param first First character in the range that will be replaced.
     * @param last Last character in the range that will be replaced.
     * @param replaceText String to use for replacement.
     * @return Returns the string given in target, containing the modified result.
     */
    static inline QString &replace(QString &target,
                                   QString::iterator first,
                                   QString::iterator last,
                                   const QString &replaceText)
    {
        QString newStr;
        for (QString::iterator iter = target.begin(); iter != first; iter++)
            newStr.append(*iter);

        newStr.append(replaceText);

        for (QString::iterator iter = last; iter != target.end(); iter++)
            newStr.append(*iter);

        target = newStr;
        return target;
    }
};

class Qt3DSString {
public:
    inline Qt3DSString() {}

    Qt3DSString(QChar c) : m_string(QString(c)) {}

    Qt3DSString(int size, QChar c) : m_string(size, c) {}

    inline Qt3DSString(QLatin1String latin1)
        : m_string(latin1) {}

    explicit Qt3DSString(const QChar *unicode, int size = -1)
        : m_string(unicode,size) {}

    inline Qt3DSString(const QString &str) Q_DECL_NOTHROW
        : m_string(str) {}

    inline Qt3DSString(const Qt3DSString &str) Q_DECL_NOTHROW
        : m_string(str.m_string) {}

    ~Qt3DSString() {}

    inline operator QString() const
    {
        return m_string;
    }

    inline void operator=(const Qt3DSString &text)
    {
        m_string = text.m_string;
        m_isDirty = true;
    }

    inline void operator=(const QString &text)
    {
        m_string = text;
        m_isDirty = true;
    }

    // QString method wrappers
    static inline Qt3DSString fromUtf8(const char *str, int size = -1)
    {
        return Qt3DSString(QString::fromUtf8(str, size));
    }

    typedef int size_type;
    typedef qptrdiff difference_type;
    typedef const QChar & const_reference;
    typedef QChar & reference;
    typedef QChar *pointer;
    typedef const QChar *const_pointer;
    typedef QChar value_type;
    typedef QChar *iterator;
    typedef const QChar *const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    inline iterator begin() { return m_string.begin(); }
    inline const_iterator begin() const { return m_string.begin(); }
    inline const_iterator cbegin() const { return m_string.cbegin(); }
    inline const_iterator constBegin() const  { return m_string.constBegin(); }
    inline iterator end()  { return m_string.end(); }
    inline const_iterator end() const  { return m_string.end(); }
    inline const_iterator cend() const  { return m_string.cend(); }
    inline const_iterator constEnd() const  { return m_string.constEnd(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    inline void push_back(QChar c)
    {
        m_string.push_back(c);
        m_isDirty = true;
    }

    inline void push_back(const QString &s)
    {
        m_string.push_back(s);
        m_isDirty = true;
    }

    inline void push_front(QChar c)
    {
        m_string.push_front(c);
        m_isDirty = true;
    }

    inline void push_front(const QString &s)
    {
        m_string.push_front(s);
        m_isDirty = true;
    }

    inline bool operator==(const QString &rhs)
    {
        return rhs == m_string;
    }

    inline bool operator<(const QString &rhs)
    {
        return m_string < rhs;
    }

    inline bool operator!=(const QString &rhs)
    {
        return !(*this == rhs);
    }

    inline bool operator>(const QString& rhs)
    {
        return (rhs < m_string);
    }

    inline bool operator<=(const QString& rhs)
    {
        return !operator> (rhs);
    }

    inline bool operator>=(const QString& rhs)
    {
        return !operator< (rhs);
    }

    inline  Qt3DSString& operator+=(const QString &rhs)
    {
        m_string += rhs;
        m_isDirty = true;
        return *this;
    }

    inline  Qt3DSString& operator+=(const char *s)
    {
        m_string += s;
        m_isDirty = true;
        return *this;
    }

    inline bool isEmpty() const
    {
        return m_string.isEmpty();
    }

    int indexOf(const QString &str, int from = 0,
                Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.indexOf(str, from, cs);
    }

    int indexOf(QChar ch, int from = 0,
                Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.indexOf(ch, from, cs);
    }

    int indexOf(QLatin1String str, int from = 0,
                Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.indexOf(str, from, cs);
    }

    int indexOf(const QStringRef &str, int from = 0,
                Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.indexOf(str, from, cs);
    }

    int lastIndexOf(const QString &str, int from = -1,
                    Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.lastIndexOf(str, from, cs);
    }

    int lastIndexOf(QChar ch, int from = -1,
                    Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.lastIndexOf(ch, from, cs);
    }

    int lastIndexOf(QLatin1String str, int from = -1,
                    Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.lastIndexOf(str, from, cs);
    }

    int lastIndexOf(const QStringRef &str, int from = -1,
                    Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.lastIndexOf(str, from, cs);
    }

    inline void clear()
    {
        m_string.clear();
        m_isDirty = true;
    }

    inline Qt3DSString &insert(int i, QChar c)
    {
        m_string.insert(i,c);
        m_isDirty = true;
        return *this;
    }
    inline Qt3DSString &insert(int i, const QChar *uc, int len)
    {
        m_string.insert(i,uc, len);
        m_isDirty = true;
        return *this;
    }
    inline Qt3DSString &insert(int i, const QString &s)
    {
        m_string.insert(i,s);
        m_isDirty = true;
        return *this;
    }
    inline Qt3DSString &insert(int i, const QStringRef &s)
    {
        m_string.insert(i,s);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &insert(int i, QLatin1String s)
    {
        m_string.insert(i,s);
        m_isDirty = true;
        return *this;
    }

    inline int size() const
    {
        return m_string.size();
    }

    inline int length() const {
        return m_string.length();
    }

    inline Qt3DSString &append(QChar c)
    {
        m_string.append(c);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(const QChar *uc, int len)
    {
        m_string.append(uc, len);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(const QString &s)
    {
        m_string.append(s);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(const QStringRef &s)
    {
        m_string.append(s);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(QLatin1String s)
    {
        m_string.append(s);
        m_isDirty = true;
        return *this;
    }

    inline int compare(const QString &s,
                       Qt::CaseSensitivity cs = Qt::CaseSensitive) const
    {
        return m_string.compare(s, cs);
    }

    // Compatibility wrappers for std::basic_string and eastl::basic_string
    // Required for now for the Qt 3D Studio's template classes.
    static const int npos = -1;

    inline char operator[](int idx) const
    {
        return m_string[idx].toLatin1();
    }

    inline void assign(const char *text)
    {
        m_string = QString::fromUtf8(text);
        m_isDirty = true;
    }

    inline void assign(const char16_t *text)
    {
        m_string = QString::fromUtf16(text);
        m_isDirty = true;
    }

    inline void assign(const char32_t *text)
    {
        m_string = QString::fromUcs4(text);
        m_isDirty = true;
    }

    inline void assign(const wchar_t *text)
    {
        m_string = QString::fromWCharArray(text);
        m_isDirty = true;
    }

    inline void assign(const eastl::string &text)
    {
        m_string = QString::fromUtf8(text.c_str());
        m_isDirty = true;
    }

    inline void operator=(const char *text)
    {
        assign(text);
    }

    inline void operator=(const char16_t *text)
    {
        assign(text);
    }

    inline void operator=(const char32_t *text)
    {
        assign(text);
    }

    inline void operator=(const wchar_t *text)
    {
        assign(text);
    }

    inline void operator=(const eastl::string &text)
    {
        assign(text);
    }

    inline void operator=(const eastl::basic_string<char*> &text)
    {
        assign(*text.c_str());
    }

    inline Qt3DSString &append(QString::iterator first,
                                 QString::iterator last)
    {
        Qt3DSStringUtils::append(m_string, first, last);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(QString::const_iterator first,
                                 QString::const_iterator last)
    {
        Qt3DSStringUtils::append(m_string, first, last);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(const char *text, int size)
    {
        m_string.append(QString::fromUtf8(text, size));
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &append(const char *text)
    {
        m_string.append(QString::fromUtf8(text));
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &replace(QString::iterator replaceBegin,
                                  QString::iterator replaceEnd,
                                  const char *replaceText, int len)
    {
        return replace(replaceBegin, replaceEnd,
                       QString::fromUtf8(replaceText, len));
    }

    inline Qt3DSString &replace(QString::iterator replaceBegin,
                                  QString::iterator replaceEnd,
                                  const QString &replaceText)
    {
        m_string = Qt3DSStringUtils::replace(this->m_string,
                                             replaceBegin, replaceEnd,
                                             replaceText);
        m_isDirty = true;
        return *this;
    }

    inline Qt3DSString &replace(QString::const_iterator replaceBegin,
                                  QString::const_iterator replaceEnd,
                                  const QString &replaceText)
    {
        m_string = Qt3DSStringUtils::replace(this->m_string,
                                             replaceBegin, replaceEnd,
                                             replaceText);
        m_isDirty = true;
        return *this;
    }

    inline QByteArray toUtf8() const
    {
        updateCache();
        return m_array;
    }

    inline const char *c_str() const
    {
        updateCache();
        return m_array.constData();
    }

    inline const char *c_str()
    {
        updateCache();
        return m_array.constData();
    }

private:
    inline void updateCache() const
    {
        if (m_isDirty) {
            m_array = m_string.toUtf8();
            m_isDirty = false;
        }
    }

    QString m_string;
    mutable bool m_isDirty = true;
    mutable QByteArray m_array;
};

}
}

#endif
