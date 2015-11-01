import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: playlist
  objectName: "playlist"

  property alias scrollbar: scrollbar

  property var header_bg: "#df1f1f1f"
  property var header_fg: "#ffffffff"
  property var zebra_bg_0: "#00000000"
  property var zebra_bg_1: "#3f000000"
  property var separator_color: "#7f7f7f7f"
  property var footer_fg: "#7fffffff"
  property var highlight_color: "#3fff0000"
  property var selection_color: "#ff0000"
  property var label_bg: "#7f7f7f7f"
  property var label_fg: "white"
  property var filter_bg: "#7f7f7f"
  property var sort_fg: "#9f9f9f"

  readonly property var cellWidth: (
    calc_cell_width(playlistView.width));

  readonly property var cellHeight: (
    calc_cell_height(playlist.cellWidth));

  function calc_cell_width(w)
  {
    var n = Math.min(5, Math.floor(w / 160.0));
    return n < 1.0 ? w : w / n;
  }

  function calc_cell_height(cell_width)
  {
    var h = Math.floor(cell_width * 9.0 / 16.0);
    return h;
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
    return Math.max(min_height, 24.0 * w / 800.0);
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
    var origin_y = playlistView.originY;
    var delta_y = content_y - origin_y;

    // this has a side effect of changing playlistView.contentY
    playlistView.currentIndex = index;

    // restore original playlistView.contentY
    playlistView.contentY = playlistView.originY + delta_y;
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
                                              "gridView");
    return gridView;
  }

  function lookup_current_gridview_and_item()
  {
    var found = {
      groupRow: -1,
      itemRow: -1,
      gridView: null,
      item: null
    };

    var groupContainer = playlistView.currentItem;
    if (groupContainer)
    {
      found.groupRow = playlistView.currentIndex;
      found.gridView = yae_qml_utils.find_qobject(groupContainer,
                                                  "gridView");

      var itemContainer = found.gridView.currentItem;
      if (itemContainer)
      {
        // yae_qml_utils.dump_object_tree(itemContainer);
        found.itemRow = found.gridView.currentIndex;
        found.item = yae_qml_utils.find_qobject(itemContainer,
                                                "itemDelegate");
        // yae_qml_utils.dump_object_tree(item);
      }
    }

    return found;
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

  function sync_current_item(groupRow, itemRow)
  {
    // console.log("sync_current_item(" + groupRow + ", " + itemRow + ")");
    var groupContainer;
    var gridView;

    if (playlistView.currentIndex != groupRow)
    {
      gridView = yae_qml_utils.find_qobject(playlistView.currentItem,
                                            "gridView");
      if (gridView)
      {
        gridView.currentIndex = -1;
      }
    }

    assign_playlistview_current_index(groupRow);

    gridView = yae_qml_utils.find_qobject(playlistView.currentItem,
                                          "gridView");
    if (gridView && gridView.currentIndex != itemRow)
    {
      gridView.currentIndex = itemRow;
      /*
      console.log("sync_current_item(" + groupRow + ", " + itemRow + ") = " +
                  (gridView.currentItem ?
                   gridView.currentItem.label :
                   "null"));
      */
    }

    /*
    // for debugging:
    var found = lookup_current_gridview_and_item();

    if (found)
    {
      // Utils.dump_properties(found.item);
      calc_delta_scroll_to(found.item);
    }
    */
  }

  function select_items(groupRow, itemRow, selectionFlags)
  {
    yae_playlist_model.selectItems(groupRow, itemRow, selectionFlags);
    yae_playlist_model.setCurrentItem(groupRow, itemRow);
    sync_current_item(groupRow, itemRow);
  }

  function calc_delta_scroll_to(item)
  {
    if (!item)
    {
      return 0;
    }

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

    if (selectionFlags == ItemSelectionModel.SelectCurrent)
    {
      // set the selection anchor, if not already set:
      select_items(current.groupRow, current.itemRow, selectionFlags);
    }

    funcMoveCursor(current);

    select_items(playlistView.currentIndex,
                 current.gridView.currentIndex,
                 selectionFlags);
  }

  function move_cursor_left(selectionFlags)
  {
    move_cursor(selectionFlags, function(current) {

      if (current.itemRow > 0)
      {
        current.gridView.moveCurrentIndexLeft();
      }
      else if (current.groupRow > 0)
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

      if (current.itemRow + 1 < current.gridView.count)
      {
        current.gridView.moveCurrentIndexRight();
      }
      else if (current.groupRow + 1 < playlistView.count)
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
      if (current.itemRow > itemsPerRow)
      {
        current.gridView.moveCurrentIndexUp();
      }
      else if (current.itemRow > 0)
      {
        current.gridView.currentIndex = 0;
      }
      else if (current.groupRow > 0)
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
      if (current.itemRow + itemsPerRow < current.gridView.count)
      {
        current.gridView.moveCurrentIndexDown();
      }
      else if (current.itemRow + 1 < current.gridView.count)
      {
        current.gridView.currentIndex = current.gridView.count - 1;
      }
      else if (current.groupRow + 1 < playlistView.count)
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

  function get_selection_flags(event)
  {
    var selectionFlags = ItemSelectionModel.ClearAndSelect;

    if (event.modifiers & Qt.ControlModifier)
    {
      selectionFlags = ItemSelectionModel.ToggleCurrent;
    }
    else if (event.modifiers & Qt.ShiftModifier)
    {
      selectionFlags = ItemSelectionModel.SelectCurrent;
    }

    return selectionFlags;
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
      var selectionFlags = get_selection_flags(event);

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
  }

  Item
  {
    id: filter
    objectName: "filter"
    z: 2

    property alias container: container

    anchors.margins: 0
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: (calc_title_height(24.0, playlist.width) * 1.5)

    Rectangle
    {
      anchors.fill: parent
      anchors.margins: 0
      anchors.bottomMargin: -(parent.height * 2)

      gradient: Gradient {
        GradientStop { position: 0.000000; color: '#ff000000'; }
        GradientStop { position: 0.500000; color: '#b0000000'; }
        GradientStop { position: 1.000000; color: '#01000000'; }
      }
    }

    Item
    {
      id: container
      objectName: "container"

      property alias text: textInput

      anchors.fill: parent
      anchors.margins: 0
      opacity: (textInput.activeFocus ? 1.0 : 0.5)

      Rectangle
      {
        id: filterRect
        objectName: "filterRect"

        anchors.fill: parent
        anchors.margins: filter.height * 0.05
        radius: 3
        color: filter_bg
      }

      FilterIcon
      {
        id: filterIcon
        objectName: "filterIcon"

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: filter.height * 0.25
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        height: parent.height * 0.8
        width: height * 0.5
        color_fg: label_fg
      }

      TextInput
      {
        id: textInput
        objectName: "textInput"

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: filterIcon.right
        anchors.right: xbutton.left
        anchors.leftMargin: filter.height * 0.25

        font.bold: true
        font.pixelSize: calc_title_height(24.0, playlist.width) * 0.55

        selectionColor: label_fg
        selectedTextColor: label_bg
        color: label_fg
        selectByMouse: true

        KeyNavigation.tab: playlist.parent
        KeyNavigation.backtab: playlist.parent

        onAccepted: { playlist.parent.focus = true; }

        onTextChanged: { yae_playlist_model.setItemFilter(text); }

        Text
        {
          id: placeholder
          objectName: "placeholder"

          anchors.fill: parent
          anchors.margins: 0

          text: qsTr("SEARCH AND FILTER")
          color: label_fg
          font: textInput.font
          visible: (!textInput.activeFocus && textInput.text.length < 1)
        }
      }

      XButton
      {
        id: xbutton
        objectName: "xbutton"

        anchors.right: parent.right
        anchors.verticalCenter: textInput.verticalCenter
        anchors.margins: filter.height * 0.25

        height: textInput.height
        width: height
        visible: (textInput.text.length > 0)

        on_click: function (mouse)
        {
          textInput.text = "";
          playlist.parent.focus = true
          mouse.accepted = true;
        }
      }

    }
  }

  SortAndOrder
  {
    id: sortAndOrder
    objectName: "sortAndOrder"
    z: 2

    anchors.top: filter.bottom
    anchors.left: parent.left
    anchors.topMargin: -underscore_thickness
    anchors.leftMargin: filter.height * 0.25
    height: (calc_title_height(24.0, playlist.width) * 0.45);

    text_color: sort_fg
    text_outline_color: zebra_bg_1
    underscore_color: selection_color
    underscore_thickness: cellHeight * 0.02
  }

  Item
  {
    id: scrollbar
    objectName: "scrollbar"
    z: 2

    anchors.top: playlistView.top
    anchors.right: playlistView.right
    anchors.bottom: playlistView.bottom

    width: calc_title_height(24.0, playlist.width) * 0.5
    visible: playlistView.visibleArea.heightRatio < 1.0

    Item
    {
      id: "dragItem"
      objectName: "dragItem"
      width: 0
      height: 0

      property var dragStart: null
      property var sliderSize: 0
      property var itemMoved: null

      onYChanged: {
        if (itemMoved) {
          itemMoved();
        }
      }
    }

    Rectangle
    {
      id: slider
      objectName: "slider"

      x: parent.width * 0.2
      y: (playlistView.visibleArea.yPosition *
          (parent.height - width * 5));

      width: parent.width * 0.6
      height: (playlistView.visibleArea.heightRatio *
               (parent.height - width * 5) + width * 5);

      radius: width * 0.3
      color: filter_bg
      opacity: 0.5

      function move_slider()
      {
        var t = (dragItem.y / (scrollbar.height - dragItem.sliderSize));
        var y_top = playlistView.contentHeight - playlistView.height;

        playlistView.contentY = (
          playlistView.originY + Math.max(0, Math.min(y_top, t * y_top)));
      }

      MouseArea
      {
        id: dragArea
        objectName: "dragArea"
        anchors.fill: parent
        drag.target: dragItem
        drag.axis: Drag.YAxis
        drag.threshold: 0

        onPressed: {
          dragItem.x = slider.x;
          dragItem.y = slider.y;
          dragItem.dragStart = dragItem.y;
          dragItem.sliderSize = slider.height;

          dragItem.itemMoved = function() {
            if (dragArea.drag.active) {
              slider.move_slider();
            }
          }
        }

        onReleased: {
          dragItem.itemMoved = undefined;
          slider.move_slider();
        }
      }
    }
  }

  ListView
  {
    id: playlistView
    objectName: "playlistView"

    anchors.margins: 0
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: filter.bottom
    anchors.bottom: parent.bottom

    model: yae_playlist_model
    delegate: groupDelegate
    footer: footerComponent

    highlightFollowsCurrentItem: false
    currentIndex: -1

    /*
    // for debugging:
    onCurrentIndexChanged: {
      console.log("FIXME: PlaylistView, currentIndex changed: " +
                  currentIndex);
    }
    */
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
        sync_current_item(groupRow, itemRow);
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
      objectName: "footer"

      width: playlist.width
      height: footnote.height * 3 + 2

      Rectangle
      {
        color: separator_color
        height: 1
        width: playlist.width
        anchors.left: parent.left
        anchors.right: parent.right
      }

      Text
      {
        id: footnote
        objectName: "footnote"

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 2
        anchors.leftMargin: height / 2
        anchors.rightMargin: height / 2 + scrollbar.width
        height: calc_title_height(24.0, playlist.width)
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: "ElideMiddle"
        font.pixelSize: height * 0.45
        text: ((yae_playlist_model.itemCount == 1) ?
               "1 item, end of playlist" :
               "" + yae_playlist_model.itemCount + " items, end of playlist");
        color: footer_fg
      }

      // YDebug { id: ydebug; z: 4; container: playlistView; }
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
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.rightMargin: scrollbar.width

      Item
      {
        id: spacer
        objectName: "spacer"

        anchors.left: parent.left
        anchors.right: parent.right
        height: 0.2 * cellHeight
      }

      Item
      {
        id: groupItems
        objectName: "groupItems"

        height: calc_title_height(24.0, playlist.width)
        anchors.left: parent.left
        anchors.right: parent.right

        Image
        {
          id: disclosureBtn
          objectName: "disclosureBtn"

          width: height
          height: Math.max(9, groupItems.height * 0.5)
          anchors.leftMargin: filter.height * 0.25
          antialiasing: true

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
              // console.log("Playlist group: CLICKED!")
              model.collapsed = !model.collapsed;
            }
          }
        }

        Text
        {
          id: groupTag
          objectName: "groupTag"

          anchors.verticalCenter: parent.verticalCenter
          anchors.right: parent.right
          anchors.left: disclosureBtn.right
          anchors.leftMargin: groupItems.height / 2

          elide: "ElideMiddle"
          font.bold: true
          font.pixelSize: groupItems.height * 0.55
          text: (model.label || "")
          color: header_fg
          style: Text.Outline;
          styleColor: "black";
        }

        XButton
        {
          id: xbutton
          objectName: "xbutton"

          anchors.right: parent.right
          anchors.verticalCenter: groupTag.verticalCenter
          anchors.rightMargin: cellHeight * 0.05

          height: Math.max(13, Utils.make_odd(cellHeight * 0.1))
          width: height

          on_click: function (mouse)
          {
            yae_playlist_model.removeItems(model.index, -1);
            mouse.accepted = true;
          }
        }

        // YDebug { id: ydebug; z: 4; container: playlistView; }
        // onYChanged: { ydebug.refresh(); }
      }

      Loader
      {
        id: groupsLoader
        objectName: "groupsLoader"

        anchors.left: parent.left
        anchors.right: parent.right

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

      property alias model : gridView.model
      width: parent.width

      // size-to-fit:
      height: (!gridView.count ? 0 :
               gridView.cellHeight *
               (0.3 + calc_rows(width, gridView.cellWidth, gridView.count)));

      Rectangle
      {
        color: separator_color
        height: 1
        width: parent.width
        anchors.left: parent.left
        anchors.right: parent.right
      }

      Component
      {
        id: gridViewHighlight

        Item
        {
          visible: gridView.currentItem != null
          x: (gridView.currentItem ?
              gridView.currentItem.x :
              0)
          y: (gridView.currentItem ?
              gridView.currentItem.y :
              0)
          z: 3
          width: gridView.cellWidth;
          height: gridView.cellHeight
          anchors.margins: 0

          Behavior on x { SpringAnimation { spring: 3; damping: 0.2 } }
          Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }

          Rectangle
          {
            id: overlay
            objectName: "overlay"
            anchors.fill: parent
            anchors.margins: 0
            color: highlight_color;
          }
        }
      }

      GridView
      {
        id: gridView
        objectName: "gridView"

        anchors.fill: parent
        anchors.topMargin: 2
        width: parent.width
        height: parent.height - anchors.topMargin
        cellWidth: calc_cell_width(width)
        cellHeight: calc_cell_height(this.cellWidth)

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

            height: gridView.cellHeight
            width: gridView.cellWidth

            property var label: model.label

            function set_selected(selectionFlags)
            {
              if (selectionFlags == ItemSelectionModel.ToggleCurrent)
              {
                model.selected = !model.selected;
              }
              else if (selectionFlags == ItemSelectionModel.SelectCurrent)
              {
                model.selected = true;
              }
              else
              {
                yae_playlist_model.unselectAll();
                model.selected = true;
              }
            }

            // YDebug { id: ydebug; z: 4; container: playlistView; }
            // onYChanged: { ydebug.refresh(); }

            Rectangle
            {
              id: backgroundRect
              objectName: "backgroundRect"

              anchors.fill: parent
              clip: true
              color: (calc_zebra_index(index, gridView.cellWidth, width) ?
                      zebra_bg_1 : zebra_bg_0); // argb

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

                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
              }

              Rectangle
              {
                id: labelBackgroundRect
                objectName: "labelBackgroundRect"

                color: label_bg
                anchors.margins: 0;
                anchors.leftMargin: -gridView.cellHeight * 0.02;
                anchors.rightMargin: -gridView.cellHeight * 0.02;
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
                anchors.margins: parent.height * 0.05
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlist.width) *
                                 0.45);
                wrapMode: "Wrap"
                elide: "ElideMiddle"
                text: (model.label || "")
                color: label_fg;

                style: Text.Outline;
                styleColor: label_bg;
              }

              Rectangle
              {
                id: nowPlayingRect
                objectName: "nowPlayingRect"

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
                anchors.leftMargin: -underscore.height
                anchors.rightMargin: -underscore.height
                anchors.left: nowPlayingTag.left
                anchors.right: nowPlayingTag.right
                anchors.bottom: nowPlayingTag.bottom
                height: Utils.make_odd(nowPlayingTag.height)
                radius: 3

                Rectangle
                {
                  id: underscore
                  objectName: "underscore"

                  z: 2

                  anchors.bottom: parent.bottom
                  anchors.left: parent.left
                  anchors.right: parent.right
                  anchors.bottomMargin: -height * 2

                  antialiasing: true
                  height: gridView.cellHeight * 0.02
                  color: selection_color
                }
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

                anchors.right: (xbutton.view.visible ?
                                xbutton.left :
                                parent.right);
                anchors.top: parent.top
                anchors.margins: parent.height * 0.05
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlist.width) *
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
                  var selectionFlags = get_selection_flags(mouse);
                  select_items(gridView.model.rootIndex.row,
                               model.index,
                               selectionFlags);
                  mouse.accepted = true;
                  playlist.parent.focus = true;
                }

                onDoubleClicked: {
                  // console.log("Playlist item: DOUBLE CLICKED!")
                  sync_current_item(gridView.model.rootIndex.row,
                                    model.index);
                  model.playing = true;
                  mouse.accepted = true;
                  playlist.parent.focus = true;
                }
              }

              XButton
              {
                id: xbutton
                objectName: "xbutton"

                anchors.right: parent.right
                anchors.verticalCenter: nowPlayingRect.verticalCenter
                anchors.margins: parent.height * 0.05

                height: Math.max(13, Utils.make_odd(gridView.cellHeight * 0.1))
                width: height

                on_click: function (mouse)
                {
                  yae_playlist_model.removeItems(gridView.model.rootIndex.row,
                                                 model.index);
                  mouse.accepted = true;
                }
              }
            }

            Item
            {
              id: selectionDeco
              objectName: "selectionDeco"
              z: 2

              anchors.fill: parent
              anchors.margins: 0
              visible: (model.selected || false)

              Rectangle
              {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.leftMargin: height
                anchors.rightMargin: height
                anchors.bottomMargin: height

                antialiasing: true
                height: gridView.cellHeight * 0.02
                color: selection_color
              }
            }


          }
        }
      }

    }
  }

}
