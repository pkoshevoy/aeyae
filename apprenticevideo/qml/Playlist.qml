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

  /*
  // The checkers background
  ShaderEffect
  {
  id: tileBackground
  anchors.fill: parent

  property real tileSize: 16
  property color color1: Qt.rgba(0.9, 0.9, 0.9, 1);
  property color color2: Qt.rgba(0.85, 0.85, 0.85, 1);

  property size pixelSize: Qt.size(width / tileSize, height / tileSize);

  fragmentShader: "
  uniform lowp vec4 color1;
  uniform lowp vec4 color2;
  uniform highp vec2 pixelSize;
  varying highp vec2 qt_TexCoord0;

  void main() {
  highp vec2 tc = sign(sin(3.14152 * qt_TexCoord0 * pixelSize));
  if (tc.x != tc.y)
  gl_FragColor = color1;
  else
  gl_FragColor = color2;
  }
  "
  }
  */


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

  property var header_bg: "#7f1f1f1f"
  property var header_fg: "#ffffffff"
  property var zebra_bg: [ "#7f000000", "#7f1f1f1f"  ]
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

        Rectangle {
          id: disclosureBtn
          color: "black"
          width: groupItem.height - 8
          height: groupItem.height - 8
          anchors.left: parent.left
          anchors.leftMargin: 8
          anchors.verticalCenter: parent.verticalCenter

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' property
            onClicked: playlistModel.setProperty(index,
                                                 "collapsed",
                                                 !collapsed)
          }
        }

        Text {
          anchors.verticalCenter: parent.verticalCenter
          anchors.right: parent.right
          anchors.left: disclosureBtn.right
          anchors.leftMargin: groupItem.height / 2
          elide: "ElideMiddle"
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

        // visible: !collapsed
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

    Column {
      property alias model : groupItemsGridView.model
      width: playlistView.width
      visible: false

      Item
      {
        width: playlistView.width

        // size-to-fit:
        height: (!groupItemsGridView.count ? 0 :
                 1 + // calc_title_height(24.0, playlistView.width) +
                 groupItemsGridView.cellHeight *
                 Math.max(1.0,
                          Math.ceil(groupItemsGridView.count /
                                    Math.floor(playlistView.width /
                                               groupItemsGridView.
                                               cellWidth))));

        GridView {
          id: groupItemsGridView
          anchors.fill: parent
          width: parent.width
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
                // anchors.rightMargin: 1
                // anchors.bottomMargin: 1
                color: (calc_zebra_index(index,
                                         groupItemsGridView.cellWidth,
                                         playlistView.width) ?
                        zebra_bg[1] : zebra_bg[0]) // argb

                Image {
                  opacity: 1.0
                  anchors.fill: parent
                  // anchors.margins: 8
                  fillMode: Image.PreserveAspectFit
                  source: thumbnail // "qrc:///images/apprenticevideo-256.png"
                }

                Text {
                  // anchors.verticalCenter: parent.verticalCenter
                  verticalAlignment: Text.AlignBottom
                  anchors.fill: parent
                  anchors.margins: 5
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

}
