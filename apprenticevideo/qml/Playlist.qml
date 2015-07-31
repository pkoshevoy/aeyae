import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.appenticevideo 1.0
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
    anchors.fill: parent
    anchors.margins: 0
    // objectName: "CanvasQuickFbo"

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



  ListView {
    id: playlistView
    anchors.fill: parent
    model: playlistModel
    delegate: groupDelegate
  }

  Component {
    id: groupDelegate
    Column {
      width: playlistView.width

      Rectangle {
        id: groupItem
        color: "#7fFFFFFF"
        height: 25
        width: playlistView.width
        anchors.left: parent.left
        anchors.right: parent.right

        Rectangle {
          id: disclosureBtn
          color: "red"
          // x: 5
          width: groupItem.height - 8
          height: groupItem.height - 8
          anchors.left: parent.left
          anchors.leftMargin: 8
          anchors.verticalCenter: parent.verticalCenter

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' property
            onClicked: playlistModel.setProperty(index, "collapsed", !collapsed)
          }
        }

        Text {
          anchors.verticalCenter: parent.verticalCenter
          anchors.right: parent.right
          anchors.left: disclosureBtn.right
          anchors.leftMargin: 8
          elide: "ElideMiddle"
          font.pixelSize: 14
          text: label
          color: "white"
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
          // item.model.rootIndex = item.model.modelIndex(index)
          /*
          item.model = DelegateModel {
            model: playlistModel
            rootIndex: modelIndex(index)
          }*/
        }
      }
    }
  }

  function calc_cell_width(w, h) {
    var n = Math.min(5, Math.floor(w / 160.0))
    return n < 1.0 ? w : w / n;
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
        height: (groupItemsGridView.cellHeight *
                 Math.max(1.0,
                          Math.ceil(groupItemsGridView.count /
                                    Math.floor(playlistView.width /
                                               groupItemsGridView.cellWidth))));

        GridView {
          id: groupItemsGridView
          anchors.fill: parent
          width: parent.width
          cellWidth: calc_cell_width(playlistView.width,
                                     playlistView.height)
          cellHeight: this.cellWidth * 9.0 / 16.0

          model: DelegateModel {
            id: modelDelegate
            model: playlistModel

            delegate: Rectangle {
              height: groupItemsGridView.cellHeight
              width: groupItemsGridView.cellWidth
              border.color: "black"
              border.width: 1
              color: "#7f1f2f7f" // argb

              Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.fill: parent
                x: 5
                font.pixelSize: 11
                wrapMode: "Wrap"
                elide: "ElideMiddle"
                text: label
                color: ((index % 2) ? "green" : "blue")
              }
            }
          }
        }
      }
    }
  }

}
