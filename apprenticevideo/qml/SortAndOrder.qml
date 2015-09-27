import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: sort
  objectName: "sort"

  width: bbox.width
  height: bbox.height

  property var font_size: height * 0.8
  property var text_color: "white"
  property var text_outline_color: "#7f7f7f7f"
  property var underscore_color: "red"
  property var underscore_thickness: 2

  Item
  {
    id: bbox
    objectName: "bbox"

    anchors.left: sortBy.left
    anchors.right: sortByNameOrTimeAscOrDesc.right
    anchors.top: sort.top
    anchors.bottom: sortBy.bottom
  }

  Text
  {
    id: sortBy
    objectName: "sortBy"

    anchors.top: parent.top

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("sort by ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;
  }

  Text
  {
    id: sortByName
    objectName: "sortByName"

    anchors.top: parent.top
    anchors.left: sortBy.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("name ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;

    MouseArea
    {
      anchors.fill: parent
      onClicked: {
        yae_playlist_model.sortBy = TPlaylistModel.SortByName;
      }
    }

    Rectangle
    {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottomMargin: -height * 2

      antialiasing: true
      height: underscore_thickness
      color: underscore_color

      visible: (yae_playlist_model.sortBy == TPlaylistModel.SortByName)
    }
  }

  Text
  {
    id: sortByNameOr
    objectName: "sortByNameOr"

    anchors.top: parent.top
    anchors.left: sortByName.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("or ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;
  }

  Text
  {
    id: sortByNameOrTime
    objectName: "sortByNameOrTime"

    anchors.top: parent.top
    anchors.left: sortByNameOr.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("time")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;

    MouseArea
    {
      anchors.fill: parent
      onClicked: {
        yae_playlist_model.sortBy = TPlaylistModel.SortByTime;
      }
    }

    Rectangle
    {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottomMargin: -height * 2

      antialiasing: true
      height: underscore_thickness
      color: underscore_color

      visible: (yae_playlist_model.sortBy == TPlaylistModel.SortByTime)
    }
  }

  Text
  {
    id: sortByNameOrTimeComma
    objectName: "sortByNameOrTimeComma"

    anchors.top: parent.top
    anchors.left: sortByNameOrTime.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr(", ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;
  }

  Text
  {
    id: sortByNameOrTimeAsc
    objectName: "sortByNameOrTimeAsc"

    anchors.top: parent.top
    anchors.left: sortByNameOrTimeComma.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("ascending ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;

    MouseArea
    {
      anchors.fill: parent
      onClicked: {
        yae_playlist_model.sortOrder = Qt.AscendingOrder;
      }
    }

    Rectangle
    {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottomMargin: -height * 2

      antialiasing: true
      height: underscore_thickness
      color: underscore_color

      visible: (yae_playlist_model.sortOrder == Qt.AscendingOrder)
    }
  }

  Text
  {
    id: sortByNameOrTimeAscOr
    objectName: "sortByNameOrTimeAscOr"

    anchors.top: parent.top
    anchors.left: sortByNameOrTimeAsc.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("or ")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;
  }

  Text
  {
    id: sortByNameOrTimeAscOrDesc
    objectName: "sortByNameOrTimeAscOrDesc"

    anchors.top: parent.top
    anchors.left: sortByNameOrTimeAscOr.right

    font.bold: true
    font.pixelSize: font_size
    text: qsTr("descending")
    color: text_color;

    style: Text.Outline;
    styleColor: text_outline_color;

    MouseArea
    {
      anchors.fill: parent
      onClicked: {
        yae_playlist_model.sortOrder = Qt.DescendingOrder;
      }
    }

    Rectangle
    {
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottomMargin: -height * 2

      antialiasing: true
      height: underscore_thickness
      color: underscore_color

      visible: (yae_playlist_model.sortOrder == Qt.DescendingOrder)
    }
  }
}
