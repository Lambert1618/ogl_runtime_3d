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

import QtQuick 2.7
import QtStudio3D.OpenGL 2.4

Item {
    id: mainview
    width: 1280
    height: 768
    visible: true

    // Show the list of datainputs queried by Q3DSPresentation queried from Presentation
    Column {
        id : diColumn
        opacity: 50
        z: 1

        Text { text: "Datainputs"; color: "red"; bottomPadding: 5; }

        Repeater {
            id: diList
            height: childrenRect.height

            property var diObjList

            model: diObjList

            function updateModel() {
                diObjList = studio3D.presentation.getDataInputs();

                for (var i = 0; i < count; ++i) {
                    // Use dynamically queried datainput to set value.
                    if (diObjList[i].name === "queriedInput") {
                        console.log("Updating " + diObjList[i].name);
                        diObjList[i].value = "This text is controlled by " + diObjList[i].name;
                    }
                }
            }

            Text {
                id: diItem
                color: "white"
                // Show the metadata keys available for this datainput and
                // min-max range for datainput type Ranged Number
                text: "name: " + modelData.name
                      + (modelData.max !== 0 ? "\n    range min: " + modelData.min
                                               + "\n    range max: " + modelData.max
                                             : "")
                      + " \n    metadata keys: [" + modelData.metadataKeys + "]"
                font.pointSize: 6
            }
        }
    }

    Studio3D {
        id: studio3D
        anchors.fill: parent
        height: 900
        width: 1280

        property real inputNumber: 0
        property vector4d inputColorVec4: Qt.vector4d(0, 0, 0, 1)
        property vector3d inputCamRotVec3: Qt.vector3d(0, 0, 0)
        property vector3d inputScaleVec3: Qt.vector3d(0, 0, 0)
        property string inputString: ""
        property variant inputVariant: 0

        // A changing property to demonstrate DataInput
        NumberAnimation {
            target: studio3D
            property: "inputNumber"
            duration: 20000
            from: 0
            to: 360
            loops: Animation.Infinite
            running: true
        }
        Vector3dAnimation {
            target: studio3D
            property: "inputScaleVec3"
            duration: 12000
            from: Qt.vector3d(0.3, 0.5, 0.5)
            to: Qt.vector3d(4.0, 4.0, 1.0)
            loops: Animation.Infinite
            running: true
        }
        PropertyAnimation {
            target: studio3D
            property: "inputColorVec4"
            duration: 6000
            from: Qt.vector4d(0.1, 0.1, 0.3, 1.0)
            to: Qt.vector4d(1.0, 0.5, 1.0, 1.0)
            loops: Animation.Infinite
            running: true
        }
        Vector3dAnimation {
            target: studio3D
            property: "inputCamRotVec3"
            duration: 20000
            from: Qt.vector3d(-5, -5, 0.0)
            to: Qt.vector3d(10.0, 10.1, 10.0)
            loops: Animation.Infinite
            running: true
        }
        NumberAnimation {
            target: studio3D
            property: "inputString"
            duration: 20000
            from: 0
            to: 1
            loops: Animation.Infinite
            running: true
        }
        NumberAnimation {
            target: studio3D
            property: "inputVariant"
            duration: 5000
            from: 20
            to: 100
            loops: Animation.Infinite
            running: true
        }

        // Presentation item is used to control the presentation.
        //![1]
        Presentation {
            id: presentation

            source: "qrc:/presentation/datainput.uia"

            onDataInputsReady: {
                diList.updateModel();
            }
            DataInput {
                // Name must match the data input name specified in the presentation
                name: "rangeInput"
                value: studio3D.inputNumber
            }
            DataInput {
                name: "scaleInput"
                value: studio3D.inputScaleVec3
            }
            DataInput {
                name: "colorInput"
                value: studio3D.inputColorVec4
            }
            DataInput {
                name: "cameraRotInput"
                value: studio3D.inputCamRotVec3
            }
            DataInput {
                name: "stringInput"
                value: studio3D.inputString
            }
            DataInput {
                name: "variantInput"
                value: studio3D.inputVariant
            }
        }
        //![1]
    }

}
