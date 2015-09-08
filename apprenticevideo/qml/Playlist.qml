import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  objectName: "Playlist.qml Item"

  property alias view: playlistView

  property var header_bg: "#df1f1f1f"
  property var header_fg: "#ffffffff"
  property var zebra_bg_0: "#00000000"
  property var zebra_bg_1: "#3f000000"
  property var separator_color: "#7f7f7f7f"
  property var footer_fg: "#7fffffff"
  property var highlight_color: "#3fff0000"
  property var label_bg: "#7f7f7f7f"
  property var label_fg: "white"

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

  function calc_zebra_index(index, cell_width, view_width)
  {
    var columns = Math.round(view_width / cell_width);
    var col = index % columns
    var row = (index - col) / columns;
    return (row % 2 + col) % 2;
  }

  function assign_playlistview_current_index(index)
  {
    // save current playlistView.contentY
    var content_y = playlistView.contentY;

    // this has a side effect of changing playlistView.contentY
    playlistView.currentIndex = index;

    // restore original playlistView.contentY
    playlistView.contentY = content_y;
  }

  function find_current_item()
  {
    var sel = yae_playlist_model.itemSelectionModel().currentIndex;
    if (!sel.parent)
    {
      return null;
    }

    var savedCurrentIndex = playlistView.currentIndex;
    assign_playlistview_current_index(sel.parent.row);
    var groupContainer = playlistView.currentItem;
    assign_playlistview_current_index(savedCurrentIndex);

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
      assign_playlistview_current_index(suggestedGroupIndex);
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
      assign_playlistview_current_index(0);
    }

    var groupContainer = playlistView.currentItem;

    var gridView =
        groupContainer ?
        yae_qml_utils.find_qobject(groupContainer, "groupItemsGridView") :
        null;

    if (!gridView)
    {
      return {
        groupIndex: -1,
        itemIndex: -1,
        gridView: null,
        item: null
      }
    }

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

  function get_item_y(item)
  {
    if (!item)
    {
      return -1;
    }

    var pt = playlistView.mapFromItem(item, 0, 0);
    return playlistView.contentY + pt.y
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

    assign_playlistview_current_index(groupRow);

    gridView = yae_qml_utils.find_qobject(playlistView.currentItem,
                                          "groupItemsGridView");
    if (gridView && gridView.currentIndex != itemRow)
    {
      gridView.currentIndex = itemRow;
    }

    /*
    // for debugging:
    var found = lookup_current_gridview_and_item();

    if (found)
    {
      calc_delta_scroll_to(found.item)
    }
    */
  }

  function calc_delta_scroll_to(item)
  {
    var delta_y = 0;

    var view_y0 = playlistView.contentY
    var view_y1 = view_y0 + playlistView.height

    var item_y0 = get_item_y(item);
    var item_y1 = item_y0 + item.height

    if (item_y0 < view_y0)
    {
      delta_y = item_y0 - view_y0;
    }
    else if (item_y1 > view_y1)
    {
      delta_y = item_y1 - view_y1;
    }

    /*
    // for debugging:
    console.log("\n\nscroll to: " +
                "\nitem_y0: " + item_y0 +
                "\nitem_y1: " + item_y1 +
                "\nview_y0: " + view_y0 +
                "\nview_y1: " + view_y1 +
                "\ndelta_y: " + delta_y + "\n\n");
    */
    return delta_y;
  }

  function scroll_to(item)
  {
    var delta = calc_delta_scroll_to(item);

    if (delta != 0)
    {
      // console.log("delta: " + delta);
      playlistView.contentY += delta;
      // console.log("contentY: " + playlistView.contentY + "\n\n");
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
        assign_playlistview_current_index(playlistView.currentIndex - 1);
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
        assign_playlistview_current_index(playlistView.currentIndex + 1);
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
        assign_playlistview_current_index(playlistView.currentIndex - 1);
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
        assign_playlistview_current_index(playlistView.currentIndex + 1);
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

  function handle_event_on_key_pressed(event)
  {
    // console.log("handle_event_on_key_pressed");
    // Utils.dump_properties(event);

    event.accepted = false;

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

  ListView
  {
    id: playlistView
    objectName: "playlistView"

    anchors.fill: parent
    model: yae_playlist_model
    delegate: groupDelegate
    footer: footerComponent

    highlightFollowsCurrentItem: false
    currentIndex: -1

    /*
    // for debugging:
    onContentYChanged: {
      console.log("CONTENT Y CHANGED: " + contentY);
    }
    */

    Connections
    {
      target: yae_playlist_model
      onCurrentItemChanged: {
        // console.log("onCurrentItemChanged: " + groupRow + ", " + itemRow);
        set_current_item(groupRow, itemRow);
        var found = lookup_current_gridview_and_item();
        scroll_to(found.item);
      }
    }

    MouseArea
    {
      id: mouseArea
      objectName: "playlistViewMouseArea"

      anchors.fill: parent
      propagateComposedEvents: true
    }

  }

  Component
  {
    id: footerComponent

    Item
    {
      id: footer
      objectName: "footerComponentRect"

      width: playlistView.width
      height: calc_title_height(24.0, playlistView.width) + 2

      Rectangle
      {
        color: separator_color
        height: 1
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right
      }

      Text
      {
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.leftMargin: footer.height / 2
        anchors.rightMargin: footer.height / 2
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: "ElideMiddle"
        font.pixelSize: footer.height * 0.45
        text: ((yae_playlist_model.itemCount == 1) ?
               "1 item, end of playlist" :
               "" + yae_playlist_model.itemCount + " items, end of playlist");
        color: footer_fg
      }

      // YDebug { id: ydebug; z: 1; container: playlistView; }
      // onYChanged: { ydebug.refresh(); }
    }
  }

  Component
  {
    id: groupDelegate

    Column
    {
      id: groupDelegateColumn
      objectName: "groupDelegateColumn"
      width: playlistView.width

      Item
      {
        id: groupItem
        objectName: "groupItem"

        height: calc_title_height(24.0, playlistView.width)
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right

        Image
        {
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

          MouseArea
          {
            anchors.fill: parent

            // Toggle the 'collapsed' item data role
            onClicked: {
              console.log("Playlist group: CLICKED!")
              model.collapsed = !model.collapsed;
            }
          }
        }

        Text
        {
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

        // YDebug { id: ydebug; z: 1; container: playlistView; }
        // onYChanged: { ydebug.refresh(); }
      }

      Loader
      {
        id: groupsLoader
        objectName: "groupsLoader"

        // This is a workaround for a bug/feature in the Loader element.
        // If sourceComponent is set to null the Loader element retains
        // the same height it had when sourceComponent was set. Setting visible
        // to false makes the parent Column treat it as if it's height was 0.

        visible: !model.collapsed

        sourceComponent: groupItemsColumnDelegate
        onStatusChanged: if (status == Loader.Ready)
        {
          // console.log("loaded: " + label + ", index: " + index)
          // console.log(item)
          item.model.rootIndex = item.model.modelIndex(index)
          item.visible = true

          // console.log("item.model: " + item.model +
          //             ", rootIndex: " + item.model.rootIndex);
        }
      }
    }
  }

  Component
  {
    id: groupItemsColumnDelegate

    Item
    {
      id: groupItemsColumnDelegateRect
      objectName: "groupItemsColumnDelegateRect"

      property alias model : groupItemsGridView.model
      width: playlistView.width

      // size-to-fit:
      height: (!groupItemsGridView.count ? 0 :
               groupItemsGridView.cellHeight *
               (0.5 + calc_rows(playlistView.width,
                                groupItemsGridView.cellWidth,
                                groupItemsGridView.count)));

      Rectangle
      {
        color: separator_color
        height: 1
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right
      }

      Component
      {
        id: gridViewHighlight

        Rectangle
        {
          visible: groupItemsGridView.currentItem != null
          x: (groupItemsGridView.currentItem ?
              groupItemsGridView.currentItem.x :
              0)
          y: (groupItemsGridView.currentItem ?
              groupItemsGridView.currentItem.y :
              0)
          z: 1
          width: groupItemsGridView.cellWidth;
          height: groupItemsGridView.cellHeight
          color: highlight_color;
          anchors.margins: -2

          Behavior on x { SpringAnimation { spring: 3; damping: 0.2 } }
          Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
        }
      }

      GridView
      {
        id: groupItemsGridView
        objectName: "groupItemsGridView"

        anchors.fill: parent
        anchors.topMargin: 2
        width: parent.width
        height: parent.height - anchors.topMargin
        cellWidth: calc_cell_width(playlistView.width)
        cellHeight: Math.floor(this.cellWidth * 9.0 / 16.0)

        highlight: gridViewHighlight
        highlightFollowsCurrentItem: false
        currentIndex: -1

        model: DelegateModel
        {
          id: modelDelegate
          model: yae_playlist_model

          delegate: Item
          {
            id: itemDelegate
            objectName: "itemDelegate"

            height: groupItemsGridView.cellHeight
            width: groupItemsGridView.cellWidth

            property var label: model.label

            // YDebug { id: ydebug; z: 1; container: playlistView; }
            // onYChanged: { ydebug.refresh(); }

            Rectangle
            {
              id: backgroundRect
              objectName: "backgroundRect"

              anchors.fill: parent
              color: (calc_zebra_index(index,
                                       groupItemsGridView.cellWidth,
                                       playlistView.width) ?
                      zebra_bg_1 : zebra_bg_0) // argb

              Image
              {
                id: thumbnailImage
                objectName: "thumbnailImage"

                // model.thumbnail is 'undefined' while the is being loaded,
                // and causes this error:
                //   Unable to assign [undefined] to QUrl
                //
                // (model.thumbnail || "") is a workaround expression
                // that is evaluates to string type assignable to QUrl
                // and avoids the above error:
                //
                source: (model.thumbnail || "")

                opacity: 1.0
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
              }

              Rectangle
              {
                id: labelBackgroundRect
                objectName: "labelBackgroundRect"

                color: label_bg
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

              Text
              {
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
                color: label_fg;

                style: Text.Outline;
                styleColor: label_bg;
              }

              Rectangle
              {
                id: nowPlayingBackgroundRect
                objectName: "nowPlayingBackgroundRect"

                // model.playing is 'undefined' while the is being loaded,
                // and causes this error:
                //   Unable to assign [undefined] to bool
                //
                // (model.playing || false) is a workaround expression
                // that evaluates to the expected boolean type
                // and avoids the above error:
                //
                visible: (model.playing || false)

                color: label_bg
                anchors.margins: 0;
                anchors.leftMargin: -3;
                anchors.rightMargin: -3;
                anchors.fill: nowPlayingTag
                radius: 3
              }

              Text
              {
                id: nowPlayingTag
                objectName: "nowPlayingTag"

                // model.playing is 'undefined' while the is being loaded,
                // and causes this error:
                //   Unable to assign [undefined] to bool
                //
                // (model.playing || false) is a workaround expression
                // that evaluates to the expected boolean type
                // and avoids the above error:
                //
                visible: (model.playing || false)

                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 5
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlistView.width) *
                                 0.30);
                text: qsTr("NOW PLAYING")
                color: label_fg
              }

              MouseArea
              {
                id: mouseArea
                objectName: "mouseArea"

                anchors.fill: parent
                // propagateComposedEvents: true
                // preventStealing: true

                onClicked: {
                  // console.log("Playlist item: CLICKED!")
                  set_current_item(groupItemsGridView.model.rootIndex.row,
                                   model.index);
                  mouse.accepted = true;
                }

                onDoubleClicked: {
                  // console.log("Playlist item: DOUBLE CLICKED!")
                  set_current_item(groupItemsGridView.model.rootIndex.row,
                                   model.index);
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
