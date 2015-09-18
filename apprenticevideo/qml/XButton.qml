import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: xbutton
  objectName: "xbutton"

  property alias view: view

  property var color_bg: "#7f7f7f7f"
  property var color_fg: "white"
  property var on_click: null

  Rectangle
  {
    id: view
    objectName: "view"

    anchors.fill: parent
    anchors.margins: 0
    radius: 3
    color: (mouseArea.containsMouse ? color_bg : "#00000000")

    Item
    {
      id: crosslines
      objectName: "crosslines"

      anchors.fill: parent
      anchors.margins: 0

      opacity: mouseArea.containsMouse ? 1.0 : 0.5
      layer.enabled: true
      layer.smooth: true

      transform: [ Rotation {
        angle: -45;
        origin.x: crosslines.width / 2.0;
        origin.y: crosslines.height / 2.0;
      } ]

      Rectangle
      {
        id: vline
        objectName: "vline"

        anchors.verticalCenter: crosslines.verticalCenter
        anchors.horizontalCenter: crosslines.horizontalCenter
        width: Math.max(3, Utils.make_odd(crosslines.height * 0.1))
        height: Math.max(11, Utils.make_odd(crosslines.height * 0.8))
        color: color_fg
      }

      Rectangle
      {
        id: hline
        objectName: "hline"

        anchors.verticalCenter: crosslines.verticalCenter
        anchors.horizontalCenter: crosslines.horizontalCenter
        width: vline.height
        height: vline.width
        color: color_fg
      }
    }
  }

  MouseArea
  {
    id: mouseArea
    objectName: "mouseArea"

    anchors.fill: parent
    anchors.margins: 0
    hoverEnabled: true

    onClicked: {
      // console.log("XButton: clicked!");

      if (on_click)
      {
        on_click(mouse);
      }
    }
  }
}
