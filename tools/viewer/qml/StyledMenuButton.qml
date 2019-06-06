/****************************************************************************
**
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

import QtQuick 2.7
import QtQuick.Controls 2.2

Button {
    id: control

    property Menu menu: null
    property ApplicationWindow window: null

    onPressed: {
        if (menu.visible)
            menu.close();
        else
            menu.open();
    }

    onHoveredChanged: {
        if (hovered && window.menuOpen) {
            window.closeMenus();
            menu.open();
        }
    }

    hoverEnabled: true
    width: contentItem.contentWidth + leftPadding + rightPadding
    leftPadding: _controlPadding
    rightPadding: _controlPadding
    height: _controlBaseHeight
    contentItem: Text {
        text: control.text
        font.pixelSize: _fontSize
        opacity: enabled ? 1.0 : 0.3
        color: _textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        opacity: enabled ? 1 : 0.3
        color: control.down || control.hovered
                      ? _menuSelectionColor : _menuBackgroundColor
    }
}
