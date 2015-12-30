import QtQuick 2.4
import QtQml 2.2

Item
{
  id: "timelineMarkerDragItem"
  objectName: "timelineMarkerDragItem"
  width: 0
  height: 0

  property var dragStart: null
  property var itemMoved: null

  onXChanged: {
    if (itemMoved) {
      itemMoved();
    }
  }
}
