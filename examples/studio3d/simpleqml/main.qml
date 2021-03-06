/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtStudio3D.OpenGL 2.4
import QtQuick.Window 2.3
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3

Rectangle {
    id: root
    color: "lightGray"

    MessageDialog {
        id: errorDialog
    }

    Studio3D {
        id: s3d
        focus: true
        anchors.margins: 60
        anchors.fill: parent
        property string textValue: "hello world"

        ViewerSettings {
            id: viewerSettings
        }

        Presentation {
            id: s3dpres
            source: "qrc:/presentation/barrel.uia"
            onCustomSignalEmitted: customSignalName.text = Date.now() + ": customSignal:" + name
            onSlideEntered: slideEnter.text = "Entered slide " + name + "(index " + index + ") on " + elementPath
            onSlideExited: slideExit.text = "Exited slide " + name + "(index " + index + ") on " + elementPath

            DataInput {
                name: "di_text"
                value: s3d.textValue
            }

            SceneElement {
                id: sceneElementForScene
                elementPath: "Scene" // could also refer to a Component node instead
                onCurrentSlideIndexChanged: console.log("Current slide index for 'Scene': " + currentSlideIndex)
                onCurrentSlideNameChanged: console.log("Current slide name for 'Scene': " + currentSlideName)
            }

            // Exercise Element a bit. This is no different from using the
            // functions on Presentations, just avoids the need to specify the
            // name/path repeatedly.
            Element {
                id: barrelRef
                elementPath: "Barrel" // or Scene.Layer.Barrel but as long as it's unique the name's good enough
            }
        }
        ignoredEvents: mouseEvCb.checked ? Studio3D.EnableAllEvents : (Studio3D.IgnoreMouseEvents | Studio3D.IgnoreWheelEvents)
        onRunningChanged: console.log("running: " + s3d.running)
        onPresentationReady: console.log("presentationReady")
        onErrorChanged: {
            if (s3d.error !== "") {
                errorDialog.text = s3d.error;
                errorDialog.open();
            }
        }

        property int frameCount: 0
        onFrameUpdate: frameCount += 1

        Timer {
            running: true
            repeat: true
            interval: 1000
            onTriggered: {
                fpsCount.text = "~" + s3d.frameCount + " FPS";
                s3d.frameCount = 0;
            }
        }

        Timer {
            interval: 2000
            running: true
            repeat: true
        }

        NumberAnimation on opacity {
            id: opacityAnimation
            from: 1
            to: 0
            duration: 5000
            running: false
            onStopped: s3d.opacity = 1
        }
    }

    Window {
        id: w
        visible: false
        width: 500
        height: 500
        Item {
            id: wroot
            anchors.fill: parent
        }
        title: "Second window"
    }

    RowLayout {
        Button {
            text: "Move to other window"
            onClicked: {
                w.visible = true;
                if (s3d.parent === wroot) s3d.parent = root; else s3d.parent = wroot;
            }
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Open barrel without background"
            onClicked: s3dpres.source = "qrc:/presentation/barrel_no_background.uip"
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Animate opacity"
            onClicked: opacityAnimation.running = true
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Reload"
            onClicked: {
                var src = s3dpres.source
                s3dpres.source = ""
                s3dpres.source = src
            }
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Open"
            onClicked: openDialog.open()
            focusPolicy: Qt.NoFocus
        }
        CheckBox {
            id: mouseEvCb
            text: "Let mouse events through"
            checked: true
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Toggle camera"
            property bool eyeball: true
            onClicked: {
                eyeball = !eyeball
                s3dpres.setAttribute("Scene.Layer.Camera", "eyeball", eyeball)
            }
            focusPolicy: Qt.NoFocus
        }
        Button {
            text: "Send new data input value"
            property int invocationCount: 0
            onClicked: s3d.textValue = "Data input value " + (++invocationCount)
            focusPolicy: Qt.NoFocus
        }
    }

    Text {
        id: fpsCount
        text: "0 FPS"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }
    Text {
        id: customSignalName
        anchors.bottom: parent.bottom
        anchors.left: fpsCount.right
        anchors.leftMargin: 30
    }
    Text {
        id: slideEnter
        anchors.bottom: parent.bottom
        anchors.left: customSignalName.right
        anchors.leftMargin: 30
    }
    Text {
        id: slideExit
        anchors.bottom: parent.bottom
        anchors.left: slideEnter.right
        anchors.leftMargin: 30
    }
    Button {
        id: nextSlideByIndex
        text: "Next slide (via pres., wrap)"
        anchors.left: parent.left
        anchors.bottom: fpsCount.top
        onClicked: s3dpres.goToSlide("Scene", true, true)
        focusPolicy: Qt.NoFocus
    }
    Button {
        id: seekBtn
        text: "Seek to 5 seconds (via pres.)"
        anchors.left: nextSlideByIndex.right
        anchors.bottom: fpsCount.top
        onClicked: s3dpres.goToTime("Scene", 5)
        focusPolicy: Qt.NoFocus
    }
    Button {
        id: nextSlideViaSceneElement
        text: "Next slide (via SceneElement, no wrap)"
        anchors.left: seekBtn.right
        anchors.bottom: fpsCount.top
        onClicked: sceneElementForScene.currentSlideIndex += 1
        focusPolicy: Qt.NoFocus
    }
    Button {
        id: seekBtn2
        text: "Seek to 5 seconds (via SceneElement)"
        anchors.left: nextSlideViaSceneElement.right
        anchors.bottom: fpsCount.top
        onClicked: sceneElementForScene.goToTime(5)
        focusPolicy: Qt.NoFocus
    }

    Button {
        id: profTogBtn
        text: "Toggle render stats"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        focusPolicy: Qt.NoFocus
        onClicked: viewerSettings.showRenderStats = !viewerSettings.showRenderStats
    }
    Slider {
        id: profUiScale
        width: profTogBtn.width
        anchors.right: profTogBtn.left
        anchors.bottom: parent.bottom
        from: 50
        to: 400
        value: 100
        focusPolicy: Qt.NoFocus
    }

    FileDialog {
        id: openDialog
        nameFilters: ["UIP files (*.uip)", "UIA files (*.uia)", "All files (*)"]
        selectExisting: true
        selectFolder: false
        selectMultiple: false
        onAccepted: s3dpres.source = fileUrl
    }
}
