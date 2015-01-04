import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

Item {

    property alias prevCharButton: prevChar
    property alias nextCharButton: nextChar
    property alias canvas: canvas

    width: 640
    height: 480

    RowLayout {
        id: rowLayout1
        x: 8
        y: 270
        width: 94
        height: 33

        Button {
            id: prevChar
            text: qsTr("<")
        }

        Button {
            id: nextChar
            text: qsTr(">")
        }
    }

    Letter {
        id: canvas
        x: 8
        y: 8
    }
}
