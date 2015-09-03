import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: player
  objectName: "player"

  signal exitFullScreen()
  signal toggleFullScreen()
  signal skipForward()
  signal skipBack()
  signal stepOneFrameForward()

  focus: true;

  Keys.onPressed:
  {
    if (playlist.view.visible)
    {
      playlist.handle_event_on_key_pressed(event);
    }
  }

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

  MouseArea
  {
    anchors.fill: parent
    anchors.margins: 0

    onDoubleClicked: {
      toggleFullScreen();
      mouse.accepted = true;
    }
  }

  Playlist
  {
    id: playlist
    objectName: "playlist"
    anchors.margins: 0
    anchors.fill: parent
  }

  Rectangle
  {
    id: timeline
    objectName: "timeline"

    anchors.margins: 0
    anchors.top: playlist.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.topMargin: -this.height
    height: (playlist.calc_title_height(24.0, playlist.width) * 3 / 2)

    gradient: Gradient {
      GradientStop { position: 0.0; color: "#00000000"; }
      GradientStop { position: 1.0; color: "#7f000000"; }
    }

    Item
    {
      anchors.fill: parent
      anchors.margins: 0
      anchors.leftMargin: height / 3
      anchors.rightMargin: height / 3

      Rectangle
      {
        id: timelineIn
        objectName: "timelineIn"
        color: "#7f7f7f"
        y: -this.height / 2
        anchors.left: parent.left
        height: parent.height / 12
        width: parent.width / 5
      }

      Rectangle
      {
        id: timelinePlayhead
        objectName: "timelinePlayhead"
        color: "#ff0f0f"
        y: -this.height / 2
        anchors.left: timelineIn.right
        height: parent.height / 12
        width: parent.width / 5 * 2
      }

      Rectangle
      {
        id: timelineOut
        objectName: "timelineOut"
        color: "#ffffff"
        y: -this.height / 2
        anchors.left: timelinePlayhead.right
        height: parent.height / 12
        width: parent.width / 5
      }

      Rectangle
      {
        id: timelineEnd
        objectName: "timelineEnd"
        color: "#7f7f7f"
        y: -this.height / 2
        anchors.left: timelineOut.right
        anchors.right: parent.right
        height: parent.height / 12
      }

      Rectangle
      {
        id: playhead
        objectName: "playhead"
        color: timelinePlayhead.color
        y: -this.height / 2
        x: timelineOut.x - this.width / 2
        height: parent.height / 4
        width: this.height
        radius: this.width / 2
      }

      Text
      {
        id: playheadAux
        objectName: "playheadAux"

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter

        text: "00:00:00:00"
        color: playlist.greeting_color
        style: Text.Outline
        styleColor: "black"
        font.pixelSize: parent.height / 2
      }
    }
  }
}
