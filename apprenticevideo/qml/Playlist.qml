import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'

Item
{
  id: root
  width: 320
  height: 480

  CanvasQuickFbo
  {
    id: renderer
    objectName: "renderer"

    anchors.fill: parent
    anchors.margins: 0

    // flip it right-side-up:
    transform:
    [
      Scale { yScale: -1; },
      Translate { y: renderer.height; }
    ]
  }


  // FIXME: for debugging
  function dump_properties(item, indentation)
  {
    if (!indentation)
    {
      indentation = "";
    }

    console.log("\n" + indentation + item);
    for (var p in item)
    {
      console.log(indentation + p + ": " + item[p]);
    }
  }

  function dump_item_tree(item, indentation)
  {
    if (!indentation)
    {
      indentation = "";
    }

    var str = indentation;
    if (item.objectName)
    {
      str += item.objectName + " ";
    }
    str += item;

    if (item.children && item.children.length > 0)
    {
      str += ", " + item.children.length + " children"
    }

    if (item.parent)
    {
      str += ", parent: ";
      if (parent.objectName)
      {
        str += item.parent.objectName + " ";
      }
      str += item.parent;
    }
    console.log(str);

    if (item.children)
    {
      var n = item.children.length;
      for (var i = 0; i < n; i++)
      {
        dump_item_tree(item.children[i], indentation + "  ");
      }
    }
  }

  function calc_cell_width(w, h)
  {
    var n = Math.min(5, Math.floor(w / 160.0));
    return n < 1.0 ? w : w / n;
  }

  function calc_title_height(min_height, w)
  {
    return Math.max(min_height, 24.0 * playlistView.width / 800.0);
  }

  function calc_greeting_font_size(w, h)
  {
    return Math.max(12, 56.0 * Math.min(w / 1920.0, h / 1080.0));
  }

  function calc_zebra_index(index, cell_width, view_width)
  {
    var columns = Math.round(view_width / cell_width);
    var col = index % columns
    var row = (index - col) / columns;
    return (row % 2 + col) % 2;
  }

  function find_current_item()
  {
    var sel = yae_playlist_model.itemSelectionModel().currentIndex;
    if (!sel.parent)
    {
      return null;
    }

    playlistView.currentIndex = sel.parent.row;
    var groupContainer = playlistView.currentItem;
    playlistView.currentIndex = -1;

    var gridView = yae_qml_utils.find_qobject(groupContainer,
                                              "groupItemsGridView");
    // yae_qml_utils.dump_object_tree(gridView);

    gridView.currentIndex = sel.row;
    var itemContainer = gridView.currentItem;
    gridView.currentIndex = -1;

    // yae_qml_utils.dump_object_tree(itemContainer);

    var item = yae_qml_utils.find_qobject(itemContainer,
                                          "itemDelegate");
    // yae_qml_utils.dump_object_tree(item);

    return {
      gridView: gridView,
      item: item
    };
  }


  function lookup_current_gridview(suggestedGroupIndex)
  {
    if (suggestedGroupIndex == null)
    {
      suggestedGroupIndex = 0;
    }

    if (playlistView.currentIndex == -1)
    {
      playlistView.currentIndex = suggestedGroupIndex;
    }

    var groupContainer = playlistView.currentItem;
    var gridView = yae_qml_utils.find_qobject(groupContainer,
                                              "groupItemsGridView");
    return gridView;
  }

  function lookup_current_item(gridView, suggestedItemIndex)
  {
    if (suggestedItemIndex == null)
    {
      suggestedItemIndex = 0;
    }

    if (gridView.currentIndex == -1)
    {
      gridView.currentIndex = suggestedItemIndex;
    }

    var itemContainer = gridView.currentItem;
    var item = yae_qml_utils.find_qobject(itemContainer,
                                          "itemDelegate");
    return item;
  }


  function lookup_current_gridview_and_item()
  {
    if (playlistView.currentIndex == -1)
    {
      playlistView.currentIndex = 0;
    }

    var groupContainer = playlistView.currentItem;

    var gridView = yae_qml_utils.find_qobject(groupContainer,
                                              "groupItemsGridView");
    if (gridView.currentIndex == -1)
    {
      gridView.currentIndex = 0;
    }

    var itemContainer = gridView.currentItem;

    // yae_qml_utils.dump_object_tree(itemContainer);

    var item = yae_qml_utils.find_qobject(itemContainer,
                                          "itemDelegate");
    // yae_qml_utils.dump_object_tree(item);

    return {
      groupIndex: playlistView.currentIndex,
      itemIndex: gridView.currentIndex,
      gridView: gridView,
      item: item
    };
  }


  property var header_bg: "#df1f1f1f"
  // property var header_bg: "#df000000"
  property var header_fg: "#ffffffff"
  property var zebra_bg: [ "#00000000", "#3f000000"  ]
  property var zebra_fg: [ "#ffdfdfdf", "#ffffffff"  ]
  property var greeting_message: "Hi!"

  /*
    Component {
    id: listViewHighlight

    Rectangle {
    color: "#FFFF00";
    y: playlistView.currentItem.y
    width: playlistView.width;
    height: playlistView.currentItem.height
    anchors.margins: -3

    Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
    }
    }
  */

  ListView {
    id: playlistView
    objectName: "playlistView"
    focus: true; // playlistView.visible

    visible: true
    anchors.fill: parent
    model: yae_playlist_model
    delegate: groupDelegate
    footer: greetingComponent

    // highlight: listViewHighlight
    highlightFollowsCurrentItem: false
    currentIndex: -1
  }

  Component {
    id: greetingComponent

    Rectangle {
      objectName: "greetingComponentRect"

      width: playlistView.width
      height: playlistView.height
      color: "#df000000"

      Text {
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: calc_greeting_font_size(width, height)
        wrapMode: "Wrap"
        elide: "ElideMiddle"
        text: greeting_message
        color: "#7f7f7f7f"
        style: Text.Outline;
        styleColor: "black";
      }
    }
  }

  Component {
    id: groupDelegate

    Column {
      id: groupDelegateColumn
      objectName: "groupDelegateColumn"
      focus: true
      width: playlistView.width

      Rectangle {
        id: groupItem
        objectName: "groupItem"
        focus: true

        color: header_bg
        height: calc_title_height(24.0, playlistView.width)
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right

        Image {
          id: disclosureBtn
          objectName: "disclosureBtn"

          width: groupItem.height
          height: groupItem.height
          anchors.leftMargin: 0

          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          source: (model.collapsed ?
                   "qrc:///images/group-collapsed.png" :
                   "qrc:///images/group-exposed.png");

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' item data role
            onClicked: {
              model.collapsed = !model.collapsed;
            }
          }
        }

        Text {
          anchors.verticalCenter: parent.verticalCenter
          anchors.right: parent.right
          anchors.left: disclosureBtn.right
          anchors.leftMargin: groupItem.height / 2
          elide: "ElideMiddle"
          font.bold: true
          font.pixelSize: groupItem.height * 0.55
          text: label
          color: header_fg
          style: Text.Outline;
          styleColor: "black";
        }
      }

      Loader {
        id: groupsLoader
        objectName: "groupsLoader"
        focus: true

        // This is a workaround for a bug/feature in the Loader element.
        // If sourceComponent is set to null the Loader element retains
        // the same height it had when sourceComponent was set. Setting visible
        // to false makes the parent Column treat it as if it's height was 0.

        visible: !model.collapsed

        sourceComponent: groupItemsColumnDelegate
        onStatusChanged: if (status == Loader.Ready) {
          // console.log("loaded: " + label + ", index: " + index)
          // console.log(item)
          item.model.rootIndex = item.model.modelIndex(index)
          item.visible = true
          item.focus = true

          // console.log("item.model: " + item.model +
          //             ", rootIndex: " + item.model.rootIndex);
        }

        Keys.onPressed: {
          console.log("groupsLoader, onPressed: " + event.key);
          event.accepted = false;
        }
      }

      Keys.onPressed: {
        console.log("groupDelegateColumn Keys.onPressed");
        if (event.key == Qt.Key_Left ||
            event.key == Qt.Key_Right ||
            event.key == Qt.Key_Up ||
            event.key == Qt.Key_Down ||
            event.key == Qt.Key_PageUp ||
            event.key == Qt.Key_PageDown ||
            event.key == Qt.Key_Home ||
            event.key == Qt.Key_End)
        {
          var found = lookup_current_gridview_and_item();
          if (found.item)
          {
            var bbox = playlistView.mapFromItem(found.item,
                                                found.item.x,
                                                found.item.y,
                                                found.item.width,
                                                found.item.height);
            console.log("current item " + found.item + ", bbox: " + bbox +
                        ", title: " + found.item.label +
                        ", group index: " + found.groupIndex +
                        " (" + playlistView.count + ")" +
                        ", item index: " + found.itemIndex +
                        " (" + found.gridView.count + ")");
            // yae_qml_utils.dump_object_tree(found.item);
            // yae_qml_utils.dump_object_info(found.item);
            // dump_properties(found.item);

            // yae_qml_utils.dump_object_tree(found.item);
            // dump_properties(found.item, "    ");

            var itemsPerRow = Math.floor(playlistView.width /
                                         calc_cell_width(playlistView.width,
                                                         playlistView.height));

            if (event.key == Qt.Key_Left)
            {
              // yae_playlist_model.changeCurrentItem(itemsPerRow, -1);
              if (found.itemIndex > 0)
              {
                found.gridView.moveCurrentIndexLeft();
              }
              else if (found.groupIndex > 0)
              {
                found.gridView.currentIndex = -1;
                playlistView.decrementCurrentIndex();
                found.gridView = lookup_current_gridview(0);
                found.gridView.currentIndex = found.gridView.count - 1;
              }
            }
            else if (event.key == Qt.Key_Right)
            {
              // yae_playlist_model.changeCurrentItem(itemsPerRow, 1);
              if (found.itemIndex + 1 < found.gridView.count)
              {
                found.gridView.moveCurrentIndexRight();
              }
              else if (found.groupIndex + 1 < playlistView.count)
              {
                found.gridView.currentIndex = -1;
                playlistView.incrementCurrentIndex();
                found.gridView = lookup_current_gridview(0);
                found.gridView.currentIndex = 0;
              }
            }
            else if (event.key == Qt.Key_Up)
            {
              // yae_playlist_model.changeCurrentItem(itemsPerRow, -itemsPerRow);
              if (found.itemIndex > itemsPerRow)
              {
                found.gridView.moveCurrentIndexUp();
              }
              else if (found.groupIndex > 0)
              {
                found.gridView.currentIndex = -1;
                playlistView.decrementCurrentIndex();
                found.gridView = lookup_current_gridview(0);
                found.gridView.currentIndex = found.gridView.count - 1;
              }
              else
              {
                found.gridView.currentIndex = 0;
              }
            }
            else if (event.key == Qt.Key_Down)
            {
              // yae_playlist_model.changeCurrentItem(itemsPerRow, itemsPerRow);
              if (found.itemIndex + itemsPerRow < found.gridView.count)
              {
                found.gridView.moveCurrentIndexDown();
              }
              else if (found.groupIndex + 1 < playlistView.count)
              {
                found.gridView.currentIndex = -1;
                playlistView.incrementCurrentIndex();
                found.gridView = lookup_current_gridview(0);
                found.gridView.currentIndex = 0;
              }
              else
              {
                found.gridView.currentIndex = found.gridView.count - 1;
              }
            }
          }

          found = lookup_current_gridview_and_item();
          bbox = playlistView.mapFromItem(found.item,
                                          found.item.x,
                                          found.item.y,
                                          found.item.width,
                                          found.item.height);
          console.log("NEXT item " + found.item + ", bbox: " + bbox +
                      ", title: " + found.item.label +
                      ", group index: " + found.groupIndex +
                      " (" + playlistView.count + ")" +
                      ", item index: " + found.itemIndex +
                      " (" + found.gridView.count + ")");

          var itemY = playlistView.contentY + bbox.y;
          if (itemY < 0)
          {
            playlistView.contentY = itemY;
          }

          var itemY1 = itemY + bbox.height;
          var viewY1 = playlistView.contentY + playlistView.height;
          var delta = itemY1 - viewY1;
          console.log("itemY1(" + itemY1 + ") - viewY1(" + viewY1 +
                      ") = delta(" + delta + ")");
          if (itemY1 > viewY1)
          {
            playlistView.contentY += delta;
          }

          event.accepted = true;
        }
        else if (event.key == Qt.Key_Return ||
                 event.key == Qt.Key_Enter ||
                 event.key == Qt.Key_Space)
        {
          event.accepted = true;
        }
        else if (event.key == Qt.Key_Escape)
        {
          event.accepted = true;
        }
      }
      /*
        Keys.onPressed: {
        console.log("Column Item, onPressed: " + event.key);
        event.accepted = false;
        }
      */
    }
  }

  Component {
    id: groupItemsColumnDelegate

    Rectangle {
      id: groupItemsColumnDelegateRect
      objectName: "groupItemsColumnDelegateRect"
      focus: true

      color: header_bg
      property alias model : groupItemsGridView.model
      width: playlistView.width

      // size-to-fit:
      height: (!groupItemsGridView.count ? 0 :
               groupItemsGridView.cellHeight *
               (0.5 + Math.max(1.0,
                               Math.ceil(groupItemsGridView.count /
                                         Math.floor(playlistView.width /
                                                    groupItemsGridView.
                                                    cellWidth)))));

      Rectangle {
        color: "red"
        height: 1
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right
      }

      Component {
        id: gridViewHighlight

        Rectangle {
          visible: groupItemsGridView.currentItem != null
          x: groupItemsGridView.currentItem.x
          y: groupItemsGridView.currentItem.y
          z: 1
          width: groupItemsGridView.cellWidth;
          height: groupItemsGridView.cellHeight
          color: "#3fff0000";
          anchors.margins: -2

          Behavior on x { SpringAnimation { spring: 3; damping: 0.2 } }
          Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
        }
      }

      GridView {
        id: groupItemsGridView
        objectName: "groupItemsGridView"
        focus: true

        anchors.fill: parent
        anchors.topMargin: 2
        width: parent.width
        height: parent.height - anchors.topMargin
        cellWidth: calc_cell_width(playlistView.width,
                                   playlistView.height)
        cellHeight: Math.floor(this.cellWidth * 9.0 / 16.0)

        highlight: gridViewHighlight
        highlightFollowsCurrentItem: false
        currentIndex: -1

        /*
          Keys.onSelectPressed: {
          console.log("GridView, onSelectPressed");
          event.accepted = true;
          }
        */

        Keys.onPressed: {
          console.log("GridView, onPressed: " + event.key);
          event.accepted = false;
        }

        model: DelegateModel {
          id: modelDelegate
          model: yae_playlist_model

          delegate: Item {
            id: itemDelegate
            objectName: "itemDelegate"
            focus: true

            height: groupItemsGridView.cellHeight
            width: groupItemsGridView.cellWidth

            property var label: model.label

            Rectangle {
              id: backgroundRect
              objectName: "backgroundRect"

              anchors.fill: parent
              color: (calc_zebra_index(index,
                                       groupItemsGridView.cellWidth,
                                       playlistView.width) ?
                      zebra_bg[1] : zebra_bg[0]) // argb

              Image {
                id: thumbnailImage
                objectName: "thumbnailImage"

                opacity: 1.0
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: model.thumbnail
              }

              Rectangle {
                id: labelBackgroundRect
                objectName: "labelBackgroundRect"

                color: "#7f7f7f7f"
                anchors.margins: 0;
                anchors.leftMargin: -3;
                anchors.rightMargin: -3;
                anchors.bottom: labelTag.bottom
                anchors.left: labelTag.left
                width: (labelTag.contentWidth -
                        anchors.leftMargin -
                        anchors.rightMargin)
                height: labelTag.contentHeight
                radius: 3
              }

              Text {
                id: labelTag
                objectName: "labelTag"

                verticalAlignment: Text.AlignBottom
                anchors.fill: parent
                anchors.margins: 5
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlistView.width) *
                                 0.45);
                wrapMode: "Wrap"
                elide: "ElideMiddle"
                text: model.label
                color: "white";

                style: Text.Outline;
                styleColor: "#7f7f7f7f";
              }

              Rectangle {
                id: nowPlayingBackgroundRect
                objectName: "nowPlayingBackgroundRect"

                color: "#7f7f7f7f"
                anchors.margins: 0;
                anchors.leftMargin: -3;
                anchors.rightMargin: -3;
                anchors.fill: nowPlayingTag
                visible: model.playing
                radius: 3
              }

              Text {
                id: nowPlayingTag
                objectName: "nowPlayingTag"

                visible: model.playing
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 5
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlistView.width) *
                                 0.30);
                text: qsTr("NOW PLAYING")
                color: "white"
              }

              MouseArea {
                id: mouseArea
                objectName: "mouseArea"

                anchors.fill: parent

                onDoubleClicked: {
                  model.playing = true;
                }
              }


              Keys.onPressed: {
                console.log("GridView Item, onPressed: " + event.key);
                event.accepted = false;
              }

            }
          }
        }
      }

    }
  }

}
