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
  function listProperties(item)
  {
    for (var p in item)
    {
      console.log(p + ": " + item[p]);
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

  property var header_bg: "#df1f1f1f"
  // property var header_bg: "#df000000"
  property var header_fg: "#ffffffff"
  property var zebra_bg: [ "#00000000", "#3f000000"  ]
  property var zebra_fg: [ "#ffdfdfdf", "#ffffffff"  ]
  property var greeting_message: "Hi!"

  ListView {
    id: playlistView
    objectName: "playlistView"

    anchors.fill: parent
    model: playlistModel
    delegate: groupDelegate
    footer: greetingComponent
  }

  Component {
    id: greetingComponent

    Rectangle {
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
      width: playlistView.width

      Rectangle {
        id: groupItem
        color: header_bg
        height: calc_title_height(24.0, playlistView.width)
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right

        Image {
          id: disclosureBtn
          width: groupItem.height
          height: groupItem.height
          anchors.leftMargin: 0

          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          source: (collapsed ?
                   "qrc:///images/group-collapsed.png" :
                   "qrc:///images/group-exposed.png");

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' item data role
            onClicked: {
              model.collapsed = !collapsed;
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

        // This is a workaround for a bug/feature in the Loader element.
        // If sourceComponent is set to null the Loader element retains
        // the same height it had when sourceComponent was set. Setting visible
        // to false makes the parent Column treat it as if it's height was 0.

        visible: !collapsed

	// property variant groupItemsModel : groupItemss
        sourceComponent: groupItemsColumnDelegate
        onStatusChanged: if (status == Loader.Ready) {
          // console.log("loaded: " + label + ", index: " + index)
          // console.log(item)
          item.model.rootIndex = item.model.modelIndex(index)
          item.visible = true

          console.log("item.model: " + item.model +
                      ", rootIndex: " + item.model.rootIndex);
        }
      }
    }
  }

  Component {
    id: groupItemsColumnDelegate

    Rectangle {
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

      GridView {
        id: groupItemsGridView
        anchors.fill: parent
        anchors.topMargin: 2
        width: parent.width
        height: parent.height - anchors.topMargin
        cellWidth: calc_cell_width(playlistView.width,
                                   playlistView.height)
        cellHeight: Math.floor(this.cellWidth * 9.0 / 16.0)

        model: DelegateModel {
          id: modelDelegate
          model: playlistModel

          delegate: Item {
            height: groupItemsGridView.cellHeight
            width: groupItemsGridView.cellWidth

            Rectangle {
              anchors.fill: parent
              color: (calc_zebra_index(index,
                                       groupItemsGridView.cellWidth,
                                       playlistView.width) ?
                      zebra_bg[1] : zebra_bg[0]) // argb

              Image {
                opacity: 1.0
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: thumbnail
              }

              Text {
                verticalAlignment: Text.AlignBottom
                anchors.fill: parent
                anchors.margins: 5
                font.bold: true
                font.pixelSize: (calc_title_height(24.0, playlistView.width) *
                                 0.45);
                wrapMode: "Wrap"
                elide: "ElideMiddle"
                text: label
                color: (calc_zebra_index(index,
                                         groupItemsGridView.cellWidth,
                                         playlistView.width) ?
                        zebra_fg[1] : zebra_fg[0])

                style: Text.Outline;
                styleColor: "black";
              }
            }
          }
        }
      }

    }
  }

}
