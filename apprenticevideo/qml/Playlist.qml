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

  /*
  Rectangle
  {
    anchors.fill: parent
    anchors.margins: 30
    color: "#7f1f3f7f" // ARGB

    ListView
    {
      id: playlist
      model: playlistModel

      delegate: Component {
        Loader {
          sourceComponent: Component {
            Text { text: label }
          }
        }
      }
      highlight: Rectangle { color: "lightsteelblue"; radius: 2 }
      focus: true
    }
  }
  */

/*
  ListView {
    anchors.fill: parent
    model: nestedModel
    delegate: categoryDelegate
  }

  ListModel {
    id: nestedModel
    ListElement {
      categoryName: "Veggies"
      collapsed: true

      // A ListElement can't contain child elements, but it can contain
      // a list of elements. A list of ListElements can be used as a model
      // just like any other model type.
      subItems: [
        ListElement { itemName: "Tomato" },
        ListElement { itemName: "Cucumber" },
        ListElement { itemName: "Onion" },
        ListElement { itemName: "Brains" }
      ]
    }

    ListElement {
      categoryName: "Fruits"
      collapsed: true
      subItems: [
        ListElement { itemName: "Orange" },
        ListElement { itemName: "Apple" },
        ListElement { itemName: "Pear" },
        ListElement { itemName: "Lemon" }
      ]
    }

    ListElement {
      categoryName: "Cars"
      collapsed: true
      subItems: [
        ListElement { itemName: "Nissan" },
        ListElement { itemName: "Toyota" },
        ListElement { itemName: "Chevy" },
        ListElement { itemName: "Audi" }
      ]
    }
  }

  Component {
    id: categoryDelegate
    Column {
      width: 200

      Rectangle {
        id: categoryItem
        border.color: "black"
        border.width: 5
        color: "white"
        height: 50
        width: 200

        Text {
          anchors.verticalCenter: parent.verticalCenter
          x: 15
          font.pixelSize: 24
          text: categoryName
        }

        Rectangle {
          color: "red"
          width: 30
          height: 30
          anchors.right: parent.right
          anchors.rightMargin: 15
          anchors.verticalCenter: parent.verticalCenter

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' property
            onClicked: nestedModel.setProperty(index, "collapsed", !collapsed)
          }
        }
      }

      Loader {
        id: subItemLoader

        // This is a workaround for a bug/feature in the Loader element.
        // If sourceComponent is set to null the Loader element retains
        // the same height it had when sourceComponent was set. Setting visible
        // to false makes the parent Column treat it as if it's height was 0.
        visible: !collapsed
        property variant subItemModel : subItems
        sourceComponent: collapsed ? null : subItemColumnDelegate
        onStatusChanged: if (status == Loader.Ready) item.model = subItemModel
      }
    }

  }

  Component {
    id: subItemColumnDelegate
    Column {
      property alias model : subItemRepeater.model
      width: 200
      Repeater {
        id: subItemRepeater
        delegate: Rectangle {
          color: "#cccccc"
          height: 40
          width: 200
          border.color: "black"
          border.width: 2

          Text {
            anchors.verticalCenter: parent.verticalCenter
            x: 30
            font.pixelSize: 18
            text: itemName
          }
        }
      }
    }
  }
*/

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
        anchors.leftMargin: 5

        Text {
          anchors.verticalCenter: parent.verticalCenter
          x: groupItem.height
          font.pixelSize: 14
          text: label
          color: "white"
        }

        Rectangle {
          color: "red"
          width: groupItem.height - 8
          height: groupItem.height - 8
          anchors.right: parent.right
          anchors.rightMargin: 8
          anchors.verticalCenter: parent.verticalCenter

          MouseArea {
            anchors.fill: parent

            // Toggle the 'collapsed' property
            onClicked: playlistModel.setProperty(index, "collapsed", !collapsed)
          }
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
                 Math.floor((groupItemsGridView.count *
                             groupItemsGridView.cellWidth +
                             playlistView.width - 1) /
                            playlistView.width));

        GridView {
          id: groupItemsGridView
          anchors.fill: parent
          width: parent.width
          cellHeight: 90
          cellWidth: 160

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
