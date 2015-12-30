import QtQuick 2.4
import QtQml 2.2


Item
{
  property var container: parent

  function get_item_y(item)
  {
    if (!item)
    {
      return -1;
    }

    var y = container.mapFromItem(item, item.x, item.y).y;
    if (container.contentY == "undefined")
    {
      return "" + y;
    }

    var offset = container.contentY;
    var yy = offset + y;
    return "" + yy + " = " + offset + " + " + y;
  }

  function refresh()
  {
    debug.msg.text = get_item_y(debug);
  }

  Rectangle
  {
    id: debug
    z: 1
    anchors.left: parent.left
    anchors.top: parent.top
    width: msg.width + 8
    height: msg.height + 4
    color: "white"
    property alias msg: msg

    Text
    {
      id: msg
      anchors.verticalCenter: parent.verticalCenter
      anchors.horizontalCenter: parent.horizontalCenter
      font.pixelSize: 12
      text: get_item_y(debug)
      color: "black"
    }

    MouseArea
    {
      anchors.fill: parent
      onClicked: { refresh(); }
    }
  }
}
