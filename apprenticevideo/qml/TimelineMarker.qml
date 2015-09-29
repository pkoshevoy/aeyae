import QtQuick 2.4
import QtQml 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: marker
  objectName: "marker"
  anchors.margins: -(parent.width / 2)

  y: -height / 2.0
  x: (dragArea.drag.active ? dragItem.x :
      position ? position.x - width / 2.0 : 0);
  width: height

  property var position: null
  property var container: null
  property var moveMarker: null
  property var dragItem: null
  property alias color: markerRect.color

  Rectangle
  {
    id: markerRect
    objectName: "markerRect"

    anchors.fill: marker
    anchors.margins: 0

    radius: marker.height / 2.0
  }

  MouseArea
  {
    id: dragArea
    objectName: "dragArea"
    anchors.fill: parent
    drag.target: dragItem
    drag.axis: Drag.XAxis
    drag.threshold: 0

    onPressed: {
      dragItem.x = marker.x;
      dragItem.y = marker.y;
      dragItem.dragStart = dragItem.x;

      dragItem.itemMoved = function() {
        if (dragArea.drag.active) {
          move_marker();
        }
      }
    }

    onReleased: {
      dragItem.itemMoved = undefined

      marker.move_marker();
    }
  }

  function move_marker()
  {
    if (moveMarker)
    {
      var t = (dragItem.x + marker.width / 2) / container.width;
      moveMarker(t);
    }
  }
}
