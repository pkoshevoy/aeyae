import QtQuick 2.0
import '.'

Item {
  id: root
  anchors.fill: parent
  property color color: 'red'
  anchors.margins: -0.5
  opacity: 0.5
  visible: Style.debugMode

  Rectangle {
    anchors.fill: parent
    color: 'transparent'
    border.color: root.color
  }

  Text {
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.margins: 4
    text: root.parent.objectName
    color: root.color
  }
}
