import QtQuick 2.4
import QtQml 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: filterIcon
  objectName: "filterIcon"

  property var color_fg: "white"

  Item
  {
    // color: "green"
    anchors.fill: parent
    anchors.margins: 0

    Item
    {
      id: icon
      objectName: "icon"

      anchors.fill: parent
      anchors.margins: -parent.height * 0.05
      anchors.topMargin: 0
      anchors.bottomMargin: 0

      layer.enabled: true
      layer.smooth: true

      transform: [ Rotation {
        angle: -45;
        origin.x: icon.width / 2.0;
        origin.y: icon.height / 2.0;
      } ]

      Rectangle
      {
        id: glass
        objectName: "glass"

        anchors.verticalCenter: icon.verticalCenter
        anchors.horizontalCenter: icon.horizontalCenter
        width: Utils.make_odd(icon.height * 0.5)
        height: width
        radius: width * 0.5
        color: "#00000000"
        border.color: color_fg
        border.width: handle.width * 0.7
      }

      Rectangle
      {
        id: handle
        objectName: "handle"

        anchors.top: glass.bottom
        anchors.horizontalCenter: icon.horizontalCenter
        anchors.topMargin: -glass.border.width * 0.5
        width: icon.height * 0.1
        height: glass.radius * 0.9
        color: color_fg
        radius: 2
      }
    }
  }
}
