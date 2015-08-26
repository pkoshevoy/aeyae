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

  function calc_cell_width(w)
  {
    var n = Math.min(5, Math.floor(w / 160.0));
    return n < 1.0 ? w : w / n;
  }

  function calc_items_per_row()
  {
    var c = calc_cell_width(playlistView.width)
    var n = Math.floor(playlistView.width / c);
    return n;
  }

  function calc_rows(viewWidth, cellWidth, numItems)
  {
    var cellsPerRow = Math.floor(viewWidth / cellWidth);
    var n = Math.max(1, Math.ceil(numItems / cellsPerRow));
    return n;
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

  function set_current_item(groupRow, itemRow)
  {
    var groupContainer;
    var gridView;

    if (playlistView.currentIndex != groupRow)
    {
      gridView = yae_qml_utils.find_qobject(playlistView.currentItem,
                                            "groupItemsGridView");
      if (gridView)
      {
        gridView.currentIndex = -1;
      }
    }

    playlistView.currentIndex = groupRow;

    gridView = yae_qml_utils.find_qobject(playlistView.currentItem,
                                          "groupItemsGridView");
    if (gridView && gridView.currentIndex != itemRow)
    {
      gridView.currentIndex = itemRow;
    }
  }

  function scroll_to(item)
  {
    var bbox = playlistView.mapFromItem(item,
                                        item.x,
                                        item.y,
                                        item.width,
                                        item.height);
    var itemY = playlistView.contentY + bbox.y;
    if (bbox.y < 0)
    {
      playlistView.contentY = itemY;
    }

    var itemY1 = itemY + bbox.height;
    var viewY1 = playlistView.contentY + playlistView.height;
    var delta = itemY1 - viewY1;

    if (itemY1 > viewY1)
    {
      playlistView.contentY += delta;
    }
  }


  function move_cursor(selectionFlags, funcMoveCursor)
  {
    var current = lookup_current_gridview_and_item();
    if (!current.item)
    {
      return;
    }

    funcMoveCursor(current);

    yae_playlist_model.setCurrentItem(playlistView.currentIndex,
                                      current.gridView.currentIndex,
                                      selectionFlags);
  }

  function move_cursor_left(selectionFlags)
  {
    move_cursor(selectionFlags, function(current) {

      if (current.itemIndex > 0)
      {
        current.gridView.moveCurrentIndexLeft();
      }
      else if (current.groupIndex > 0)
      {
        current.gridView.currentIndex = -1;
        playlistView.decrementCurrentIndex();
        current.gridView = lookup_current_gridview(0);
        current.gridView.currentIndex = current.gridView.count - 1;
      }
    });
  }

  function move_cursor_right(selectionFlags)
  {
    move_cursor(selectionFlags, function(current) {

      if (current.itemIndex + 1 < current.gridView.count)
      {
        current.gridView.moveCurrentIndexRight();
      }
      else if (current.groupIndex + 1 < playlistView.count)
      {
        current.gridView.currentIndex = -1;
        playlistView.incrementCurrentIndex();
        current.gridView = lookup_current_gridview(0);
        current.gridView.currentIndex = 0;
      }
    });
  }

  function move_cursor_up(selectionFlags)
  {
    move_cursor(selectionFlags, function(current) {

      var itemsPerRow = calc_items_per_row();
      if (current.itemIndex > itemsPerRow)
      {
        current.gridView.moveCurrentIndexUp();
      }
      else if (current.itemIndex > 0)
      {
        current.gridView.currentIndex = 0;
      }
      else if (current.groupIndex > 0)
      {
        current.gridView.currentIndex = -1;
        playlistView.decrementCurrentIndex();
        current.gridView = lookup_current_gridview(0);
        current.gridView.currentIndex = current.gridView.count - 1;
      }
      else
      {
        current.gridView.currentIndex = 0;
      }
    });
  }

  function move_cursor_down(selectionFlags)
  {
    move_cursor(selectionFlags, function(current) {

      var itemsPerRow = calc_items_per_row();
      if (current.itemIndex + itemsPerRow < current.gridView.count)
      {
        current.gridView.moveCurrentIndexDown();
      }
      else if (current.itemIndex + 1 < current.gridView.count)
      {
        current.gridView.currentIndex = current.gridView.count - 1;
      }
      else if (current.groupIndex + 1 < playlistView.count)
      {
        current.gridView.currentIndex = -1;
        playlistView.incrementCurrentIndex();
        current.gridView = lookup_current_gridview(0);
        current.gridView.currentIndex = 0;
      }
      else
      {
        current.gridView.currentIndex = current.gridView.count - 1;
      }
    });
  }

  function set_playing_item()
  {
    var current = lookup_current_gridview_and_item();
    if (!current.item)
    {
      return;
    }

    yae_playlist_model.setPlayingItem(playlistView.currentIndex,
                                      current.gridView.currentIndex);
  }


  property var header_bg: "#df1f1f1f"
  property var header_fg: "#ffffffff"
  property var zebra_bg: [ "#00000000", "#3f000000"  ]
  property var zebra_fg: [ "#ffdfdfdf", "#ffffffff"  ]
  property var greeting_message: "Hi!"

  ListView {
    id: playlistView
    objectName: "playlistView"
    focus: playlistView.visible

    visible: true
    anchors.fill: parent
    model: yae_playlist_model
    delegate: groupDelegate
    footer: greetingComponent

    highlightFollowsCurrentItem: false
    currentIndex: -1

    Connections {
      target: yae_playlist_model
      onCurrentItemChanged: {
        console.log("onCurrentItemChanged: " + groupRow + ", " + itemRow);
        set_current_item(groupRow, itemRow);
        var found = lookup_current_gridview_and_item();
        scroll_to(found.item);
      }
    }
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
      }

      Keys.onPressed: {
        console.log("groupDelegateColumn Keys.onPressed");
        dump_properties(event);

        if (event.key == Qt.Key_Left ||
            event.key == Qt.Key_Right ||
            event.key == Qt.Key_Up ||
            event.key == Qt.Key_Down ||
            event.key == Qt.Key_PageUp ||
            event.key == Qt.Key_PageDown ||
            event.key == Qt.Key_Home ||
            event.key == Qt.Key_End)
        {
          var selectionFlags = ItemSelectionModel.ClearAndSelect;

          // FIXME: this won't work correctly for select/unselect:
          if (event.modifiers & Qt.ControlModifier)
          {
            selectionFlags = ItemSelectionModel.ToggleCurrent;
          }
          else if (event.modifiers & Qt.ShiftModifier)
          {
            selectionFlags = ItemSelectionModel.SelectCurrent;
          }

          if (event.key == Qt.Key_Left)
          {
            move_cursor_left(selectionFlags);
          }
          else if (event.key == Qt.Key_Right)
          {
            move_cursor_right(selectionFlags);
          }
          else if (event.key == Qt.Key_Up)
          {
            move_cursor_up(selectionFlags);
          }
          else if (event.key == Qt.Key_Down)
          {
            move_cursor_down(selectionFlags);
          }

          event.accepted = true;
        }
        else if (event.key == Qt.Key_Return ||
                 event.key == Qt.Key_Enter ||
                 event.key == Qt.Key_Space)
        {
          set_playing_item();
          event.accepted = true;
        }
        else if (event.key == Qt.Key_Escape)
        {
          event.accepted = true;
        }
      }
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
               (0.5 + calc_rows(playlistView.width,
                                groupItemsGridView.cellWidth,
                                groupItemsGridView.count)));

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
        cellWidth: calc_cell_width(playlistView.width)
        cellHeight: Math.floor(this.cellWidth * 9.0 / 16.0)

        highlight: gridViewHighlight
        highlightFollowsCurrentItem: false
        currentIndex: -1

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

                onClicked: {
                  // dump_properties(mouse);

                  var i = groupItemsGridView.indexAt(mouse.x, mouse.y);
                  console.log("indexAt(" + mouse.x + ", " + mouse.y +
                              ") = " + i);
                  dump_properties(playlistView);
                  dump_properties(groupItemsGridView);

                  mouse.accepted = true;
                }

                onDoubleClicked: {
                  // dump_properties(mouse);

                  model.playing = true;
                  mouse.accepted = true;
                }
              }

            }
          }
        }
      }

    }
  }

}
