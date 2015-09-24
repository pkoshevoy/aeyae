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

  property alias view: playlistView

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
    else if (event.key == Qt.Key_Escape)
    {
      event.accepted = true;
    }
  }

  Item
  {
    id: filter
    objectName: "filter"
    z: 2

    anchors.margins: 0
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: (calc_title_height(24.0, playlistView.width) * 1.5)

    Rectangle
    {
      anchors.fill: parent
      anchors.margins: 0
      anchors.bottomMargin: -(parent.height * 2)

      gradient: Gradient {
        GradientStop { position: 0.000000; color: '#b0000000'; }
        GradientStop { position: 0.010417; color: '#ad000000'; }
        GradientStop { position: 0.020833; color: '#aa000000'; }
        GradientStop { position: 0.031250; color: '#a7000000'; }
        GradientStop { position: 0.041667; color: '#a4000000'; }
        GradientStop { position: 0.052083; color: '#a1000000'; }
        GradientStop { position: 0.062500; color: '#9e000000'; }
        GradientStop { position: 0.072917; color: '#9b000000'; }
        GradientStop { position: 0.083333; color: '#98000000'; }
        GradientStop { position: 0.093750; color: '#96000000'; }
        GradientStop { position: 0.104167; color: '#94000000'; }
        GradientStop { position: 0.114583; color: '#91000000'; }
        GradientStop { position: 0.125000; color: '#8f000000'; }
        GradientStop { position: 0.135417; color: '#8d000000'; }
        GradientStop { position: 0.145833; color: '#89000000'; }
        GradientStop { position: 0.156250; color: '#87000000'; }
        GradientStop { position: 0.166667; color: '#85000000'; }
        GradientStop { position: 0.177083; color: '#83000000'; }
        GradientStop { position: 0.187500; color: '#80000000'; }
        GradientStop { position: 0.197917; color: '#7e000000'; }
        GradientStop { position: 0.208333; color: '#7a000000'; }
        GradientStop { position: 0.218750; color: '#78000000'; }
        GradientStop { position: 0.229167; color: '#75000000'; }
        GradientStop { position: 0.239583; color: '#72000000'; }
        GradientStop { position: 0.250000; color: '#6f000000'; }
        GradientStop { position: 0.260417; color: '#6d000000'; }
        GradientStop { position: 0.270833; color: '#6a000000'; }
        GradientStop { position: 0.281250; color: '#67000000'; }
        GradientStop { position: 0.291667; color: '#64000000'; }
        GradientStop { position: 0.302083; color: '#63000000'; }
        GradientStop { position: 0.312500; color: '#60000000'; }
        GradientStop { position: 0.322917; color: '#5d000000'; }
        GradientStop { position: 0.333333; color: '#5a000000'; }
        GradientStop { position: 0.343750; color: '#58000000'; }
        GradientStop { position: 0.354167; color: '#54000000'; }
        GradientStop { position: 0.364583; color: '#52000000'; }
        GradientStop { position: 0.375000; color: '#4f000000'; }
        GradientStop { position: 0.385417; color: '#4c000000'; }
        GradientStop { position: 0.395833; color: '#49000000'; }
        GradientStop { position: 0.406250; color: '#47000000'; }
        GradientStop { position: 0.416667; color: '#44000000'; }
        GradientStop { position: 0.427083; color: '#42000000'; }
        GradientStop { position: 0.437500; color: '#40000000'; }
        GradientStop { position: 0.447917; color: '#3d000000'; }
        GradientStop { position: 0.458333; color: '#3a000000'; }
        GradientStop { position: 0.468750; color: '#38000000'; }
        GradientStop { position: 0.479167; color: '#35000000'; }
        GradientStop { position: 0.489583; color: '#33000000'; }
        GradientStop { position: 0.500000; color: '#31000000'; }
        GradientStop { position: 0.510417; color: '#2e000000'; }
        GradientStop { position: 0.520833; color: '#2c000000'; }
        GradientStop { position: 0.531250; color: '#2a000000'; }
        GradientStop { position: 0.541667; color: '#28000000'; }
        GradientStop { position: 0.552083; color: '#26000000'; }
        GradientStop { position: 0.562500; color: '#24000000'; }
        GradientStop { position: 0.572917; color: '#22000000'; }
        GradientStop { position: 0.583333; color: '#21000000'; }
        GradientStop { position: 0.593750; color: '#1f000000'; }
        GradientStop { position: 0.604167; color: '#1d000000'; }
        GradientStop { position: 0.614583; color: '#1b000000'; }
        GradientStop { position: 0.625000; color: '#1a000000'; }
        GradientStop { position: 0.635417; color: '#18000000'; }
        GradientStop { position: 0.645833; color: '#17000000'; }
        GradientStop { position: 0.656250; color: '#16000000'; }
        GradientStop { position: 0.666667; color: '#14000000'; }
        GradientStop { position: 0.677083; color: '#13000000'; }
        GradientStop { position: 0.687500; color: '#12000000'; }
        GradientStop { position: 0.697917; color: '#10000000'; }
        GradientStop { position: 0.708333; color: '#0f000000'; }
        GradientStop { position: 0.718750; color: '#0f000000'; }
        GradientStop { position: 0.729167; color: '#0d000000'; }
        GradientStop { position: 0.739583; color: '#0c000000'; }
        GradientStop { position: 0.750000; color: '#0b000000'; }
        GradientStop { position: 0.760417; color: '#0b000000'; }
        GradientStop { position: 0.770833; color: '#0a000000'; }
        GradientStop { position: 0.781250; color: '#09000000'; }
        GradientStop { position: 0.791667; color: '#08000000'; }
        GradientStop { position: 0.802083; color: '#07000000'; }
        GradientStop { position: 0.812500; color: '#07000000'; }
        GradientStop { position: 0.822917; color: '#06000000'; }
        GradientStop { position: 0.833333; color: '#06000000'; }
        GradientStop { position: 0.843750; color: '#05000000'; }
        GradientStop { position: 0.854167; color: '#04000000'; }
        GradientStop { position: 0.864583; color: '#04000000'; }
        GradientStop { position: 0.875000; color: '#03000000'; }
        GradientStop { position: 0.885417; color: '#03000000'; }
        GradientStop { position: 0.895833; color: '#03000000'; }
        GradientStop { position: 0.906250; color: '#03000000'; }
        GradientStop { position: 0.916667; color: '#03000000'; }
        GradientStop { position: 0.927083; color: '#02000000'; }
        GradientStop { position: 0.937500; color: '#02000000'; }
        GradientStop { position: 0.947917; color: '#02000000'; }
        GradientStop { position: 0.958333; color: '#01000000'; }
        GradientStop { position: 0.968750; color: '#01000000'; }
        GradientStop { position: 0.979167; color: '#01000000'; }
        GradientStop { position: 0.989583; color: '#01000000'; }
        GradientStop { position: 1.000000; color: '#01000000'; }
      }
    }

    Item
    {
      anchors.fill: parent
      anchors.margins: 0
      opacity: (textInput.activeFocus ? 1.0 : 0.5)

      Rectangle
      {
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
        font.pixelSize: calc_title_height(24.0, playlistView.width) * 0.55

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
          mouse.accepted = true;
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
      width: playlistView.width

      Item
      {
        id: spacer
        objectName: "spacer"

        anchors.left: parent.left
        anchors.right: parent.right
        height: 0.2 * groupItem.cellHeight
      }

      Item
      {
        id: groupItem
        objectName: "groupItem"

        height: calc_title_height(24.0, playlistView.width)
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property var cellHeight: (
          calc_cell_height(calc_cell_width(playlistView.width)));

        Image
        {
          id: disclosureBtn
          objectName: "disclosureBtn"

          width: height
          height: Math.max(9, groupItem.height * 0.5)
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
          anchors.leftMargin: groupItem.height / 2

          elide: "ElideMiddle"
          font.bold: true
          font.pixelSize: groupItem.height * 0.55
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
          anchors.rightMargin: groupItem.cellHeight * 0.05

          height: Math.max(13, Utils.make_odd(groupItem.cellHeight * 0.1))
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
      width: playlistView.width

      // size-to-fit:
      height: (!gridView.count ? 0 :
               gridView.cellHeight *
               (0.3 + calc_rows(playlistView.width,
                                gridView.cellWidth,
                                gridView.count)));

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
        cellWidth: calc_cell_width(playlistView.width)
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
              color: (calc_zebra_index(index,
                                       gridView.cellWidth,
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

                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
              }

              Rectangle
              {
                id: labelBackgroundRect
                objectName: "labelBackgroundRect"

                color: label_bg
                anchors.margins: 0;
                anchors.leftMargin: -parent.height * 0.02;
                anchors.rightMargin: -parent.height * 0.02;
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
                font.pixelSize: (calc_title_height(24.0, playlistView.width) *
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
                anchors.leftMargin: -parent.height * 0.02;
                anchors.rightMargin: -parent.height * 0.02;
                anchors.left: nowPlayingTag.left
                anchors.right: nowPlayingTag.right
                anchors.bottom: nowPlayingTag.bottom
                height: Utils.make_odd(nowPlayingTag.height)
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

                anchors.right: (xbutton.view.visible ?
                                xbutton.left :
                                parent.right);
                anchors.top: parent.top
                anchors.margins: parent.height * 0.05
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
                  var selectionFlags = get_selection_flags(mouse);
                  select_items(gridView.model.rootIndex.row,
                               model.index,
                               selectionFlags);
                  mouse.accepted = true;
                }

                onDoubleClicked: {
                  // console.log("Playlist item: DOUBLE CLICKED!")
                  sync_current_item(gridView.model.rootIndex.row,
                                    model.index);
                  model.playing = true;
                  mouse.accepted = true;
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
                anchors.leftMargin: parent.height * 0.02
                anchors.rightMargin: parent.height * 0.02
                anchors.bottomMargin: height
                height: parent.height * 0.02;
                color: selection_color
              }
            }


          }
        }
      }

    }
  }

}
