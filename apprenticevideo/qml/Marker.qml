import QtQuick 2.4
import '.'

Item {
  id: root
  width: implicitWidth
  height: implicitHeight
  implicitWidth: 200
  implicitHeight: 200
  property string text: objectName
  property string centerText
  signal clicked()
  property bool checkable: false
  property bool checked: false
  property color color: '#165b8c'
  property string icon
  property alias iconSource: image.source
  property alias pressed: area.pressed

  Rectangle {
    anchors.fill: parent
    anchors.margins: 1
    color: root.color
    opacity: root.checked ? 0.4 : 0.2
  }

  Rectangle {
    anchors.fill: parent
    anchors.margins: 1
    color: 'transparent'
    opactity: 0.4
    border.width: 1
    border.color: Qt.darker(root.color, root.checked ? 3.0 : 1.0)
  }

  Image {
    id: image
    anchors.fill: parent
    fillMode: Image.PreserveAspectFit
    source: Style.icon(root.icon)
  }

  Rectangle {
    anchors.fill: label
    anchors.leftMargin: -8
    anchors.rightMargin: -2
    anchors.topMargin: -2
    anchors.bottomMargin: -2
    visible: label.text
    color: root.color
    opacity: 0.05
    border.color: Qt.lighter(color, 1.2)
  }

  Text {
    id: label
    anchors.top: parent.top
    anchors.right: parent.right
    anchors.margin: 2
    font.family: 'Open Sans'
    font.pixelSize: 14
    color: '#ffffff'
    text: root.text
  }

  Text {
    id: centerLabel
    anchors.centerIn: parent
    font.family: 'Open Sans'
    font.pixelSize: 28
    color: '#ffffff'
    text: root.centerText
  }

  MouseArea {
    id: area
    anchors.fill: parent
    onClicked: {
      if (root.checkable) {
        root.checked = !root.checked
      } else {
        root.checked = false
      }
      root.clicked()
    }
  }

  Tracer {}

}
